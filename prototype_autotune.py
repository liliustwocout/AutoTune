import sys
import numpy as np
import sounddevice as sd
import math
import time

if hasattr(sys.stdout, 'reconfigure'):
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass

# ==============================================================================
# KARATUNE PROTOTYPE V4.3 - CHẾ ĐỘ CHẨN ĐOÁN & KIỂM TRA PHẦN CỨNG (DIAGNOSTIC)
# Cho phép bật/tắt AutoTune tức thì để phân biệt:
# 1. Do Mic Laptop + Loa Laptop bị phần cứng Intel ngắt tiếng (AEC Ducking)?
# 2. Hay do thuật toán bẻ nốt Granular của Python?
# ==============================================================================

SAMPLE_RATE = 48000
BLOCK_SIZE = 256
CHANNELS_IN = 1
CHANNELS_OUT = 2

PREAMP_BOOST = 6.0       # Mức khuếch đại vừa phải để tránh kích hoạt bộ dập âm của laptop
ENABLE_AUTOTUNE = True   # True: Bật AutoTune | False: Tiếng Mộc (Bypass) để test mic

NOTE_NAMES = [
    ("C3", 130.81), ("D3", 146.83), ("E3", 164.81), ("F3", 174.61), ("G3", 196.00), ("A3", 220.00), ("B3", 246.94),
    ("C4", 261.63), ("D4", 293.66), ("E4", 329.63), ("F4", 349.23), ("G4", 392.00), ("A4", 440.00), ("B4", 493.88),
    ("C5", 523.25), ("D5", 587.33), ("E5", 659.25), ("F5", 698.46), ("G5", 783.99), ("A5", 880.00), ("B5", 987.77)
]
SCALE_FREQS = np.array([freq for _, freq in NOTE_NAMES], dtype=np.float32)

# ==============================================================================
# BỘ DỊCH CAO ĐỘ DUAL-TAP
# ==============================================================================
class SmoothDualTapPitchShifter:
    def __init__(self, sample_rate=48000, window_ms=40):
        self.sr = sample_rate
        self.win_size = int(window_ms * sample_rate / 1000.0) # ~1920 samples
        self.buf_size = self.win_size * 4
        self.buffer = np.zeros(self.buf_size, dtype=np.float32)
        self.write_pos = 0
        self.phase = 0.0

    def process(self, chunk, shift_ratio=1.0):
        # Nếu tỉ số dịch nốt quá nhỏ, giữ nguyên âm thanh mộc để không bị gợn
        if abs(shift_ratio - 1.0) < 0.01:
            return chunk

        N = len(chunk)
        w_indices = (self.write_pos + np.arange(N)) % self.buf_size
        self.buffer[w_indices] = chunk
        self.write_pos = (self.write_pos + N) % self.buf_size

        rate = (1.0 - shift_ratio) / self.win_size
        phase_ramp = (self.phase + rate * np.arange(N)) % 1.0
        self.phase = (self.phase + rate * N) % 1.0

        delay1 = phase_ramp * self.win_size
        r1 = (w_indices - delay1) % self.buf_size
        idx1 = (r1.astype(np.int32)) % self.buf_size
        f1 = (r1 - idx1).astype(np.float32)
        s1 = self.buffer[idx1] * (1.0 - f1) + self.buffer[(idx1 + 1) % self.buf_size] * f1
        w1 = 0.5 * (1.0 - np.cos(2.0 * np.pi * phase_ramp))

        p2 = (phase_ramp + 0.5) % 1.0
        delay2 = p2 * self.win_size
        r2 = (w_indices - delay2) % self.buf_size
        idx2 = (r2.astype(np.int32)) % self.buf_size
        f2 = (r2 - idx2).astype(np.float32)
        s2 = self.buffer[idx2] * (1.0 - f2) + self.buffer[(idx2 + 1) % self.buf_size] * f2
        w2 = 0.5 * (1.0 - np.cos(2.0 * np.pi * p2))

        return (s1 * w1 + s2 * w2).astype(np.float32)

