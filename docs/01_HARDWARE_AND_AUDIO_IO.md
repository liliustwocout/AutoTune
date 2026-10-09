# 01. Phân Tích Phần Cứng & Kết Nối Thiết Bị (Audio I/O)

Tài liệu này hướng dẫn cách kết nối và tối ưu dàn thiết bị hiện có tại nhà của bạn để phần mềm AutoTune hoạt động với **độ trễ siêu thấp (Low Latency)** và **chất lượng âm thanh tốt nhất**.

---

## 1. Phân tích hiện trạng thiết bị của bạn

| Thiết bị | Đặc điểm kỹ thuật | Thách thức kỹ thuật | Giải pháp xử lý |
|:---|:---|:---|:---|
| **Laptop Acer Nitro 5** | - Windows 11/10, CPU mạnh (Intel Core i5/i7 hoặc Ryzen 5/7)<br>- Card âm thanh tích hợp: Realtek HD Audio<br>- Cổng âm thanh: 1 jack 3.5mm kết hợp (Combo Audio TRRS 4 khấc) | Driver âm thanh mặc định của Windows (MME/DirectSound) có độ trễ cao (50ms - 150ms). | Sử dụng driver **ASIO4ALL** hoặc **WASAPI Exclusive** để ép buffer xuống 128 - 256 samples (độ trễ < 10ms). |
| **Micro dây karaoke** | - Thường là Mic Dynamic (đầu jack 6.5mm mono hoặc chân XLR)<br>- Tín hiệu đầu ra rất nhỏ (mức millivolt -55dBV), cần khuếch đại (Pre-amp). | Cắm trực tiếp vào laptop qua jack chuyển 6.5mm -> 3.5mm thường bị tiếng nhỏ, rè hoặc laptop không nhận diện được chân mic TRRS. | Cần đầu chia cáp Y-Splitter TRRS hoặc sử dụng USB Audio Box/Vang cơ. |
| **Bộ loa karaoke** | - Loa kéo, loa active hoặc ampli gia đình.<br>- Cổng nhận tín hiệu: AUX 3.5mm, RCA (hoa sen), Bluetooth, Quang học. | Nếu kết nối qua **Bluetooth** sẽ bị trễ từ 150ms - 300ms, hoàn toàn không thể hát được AutoTune live. | **Bắt buộc** nối dây có dây (3.5mm AUX hoặc jack hoa sen RCA) từ laptop ra loa. |

---

## 2. Sơ đồ đấu nối dây chuẩn (Hardware Wiring)

### Phương án A: Tận dụng 100% thiết bị có sẵn (Chi phí 0đ - 50k)
Dành cho việc test thử nghiệm ban đầu bằng cổng 3.5mm của Acer Nitro 5:

```
[Micro Dây (Đầu 6.5mm)]
        │
        ▼ (Đầu chuyển 6.5mm sang 3.5mm)
        │
        ▼
[Cáp gộp Y-Splitter TRRS (Đầu đực 4 khấc cắm vào Laptop, đầu cái chia 1 Mic / 1 Tai nghe)]
   │               │
   │ (Cổng Mic In) └─── (Cổng Audio Out)
   │                           │
   ▼                           ▼ (Dây 3.5mm sang 3.5mm hoặc 3.5mm sang RCA Hoa Sen)
[Cổng 3.5mm Nitro 5]        [Cổng AUX IN trên Loa Karaoke]
```

> [!WARNING]
> Cổng 3.5mm trên laptop Nitro 5 là loại cổng combo tai nghe + mic chuẩn CTIA. Bạn **không thể cắm trực tiếp đầu 6.5mm chuyển sang 3.5mm vào máy** vì máy sẽ hiểu nhầm là bạn cắm tai nghe và không thu được tiếng mic. Bắt buộc phải có **Cáp chia Y-Splitter TRRS 4 chấu** (giá khoảng 20.000 - 40.000đ).

---

