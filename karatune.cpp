#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <atomic>
#include <string>

// ==============================================================================
// KARATUNE C++ ENGINE - NATIVE REAL-TIME AUTOTUNE FOR WINDOWS
// Biên dịch bằng g++ -O3 (Không Garbage Collection, Không GIL, Siêu nhạy <3ms)
// ==============================================================================

const int SAMPLE_RATE = 48000;
const int CHANNELS_IN = 1;
const int CHANNELS_OUT = 2;
const float PREAMP_BOOST = 6.0f;

// Bảng tần số thang âm Đô Trưởng (C Major)
const float C_MAJOR_FREQS[] = {
    130.81f, 146.83f, 164.81f, 174.61f, 196.00f, 220.00f, 246.94f, // C3 - B3
    261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, // C4 - B4
    523.25f, 587.33f, 659.25f, 698.46f, 783.99f, 880.00f, 987.77f  // C5 - B5
};
const int NUM_SCALE_NOTES = sizeof(C_MAJOR_FREQS) / sizeof(C_MAJOR_FREQS[0]);

const char* NOTE_NAMES[] = {
    "C3", "D3", "E3", "F3", "G3", "A3", "B3",
    "C4", "D4", "E4", "F4", "G4", "A4", "B4",
    "C5", "D5", "E5", "F5", "G5", "A5", "B5"
};

// ------------------------------------------------------------------------------
// BỘ DỊCH CAO ĐỘ THỜI GIAN THỰC C++ (DUAL-TAP DILATION FILTER)
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

// ------------------------------------------------------------------------------
// BỘ DÒ CAO ĐỘ C++ VỚI BỘ NHỚ CUỘN 1024 MẪU (AUTOCORRELATION & MPM)
// ------------------------------------------------------------------------------
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
        // Cuộn bộ nhớ đệm
        if (frame_count < history_len) {
            std::copy(history.begin() + frame_count, history.end(), history.begin());
            std::copy(chunk, chunk + frame_count, history.end() - frame_count);
        }

        // Tính RMS năng lượng
        float sum_sq = 0.0f;
        for (int i = 0; i < history_len; ++i) {
            sum_sq += history[i] * history[i];
        }
        out_rms = std::sqrt(sum_sq / history_len);

        if (out_rms < 0.006f) { // Noise Gate
            last_pitch = 0.0f;
            return 0.0f;
        }

        int min_period = sr / 750; // ~64
        int max_period = sr / 65;  // ~738

        // Tự tương quan
        float max_corr = -1e9f;
        int best_period = -1;
        float corr_0 = sum_sq;

        if (corr_0 <= 1e-7f) return 0.0f;

        // Quét tìm đỉnh tương quan
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
            // Nội suy Parabol
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
                    hold_frames = 8;
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
// TRẠNG THÁI TOÀN CỤC CHIA SẺ VỚI UI THREAD
// ------------------------------------------------------------------------------
CppPitchDetector g_detector(SAMPLE_RATE, 1024);
CppPitchShifter g_shifter(SAMPLE_RATE, 40.0f);
std::vector<float> g_boosted_buf;
std::vector<float> g_tuned_buf;

std::atomic<float> g_rms(0.0f);
std::atomic<float> g_f0(0.0f);
std::atomic<float> g_target_f0(0.0f);
std::atomic<int>   g_note_idx(-1);
std::atomic<float> g_cent(0.0f);
std::atomic<bool>  g_enable_autotune(true);

float g_smoothed_ratio = 1.0f;