pitch_shifter = SmoothDualTapPitchShifter(sample_rate=SAMPLE_RATE)
current_shift_ratio = 1.0

# ==============================================================================
# BỘ DÒ CAO ĐỘ
# ==============================================================================
class ContinuousPitchDetector:
    def __init__(self, sample_rate=48000, history_len=1024):
        self.sr = sample_rate
        self.history_len = history_len
        self.history = np.zeros(history_len, dtype=np.float32)
        self.last_pitch = None
        self.hold_frames = 0

    def process(self, chunk):
        N = len(chunk)
        self.history = np.roll(self.history, -N)
        self.history[-N:] = chunk

        rms = float(np.sqrt(np.mean(self.history**2)))
        if rms < 0.005:
            self.last_pitch = None
            return None, rms

        min_period = int(self.sr / 750)
        max_period = int(self.sr / 65)

        corr = np.correlate(self.history, self.history, mode='full')
        corr = corr[len(corr)//2 :]
        d_corr = corr[min_period:max_period]

        if len(d_corr) < 3 or corr[0] <= 1e-6:
            return None, rms

        raw_peak = int(np.argmax(d_corr))
        peak_idx = raw_peak + min_period

        if corr[peak_idx] > 0.28 * corr[0]:
            a = float(corr[peak_idx - 1])
            b = float(corr[peak_idx])
            c = float(corr[peak_idx + 1])
            denom = 2.0 * (2.0 * b - a - c)
            delta = (c - a) / denom if denom > 1e-5 else 0.0
            delta = max(-0.5, min(0.5, delta))
            refined_period = float(peak_idx) + delta

            if refined_period > 1.0:
                pitch = self.sr / refined_period
                if 65.0 <= pitch <= 850.0:
                    self.last_pitch = pitch
                    self.hold_frames = 8
                    return pitch, rms

        if self.hold_frames > 0 and self.last_pitch is not None:
            self.hold_frames -= 1
            return self.last_pitch, rms

        self.last_pitch = None
        return None, rms

pitch_detector = ContinuousPitchDetector(sample_rate=SAMPLE_RATE)

shared_state = {
    "rms": 0.0,
    "f0": None,
    "target_f0": 0.0,
    "note_name": "",
    "cent_diff": 0.0
}

def audio_callback(indata, outdata, frames, time_info, status):
    global current_shift_ratio, shared_state

    try:
        raw_mic = indata[:, 0]
        boosted_mic = raw_mic * PREAMP_BOOST

        detected_f0, rms = pitch_detector.process(boosted_mic)

        target_ratio = 1.0
        note_name = ""
        target_f0 = 0.0
        cent_diff = 0.0

        if detected_f0 is not None and detected_f0 > 20.0 and ENABLE_AUTOTUNE:
            closest_idx = int(np.argmin(np.abs(SCALE_FREQS - detected_f0)))
            target_f0 = float(SCALE_FREQS[closest_idx])
            note_name = NOTE_NAMES[closest_idx][0]

            ratio = target_f0 / detected_f0
            if ratio > 0.01:
                ratio = max(0.90, min(1.10, ratio))
                target_ratio = 1.0 + 0.75 * (ratio - 1.0)
                cent_diff = 1200.0 * math.log2(target_f0 / detected_f0)

        # Làm mượt chuyển nốt
        current_shift_ratio = 0.80 * current_shift_ratio + 0.20 * target_ratio

        if ENABLE_AUTOTUNE:
            tuned_audio = pitch_shifter.process(boosted_mic, current_shift_ratio)
        else:
            # Chế độ tiếng mộc (Passthrough 100%)
            tuned_audio = boosted_mic

        final_audio = np.tanh(tuned_audio * 0.90)

        outdata[:, 0] = final_audio
        outdata[:, 1] = final_audio

        shared_state["rms"] = float(rms)
        shared_state["f0"] = detected_f0
        shared_state["target_f0"] = target_f0
        shared_state["note_name"] = note_name
        shared_state["cent_diff"] = cent_diff

    except Exception:
        raw = indata[:, 0] * PREAMP_BOOST
        fallback = np.clip(raw, -0.90, 0.90)
        outdata[:, 0] = fallback
        outdata[:, 1] = fallback

def get_wasapi_devices():
    wasapi_id = None
    for i, api in enumerate(sd.query_hostapis()):
        if 'WASAPI' in api['name']:
            wasapi_id = i
            break

    input_dev = None
    output_dev = None

    if wasapi_id is not None:
        for i, dev in enumerate(sd.query_devices()):
            if dev['hostapi'] == wasapi_id:
                name_l = dev['name'].lower()
                if dev['max_input_channels'] > 0 and input_dev is None:
                    if 'intel' in name_l or 'microphone array' in name_l:
                        input_dev = i
                if dev['max_output_channels'] > 0 and output_dev is None:
                    if 'realtek' in name_l or 'speakers' in name_l:
                        output_dev = i

    if input_dev is None:
        input_dev = sd.default.device[0]
    if output_dev is None:
        output_dev = sd.default.device[1]

    return input_dev, output_dev

if __name__ == "__main__":
    print("=" * 72)
    print("   KARATUNE PROTOTYPE V4.3 - BẢN KIỂM ĐỊNH PHẦN CỨNG & CHẤT ÂM")
    print("=" * 72)

    in_dev, out_dev = get_wasapi_devices()
    in_info = sd.query_devices(in_dev)
    out_info = sd.query_devices(out_dev)

    print(f"[OK] Mic Thu Vào   : [{in_dev}] {in_info['name']}")
    print(f"[OK] Loa Xuất Ra   : [{out_dev}] {out_info['name']}")
    print(f"[OK] Trạng Thái    : {'ĐANG BẬT AUTOTUNE' if ENABLE_AUTOTUNE else 'TIẾNG MỘC (BYPASS KHÔNG AUTOTUNE)'}")
    print("-" * 72)
    print("MẸO KIỂM TRA ĐỘ MƯỢT (QUAN TRỌNG):")
    print("  1. Hãy cắm TAI NGHE vào laptop để loại bỏ hoàn toàn việc loa dội vào mic.")
    print("  2. Ngân một nốt dài để cảm nhận sự khác biệt rõ rệt khi không bị dội âm.")
    print("  3. Nhấn Ctrl+C để dừng.")
    print("=" * 72 + "\n")

    try:
        with sd.Stream(
            device=(in_dev, out_dev),
            samplerate=SAMPLE_RATE,
            blocksize=BLOCK_SIZE,
            channels=(CHANNELS_IN, CHANNELS_OUT),
            callback=audio_callback,
            latency='low'
        ):
            while True:
                time.sleep(0.08)
                rms = shared_state["rms"]
                f0 = shared_state["f0"]

                meter_len = int(min(16, rms * 140))
                vu_bar = "#" * meter_len + "-" * (16 - meter_len)

                if f0 is not None and ENABLE_AUTOTUNE:
                    note = shared_state["note_name"]
                    target = shared_state["target_f0"]
                    cents = shared_state["cent_diff"]
                    dir_txt = "Non" if cents > 0 else "Qua"
                    sys.stdout.write(f"\r[MIC: {vu_bar}] Hat: {f0:5.1f}Hz -> Not: {note} ({target:5.1f}Hz) | {cents:+3.0f} cent ({dir_txt})   ")
                else:
                    status = "DANG NOI" if rms > 0.005 else "Im lang "
                    mode_txt = "AutoTune" if ENABLE_AUTOTUNE else "Tieng Moc"
                    sys.stdout.write(f"\r[MIC: {vu_bar}] Che do: {mode_txt} | Trang thai: {status}                      ")
                sys.stdout.flush()

    except KeyboardInterrupt:
        print("\n\n[DA DUNG] Ket thuc chuong trinh thanh cong.")
    except Exception as e:
        print(f"\n[Loi ket noi]: {e}")
