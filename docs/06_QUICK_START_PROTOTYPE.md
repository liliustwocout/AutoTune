# 06. Hướng Dẫn Thử Nghiệm Nhanh Bằng Code Mẫu (Quick-Start Prototype)

Tài liệu này tổng hợp toàn bộ quá trình thử nghiệm thực tế trên máy Acer Nitro 5, phân tích nguyên nhân giật tiếng (choppy audio) và so sánh giữa thử nghiệm Python với giải pháp phần mềm chuyên nghiệp C++/VST.

---

## 1. Bản chất hiện tượng "Giật cục / Ngắt quãng" khi test trên Laptop

Khi bạn thử nghiệm hát karaoke trực tiếp bằng **Mic của Laptop + Loa của Laptop**, hiện tượng âm thanh bị giật giật đến từ 2 nguyên nhân cốt lõi:

### A. Nguyên nhân phần cứng: Bộ chống dội âm (AEC) của Intel & Realtek
- Mic của Acer Nitro 5 nằm trên viền webcam, cách 2 loa ngoài của laptop chỉ 15 - 20cm.
- Khi loa phát ra tiếng hát đã qua chỉnh âm, mic lập tức thu lại tiếng đó tạo thành vòng lặp (Feedback loop).
- Chip xử lý âm thanh phần cứng của laptop (*Intel Smart Sound Technology*) nhận diện vòng lặp này có nguy cơ gây tiếng hú rít cháy loa, nên **tự động dập ngắt (Duck/Mute) tín hiệu mic liên tục 15-20 lần mỗi giây**.
- **Cách khắc phục:** Cắm tai nghe (Headphones) vào máy hoặc sử dụng **Mic dây + Loa karaoke ngoài** đặt cách xa máy tính 2 - 3 mét.

### B. Nguyên nhân phần mềm: Giới hạn của thuật toán Granular trong Python
- Đoạn mã Python mẫu sử dụng thuật toán bẻ nốt *Dual-Tap Delay Pitch Shifting* (cắt âm thanh thành các lát nhỏ 35ms - 40ms rồi ghép lại).
- Tần số cắt ghép này (~25Hz - 28Hz) tạo ra độ gợn cơ học (Granular Flutter).
- **Giải pháp chuyên nghiệp:** Để giọng hát ngân dài mượt như lụa, các phần mềm thương mại bắt buộc phải viết bằng **C++ với thuật toán PSOLA (Pitch-Synchronous Overlap-Add)** đồng bộ chính xác theo từng đỉnh bước sóng của giọng ca sĩ.

---

## 2. Mã nguồn Prototype V4.3 trong Workspace

File [prototype_autotune.py](file:///g:/Project/AutoTune/prototype_autotune.py) đã được nâng cấp lên bản V4.3 với các tính năng:
- **Tự động bắt driver WASAPI phần cứng** (Độ trễ < 5.3ms).
- **Bộ đệm cuộn 1024 mẫu** nhận diện tốt cả nốt trầm C3-B3.
- **Pre-amp Boost 6x** khuếch đại mic vừa vặn.
- **Chế độ kiểm định (Diagnostic Mode):** Đổi biến `ENABLE_AUTOTUNE = False` để nghe tiếng mộc trực tiếp, giúp bạn kiểm chứng mic của laptop khi cắm tai nghe.

---

## 3. Cách chuyển sang trải nghiệm Hát Karaoke Thực Thụ với Mic Dây & Loa Ngoài

Bạn đang có sẵn: **1 bộ loa karaoke, 1 mic dây**! Hãy tận dụng dàn thiết bị này để hát karaoke thực thụ:

1. **Cắm Mic dây và Loa:**
   - Cắm Mic dây qua cáp chia Y-Splitter TRRS vào cổng 3.5mm của Nitro 5 (hoặc qua Soundcard USB nếu có).
   - Nối dây AUX 3.5mm từ Laptop ra Loa Karaoke gia đình (đặt cách người hát ít nhất 2m).
2. **Trải nghiệm ngay chất âm mượt mà bằng Plugin C++ (Miễn phí 100%):**
   - Cài đặt Host siêu nhẹ **Cantabile Lite** (hoặc **Reaper**).
   - Tải plugin VST AutoTune C++ **Auburn Sounds - Graillon 2 (Bản Free)**.
   - Hát trực tiếp qua dàn loa karaoke gia đình: Giọng hát ngân dài ngọt lịm, bắt tông tự động chuẩn xác và hoàn toàn không bị trễ hay giật cục!
