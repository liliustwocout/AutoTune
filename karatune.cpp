#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <windows.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <iostream>

// ------------------------------------------------------------------------------
// CAU HINH AUDIO CHUAN PHONG THU
// ------------------------------------------------------------------------------
const int SAMPLE_RATE = 48000;
const int BUFFER_FRAMES = 256;      // ~5.33ms round-trip latency
const int CHANNELS_IN = 1;
const int CHANNELS_OUT = 2;

// Bo nho dem tieng vang Karaoke Echo
const int ECHO_DELAY_SAMPLES = 48000 * 200 / 1000; // 200ms delay
std::vector<float> g_echo_buffer(ECHO_DELAY_SAMPLES, 0.0f);
int g_echo_idx = 0;

// ------------------------------------------------------------------------------
// THANG AM (MUSICAL SCALES) - HO TRO TIENG VIET CO DAU
// ------------------------------------------------------------------------------
struct ScaleDef {
    std::wstring name;
    std::vector<float> freqs;
    std::vector<std::string> names;
};

std::vector<ScaleDef> g_scales;
std::atomic<int> g_scale_idx(0);
std::atomic<bool> g_enable_autotune(true);
std::atomic<int> g_retune_mode(0); // 0: Nhanh (0ms), 1: Vua (25ms), 2: Tu nhien (50ms)
std::atomic<float> g_gain_mult(8.0f);
std::atomic<bool> g_enable_echo(false);

// Bien telemetry giao tiep Audio Thread -> GUI Thread
std::atomic<float> g_rms(0.0f);
std::atomic<float> g_f0(0.0f);
std::atomic<float> g_target_f0(0.0f);
std::atomic<int>   g_note_idx(-1);
std::atomic<float> g_cent(0.0f);

void init_scales() {
    g_scales.clear();

    // 1. C Major (Đô Trưởng)
    ScaleDef c_maj;
    c_maj.name = L"ĐÔ TRƯỞNG (C MAJOR)";
    const char* c_maj_names[] = {
        "C3", "D3", "E3", "F3", "G3", "A3", "B3",
        "C4", "D4", "E4", "F4", "G4", "A4", "B4",
        "C5", "D5", "E5", "F5", "G5", "A5", "B5"
    };
    const float c_maj_freqs[] = {
        130.81f, 146.83f, 164.81f, 174.61f, 196.00f, 220.00f, 246.94f,
        261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f,
        523.25f, 587.33f, 659.25f, 698.46f, 783.99f, 880.00f, 987.77f
    };
    for (int i = 0; i < 21; ++i) {
        c_maj.freqs.push_back(c_maj_freqs[i]);
        c_maj.names.push_back(c_maj_names[i]);
    }
    g_scales.push_back(c_maj);

    // 2. A Minor (La Thứ)
    ScaleDef a_min;
    a_min.name = L"LA THỨ (A MINOR)";
    const char* a_min_names[] = {
        "A2", "B2", "C3", "D3", "E3", "F3", "G3",
        "A3", "B3", "C4", "D4", "E4", "F4", "G4",
        "A4", "B4", "C5", "D5", "E5", "F5", "G5", "A5"
    };
    const float a_min_freqs[] = {
        110.00f, 123.47f, 130.81f, 146.83f, 164.81f, 174.61f, 196.00f,
        220.00f, 246.94f, 261.63f, 293.66f, 329.63f, 349.23f, 392.00f,
        440.00f, 493.88f, 523.25f, 587.33f, 659.25f, 698.46f, 783.99f, 880.00f
    };
    for (int i = 0; i < 22; ++i) {
        a_min.freqs.push_back(a_min_freqs[i]);
        a_min.names.push_back(a_min_names[i]);
    }
    g_scales.push_back(a_min);

    // 3. Chromatic (12 Bán Âm Toàn Phần)
    ScaleDef chrom;
    chrom.name = L"12 BÁN ÂM (CHROMATIC)";
    const char* note_letters[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int midi = 48; midi <= 84; ++midi) {
        float f = 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
        int oct = (midi / 12) - 1;
        std::string n = std::string(note_letters[midi % 12]) + std::to_string(oct);
        chrom.freqs.push_back(f);
        chrom.names.push_back(n);
    }
    g_scales.push_back(chrom);

    // 4. G Major (Sol Trưởng)
    ScaleDef g_maj;
    g_maj.name = L"SOL TRƯỞNG (G MAJOR)";
    const char* g_maj_names[] = {
        "G2", "A2", "B2", "C3", "D3", "E3", "F#3",
        "G3", "A3", "B3", "C4", "D4", "E4", "F#4",
        "G4", "A4", "B4", "C5", "D5", "E5", "F#5", "G5"
    };
    const float g_maj_freqs[] = {
        98.00f, 110.00f, 123.47f, 130.81f, 146.83f, 164.81f, 185.00f,
        196.00f, 220.00f, 246.94f, 261.63f, 293.66f, 329.63f, 369.99f,
        392.00f, 440.00f, 493.88f, 523.25f, 587.33f, 659.25f, 739.99f, 783.99f
    };
    for (int i = 0; i < 22; ++i) {
        g_maj.freqs.push_back(g_maj_freqs[i]);
        g_maj.names.push_back(g_maj_names[i]);
    }
    g_scales.push_back(g_maj);

    // 5. D Minor (Rê Thứ)
    ScaleDef d_min;
    d_min.name = L"RÊ THỨ (D MINOR)";
    const char* d_min_names[] = {
        "D3", "E3", "F3", "G3", "A3", "Bb3", "C4",
        "D4", "E4", "F4", "G4", "A4", "Bb4", "C5",
        "D5", "E5", "F5", "G5", "A5", "Bb5", "C6"
    };
    const float d_min_freqs[] = {
        146.83f, 164.81f, 174.61f, 196.00f, 220.00f, 233.08f, 261.63f,
        293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 466.16f, 523.25f,
        587.33f, 659.25f, 698.46f, 783.99f, 880.00f, 932.33f, 1046.50f
    };
    for (int i = 0; i < 21; ++i) {
        d_min.freqs.push_back(d_min_freqs[i]);
        d_min.names.push_back(d_min_names[i]);
    }
    g_scales.push_back(d_min);
}

