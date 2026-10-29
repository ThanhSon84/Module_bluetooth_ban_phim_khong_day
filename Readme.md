# Module Bluetooth: Biến Bàn Phím Có Dây Thành Bàn Phím Không Dây Multi-Host Dùng ESP32-S3

* **Học phần:** Xây dựng các hệ thống nhúng (INT1461) — Học kỳ 1, Năm học 2026 - 2027
* **Cơ sở đào tạo:** Học viện Công nghệ Bưu chính Viễn thông (PTIT)
* **Giảng viên hướng dẫn:** ThS. Hồ Nhựt Minh
* **Sinh viên thực hiện:** Hoàng Thanh Sơn
* **Mã số sinh viên:** K24DTCN419
* **Lớp:** D24TXCN10-K
* **Nhóm:** Nhóm 02 (Đồ án cá nhân — 1 thành viên)

---

## 1. Môi trường phát triển và Nền tảng (Platform & IDE)

* **IDE:** Visual Studio Code (phiên bản `1.90.0` trở lên)
* **Phần mở rộng (Extension):** PlatformIO IDE (phiên bản `v3.3.3` trở lên / PlatformIO Core `v6.1.15`)
* **Nền tảng vi điều khiển:** Espressif 32 (`espressif32 @ 6.6.0`)
* **Framework:** Arduino Framework (xây dựng trên nền tảng ESP-IDF `v4.4.7` / `v5.1.x`)
* **Hệ điều hành kiểm thử:** Windows 11 Pro 64-bit

---

## 2. Thông tin phần cứng (Hardware Specifications)

* **Bo mạch chính:** Espressif ESP32-S3-DevKitC-1-N8
  * Vi xử lý: Xtensa® Dual-Core 32-bit LX7, xung nhịp 240 MHz
  * Bộ nhớ: 8 MB Quad-SPI Flash, 512 KB SRAM nội bộ, không dùng PSRAM
  * Giao tiếp nạp/giám sát: Cổng USB Type-C nối chip UART WCH CH343
  * Giao tiếp ngoại vi USB Host: Bộ điều khiển phần cứng USB-OTG Full Speed (12 Mbps)
* **Bàn phím cơ thử nghiệm:** Aula 68 (Layout 68 phím rút gọn, chuẩn giao tiếp USB HID Boot Protocol)
* **Khối nguồn độc lập:**
  * Pin sạc: Li-ion 18650 3.7V dung lượng 2600 mAh
  * Module quản lý sạc/bảo vệ: TP4056 (tích hợp IC bảo vệ pin DW01A + FS8205)
  * Module tăng áp DC-DC: MT3608 (điều chỉnh ngõ ra 5.05V – 5.15V)

---

## 3. Danh sách thư viện sử dụng (Dependencies & Libraries)

Toàn bộ thư viện được quản lý tự động qua `platformio.ini`:

| Tên thư viện | Phiên bản | Nguồn cung cấp / Tác giả | Chức năng trong hệ thống |
| :--- | :--- | :--- | :--- |
| **NimBLE-Arduino** | `^1.4.1` | `h2zero/NimBLE-Arduino` | Ngăn xếp BLE nhẹ, tối ưu hóa bộ nhớ và ép chu kỳ kết nối xuống 7.5 ms – 11.25 ms |
| **ESP32-BLE-Combo** | `Latest (Git)` | `blackketter/ESP32-BLE-Combo` | Quản lý đóng gói báo cáo HID Keyboard và BLE GATT Service |
| **ESP-IDF USB Host** | Tích hợp sẵn | Espressif Systems (`usb/usb_host.h`, `esp_hidh.h`) | Driver USB Host điều khiển cổng USB-OTG phần cứng của ESP32-S3 |
| **Preferences** | Tích hợp sẵn | Arduino-ESP32 Core | Đọc/ghi cấu hình Slot và dữ liệu NVS Flash |

---

## 4. Sơ đồ chân kết nối (Pinout & Wiring Diagram)

> **CẢNH BÁO NGUỒN ĐIỆN:**
> * Toàn bộ chân GPIO của ESP32-S3 chỉ chịu mức logic **3.3V**. Tuyệt đối không nối nguồn 5V vào các chân GPIO.
> * Cổng USB của bàn phím Aula 68 yêu cầu nguồn 5V ổn định; đường 5V này được cấp từ ngõ ra của module tăng áp MT3608, không lấy từ chân 3V3 của ESP32.

### Bảng đấu nối chi tiết:

| Chân linh kiện ngoại vi | Chân kết nối ESP32-S3 | Mức điện áp | Ghi chú kỹ thuật |
| :--- | :--- | :--- | :--- |
| **USB-A Female D-** (Dây trắng) | **GPIO 19** (`USB_D-`) | 3.3V Logic | Chân USB Host phần cứng cố định |
| **USB-A Female D+** (Dây xanh) | **GPIO 20** (`USB_D+`) | 3.3V Logic | Chân USB Host phần cứng cố định |
| **USB-A Female VBUS** (Dây đỏ) | Ngõ ra `5V` (MT3608) | 5.05V Power | Cấp nguồn nuôi bàn phím có dây |
| **USB-A Female GND** (Dây đen) | `GND` chung | 0V Ground | Nối chung mass toàn hệ thống |
| **MT3608 VOUT (+)** | Chân `5V` / `VIN` của board | 5.05V Power | Cấp nguồn nuôi bo mạch qua IC hạ áp LDO |
| **LED Feedback Anode (+)** | **GPIO 2** (qua trở 330Ω) | 3.3V Logic | Báo trạng thái kết nối và phản hồi đổi Slot |
| **LED Feedback Cathode (-)** | `GND` chung | 0V Ground | Nối mass qua điện trở hạn dòng |