### Phương án B: Tối ưu chuyên nghiệp (Khuyên dùng - Chi phí 300k - 800k)
Để hát karaoke lâu dài không bị rè, mic bắt nhạy và âm thanh dày ấm:

```
[Micro Dây] ─────────► [USB Audio Interface / Soundcard mini / Vang cơ có cổng USB]
                                     │
                             (Cáp USB kết nối)
                                     │
                                     ▼
                            [Laptop Acer Nitro 5]
                           (Xử lý AutoTune real-time)
                                     │
                                     ▼
                      [Cổng Out của Soundcard / Laptop]
                                     │ (Dây tín hiệu)
                                     ▼
                            [Loa Karaoke Gia Đình]
```
- Các thiết bị hỗ trợ cực tốt: *Vang cơ gia đình có USB Audio*, *Soundcard K10 / K300 / H9*, hoặc *Behringer U-Phoria UM2*.

---

## 3. Khắc phục bài toán Độ trễ (Audio Latency)

### Vì sao độ trễ lại quyết định sự thành bại của AutoTune?
- **> 30ms:** Người hát nghe tiếng mình dội lại chậm hơn khẩu hình miệng, gây phản xạ ức chế não bộ (hiện tượng nói lắp, lệch nhịp).
- **15ms - 25ms:** Cảm giác hát hơi nặng, giọng không tự nhiên.
- **< 10ms (Chuẩn phòng thu):** Giọng hát đồng bộ hoàn hảo với tai nghe và loa, người hát cảm nhận tiếng AutoTune tức thì.

### Cấu hình Driver âm thanh trên Windows / Acer Nitro 5
1. **Cài đặt ASIO4ALL:**
   - Tải và cài đặt miễn phí từ [asio4all.org](https://www.asio4all.org/).
   - Thiết lập cấu hình:
     - **Buffer Size:** `128 Samples` (hoặc `256 Samples` nếu máy có hiện tượng lẹt xẹt xé tiếng).
     - Với sample rate 48.000Hz:
       $$\text{Latency} = \frac{128}{48000} \approx 2.67\text{ ms (Độ trễ xử lý buffer)}$$
2. **Tắt các bộ xử lý rác của Windows và Nitro 5:**
   - Acer Nitro 5 thường cài sẵn phần mềm **DTS:X Ultra** hoặc **Acer TrueHarmony** -> **Cần tắt chế độ này** khi hát karaoke vì nó chèn thêm DSP phụ gây tăng độ trễ lên thêm 30-50ms.
   - Vào `Sound Control Panel` -> Click đúp vào Loa/Micro -> Tab `Enhancements` -> Tích chọn `Disable all sound effects`.
   - Tab `Advanced`: Đặt định dạng mặc định là `24 bit, 48000 Hz (Studio Quality)`.

---

## 4. Giải pháp chống hú rít (Acoustic Feedback Prevention)

Khi hát karaoke qua máy tính và phát ra loa công suất lớn trong phòng kín, âm thanh từ loa sẽ lọt ngược lại vào micro, tạo vòng lặp vô hạn gây tiếng **hú rít (feedback loop)**.

1. **Vị trí bố trí vật lý:**
   - Người cầm micro luôn đứng **phía sau hoặc ngang hàng** với mặt phẳng loa. Tuyệt đối không chĩa đầu micro thẳng vào màng loa.
   - Khoảng cách tối thiểu từ mic tới loa: **2.5m - 3m**.
2. **Bộ lọc trên phần mềm:**
   - **Noise Gate:** Tự động ngắt tín hiệu khi người dùng ngừng hát (khi mức âm lượng dưới ngưỡng threshold -45dB).
   - **High-pass Filter (Low-cut):** Cắt bỏ dải tần siêu trầm dưới 80Hz (tiếng va chạm tay vào mic, tiếng rung sàn).
   - **Notch Filter / Feedback Suppressor:** Tự động phát hiện đỉnh tần số nhọn có xu hướng hú (thường ở dải 2.5kHz - 6kHz) và hạ âm lượng tần số đó xuống 3-6dB.