// ------------------------------------------------------------------------------
// DSP ENGINE C++
// ------------------------------------------------------------------------------
class CppPitchShifter {
public:
    int win_size;
    int buf_size;
    std::vector<float> buffer;
    int write_pos;
    double phase;

    CppPitchShifter(int sample_rate = 48000, float win_ms = 40.0f) {
        win_size = (int)(sample_rate * (win_ms / 1000.0f));
        buf_size = win_size * 4;
        buffer.assign(buf_size, 0.0f);
        write_pos = 0;
        phase = 0.0;
    }

    void process(const float* input, float* output, int frame_count, float shift_ratio) {
        if (std::abs(shift_ratio - 1.0f) < 0.008f) {
            for (int i = 0; i < frame_count; ++i) {
                buffer[write_pos] = input[i];
                write_pos = (write_pos + 1) % buf_size;
                output[i] = input[i];
            }
            return;
        }

        double rate = (1.0 - (double)shift_ratio) / (double)win_size;

        for (int i = 0; i < frame_count; ++i) {
            buffer[write_pos] = input[i];

            // Tap 1
            double p1 = phase;
            double d1 = p1 * win_size;
            double r1 = (double)write_pos - d1;
            while (r1 < 0) r1 += buf_size;
            int idx1 = (int)r1 % buf_size;
            float f1 = (float)(r1 - (int)r1);
            float s1 = buffer[idx1] * (1.0f - f1) + buffer[(idx1 + 1) % buf_size] * f1;
            float w1 = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * (float)p1));

            // Tap 2 (Lệch pha 0.5 = 180 độ)
            double p2 = phase + 0.5;
            if (p2 >= 1.0) p2 -= 1.0;
            double d2 = p2 * win_size;
            double r2 = (double)write_pos - d2;
            while (r2 < 0) r2 += buf_size;
            int idx2 = (int)r2 % buf_size;
            float f2 = (float)(r2 - (int)r2);
            float s2 = buffer[idx2] * (1.0f - f2) + buffer[(idx2 + 1) % buf_size] * f2;
            float w2 = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * (float)p2));

            output[i] = s1 * w1 + s2 * w2;

            write_pos = (write_pos + 1) % buf_size;
            phase += rate;
            if (phase >= 1.0) phase -= 1.0;
            else if (phase < 0.0) phase += 1.0;
        }
    }
};

// ------------------------------------------------------------------------------
// THUAT TOAN YIN PITCH DETECTION CHUYEN NGHIEP (TRIET TIEU 100% NHAY QUANG & GIAT CUC)
// ------------------------------------------------------------------------------
class YINPitchDetector {
public:
    int sr;
    int win_size;    // Cua so tich phan W = 512 mau (~10.7ms)
    int max_tau;     // Chu ky lon nhat (65 Hz -> ~738 mau)
    int min_tau;     // Chu ky nho nhat (850 Hz -> ~56 mau)
    int buf_len;     // Bo nho dem cuon = 512 + 738 + 64 = 1314 mau
    std::vector<float> buffer;
    float last_valid_pitch;
    int hold_frames;
    bool is_gate_open;

    YINPitchDetector(int sample_rate = 48000) 
        : sr(sample_rate), win_size(512), last_valid_pitch(0.0f), hold_frames(0), is_gate_open(false) {
        max_tau = sr / 65;
        min_tau = sr / 850;
        buf_len = win_size + max_tau + 64;
        buffer.assign(buf_len, 0.0f);
    }

