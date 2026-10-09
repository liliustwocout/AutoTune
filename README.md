# KaraTune - Phần Mềm AutoTune Karaoke Thời Gian Thực (Low-Latency)

Hệ thống xử lý và hiệu chỉnh cao độ giọng hát tự động thời gian thực (Real-time Pitch Correction) tối ưu hóa cho máy tính cá nhân (Acer Nitro 5) kết hợp dàn âm thanh karaoke gia đình thực tế: **Vang cơ Calidona P828, Cục đẩy Yamaha P7000S và Cặp loa JBL**.

---

## 1. Giới thiệu

KaraTune giải quyết các vấn đề thường gặp khi hát karaoke gia đình:
- Tự động phát hiện và bẻ các nốt hát chênh, phô về đúng thang âm bài hát (C Major, A Minor...).
- Tích hợp chuỗi hiệu ứng âm thanh karaoke (Pre-amp Boost, Noise Gate, Soft Clipper, Echo/Reverb).
- Tối ưu hóa độ trễ cực thấp (< 5.3ms) qua Windows WASAPI native C++, loại bỏ hiện tượng nói lắp hoặc giật tiếng.

---

## 2. Cấu trúc dự án

```
AutoTune/
├── docs/                        # Tài liệu kỹ thuật chi tiết
│   ├── README.md                # Tổng quan kiến trúc & thiết bị
│   ├── 01_HARDWARE_AND_AUDIO_IO.md  # [CẬP NHẬT] Hướng dẫn đấu nối dàn Calidona P828 + Yamaha P7000S + JBL
│   ├── 02_CORE_DSP_PIPELINE.md  # Thuật toán DSP, YIN/MPM, PSOLA
│   ├── 03_SOFTWARE_ARCHITECTURE.md # Kiến trúc đa luồng C++ vs Python
│   ├── 04_FEATURES_SPEC.md      # Đặc tả tính năng AutoTune & Presets
│   ├── 05_DEVELOPMENT_ROADMAP.md # Lộ trình triển khai
│   └── 06_QUICK_START_PROTOTYPE.md # Hướng dẫn chạy thử nghiệm & chẩn đoán
│
├── karatune.cpp                 # Mã nguồn C++ Native hoàn chỉnh (Khuyên dùng)
├── miniaudio.h                  # Thư viện âm thanh single-header cho C++
├── prototype_autotune.py        # Bản thử nghiệm Prototype bằng Python
└── .gitignore                   # Cấu hình bỏ qua file nhị phân
```

---

## 3. Hướng dẫn cài đặt & Chạy ứng dụng

### Cách 1: Chạy bản C++ Native (Tối ưu nhất - Siêu mượt, không trễ)
Yêu cầu: Đã cài `g++` (MinGW / MSYS2 UCRT64).

1. **Biên dịch:**
   ```bash
   g++ -O3 -mavx2 karatune.cpp -o karatune.exe -lole32
   ```
2. **Khởi chạy:**
   ```bash
   .\karatune.exe
   ```

### Cách 2: Chạy bản Python Prototype (Thử nghiệm nhanh)
Yêu cầu: Python 3.10+
```bash
pip install sounddevice numpy scipy
python prototype_autotune.py
```

---

## 4. Hướng dẫn phối ghép dàn thiết bị gia đình thực tế
Chi tiết cách chọn Soundcard K300/XOX K10, dây 3.5 ra RCA hoa sen và cách cắm vào Vang cơ Calidona P828 + Cục đẩy Yamaha P7000S xem tại:
👉 [docs/01_HARDWARE_AND_AUDIO_IO.md](file:///g:/Project/AutoTune/docs/01_HARDWARE_AND_AUDIO_IO.md)