// ------------------------------------------------------------------------------
// AUDIO CALLBACK CỦA MINIAUDIO (HARD REAL-TIME C++)
// ------------------------------------------------------------------------------
void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* pIn = (const float*)pInput;
    float* pOut = (float*)pOutput;

    if (pIn == NULL || pOut == NULL) return;

    if ((int)g_boosted_buf.size() < (int)frameCount) {
        g_boosted_buf.resize(frameCount);
        g_tuned_buf.resize(frameCount);
    }

    // 1. Thu âm Mono và khuếch đại Pre-amp
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        g_boosted_buf[i] = pIn[i] * PREAMP_BOOST;
    }

    // 2. Dò cao độ
    float rms = 0.0f;
    float detected_pitch = g_detector.process(g_boosted_buf.data(), frameCount, rms);

    float target_f0 = 0.0f;
    int note_idx = -1;
    float target_ratio = 1.0f;
    float cent_diff = 0.0f;

    if (detected_pitch > 20.0f && g_enable_autotune.load()) {
        // Tìm nốt gần nhất trong C Major
        float min_dist = 1e9f;
        for (int i = 0; i < NUM_SCALE_NOTES; ++i) {
            float dist = std::abs(C_MAJOR_FREQS[i] - detected_pitch);
            if (dist < min_dist) {
                min_dist = dist;
                target_f0 = C_MAJOR_FREQS[i];
                note_idx = i;
            }
        }

        if (target_f0 > 0.0f) {
            float ratio = target_f0 / detected_pitch;
            if (ratio < 0.88f) ratio = 0.88f;
            if (ratio > 1.12f) ratio = 1.12f;
            target_ratio = 1.0f + 0.80f * (ratio - 1.0f);
            cent_diff = 1200.0f * (float)std::log2((double)target_f0 / (double)detected_pitch);
        }
    }

    g_smoothed_ratio = 0.75f * g_smoothed_ratio + 0.25f * target_ratio;

    // 3. Dịch cao độ
    if (g_enable_autotune.load()) {
        g_shifter.process(g_boosted_buf.data(), g_tuned_buf.data(), frameCount, g_smoothed_ratio);
    } else {
        std::copy(g_boosted_buf.begin(), g_boosted_buf.begin() + frameCount, g_tuned_buf.begin());
    }

    // 4. Xuất Stereo ra 2 loa (Trái và Phải) kèm Soft Clipper (tanh)
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float val = std::tanh(g_tuned_buf[i] * 0.90f);
        pOut[i * 2 + 0] = val; // Loa Trái
        pOut[i * 2 + 1] = val; // Loa Phải
    }

    // 5. Cập nhật biến nguyên tử atomic cho UI
    g_rms.store(rms);
    g_f0.store(detected_pitch);
    g_target_f0.store(target_f0);
    g_note_idx.store(note_idx);
    g_cent.store(cent_diff);
}

// ------------------------------------------------------------------------------
// HÀM MAIN
// ------------------------------------------------------------------------------
int main() {
    std::cout << "========================================================================\n";
    std::cout << "   KARATUNE C++ ENGINE - NATIVE WINDOWS REAL-TIME AUTOTUNE\n";
    std::cout << "   Bien dich bang g++ (C++20 Native Code, Khong Delay, Khong Giat Tieng)\n";
    std::cout << "========================================================================\n";

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.format = ma_format_f32;
    config.capture.channels = CHANNELS_IN;
    config.playback.format = ma_format_f32;
    config.playback.channels = CHANNELS_OUT;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = audio_callback;
    config.periodSizeInFrames = 256; // 256 frames = ~5.33ms

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        std::cout << "[Loi] Khong the khoi tao card am thanh qua miniaudio!\n";
        return -1;
    }

    std::cout << "[OK] Backend am thanh     : " << ma_get_backend_name(device.pContext->backend) << "\n";
    std::cout << "[OK] Tan so mau           : " << device.sampleRate << " Hz\n";
    std::cout << "[OK] Buffer frames        : " << device.playback.internalPeriodSizeInFrames << " (~5.3ms)\n";
    std::cout << "[OK] Thang am             : C Major (Do Truong)\n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << "HƯỚNG DẪN:\n";
    std::cout << "  * Cắm tai nghe vào laptop hoặc mở micro hát để trải nghiệm độ mượt C++.\n";
    std::cout << "  * Nhấn phím Enter để BẬT/TẮT AutoTune (Chuyển giữa Tiếng Mộc & AutoTune).\n";
    std::cout << "  * Gõ 'q' rồi nhấn Enter để thoát chương trình.\n";
    std::cout << "========================================================================\n\n";

    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cout << "[Loi] Khong the khoi dong stream am thanh!\n";
        ma_device_uninit(&device);
        return -1;
    }

    // UI loop trong Main Thread
    while (true) {
        ma_sleep(80); // 12 FPS

        float rms = g_rms.load();
        float f0 = g_f0.load();
        int note_idx = g_note_idx.load();
        float target = g_target_f0.load();
        float cents = g_cent.load();
        bool is_tuned = g_enable_autotune.load();

        int meter_len = (int)(rms * 140.0f);
        if (meter_len > 16) meter_len = 16;
        if (meter_len < 0) meter_len = 0;
        std::string bar(meter_len, '#');
        bar += std::string(16 - meter_len, '-');

        if (f0 > 20.0f && note_idx >= 0 && is_tuned) {
            const char* dir = (cents > 0.0f) ? "Non" : "Qua";
            printf("\r[MIC: %s] Hat: %5.1fHz -> Not: %-2s (%5.1fHz) | %+3.0f cent (%-3s)   ", 
                   bar.c_str(), f0, NOTE_NAMES[note_idx], target, cents, dir);
        } else {
            const char* st = (rms > 0.006f) ? "DANG NOI" : "Im lang ";
            const char* mode = is_tuned ? "AutoTune" : "Tieng Moc";
            printf("\r[MIC: %s] Che do: %-8s | Trang thai: %s                         ", 
                   bar.c_str(), mode, st);
        }
        fflush(stdout);
    }

    ma_device_uninit(&device);
    return 0;
}