    float process(const float* chunk, int frame_count, float& out_rms) {
        // Cuon bo nho dem
        if (frame_count < buf_len) {
            std::copy(buffer.begin() + frame_count, buffer.end(), buffer.begin());
            std::copy(chunk, chunk + frame_count, buffer.end() - frame_count);
        }

        // Tinh RMS tren cac mau moi nhat
        float sum_sq = 0.0f;
        for (int i = buf_len - win_size; i < buf_len; ++i) {
            sum_sq += buffer[i] * buffer[i];
        }
        out_rms = std::sqrt(sum_sq / win_size);

        // Schmitt Trigger Hysteresis (Mo cong o -54dBFS, giu cong toi -62dBFS)
        // Chon loc tieng tho, tieng go ban phim, khong bi ngat nhap nhay
        if (!is_gate_open) {
            if (out_rms > 0.0020f) is_gate_open = true;
        } else {
            if (out_rms < 0.0008f) is_gate_open = false;
        }

        if (!is_gate_open) {
            if (hold_frames > 0) {
                hold_frames--;
                return last_valid_pitch;
            }
            last_valid_pitch = 0.0f;
            return 0.0f;
        }

        // BƯỚC 1: Ham sai phan YIN d(tau) = sum( (x[j] - x[j+tau])^2 )
        std::vector<float> d(max_tau + 1, 0.0f);
        const float* x = buffer.data() + (buf_len - win_size - max_tau);
        for (int tau = min_tau; tau <= max_tau; ++tau) {
            float diff_sum = 0.0f;
            for (int j = 0; j < win_size; ++j) {
                float diff = x[j] - x[j + tau];
                diff_sum += diff * diff;
            }
            d[tau] = diff_sum;
        }

        // BƯỚC 2: Chuan hoa trung binh luy tien CMNDF d'(tau)
        std::vector<float> cmndf(max_tau + 1, 1.0f);
        float running_sum = 0.0f;
        for (int tau = 1; tau <= max_tau; ++tau) {
            running_sum += d[tau];
            if (running_sum > 1e-6f) {
                cmndf[tau] = d[tau] / (running_sum / (float)tau);
            } else {
                cmndf[tau] = 1.0f;
            }
        }

        // BƯỚC 3: Nguong tuyet doi - Chon cuc tieu DAU TIEN duoi nguong 0.20
        // (Khoa chat tan so goc, loai bo 100% loi nhay quang / subharmonic)
        const float threshold = 0.20f;
        int tau_best = -1;
        for (int tau = min_tau; tau < max_tau; ++tau) {
            if (cmndf[tau] < threshold) {
                while (tau + 1 < max_tau && cmndf[tau + 1] < cmndf[tau]) {
                    tau++;
                }
                tau_best = tau;
                break;
            }
        }

        // Neu khong co diem duoi 0.20, tim cuc tieu toan cuc trong tam kiem soat
        if (tau_best == -1) {
            float min_val = 1e9f;
            int min_idx = -1;
            for (int tau = min_tau; tau < max_tau; ++tau) {
                if (cmndf[tau] < min_val) {
                    min_val = cmndf[tau];
                    min_idx = tau;
                }
            }
            if (min_val < 0.38f) {
                tau_best = min_idx;
            }
        }

        if (tau_best >= min_tau && tau_best <= max_tau) {
            // BƯỚC 4: Noi suy Parabol de lay chu ky chinh xac duoi mau (Sub-sample)
            float alpha = cmndf[tau_best - 1];
            float beta  = cmndf[tau_best];
            float gamma = (tau_best + 1 <= max_tau) ? cmndf[tau_best + 1] : beta;
            float denom = 2.0f * (2.0f * beta - alpha - gamma);
            float delta = 0.0f;
            if (std::abs(denom) > 1e-6f) {
                delta = (gamma - alpha) / denom;
                if (delta > 0.5f) delta = 0.5f;
                if (delta < -0.5f) delta = -0.5f;
            }
            float period = (float)tau_best + delta;
            if (period > 1.0f) {
                float freq = (float)sr / period;
                if (freq >= 65.0f && freq <= 850.0f) {
                    // Lam min cao do giua cac khung hinh ke tiep
                    if (last_valid_pitch > 50.0f && std::abs(freq - last_valid_pitch) / last_valid_pitch < 0.08f) {
                        freq = 0.80f * freq + 0.20f * last_valid_pitch;
                    }
                    last_valid_pitch = freq;
                    hold_frames = 16; // Duy tri cao do ~85ms neu tin hieu hut hoi nhe
                    return freq;
                }
            }
        }

        // Pitch Hold: Giu am neu nguoi hat ngan dai hoac luyen hoi
        if (hold_frames > 0 && last_valid_pitch > 0.0f) {
            hold_frames--;
            return last_valid_pitch;
        }

        last_valid_pitch = 0.0f;
        return 0.0f;
    }
};

// ------------------------------------------------------------------------------
// TRANG THAI AUDIO TOAN CUC
// ------------------------------------------------------------------------------
YINPitchDetector g_detector(SAMPLE_RATE);
CppPitchShifter g_shifter(SAMPLE_RATE, 40.0f);
std::vector<float> g_boosted_buf;
std::vector<float> g_tuned_buf;
float g_smoothed_ratio = 1.0f;