---

## 5. Các bước biên dịch và nạp chương trình (Build & Upload)

### Bước 1: Chuẩn bị mã nguồn
Mở Visual Studio Code, chọn **File** $\rightarrow$ **Open Folder...** và trỏ đến thư mục dự án `ESP32S3-USB-Keyboard-To-BLE`.

### Bước 2: Cấu hình tệp `platformio.ini`
Đảm bảo nội dung tệp `platformio.ini` khớp với cấu hình sau:
```ini
[env:esp32s3_usb_ble]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.partitions = huge_app.csv
build_flags = 
    -DARDUINO_USB_MODE=0
    -DARDUINO_USB_CDC_ON_BOOT=1
upload_port = COM3
monitor_port = COM3
monitor_speed = 115200
lib_deps =
    h2zero/NimBLE-Arduino@^1.4.1
    https://github.com/blackketter/ESP32-BLE-Combo.git
```
*(Nếu máy tính nhận cổng COM khác, thay đổi `upload_port` và `monitor_port` tương ứng).*

### Bước 3: Biên dịch mã nguồn (Build)
* Nhìn xuống thanh trạng thái góc dưới bên trái của VS Code, bấm biểu tượng **`✓` (PlatformIO: Build)**.
* Chờ tiến trình hoàn tất và hiển thị thông báo `[SUCCESS]`.

### Bước 4: Nạp firmware vào vi điều khiển (Upload)
* Cắm cáp Type-C từ máy tính vào cổng **UART/COM** của bo mạch ESP32-S3 (cổng nối chip CH343).
* Bấm biểu tượng **`→` (PlatformIO: Upload)** ở thanh đáy VS Code.
* Nếu màn hình dừng ở dòng `Connecting......____....`, nhấn giữ nút **BOOT** trên bo mạch khoảng 1 giây rồi thả ra để kích hoạt chế độ nạp ROM.
* Chờ thông báo nạp thành công: `[SUCCESS]` và chip tự động khởi động lại qua chân RTS.

---

## 6. Cách chạy thử và nghiệm thu hệ thống (Testing & Verification)

### Bước 1: Mở giám sát Serial Monitor
* Bấm biểu tượng **chiếc phích cắm điện `🔌` (PlatformIO: Serial Monitor)** ở thanh đáy VS Code (tốc độ `115200` baud).
* Màn hình Terminal sẽ in ra banner khởi động:
  ```text
  [Config] Starting on device slot 1
  [System] Loading BLE bonds for slot 1 from 'ble_bond_0'...
  [BLE] Initializing: 'USB-BLE Dev 1'
  [USB] USB Host library ready
  [USB] HID driver ready
  READY - Connect USB devices via hub
  ```

### Bước 2: Kết nối bàn phím cơ Aula 68
* Cắm đầu cáp USB của bàn phím cơ Aula 68 vào cổng USB-A cái.
* Màn hình Terminal in thông báo xác nhận:
  ```text
  [USB] KEYBOARD connected!
  ```

### Bước 3: Ghép đôi Bluetooth với Máy tính / Điện thoại (Slot 1)
1. Trên máy tính Windows, mở **Settings** $\rightarrow$ **Bluetooth & devices** $\rightarrow$ bấm **Add device**.
2. Tìm thiết bị Bluetooth có tên **`USB-BLE Dev 1`** và bấm kết nối.
3. Cửa sổ Serial Monitor sẽ chuyển trạng thái sang:
   ```text
   [Status] Slot 1 | BLE: CONNECTED
   ```
4. Mở phần mềm Notepad và gõ thử các phím. Ký tự xuất hiện mượt mà và Terminal ghi nhận luồng mã quét:
   ```text
   [KB] mod:0x00 keys:[04 00 00 00 00 00]
   ```

### Bước 4: Kiểm tra tính năng chuyển Profile Multi-Host bằng tổ hợp `Alt + 1/2/3`
1. Trên bàn phím Aula 68, nhấn giữ phím **`Alt`** và bấm số **`2`**:
   * Hệ thống bắt sự kiện:
     ```text
     [Switch] Alt + 2 detected -> Chuyen Slot 2
     [BLE] Switching from slot 1 to slot 2
     [System] Saving BLE bonds for slot 1 to 'ble_bond_0'...
     [System] Restarting to apply new slot settings...
     ```
   * ESP32-S3 tự khởi động lại, nạp `ble_bond_1` và phát Bluetooth tên **`USB-BLE Dev 2`**.
2. Mở điện thoại di động (hoặc máy tính thứ hai), quét và kết nối với **`USB-BLE Dev 2`** để gõ văn bản thử nghiệm.
3. Nhấn giữ **`Alt`** và bấm số **`1`** để quay lại điều khiển máy tính ban đầu mà không cần ghép đôi lại từ đầu.
