# 01. Phân Tích Phần Cứng Thực Tế & Hướng Dẫn Đấu Nối Âm Thanh (Audio I/O)

Tài liệu này được cập nhật chính xác theo **dàn thiết bị âm thanh thực tế tại nhà của bạn** (đã xác thực qua hình ảnh hệ thống), hướng dẫn những phụ kiện cần mua và cách cắm dây chuẩn xác nhất để vừa hát karaoke hay, vừa tận dụng được phần mềm AutoTune thời gian thực.

---

## 1. Danh sách thiết bị hiện có tại nhà của bạn

| STT | Thiết bị | Tên thiết bị / Model | Vai trò trong hệ thống |
|:---:|:---|:---|:---|
| 1 | **Máy tính xử lý** | Laptop Acer Nitro 5 | Chạy phần mềm `karatune.exe` (AutoTune bẻ nốt thời gian thực) và phát nhạc beat karaoke Youtube. |
| 2 | **Bộ tiền khuếch đại (Mixer)** | **Vang cơ Calidona Audio P828** *(Stereo Mixing Digital Echo)* | Nhận tín hiệu Micro, hòa trộn tiếng nhạc, xử lý hiệu ứng vang nhại (Digital Echo / Repeat / Delay). |
| 3 | **Cục đẩy công suất** | **YAMAHA P7000S** *(Power Amplifier)* | Khuếch đại công suất cực lớn để kéo dàn loa ngoài. |
| 4 | **Hệ thống loa chính** | **Cặp loa JBL gia đình** | Phát âm thanh karaoke với công suất mạnh mẽ, âm bass uy lực và độ nhạy cao. |
| 5 | **Micro thu âm** | **Micro dây karaoke (Jack 6.5mm to)** | Micro dynamic có dây, độ bền cao, chống hú tốt. |

---

## 2. Bạn cần mua thêm những gì?

### Phương án TỐI ƯU & TIẾT KIỆM NHẤT: Tận dụng trực tiếp Laptop (Chỉ ~70.000đ - 90.000đ)
> **Khuyên dùng:** Không cần mua Soundcard rời 500.000đ vì gây lãng phí và trùng lặp tính năng! Nhà bạn đã có Vang cơ Calidona P828 chỉnh Echo/Bass/Treble cực hay, và Laptop Nitro 5 đã chạy phần mềm `karatune.exe` xử lý AutoTune thời gian thực siêu mượt (<5.3ms).

*Tổng chi phí: **~70.000đ - 90.000đ** (rẻ hơn 5-6 lần so với mua Soundcard)*