// Callback xu ly am thanh miniaudio
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pDevice;
    const float* pIn = (const float*)pInput;
    float* pOut = (float*)pOutput;

    if (g_boosted_buf.size() < frameCount) {
        g_boosted_buf.resize(frameCount);
        g_tuned_buf.resize(frameCount);
    }

    float gain = g_gain_mult.load();
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        g_boosted_buf[i] = pIn[i] * gain;
    }

    float rms = 0.0f;
    float detected_pitch = g_detector.process(g_boosted_buf.data(), frameCount, rms);

    float target_f0 = 0.0f;
    int note_idx = -1;
    float cent_diff = 0.0f;
    float target_ratio = 1.0f;

    if (detected_pitch > 50.0f) {
        int s_idx = g_scale_idx.load();
        if (s_idx >= 0 && s_idx < (int)g_scales.size()) {
            const ScaleDef& sc = g_scales[s_idx];
            float min_dist = 1e9f;
            for (size_t i = 0; i < sc.freqs.size(); ++i) {
                float dist = std::abs(sc.freqs[i] - detected_pitch);
                if (dist < min_dist) {
                    min_dist = dist;
                    target_f0 = sc.freqs[i];
                    note_idx = (int)i;
                }
            }
        }

        if (target_f0 > 0.0f) {
            float ratio = target_f0 / detected_pitch;
            if (ratio < 0.88f) ratio = 0.88f;
            if (ratio > 1.12f) ratio = 1.12f;

            float speed_factor = 0.80f;
            int rmode = g_retune_mode.load();
            if (rmode == 0) speed_factor = 0.98f;      // Nhanh (Rap 0ms)
            else if (rmode == 1) speed_factor = 0.80f; // Vua (Pop 25ms)
            else if (rmode == 2) speed_factor = 0.45f; // Tu nhien (50ms)

            target_ratio = 1.0f + speed_factor * (ratio - 1.0f);
            cent_diff = 1200.0f * (float)std::log2((double)target_f0 / (double)detected_pitch);
        }
    }

    // Lam tron chuyen dong cua ratio de tranh tieng giat / rach pha
    float smooth_k = (g_retune_mode.load() == 0) ? 0.35f : 0.70f;
    g_smoothed_ratio = smooth_k * g_smoothed_ratio + (1.0f - smooth_k) * target_ratio;

    if (g_enable_autotune.load()) {
        g_shifter.process(g_boosted_buf.data(), g_tuned_buf.data(), frameCount, g_smoothed_ratio);
    } else {
        std::copy(g_boosted_buf.begin(), g_boosted_buf.begin() + frameCount, g_tuned_buf.begin());
    }

    bool echo_on = g_enable_echo.load();
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float sample = g_tuned_buf[i];
        if (echo_on) {
            float delayed = g_echo_buffer[g_echo_idx];
            sample = sample * 0.85f + delayed * 0.28f;
            g_echo_buffer[g_echo_idx] = g_tuned_buf[i] + delayed * 0.28f;
            g_echo_idx = (g_echo_idx + 1) % ECHO_DELAY_SAMPLES;
        }
        float val = std::tanh(sample * 0.90f);
        pOut[i * 2 + 0] = val;
        pOut[i * 2 + 1] = val;
    }

    g_rms.store(rms);
    g_f0.store(detected_pitch);
    g_target_f0.store(target_f0);
    g_note_idx.store(note_idx);
    g_cent.store(cent_diff);
}

// ------------------------------------------------------------------------------
// GIAO DIEN NATIVE WIN32 - TIENG VIET CO DAU CHUAN - KHONG DUNG EMOJI / ICON
// ------------------------------------------------------------------------------
const int WIN_WIDTH = 960;
const int WIN_HEIGHT = 670;

const int PITCH_HISTORY_LEN = 160;
float g_pitch_history[PITCH_HISTORY_LEN] = {0};
int g_pitch_hist_head = 0;

// Toa do nut bam
RECT g_btn_autotune = { 40,  500, 240, 565 };
RECT g_btn_scale    = { 260, 500, 480, 565 };
RECT g_btn_speed    = { 500, 500, 720, 565 };
RECT g_btn_gain     = { 740, 500, 920, 565 };
RECT g_btn_echo     = { 740, 580, 920, 620 };

void DrawRoundedBox(HDC hdc, int left, int top, int right, int bottom, COLORREF fillColor, COLORREF borderColor, int radius = 10) {
    HBRUSH fillBrush = CreateSolidBrush(fillColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

    RoundRect(hdc, left, top, right, bottom, radius, radius);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);
}

