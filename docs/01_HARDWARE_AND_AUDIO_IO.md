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

Tùy vào nhu cầu hát của bạn, có 2 phương án đầu tư:

### Phương án A: Muốn hát AutoTune thời gian thực chuyên nghiệp nhất (Khuyên dùng)
*Tổng chi phí: ~320.000đ*

| Phụ kiện cần mua | Tác dụng | Giá tham khảo | Link mua tham khảo |
|:---|:---|:---:|:---:|
| **1. Soundcard USB (K300 hoặc XOX K10)** | - Cắm mic dây trực tiếp (chân 6.5mm có sẵn).<br>- Có núm vặn to nhỏ Gain mic, Echo, Bass, Treble bằng tay.<br>- Kết nối USB với Laptop làm card thu âm không trễ.<br>- Có sẵn nút AutoTune cứng ăn liền. | ~280.000đ | [Soundcard K300 Shopee](https://shopee.vn/search?keyword=soundcard%20k300) / [XOX K10 Shopee](https://shopee.vn/search?keyword=soundcard%20xox%20k10) |
| **2. Dây 3.5mm ra 2 đầu hoa sen (RCA)** | Dẫn âm thanh từ cổng OUT của Soundcard vào cổng TAPE/VIDEO mặt sau Vang cơ Calidona. | ~35.000đ | [Dây 3.5 ra RCA hoa sen](https://shopee.vn/search?keyword=day%203.5%20ra%20hoa%20sen%20rca) |

---

### Phương án B: Tận dụng trực tiếp Laptop không mua Soundcard (Tiết kiệm nhất)
*Tổng chi phí: ~90.000đ*

| Phụ kiện cần mua | Tác dụng | Giá tham khảo | Link mua tham khảo |
|:---|:---|:---:|:---:|
| **1. Cáp chia tai nghe & mic 3.5mm (Ugreen AV140)** | Cắm vào jack 3.5mm của Nitro 5 để tách riêng 1 cổng Mic In và 1 cổng Loa Out. | ~65.000đ | [Cáp chia Ugreen AV140](https://shopee.vn/search?keyword=ugreen%20av140) |
| **2. Đầu chuyển Jack 6.5mm (cái) sang 3.5mm (đực)** | Cắm chân to 6.5mm của mic dây vào lỗ Micro 3.5mm của cáp chia. | ~20.000đ | [Jack 6.5 cái sang 3.5 đực](https://shopee.vn/search?keyword=jack%206.5%20cai%20sang%203.5%20duc) |
| **3. Dây 3.5mm ra 2 đầu hoa sen (RCA)** | Dẫn tiếng từ cổng Loa của cáp chia vào mặt sau Vang cơ Calidona. | ~35.000đ | [Dây 3.5 ra RCA hoa sen](https://shopee.vn/search?keyword=day%203.5%20ra%20hoa%20sen%20rca) |

---

### Phương án C: Hát Karaoke truyền thống qua Dàn máy (Không cần AutoTune máy tính)
*Tổng chi phí: ~30.000đ*
- Chỉ cần mua duy nhất **1 sợi Dây 3.5mm ra 2 đầu hoa sen (RCA)** để lấy nhạc Youtube từ laptop Nitro 5 vào dàn loa JBL. Mic dây cắm trực tiếp vào Vang cơ Calidona như bình thường.

---

## 3. Sơ đồ đấu nối chi tiết (Hardware Wiring Diagrams)

### SƠ ĐỒ 1: Hát AutoTune với Soundcard USB (Chuẩn nhất)

```
[Micro Dây (Chân 6.5mm)]
       │
       ▼ (Cắm vào cổng MIC 1 trên mặt trước Soundcard)
[SOUNDCARD USB (K300 / XOX K10)] ◄────── (Dây cáp USB cắm vào cổng USB của Laptop Nitro 5)
       │                                     Laptop chạy phần mềm 'karatune.exe'
       │
       ▼ (Cắm vào cổng OUT 3.5mm của Soundcard)
[Dây 3.5mm ra 2 đầu Hoa Sen (RCA)]
       │
       ▼ (Cắm vào cổng TAPE hoặc VIDEO ở mặt sau)
[VANG CƠ Calidona P828] ──(Nhấn nút chọn cổng TAPE/VIDEO ở mặt trước)
       │
       ▼ (Dây Canon XLR hoặc Hoa Sen từ cổng Out của Vang cơ)
[CỤC ĐẨY CÔNG SUẤT Yamaha P7000S]
       │
       ▼ (Dây loa Speakon)
[CẶP LOA CHÍNH JBL]
```

---

### SƠ ĐỒ 2: Hát AutoTune trực tiếp qua Laptop (Dùng Cáp chia Y-Splitter)

```
[Micro Dây] ──► [Đầu chuyển 6.5mm cái sang 3.5mm đực]
                       │
                       ▼ (Cắm vào lỗ MICRO trên cáp chia)
[Cáp Chia Ugreen AV140] ◄──────── Cắm đầu đực 3.5mm vào Laptop Nitro 5
       │ (Lỗ TAI NGHE / LOA trên cáp chia)
       ▼
[Dây 3.5mm ra 2 đầu Hoa Sen RCA]
       │
       ▼ (Cắm vào cổng TAPE hoặc VIDEO ở mặt sau)
[VANG CƠ Calidona P828]
       │
       ▼
[CỤC ĐẨY YAMAHA P7000S]
       │
       ▼
[CẶP LOA JBL]
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