| Phụ kiện cần mua | Tác dụng | Giá tham khảo | Link tìm kiếm Shopee |
|:---|:---|:---:|:---:|
| **1. Cáp chia tai nghe & mic 3.5mm (Y-Splitter)**<br>*(Hoặc củ USB Soundcard mini)* | Cắm vào jack 3.5mm (hoặc cổng USB) của Laptop để tách riêng 1 cổng Mic In và 1 cổng Loa Out. | ~25.000đ - 45.000đ | [Cáp chia tai nghe & mic 3.5mm](https://shopee.vn/search?keyword=cap%20chia%20tai%20nghe%20va%20mic%203.5mm) hoặc [USB Sound Card mini 7.1](https://shopee.vn/search?keyword=usb%20sound%20card%207.1) |
| **2. Đầu chuyển Jack 6.5mm (cái) sang 3.5mm (đực)** | Cắm chân to 6.5mm của Micro dây vào lỗ Micro 3.5mm của cáp chia. | ~10.000đ - 15.000đ | [Jack chuyển 6.5 sang 3.5](https://shopee.vn/search?keyword=jack%20chuyen%206.5%20sang%203.5) |
| **3. Dây 3.5mm ra 2 đầu hoa sen (RCA)** | Dẫn tiếng hát AutoTune từ cổng Loa của cáp chia vào cổng VIDEO (Audio In) sau Vang cơ Calidona. | ~25.000đ - 35.000đ | [Dây 3.5 ra RCA hoa sen](https://shopee.vn/search?keyword=day%203.5%20ra%20hoa%20sen%20rca) |

---

### Khi nào mới cần Soundcard rời? (Không khuyến khích)
Các loại Soundcard livestream ngoài thị trường (như K300, XOX K10, Icon Upod) thường có giá từ 300.000đ đến hơn 500.000đ - 1.000.000đ. Chúng chỉ thực sự cần thiết nếu bạn livestream trên điện thoại hoặc dùng mic thu âm condenser 48V. Với dàn karaoke gia đình đã có sẵn Vang cơ Calidona + Loa JBL, việc bỏ ra 500k mua Soundcard là **hoàn toàn không cần thiết**.

---

### Lưu ý đặc biệt về nguồn nhạc Beat Karaoke từ Smart Tivi:
Nhà bạn đã có **Màn hình Tivi kết nối trực tiếp Youtube**, điều này giúp hệ thống càng tối ưu và chuyên nghiệp hơn:
- **Tivi phát nhạc Beat Youtube:** Xuất âm thanh từ Tivi xuống cổng **TAPE** (hoặc Optical/Bluetooth) ở mặt sau Vang cơ Calidona P828.
- **Laptop Acer Nitro 5:** Đóng vai trò là **Bộ xử lý giọng hát (Vocal Processor)** độc lập, chạy phần mềm `karatune.exe` để bẻ nốt AutoTune rồi xuất vào cổng **VIDEO** của Vang cơ.
- **Vang cơ Calidona P828:** Hòa trộn nhạc Beat từ Tivi với giọng hát AutoTune từ Laptop, đẩy xuống Cục đẩy Yamaha P7000S và phát ra Loa JBL. Laptop không cần tải video Youtube, dành 100% tài nguyên CPU để xử lý AutoTune với độ trễ tối thiểu!

---

## 3. Sơ đồ đấu nối chi tiết (Tiết kiệm nhất & Khuyên dùng)


```
[SMART TIVI (Phát Youtube Beat)]
       │
       ▼ (Dây Optical / Bluetooth hoặc Dây hoa sen RCA)
       │ (Cắm vào cổng TAPE ở mặt sau Vang cơ)
       │
       ├─────────────────────────────────────────────────┐
       │                                                 ▼
[Micro Dây (Jack 6.5mm)]                        [VANG CƠ Calidona P828]
       │                                                 ▲
       ▼ [Đầu chuyển 6.5mm sang 3.5mm]                   │ (Cắm vào cổng VIDEO Audio IN)
       │                                                 │
       ▼ (Cắm vào lỗ MIC của Cáp chia)                   │ [Dây 3.5mm ra 2 đầu Hoa Sen RCA]
[CÁP CHIA Y-SPLITTER 3.5MM (hoặc USB Sound mini)]        │
       ▲                                                 │
       │ (Cắm vào jack 3.5mm hoặc cổng USB của Laptop)   │
[LAPTOP NITRO 5 (Chạy 'karatune.exe')] ──────────────────┘
  - Nhận giọng mộc qua WASAPI (<5.3ms)
  - Xử lý AutoTune thời gian thực
  - Xuất giọng hát đã bẻ nốt qua cổng Loa
                                                         │
                                                         ▼ (Dây Canon XLR / RCA Out)
                                                [CỤC ĐẨY YAMAHA P7000S]
                                                         │
                                                         ▼ (Dây Speakon)
```

---

## 4. Hướng dẫn các nút gạt & cân chỉnh trên dàn máy

1. **Trên Vang cơ Calidona Audio P828:**
   - **Nút chọn nguồn (SELECT):** Nhấn nhả nút **TAPE / VIDEO** ở mặt trước cho đúng với cổng bạn cắm dây hoa sen ở mặt sau.
   - **Music Control:** Vặn nút `VOL` ở mức 40% - 50%, `LOW` (Bass) ở hướng 12h, `MID` hướng 11h, `HIGH` (Treble) hướng 1h để nhạc sáng rõ lời.
   - **Echo Control:** Nếu bạn đã bật AutoTune trên máy tính, hãy để `ECHO VOL` ở mức vừa phải (hướng 10h - 11h) để không bị nhại quá nhiều làm mờ hiệu ứng bẻ nốt.
2. **Trên Cục đẩy Yamaha P7000S:**
   - Vặn 2 núm volume kênh A và kênh B ở mức **hướng 12h đến 2h** (tùy độ to của phòng khách).
   - Đảm bảo đèn `PROTECTION` tắt và đèn `POWER` màu xanh sáng.
3. **Trên phần mềm `karatune.exe`:**
   - Mở phần mềm lên, chọn đúng Soundcard USB hoặc Micro Realtek.
   - Khi cất tiếng hát, âm thanh sẽ đi qua chuỗi AutoTune bẻ nốt mượt mà, truyền vào Vang cơ Calidona, khuếch đại qua Cục đẩy Yamaha và bùng nổ trên cặp loa JBL!