void DrawLabelW(HDC hdc, int x, int y, const wchar_t* text, COLORREF color, int fontSize, bool bold = false) {
    HFONT font = CreateFontW(
        fontSize, 0, 0, 0, 
        bold ? FW_BOLD : FW_NORMAL, 
        FALSE, FALSE, FALSE, 
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x, y, text, (int)wcslen(text));
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void DrawCenteredTextW(HDC hdc, RECT rect, const wchar_t* text, COLORREF color, int fontSize, bool bold = false) {
    HFONT font = CreateFontW(
        fontSize, 0, 0, 0, 
        bold ? FW_BOLD : FW_NORMAL, 
        FALSE, FALSE, FALSE, 
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void RenderGUI(HDC hdc, HWND hwnd) {
    (void)hwnd;
    // 1. Double Buffer Bitmap chong rung hinh 100%
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, WIN_WIDTH, WIN_HEIGHT);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    // Mau nen Studio Dark Theme
    COLORREF c_bg        = RGB(13, 17, 23);
    COLORREF c_card      = RGB(22, 27, 34);
    COLORREF c_card_bdr  = RGB(48, 54, 61);
    COLORREF c_cyan      = RGB(0, 229, 255);
    COLORREF c_green     = RGB(16, 185, 129);
    COLORREF c_amber     = RGB(245, 158, 11);
    COLORREF c_red       = RGB(239, 68, 68);
    COLORREF c_text_dim  = RGB(139, 148, 158);
    COLORREF c_text_wh   = RGB(240, 246, 252);
    COLORREF c_meter_bg  = RGB(18, 22, 28);

    // To toan bo nen
    HBRUSH bgBrush = CreateSolidBrush(c_bg);
    RECT fullRect = { 0, 0, WIN_WIDTH, WIN_HEIGHT };
    FillRect(memDC, &fullRect, bgBrush);
    DeleteObject(bgBrush);

    // =========================================================================
    // 1. HEADER BAR
    // =========================================================================
    DrawRoundedBox(memDC, 20, 15, WIN_WIDTH - 20, 78, c_card, c_card_bdr);
    DrawLabelW(memDC, 40, 24, L"KARATUNE PRO", c_cyan, 26, true);
    DrawLabelW(memDC, 40, 52, L"BỘ XỬ LÝ CAO ĐỘ GIỌNG HÁT THỜI GIAN THỰC | C++ NATIVE", c_text_dim, 12, false);

    // Chip thong so ky thuat ben phai
    DrawLabelW(memDC, 630, 27, L"CHUẨN ÂM THANH: WASAPI (48.000 HZ)", c_text_wh, 13, true);
    DrawLabelW(memDC, 630, 48, L"ĐỘ TRỄ PHẦN CỨNG: ~5.3 MS (BUFFER 256 MẪU)", c_green, 12, false);

    // =========================================================================
    // 2. KHU VUC TRUNG TAM: RADAR CAO DO (PITCH RADAR)
    // =========================================================================
    DrawRoundedBox(memDC, 20, 92, 620, 480, c_card, c_card_bdr);
    DrawLabelW(memDC, 40, 106, L"BẢNG THEO DÕI CAO ĐỘ (PITCH RADAR)", c_text_dim, 12, true);

    float f0 = g_f0.load();
    float target = g_target_f0.load();
    float cents = g_cent.load();
    int note_idx = g_note_idx.load();
    int scale_id = g_scale_idx.load();
    bool is_tuned = g_enable_autotune.load();
    const ScaleDef& cur_scale = g_scales[scale_id];

    // Cap nhat bo nho dem song am thanh
    g_pitch_history[g_pitch_hist_head] = (f0 > 50.0f) ? f0 : 0.0f;
    g_pitch_hist_head = (g_pitch_hist_head + 1) % PITCH_HISTORY_LEN;

    if (f0 > 50.0f && note_idx >= 0 && note_idx < (int)cur_scale.names.size()) {
        std::string note_str = cur_scale.names[note_idx];
        std::wstring note_wstr(note_str.begin(), note_str.end());

        // Hien thi Not nhac trung tam co lon
        RECT noteRect = { 40, 130, 600, 245 };
        COLORREF noteColor = (std::abs(cents) <= 6.0f) ? c_green : c_cyan;
        DrawCenteredTextW(memDC, noteRect, note_wstr.c_str(), noteColor, 96, true);

        // Thong so tan so chi tiet bang tieng Viet
        wchar_t freq_buf[128];
        swprintf(freq_buf, 128, L"TẦN SỐ ĐO ĐƯỢC: %5.1f HZ   |   MỤC TIÊU: %5.1f HZ", f0, target);
        RECT freqRect = { 40, 248, 600, 275 };
        DrawCenteredTextW(memDC, freqRect, freq_buf, c_text_wh, 14, false);

        // THANH THUOC DO DO LECH CENT (-50 den +50 Cents)
        int meter_left = 65;
        int meter_right = 575;
        int meter_y = 295;
        int meter_w = meter_right - meter_left;
        int center_x = meter_left + meter_w / 2;

        DrawRoundedBox(memDC, meter_left, meter_y, meter_right, meter_y + 16, c_meter_bg, c_card_bdr, 6);

        // Vach danh dau chia do
        HPEN subTickPen = CreatePen(PS_SOLID, 1, RGB(70, 80, 95));
        SelectObject(memDC, subTickPen);
        int tick_step = meter_w / 4; // -50, -25, 0, +25, +50
        for (int t = 0; t <= 4; ++t) {
            int tx = meter_left + t * tick_step;
            MoveToEx(memDC, tx, meter_y - 2, NULL);
            LineTo(memDC, tx, meter_y + 18);
        }
        DeleteObject(subTickPen);

        // Vach tam giua 0 Cent
        HPEN zeroPen = CreatePen(PS_SOLID, 2, c_green);
        SelectObject(memDC, zeroPen);
        MoveToEx(memDC, center_x, meter_y - 5, NULL);
        LineTo(memDC, center_x, meter_y + 21);
        DeleteObject(zeroPen);

        // Kim chi do lech
        float clamped_cents = std::max(-50.0f, std::min(50.0f, cents));
        int needle_x = center_x + (int)((clamped_cents / 50.0f) * (meter_w / 2));
        
        COLORREF needleColor = (std::abs(cents) <= 6.0f) ? c_green : ((cents > 0) ? c_amber : c_cyan);
        HBRUSH needleBrush = CreateSolidBrush(needleColor);
        RECT needleRect = { needle_x - 4, meter_y - 3, needle_x + 4, meter_y + 19 };
        FillRect(memDC, &needleRect, needleBrush);
        DeleteObject(needleBrush);

        // Nhan trang thai do lech Cent Tieng Viet
        wchar_t cent_txt[128];
        const wchar_t* status_str = (std::abs(cents) <= 6.0f) ? L"CHUẨN CAO ĐỘ" : ((cents > 0) ? L"HƠI NON (TRẦM)" : L"HƠI GIÀ (CAO)");
        swprintf(cent_txt, 128, L"%+3.0f CENTS [%ls]", cents, status_str);
        RECT centRect = { 40, 320, 600, 345 };
        DrawCenteredTextW(memDC, centRect, cent_txt, needleColor, 13, true);

    } else {
        RECT idleRect = { 40, 145, 600, 245 };
        DrawCenteredTextW(memDC, idleRect, L"--", c_text_dim, 96, true);
        RECT idleSub = { 40, 250, 600, 280 };
        DrawCenteredTextW(memDC, idleSub, L"ĐANG CHỜ TÍN HIỆU GIỌNG HÁT TỪ MICRO...", c_text_dim, 14, false);
    }

    // VE DO THI LUYEN GIONG THOI GIAN THUC
    int graph_x = 40;
    int graph_y = 360;
    int graph_w = 560;
    int graph_h = 100;
    DrawRoundedBox(memDC, graph_x, graph_y, graph_x + graph_w, graph_y + graph_h, c_meter_bg, c_card_bdr, 8);
    DrawLabelW(memDC, graph_x + 12, graph_y + 8, L"QUỸ ĐẠO LUYẾN GIỌNG THỜI GIAN THỰC", c_text_dim, 10, true);

    // Cac duong luoi tham chieu nhe
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(25, 32, 42));
    HPEN oldPen = (HPEN)SelectObject(memDC, gridPen);
    for (int g = 1; g <= 3; ++g) {
        int gy = graph_y + (graph_h * g / 4);
        MoveToEx(memDC, graph_x + 6, gy, NULL);
        LineTo(memDC, graph_x + graph_w - 6, gy);
    }
    SelectObject(memDC, oldPen);
    DeleteObject(gridPen);

    // Duong song duoc ve
    HPEN linePen = CreatePen(PS_SOLID, 2, c_cyan);
    oldPen = (HPEN)SelectObject(memDC, linePen);
    bool first_pt = true;

    for (int i = 0; i < PITCH_HISTORY_LEN; ++i) {
        int idx = (g_pitch_hist_head + i) % PITCH_HISTORY_LEN;
        float p = g_pitch_history[idx];
        int px = graph_x + 6 + (int)((float)i / (float)PITCH_HISTORY_LEN * (graph_w - 12));
        
        if (p > 50.0f) {
            float norm = (p - 65.0f) / 550.0f;
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;
            int py = (graph_y + graph_h - 12) - (int)(norm * (graph_h - 30));
            if (first_pt) {
                MoveToEx(memDC, px, py, NULL);
                first_pt = false;
            } else {
                LineTo(memDC, px, py);
            }
        } else {
            first_pt = true;
        }
    }
    SelectObject(memDC, oldPen);
    DeleteObject(linePen);

    // =========================================================================
    // 3. KHU VUC BEN PHAI: COT DO TIN HIEU & AM LUONG (SIGNAL TELEMETRY)
    // =========================================================================
    DrawRoundedBox(memDC, 640, 92, WIN_WIDTH - 20, 480, c_card, c_card_bdr);
    DrawLabelW(memDC, 660, 106, L"THÔNG SỐ TÍN HIỆU & ÂM LƯỢNG", c_text_dim, 12, true);

    float cur_rms = g_rms.load();
    int vu_total_segments = 20;
    int seg_w = 26;
    int seg_h = 9;
    int seg_spacing = 3;
    int vu_top_y = 150;
    int vu_x1 = 675;
    int vu_x2 = 720;

    int lit_segments = (int)(cur_rms * 220.0f);
    if (lit_segments > vu_total_segments) lit_segments = vu_total_segments;

    // Ve 2 cot LED rieng biet (L & R)
    for (int s = 0; s < vu_total_segments; ++s) {
        int seg_y = vu_top_y + (vu_total_segments - 1 - s) * (seg_h + seg_spacing);
        
        COLORREF seg_color_on;
        if (s >= 16) seg_color_on = c_red;
        else if (s >= 12) seg_color_on = c_amber;
        else seg_color_on = c_green;

        COLORREF seg_color_off = RGB(24, 30, 38);

        bool is_lit = (s < lit_segments);
        COLORREF cur_color = is_lit ? seg_color_on : seg_color_off;

        DrawRoundedBox(memDC, vu_x1, seg_y, vu_x1 + seg_w, seg_y + seg_h, cur_color, cur_color, 2);
        DrawRoundedBox(memDC, vu_x2, seg_y, vu_x2 + seg_w, seg_y + seg_h, cur_color, cur_color, 2);
    }

    DrawLabelW(memDC, vu_x1 + 4, vu_top_y + vu_total_segments * (seg_h + seg_spacing) + 8, L"L - TRÁI", c_text_dim, 10, true);
    DrawLabelW(memDC, vu_x2 + 4, vu_top_y + vu_total_segments * (seg_h + seg_spacing) + 8, L"R - PHẢI", c_text_dim, 10, true);

    // Thong tin dBFS & Noise Gate bang Tieng Viet
    float db = (cur_rms > 1e-5f) ? (20.0f * std::log10(cur_rms)) : -60.0f;
    wchar_t db_buf[64];
    swprintf(db_buf, 64, L"%5.1f DBFS", db);
    DrawLabelW(memDC, 770, 160, L"MỨC ĐẦU VÀO:", c_text_dim, 11, false);
    DrawLabelW(memDC, 770, 180, db_buf, c_text_wh, 15, true);

    DrawLabelW(memDC, 770, 220, L"CHỐNG ỒN (GATE):", c_text_dim, 11, false);
    if (cur_rms > 0.005f) {
        DrawLabelW(memDC, 770, 240, L"MỞ (THU TIẾNG)", c_green, 13, true);
    } else {
        DrawLabelW(memDC, 770, 240, L"ĐÓNG (CHẶN ỒN)", c_text_dim, 13, true);
    }

    DrawLabelW(memDC, 770, 280, L"CHẾ ĐỘ XỬ LÝ:", c_text_dim, 11, false);
    if (is_tuned) {
        DrawLabelW(memDC, 770, 300, L"ĐANG BẬT AUTOTUNE", c_cyan, 13, true);
    } else {
        DrawLabelW(memDC, 770, 300, L"TIẾNG MỘC (BYPASS)", c_amber, 13, true);
    }

    // =========================================================================
    // 4. BAN DIEU KHIEN CHUC NANG (INTERACTIVE CONTROL DECK)
    // =========================================================================

    // Nut 1: BẬT / TẮT AUTOTUNE
    COLORREF btn1_bg = is_tuned ? RGB(0, 60, 80) : RGB(30, 36, 45);
    COLORREF btn1_bdr = is_tuned ? c_cyan : c_card_bdr;
    DrawRoundedBox(memDC, g_btn_autotune.left, g_btn_autotune.top, g_btn_autotune.right, g_btn_autotune.bottom, btn1_bg, btn1_bdr, 8);
    DrawCenteredTextW(memDC, g_btn_autotune, is_tuned ? L"AUTOTUNE: ĐANG BẬT" : L"AUTOTUNE: TẮT (MỘC)", is_tuned ? c_cyan : c_text_dim, 13, true);

    // Nut 2: CHON THANG AM (SCALE)
    DrawRoundedBox(memDC, g_btn_scale.left, g_btn_scale.top, g_btn_scale.right, g_btn_scale.bottom, c_card, c_cyan, 8);
    DrawCenteredTextW(memDC, g_btn_scale, cur_scale.name.c_str(), c_text_wh, 12, true);

    // Nut 3: TOC DO BE NOT (RETUNE SPEED)
    const wchar_t* speed_labels[] = { 
        L"TỐC ĐỘ: NHANH (0 MS) - RAP", 
        L"TỐC ĐỘ: VỪA (25 MS) - POP", 
        L"TỐC ĐỘ: TỰ NHIÊN (50 MS)" 
    };
    DrawRoundedBox(memDC, g_btn_speed.left, g_btn_speed.top, g_btn_speed.right, g_btn_speed.bottom, c_card, c_amber, 8);
    DrawCenteredTextW(memDC, g_btn_speed, speed_labels[g_retune_mode.load()], c_amber, 12, true);

    // Nut 4: DO NHAY MIC (GAIN)
    wchar_t gain_txt[64];
    swprintf(gain_txt, 64, L"ĐỘ NHẠY MIC: %3.1fX", g_gain_mult.load());
    DrawRoundedBox(memDC, g_btn_gain.left, g_btn_gain.top, g_btn_gain.right, g_btn_gain.bottom, c_card, c_card_bdr, 8);
    DrawCenteredTextW(memDC, g_btn_gain, gain_txt, c_text_wh, 13, true);

    // Nut 5: HIỆU ỨNG VANG ECHO
    bool echo_act = g_enable_echo.load();
    COLORREF btn5_bg = echo_act ? RGB(10, 50, 35) : RGB(22, 27, 34);
    COLORREF btn5_bdr = echo_act ? c_green : c_card_bdr;
    DrawRoundedBox(memDC, g_btn_echo.left, g_btn_echo.top, g_btn_echo.right, g_btn_echo.bottom, btn5_bg, btn5_bdr, 8);
    DrawCenteredTextW(memDC, g_btn_echo, echo_act ? L"HIỆU ỨNG VANG: BẬT (25%)" : L"HIỆU ỨNG VANG: TẮT", echo_act ? c_green : c_text_dim, 11, true);

    // DONG HUONG DAN BEN DUOI
    DrawLabelW(memDC, 40, 595, L"HƯỚNG DẪN: CLICK CHUỘT VÀO CÁC NÚT ĐỂ ĐIỀU CHỈNH | PHÍM CÁCH (SPACE): BẬT/TẮT AUTOTUNE | PHÍM ESC: THOÁT", c_text_dim, 11, false);

    // Day hinh anh tu bo dem len man hinh
    BitBlt(hdc, 0, 0, WIN_WIDTH, WIN_HEIGHT, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// ------------------------------------------------------------------------------
// XU LY SU KIEN WINDOWS
// ------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            SetTimer(hwnd, 1, 16, NULL); // 60 FPS
            break;
        }
        case WM_TIMER: {
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            POINT pt = { x, y };

            // Nut 1: Toggle AutoTune
            if (PtInRect(&g_btn_autotune, pt)) {
                g_enable_autotune.store(!g_enable_autotune.load());
            }
            // Nut 2: Doi Thang Am (Scale)
            else if (PtInRect(&g_btn_scale, pt)) {
                int next_scale = (g_scale_idx.load() + 1) % (int)g_scales.size();
                g_scale_idx.store(next_scale);
            }
            // Nut 3: Doi Toc do (Speed)
            else if (PtInRect(&g_btn_speed, pt)) {
                int next_speed = (g_retune_mode.load() + 1) % 3;
                g_retune_mode.store(next_speed);
            }
            // Nut 4: Doi Do nhay Mic (Gain)
            else if (PtInRect(&g_btn_gain, pt)) {
                float g = g_gain_mult.load();
                if (g < 6.0f) g = 8.0f;
                else if (g < 10.0f) g = 12.0f;
                else if (g < 14.0f) g = 16.0f;
                else g = 4.0f;
                g_gain_mult.store(g);
            }
            // Nut 5: Toggle Echo
            else if (PtInRect(&g_btn_echo, pt)) {
                g_enable_echo.store(!g_enable_echo.load());
            }

            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            } else if (wParam == VK_SPACE) {
                g_enable_autotune.store(!g_enable_autotune.load());
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderGUI(hdc, hwnd);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------------------------------------------------------------------------
// WINMAIN
// ------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    // Bat che do sac net DPI cao cho man hinh laptop Nitro 5
    SetProcessDPIAware();

    init_scales();

    // Khoi tao Audio Engine WASAPI Duplex
    ma_device_config audioConfig = ma_device_config_init(ma_device_type_duplex);
    audioConfig.capture.format = ma_format_f32;
    audioConfig.capture.channels = CHANNELS_IN;
    audioConfig.playback.format = ma_format_f32;
    audioConfig.playback.channels = CHANNELS_OUT;
    audioConfig.sampleRate = SAMPLE_RATE;
    audioConfig.periodSizeInFrames = BUFFER_FRAMES;
    audioConfig.dataCallback = data_callback;

    ma_device device;
    ma_result res = ma_device_init(NULL, &audioConfig, &device);
    if (res != MA_SUCCESS) {
        MessageBoxW(NULL, L"Không thể khởi động thiết bị âm thanh WASAPI!\nVui lòng kiểm tra Micro hoặc Cáp kết nối.", L"Lỗi Âm Thanh", MB_ICONERROR);
        return -1;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        ma_device_uninit(&device);
        MessageBoxW(NULL, L"Không thể bắt đầu luồng âm thanh thời gian thực!", L"Lỗi Âm Thanh", MB_ICONERROR);
        return -1;
    }

    // Dang ky va tao cua so Win32 GUI
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"KaraTuneStudioGUI";

    RegisterClassExW(&wc);

    // Canh giua man hinh
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screen_w - WIN_WIDTH) / 2;
    int posY = (screen_h - WIN_HEIGHT) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"KaraTuneStudioGUI",
        L"KaraTune Pro - Bộ Xử Lý Cao Độ Giọng Hát Thời Gian Thực (WASAPI < 5.3ms)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, WIN_WIDTH, WIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        ma_device_uninit(&device);
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Vong lap thong diep
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Don dep tai nguyen khi thoat
    ma_device_stop(&device);
    ma_device_uninit(&device);

    return (int)msg.wParam;
}
