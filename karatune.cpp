#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <windows.h>
#include <vector>
#include <cmath>
#include <atomic>
#include <string>
#include <algorithm>

// ==============================================================================
// KARATUNE - PROFESSIONAL VOCAL AUTOTUNE ENGINE (NATIVE C++ GUI)
// Windows WASAPI Low-Latency Engine + Studio Dark Mode GUI
// STRICT CONSTRAINT: KHONG DUNG EMOJI, KHONG DUNG ICON
// ==============================================================================

const int SAMPLE_RATE = 48000;
const int CHANNELS_IN = 1;
const int CHANNELS_OUT = 2;

// ------------------------------------------------------------------------------
// THANG AM & NOT NHAC
// ------------------------------------------------------------------------------
struct ScaleDef {
    const char* name;
    std::vector<float> freqs;
    std::vector<std::string> names;
};

std::vector<ScaleDef> g_scales;

void init_scales() {
    // 1. C Major (Do Truong)
    ScaleDef c_maj;
    c_maj.name = "C MAJOR (DO TRUONG)";
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

    // 2. A Minor (La Thu)
    ScaleDef a_min;
    a_min.name = "A MINOR (LA THU)";
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

    // 3. Chromatic (Ban Am Toan Phan - 12 Not)
    ScaleDef chrom;
    chrom.name = "CHROMATIC (TAT CA NOT)";
    const char* note_letters[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int midi = 48; midi <= 84; ++midi) { // C3 to C6
        float f = 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
        int oct = (midi / 12) - 1;
        std::string n = std::string(note_letters[midi % 12]) + std::to_string(oct);
        chrom.freqs.push_back(f);
        chrom.names.push_back(n);
    }
    g_scales.push_back(chrom);

    // 4. G Major (Sol Truong)
    ScaleDef g_maj;
    g_maj.name = "G MAJOR (SOL TRUONG)";
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

    CppPitchShifter(int sr = 48000, float window_ms = 40.0f) {
        win_size = (int)(window_ms * sr / 1000.0f);
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

class CppPitchDetector {
public:
    int sr;
    int history_len;
    std::vector<float> history;
    float last_pitch;
    int hold_frames;

    CppPitchDetector(int sample_rate = 48000, int len = 1024) 
        : sr(sample_rate), history_len(len), last_pitch(0.0f), hold_frames(0) {
        history.assign(history_len, 0.0f);
    }

    float process(const float* chunk, int frame_count, float& out_rms) {
        if (frame_count < history_len) {
            std::copy(history.begin() + frame_count, history.end(), history.begin());
            std::copy(chunk, chunk + frame_count, history.end() - frame_count);
        }

        float sum_sq = 0.0f;
        for (int i = 0; i < history_len; ++i) {
            sum_sq += history[i] * history[i];
        }
        out_rms = std::sqrt(sum_sq / history_len);

        if (out_rms < 0.005f) {
            last_pitch = 0.0f;
            return 0.0f;
        }

        int min_period = sr / 750;
        int max_period = sr / 65;

        float max_corr = -1e9f;
        int best_period = -1;
        float corr_0 = sum_sq;

        if (corr_0 <= 1e-7f) return 0.0f;

        std::vector<float> corr(max_period + 2, 0.0f);
        for (int tau = min_period - 1; tau <= max_period + 1; ++tau) {
            float sum = 0.0f;
            for (int j = 0; j < history_len - tau; ++j) {
                sum += history[j] * history[j + tau];
            }
            corr[tau] = sum;
            if (tau >= min_period && tau <= max_period) {
                if (sum > max_corr) {
                    max_corr = sum;
                    best_period = tau;
                }
            }
        }

        if (best_period > 0 && max_corr > 0.28f * corr_0) {
            float a = corr[best_period - 1];
            float b = corr[best_period];
            float c = corr[best_period + 1];
            float denom = 2.0f * (2.0f * b - a - c);
            float delta = 0.0f;
            if (std::abs(denom) > 1e-6f) {
                delta = (c - a) / denom;
                if (delta > 0.5f) delta = 0.5f;
                if (delta < -0.5f) delta = -0.5f;
            }
            float refined_period = (float)best_period + delta;
            if (refined_period > 1.0f) {
                float freq = (float)sr / refined_period;
                if (freq >= 65.0f && freq <= 850.0f) {
                    last_pitch = freq;
                    hold_frames = 10;
                    return freq;
                }
            }
        }

        if (hold_frames > 0 && last_pitch > 0.0f) {
            hold_frames--;
            return last_pitch;
        }

        last_pitch = 0.0f;
        return 0.0f;
    }
};

// ------------------------------------------------------------------------------
// TRANG THAI AUDIO TOAN CUC
// ------------------------------------------------------------------------------
CppPitchDetector g_detector(SAMPLE_RATE, 1024);
CppPitchShifter g_shifter(SAMPLE_RATE, 40.0f);
std::vector<float> g_boosted_buf;
std::vector<float> g_tuned_buf;

std::atomic<float> g_rms(0.0f);
std::atomic<float> g_f0(0.0f);
std::atomic<float> g_target_f0(0.0f);
std::atomic<int>   g_scale_idx(0);
std::atomic<int>   g_note_idx(-1);
std::atomic<float> g_cent(0.0f);
std::atomic<bool>  g_enable_autotune(true);
std::atomic<int>   g_retune_mode(1); // 0: Fast (T-Pain), 1: Med (Pop), 2: Natural
std::atomic<float> g_gain_mult(6.0f); // Preamp gain
std::atomic<bool>  g_enable_echo(false);

float g_smoothed_ratio = 1.0f;

// Echo buffer
const int ECHO_DELAY_SAMPLES = (int)(0.18f * SAMPLE_RATE);
std::vector<float> g_echo_buffer(ECHO_DELAY_SAMPLES, 0.0f);
int g_echo_idx = 0;

void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* pIn = (const float*)pInput;
    float* pOut = (float*)pOutput;

    if (pIn == NULL || pOut == NULL) return;

    if ((int)g_boosted_buf.size() < (int)frameCount) {
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
    float target_ratio = 1.0f;
    float cent_diff = 0.0f;

    int scale_id = g_scale_idx.load();
    if (scale_id < 0 || scale_id >= (int)g_scales.size()) scale_id = 0;
    const ScaleDef& current_scale = g_scales[scale_id];

    if (detected_pitch > 20.0f && g_enable_autotune.load()) {
        float min_dist = 1e9f;
        for (int i = 0; i < (int)current_scale.freqs.size(); ++i) {
            float dist = std::abs(current_scale.freqs[i] - detected_pitch);
            if (dist < min_dist) {
                min_dist = dist;
                target_f0 = current_scale.freqs[i];
                note_idx = i;
            }
        }

        if (target_f0 > 0.0f) {
            float ratio = target_f0 / detected_pitch;
            if (ratio < 0.88f) ratio = 0.88f;
            if (ratio > 1.12f) ratio = 1.12f;

            float speed_factor = 0.80f;
            int rmode = g_retune_mode.load();
            if (rmode == 0) speed_factor = 0.98f; // Fast Robot
            else if (rmode == 1) speed_factor = 0.80f; // Med Pop
            else if (rmode == 2) speed_factor = 0.45f; // Natural

            target_ratio = 1.0f + speed_factor * (ratio - 1.0f);
            cent_diff = 1200.0f * (float)std::log2((double)target_f0 / (double)detected_pitch);
        }
    }

    float smooth_k = (g_retune_mode.load() == 0) ? 0.40f : 0.75f;
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
// GIAO DIEN WINDOWS WIN32 GUI CHUYEN NGHIEP (KHONG DUNG EMOJI, KHONG DUNG ICON)
// ------------------------------------------------------------------------------
const int WIN_WIDTH = 920;
const int WIN_HEIGHT = 640;

// Lich su tan so de ve bieu do song
const int PITCH_HISTORY_LEN = 140;
float g_pitch_history[PITCH_HISTORY_LEN] = {0};
int g_pitch_hist_head = 0;

// Toa do nut bam
RECT g_btn_autotune = { 40,  490, 240, 550 };
RECT g_btn_scale    = { 260, 490, 460, 550 };
RECT g_btn_speed    = { 480, 490, 680, 550 };
RECT g_btn_gain     = { 700, 490, 880, 550 };
RECT g_btn_echo     = { 700, 565, 880, 605 };

void DrawRoundedBox(HDC hdc, int left, int top, int right, int bottom, COLORREF fillColor, COLORREF borderColor) {
    HBRUSH fillBrush = CreateSolidBrush(fillColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

    RoundRect(hdc, left, top, right, bottom, 12, 12);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);
}

void DrawLabel(HDC hdc, int x, int y, const char* text, COLORREF color, int fontSize, bool bold = false) {
    HFONT font = CreateFontA(
        fontSize, 0, 0, 0, 
        bold ? FW_BOLD : FW_NORMAL, 
        FALSE, FALSE, FALSE, 
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, x, y, text, (int)strlen(text));
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void DrawCenteredText(HDC hdc, RECT rect, const char* text, COLORREF color, int fontSize, bool bold = false) {
    HFONT font = CreateFontA(
        fontSize, 0, 0, 0, 
        bold ? FW_BOLD : FW_NORMAL, 
        FALSE, FALSE, FALSE, 
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void RenderGUI(HDC hdc, HWND hwnd) {
    // 1. Double Buffer Bitmap de chong giat man hinh 100%
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, WIN_WIDTH, WIN_HEIGHT);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    // Mau nen Studio Dark Theme
    COLORREF c_bg       = RGB(18, 22, 28);
    COLORREF c_card     = RGB(26, 32, 42);
    COLORREF c_card_bdr = RGB(45, 55, 72);
    COLORREF c_cyan     = RGB(0, 225, 255);
    COLORREF c_green    = RGB(16, 185, 129);
    COLORREF c_amber    = RGB(245, 158, 11);
    COLORREF c_red      = RGB(239, 68, 68);
    COLORREF c_text_dim = RGB(140, 155, 175);
    COLORREF c_text_wh  = RGB(240, 245, 250);

    // To toan bo nen
    HBRUSH bgBrush = CreateSolidBrush(c_bg);
    RECT fullRect = { 0, 0, WIN_WIDTH, WIN_HEIGHT };
    FillRect(memDC, &fullRect, bgBrush);
    DeleteObject(bgBrush);

    // HEADER BAR
    DrawRoundedBox(memDC, 20, 15, WIN_WIDTH - 20, 75, c_card, c_card_bdr);
    DrawLabel(memDC, 40, 25, "KARATUNE", c_cyan, 26, true);
    DrawLabel(memDC, 40, 52, "PROFESSIONAL VOCAL ENGINE | NATIVE LOW-LATENCY C++", c_text_dim, 12, false);

    DrawLabel(memDC, 630, 28, "WASAPI EXCLUSIVE / 48000 HZ", c_text_wh, 13, true);
    DrawLabel(memDC, 630, 48, "BUFFER: 256 SAMPLES (~5.3 MS)", c_green, 12, false);

    // =========================================================================
    // KHU VUC TRUNG TAM: MAN HINH HIEN THI NOT NHAC (PITCH RADAR)
    // =========================================================================
    DrawRoundedBox(memDC, 20, 90, 590, 460, c_card, c_card_bdr);
    DrawLabel(memDC, 40, 105, "PITCH CORRECTION RADAR", c_text_dim, 12, true);

    float f0 = g_f0.load();
    float target = g_target_f0.load();
    float cents = g_cent.load();
    int note_idx = g_note_idx.load();
    int scale_id = g_scale_idx.load();
    bool is_tuned = g_enable_autotune.load();
    const ScaleDef& cur_scale = g_scales[scale_id];

    // Cap nhat lich su song de ve do thi
    g_pitch_history[g_pitch_hist_head] = (f0 > 20.0f) ? f0 : 0.0f;
    g_pitch_hist_head = (g_pitch_hist_head + 1) % PITCH_HISTORY_LEN;

    if (f0 > 20.0f && note_idx >= 0 && note_idx < (int)cur_scale.names.size()) {
        std::string note_str = cur_scale.names[note_idx];
        
        // Hien thi Not nhac to o giua
        RECT noteRect = { 40, 130, 570, 240 };
        COLORREF noteColor = (std::abs(cents) <= 6.0f) ? c_green : c_cyan;
        DrawCenteredText(memDC, noteRect, note_str.c_str(), noteColor, 90, true);

        // Thong so tan so
        char freq_buf[128];
        sprintf(freq_buf, "DETECTED: %5.1f HZ   |   TARGET: %5.1f HZ", f0, target);
        RECT freqRect = { 40, 235, 570, 260 };
        DrawCenteredText(memDC, freqRect, freq_buf, c_text_wh, 15, false);

        // THANH THUOC DO DO LECH CENT (-50 den +50 Cents)
        int meter_left = 70;
        int meter_right = 540;
        int meter_y = 285;
        int meter_w = meter_right - meter_left;
        int center_x = meter_left + meter_w / 2;

        // Nen thanh do
        DrawRoundedBox(memDC, meter_left, meter_y, meter_right, meter_y + 16, RGB(18, 22, 28), c_card_bdr);

        // Vach giua 0 Cent
        HPEN zeroPen = CreatePen(PS_SOLID, 2, c_green);
        SelectObject(memDC, zeroPen);
        MoveToEx(memDC, center_x, meter_y - 4, NULL);
        LineTo(memDC, center_x, meter_y + 20);
        DeleteObject(zeroPen);

        // Con tro kim do do lech
        float clamped_cents = std::max(-50.0f, std::min(50.0f, cents));
        int needle_x = center_x + (int)((clamped_cents / 50.0f) * (meter_w / 2));
        
        COLORREF needleColor = (std::abs(cents) <= 6.0f) ? c_green : ((cents > 0) ? c_amber : c_cyan);
        HBRUSH needleBrush = CreateSolidBrush(needleColor);
        RECT needleRect = { needle_x - 4, meter_y - 2, needle_x + 4, meter_y + 18 };
        FillRect(memDC, &needleRect, needleBrush);
        DeleteObject(needleBrush);

        // Chu thich cent
        char cent_txt[64];
        const char* status_str = (std::abs(cents) <= 6.0f) ? "IN TUNE" : ((cents > 0) ? "FLAT (-)" : "SHARP (+)");
        sprintf(cent_txt, "%+3.0f CENTS [%s]", cents, status_str);
        RECT centRect = { 40, 310, 570, 335 };
        DrawCenteredText(memDC, centRect, cent_txt, needleColor, 14, true);

    } else {
        // Trang thai im lang / cho tin hieu
        RECT idleRect = { 40, 140, 570, 240 };
        DrawCenteredText(memDC, idleRect, "--", c_text_dim, 90, true);
        RECT idleSub = { 40, 240, 570, 270 };
        DrawCenteredText(memDC, idleSub, "WAITING FOR VOCAL INPUT...", c_text_dim, 14, false);
    }

    // VE BIEU DO SONG CAO DO (PITCH TRAJECTORY STRIP)
    int graph_x = 40;
    int graph_y = 350;
    int graph_w = 530;
    int graph_h = 90;
    DrawRoundedBox(memDC, graph_x, graph_y, graph_x + graph_w, graph_y + graph_h, RGB(18, 22, 28), c_card_bdr);
    DrawLabel(memDC, graph_x + 10, graph_y + 6, "REAL-TIME TRAJECTORY", c_text_dim, 10, true);

    HPEN linePen = CreatePen(PS_SOLID, 2, c_cyan);
    HPEN oldPen = (HPEN)SelectObject(memDC, linePen);
    bool first_pt = true;

    for (int i = 0; i < PITCH_HISTORY_LEN; ++i) {
        int idx = (g_pitch_hist_head + i) % PITCH_HISTORY_LEN;
        float p = g_pitch_history[idx];
        int px = graph_x + (int)((float)i / (float)PITCH_HISTORY_LEN * graph_w);
        
        if (p > 60.0f) {
            float norm = (p - 100.0f) / 500.0f; // 100Hz den 600Hz
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;
            int py = (graph_y + graph_h - 10) - (int)(norm * (graph_h - 25));
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
    // KHU VUC BEN PHAI: VU METERS & TRANG THAI AUDIO
    // =========================================================================
    DrawRoundedBox(memDC, 610, 90, WIN_WIDTH - 20, 460, c_card, c_card_bdr);
    DrawLabel(memDC, 630, 105, "SIGNAL TELEMETRY", c_text_dim, 12, true);

    float cur_rms = g_rms.load();
    int vu_h = 240;
    int vu_w = 28;
    int vu_x1 = 660;
    int vu_x2 = 720;
    int vu_y = 150;

    // Thanh VU Trai & Phai
    DrawRoundedBox(memDC, vu_x1, vu_y, vu_x1 + vu_w, vu_y + vu_h, RGB(18, 22, 28), c_card_bdr);
    DrawRoundedBox(memDC, vu_x2, vu_y, vu_x2 + vu_w, vu_y + vu_h, RGB(18, 22, 28), c_card_bdr);

    int fill_h = (int)(cur_rms * 2800.0f);
    if (fill_h > vu_h - 4) fill_h = vu_h - 4;
    if (fill_h > 0) {
        COLORREF vu_color = (fill_h > vu_h * 0.85) ? c_red : ((fill_h > vu_h * 0.6) ? c_amber : c_green);
        HBRUSH vuBrush = CreateSolidBrush(vu_color);
        RECT r_left  = { vu_x1 + 3, (vu_y + vu_h - 2) - fill_h, vu_x1 + vu_w - 3, vu_y + vu_h - 2 };
        RECT r_right = { vu_x2 + 3, (vu_y + vu_h - 2) - fill_h, vu_x2 + vu_w - 3, vu_y + vu_h - 2 };
        FillRect(memDC, &r_left, vuBrush);
        FillRect(memDC, &r_right, vuBrush);
        DeleteObject(vuBrush);
    }

    DrawLabel(memDC, vu_x1 + 6, vu_y + vu_h + 8, "L-IN", c_text_dim, 11, true);
    DrawLabel(memDC, vu_x2 + 6, vu_y + vu_h + 8, "R-IN", c_text_dim, 11, true);

    // Thong so dB & Noise Gate
    float db = (cur_rms > 1e-5f) ? (20.0f * std::log10(cur_rms)) : -60.0f;
    char db_buf[32];
    sprintf(db_buf, "%5.1f DBFS", db);
    DrawLabel(memDC, 770, 180, "INPUT LEVEL:", c_text_dim, 11, false);
    DrawLabel(memDC, 770, 200, db_buf, c_text_wh, 14, true);

    DrawLabel(memDC, 770, 240, "NOISE GATE:", c_text_dim, 11, false);
    if (cur_rms > 0.006f) {
        DrawLabel(memDC, 770, 260, "OPEN", c_green, 14, true);
    } else {
        DrawLabel(memDC, 770, 260, "CLOSED", c_text_dim, 14, true);
    }

    DrawLabel(memDC, 770, 300, "PROCESSING:", c_text_dim, 11, false);
    DrawLabel(memDC, 770, 320, is_tuned ? "AUTOTUNE" : "BYPASS", is_tuned ? c_cyan : c_amber, 14, true);

    // =========================================================================
    // BAN DIEU KHIEN BEN DUOI (INTERACTIVE CONTROL DECK)
    // =========================================================================

    // Nut 1: BẬT/TẮT AUTOTUNE
    COLORREF btn1_bg = is_tuned ? RGB(0, 70, 90) : RGB(40, 45, 55);
    COLORREF btn1_bdr = is_tuned ? c_cyan : c_card_bdr;
    DrawRoundedBox(memDC, g_btn_autotune.left, g_btn_autotune.top, g_btn_autotune.right, g_btn_autotune.bottom, btn1_bg, btn1_bdr);
    DrawCenteredText(memDC, g_btn_autotune, is_tuned ? "AUTOTUNE: ON" : "AUTOTUNE: BYPASS", is_tuned ? c_cyan : c_text_dim, 14, true);

    // Nut 2: CHON THANG AM (SCALE)
    DrawRoundedBox(memDC, g_btn_scale.left, g_btn_scale.top, g_btn_scale.right, g_btn_scale.bottom, c_card, c_cyan);
    DrawCenteredText(memDC, g_btn_scale, cur_scale.name, c_text_wh, 12, true);

    // Nut 3: RETUNE SPEED
    const char* speed_labels[] = { "SPEED: HARD (0 MS)", "SPEED: POP (25 MS)", "SPEED: NATURAL (50 MS)" };
    DrawRoundedBox(memDC, g_btn_speed.left, g_btn_speed.top, g_btn_speed.right, g_btn_speed.bottom, c_card, c_amber);
    DrawCenteredText(memDC, g_btn_speed, speed_labels[g_retune_mode.load()], c_amber, 12, true);

    // Nut 4: INPUT GAIN
    char gain_txt[32];
    sprintf(gain_txt, "GAIN: %3.1fX", g_gain_mult.load());
    DrawRoundedBox(memDC, g_btn_gain.left, g_btn_gain.top, g_btn_gain.right, g_btn_gain.bottom, c_card, c_card_bdr);
    DrawCenteredText(memDC, g_btn_gain, gain_txt, c_text_wh, 13, true);

    // Nut 5: KARAOKE ECHO
    bool echo_act = g_enable_echo.load();
    COLORREF btn5_bg = echo_act ? RGB(10, 60, 40) : RGB(26, 32, 42);
    COLORREF btn5_bdr = echo_act ? c_green : c_card_bdr;
    DrawRoundedBox(memDC, g_btn_echo.left, g_btn_echo.top, g_btn_echo.right, g_btn_echo.bottom, btn5_bg, btn5_bdr);
    DrawCenteredText(memDC, g_btn_echo, echo_act ? "ECHO: 25% (ON)" : "ECHO: OFF", echo_act ? c_green : c_text_dim, 11, true);

    // Chi dan ben duoi
    DrawLabel(memDC, 40, 575, "CLICK CAC NUT TREN MAN HINH DE THAY DOI CHE DO HOAC NHAN PHIM ESC DE THOAT", c_text_dim, 11, false);

    // Day hinh anh len man hinh (BitBlt)
    BitBlt(hdc, 0, 0, WIN_WIDTH, WIN_HEIGHT, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// ------------------------------------------------------------------------------
// WINDOWS MESSAGE HANDLER
// ------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            SetTimer(hwnd, 1, 16, NULL); // 60 FPS Refresh
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

            // Click Nut 1: Toggle AutoTune
            if (PtInRect(&g_btn_autotune, pt)) {
                g_enable_autotune.store(!g_enable_autotune.load());
            }
            // Click Nut 2: Cycle Thang Am
            else if (PtInRect(&g_btn_scale, pt)) {
                int next_scale = (g_scale_idx.load() + 1) % (int)g_scales.size();
                g_scale_idx.store(next_scale);
            }
            // Click Nut 3: Cycle Retune Speed
            else if (PtInRect(&g_btn_speed, pt)) {
                int next_speed = (g_retune_mode.load() + 1) % 3;
                g_retune_mode.store(next_speed);
            }
            // Click Nut 4: Cycle Gain
            else if (PtInRect(&g_btn_gain, pt)) {
                float g = g_gain_mult.load();
                if (g < 5.0f) g = 6.0f;
                else if (g < 7.0f) g = 8.0f;
                else if (g < 9.0f) g = 10.0f;
                else g = 4.0f;
                g_gain_mult.store(g);
            }
            // Click Nut 5: Toggle Echo
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
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------------------------------------------------------------------------
// WINMAIN
// ------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    init_scales();

    // 1. Khoi tao Audio Engine miniaudio WASAPI Duplex
    ma_device_config audioConfig = ma_device_config_init(ma_device_type_duplex);
    audioConfig.capture.format = ma_format_f32;
    audioConfig.capture.channels = CHANNELS_IN;
    audioConfig.playback.format = ma_format_f32;
    audioConfig.playback.channels = CHANNELS_OUT;
    audioConfig.sampleRate = SAMPLE_RATE;
    audioConfig.dataCallback = audio_callback;
    audioConfig.periodSizeInFrames = 256;

    ma_device audioDevice;
    if (ma_device_init(NULL, &audioConfig, &audioDevice) != MA_SUCCESS) {
        MessageBoxA(NULL, "Khong the khoi tao card am thanh qua Windows WASAPI!", "Loi Khoi Dong", MB_ICONERROR);
        return -1;
    }

    if (ma_device_start(&audioDevice) != MA_SUCCESS) {
        MessageBoxA(NULL, "Khong the bat stream am thanh!", "Loi Khoi Dong", MB_ICONERROR);
        ma_device_uninit(&audioDevice);
        return -1;
    }

    // 2. Dang ky lop Cua so Windows Native
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "KaraTuneWindowClass";

    RegisterClassExA(&wc);

    // Can giua man hinh
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int pos_x = (screen_w - WIN_WIDTH) / 2;
    int pos_y = (screen_h - WIN_HEIGHT) / 2;

    HWND hwnd = CreateWindowExA(
        0,
        "KaraTuneWindowClass",
        "KARATUNE - PRO VOCAL ENGINE",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        pos_x, pos_y, WIN_WIDTH, WIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 3. Vong lap Message Pump Windows
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // 4. Don dep khi thoat
    ma_device_stop(&audioDevice);
    ma_device_uninit(&audioDevice);

    return (int)msg.wParam;
}
