// ==============================================================================
// Nguồn: https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE/tree/main (Bridge1.cpp)
// Giấy phép: MIT License
// Mức độ sử dụng: Sửa đổi
// Nội dung nhóm đã sửa:
//   - Giữ nguyên: Toàn bộ quy trình nạp cấu hình NVS, vòng lặp giám sát BLE và hàm switchToSlot().
//   - Thay đổi: Hàm checkDeviceSwitchCombo() được nâng cấp nhận thêm tham số 'modifiers' (byte 0).
//   - Thay đổi: Thay thế tổ hợp chuyển Profile từ "Scroll Lock + 1/2/3" thành "Alt + 1/2/3" 
//     (bằng mặt nạ bit modifiers & 0x44) để tương thích với layout bàn phím rút gọn Aula 68.
// ==============================================================================

#include "Bridge.h"
#include "NVSUtils.h"
#include <hid_usage_keyboard.h>

// [GIỮ NGUYÊN TỪ GỐC] Khởi tạo các biến tĩnh của lớp Bridge
uint8_t Bridge::_currentSlot = 0;      // Lưu trữ chỉ số Slot hiện tại (0, 1 hoặc 2)
BLEManager Bridge::_bleManager;        // Đối tượng quản lý truyền thông BLE HID
Preferences Bridge::_preferences;      // Đối tượng đọc/ghi bộ nhớ NVS Flash

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Khởi tạo cầu nối: nạp slot từ Flash, nạp bonding keys, bật BLE và USB Host
 */
void Bridge::begin() {
  // 1. Đọc Profile đã lưu gần nhất từ Flash NVS
  _preferences.begin("usb-ble", true);
  _currentSlot = _preferences.getUChar("slot", 0);
  if (_currentSlot >= NUM_DEVICE_SLOTS)
    _currentSlot = 0;
  _preferences.end();

  Serial.printf("[Config] Starting on device slot %d\n", _currentSlot + 1);

  // 2. Nạp khóa mã hóa liên kết (Bonding Keys) của Slot tương ứng vào NimBLE
  NVSUtils::loadSlotBonds(_currentSlot);

  // 3. Khởi động BLE với tên định danh tương ứng theo Slot
  const char *deviceNames[NUM_DEVICE_SLOTS] = {DEVICE_NAME_1, DEVICE_NAME_2,
                                               DEVICE_NAME_3};
  _bleManager.begin(_currentSlot, deviceNames[_currentSlot]);

  // 4. Đăng ký hàm callback nhận gói tin ngắt bàn phím và khởi động USB Host
  USBManager::setKeyboardCallback(onKeyboardReport);
  USBManager::begin();
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Vòng lặp định kỳ in trạng thái kết nối BLE ra Serial Monitor
 */
void Bridge::loop() {
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    lastStatus = millis();
    bool connected = _bleManager.isConnected();
    Serial.printf("[Status] Slot %d | BLE: %s\n", _currentSlot + 1,
                  connected ? "CONNECTED" : "waiting for pairing...");
  }
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Chuyển đổi Profile kết nối sang thiết bị khác, lưu NVS và restart chip
 * @param slot Chỉ số Slot đích (0, 1 hoặc 2)
 */
void Bridge::switchToSlot(uint8_t slot) {
  if (slot >= NUM_DEVICE_SLOTS)
    return;

  // Nếu đang ở sẵn Slot đích, không khởi động lại mà chỉ nháy LED báo hiệu
  if (slot == _currentSlot) {
    Serial.printf("[BLE] Already on slot %d\n", slot + 1);
    if (LED_FEEDBACK_PIN >= 0) {
      for (int i = 0; i <= slot; i++) {
        digitalWrite(LED_FEEDBACK_PIN, HIGH);
        delay(150);
        digitalWrite(LED_FEEDBACK_PIN, LOW);
        delay(150);
      }
    }
    return;
  }

  Serial.printf("[BLE] Switching from slot %d to slot %d\n", _currentSlot + 1,
                slot + 1);

  // 1. Sao lưu khóa mã hóa BLE hiện tại vào phân vùng Flash của Slot cũ
  NVSUtils::saveSlotBonds(_currentSlot);

  // 2. Ghi chỉ số Slot mới vào Flash NVS
  _preferences.begin("usb-ble", false);
  _preferences.putUChar("slot", slot);
  _preferences.end();

  // 3. Nháy LED phản hồi số lần tương ứng với số hiệu Slot mới
  if (LED_FEEDBACK_PIN >= 0) {
    for (int i = 0; i <= slot; i++) {
      digitalWrite(LED_FEEDBACK_PIN, HIGH);
      delay(150);
      digitalWrite(LED_FEEDBACK_PIN, LOW);
      delay(150);
    }
  }

  // 4. Giải phóng tài nguyên phần cứng USB Host trước khi reset
  usb_host_device_free_all();

  Serial.println("[System] Restarting to apply new slot settings...");
  delay(500);
  ESP.restart(); // Khởi động lại vi điều khiển
}

/**
 * @brief [SỬA ĐỔI] Callback nhận 8-byte Raw HID Report từ USB Host
 * Truyền thêm tham số kb_report->modifier.val vào hàm kiểm tra phím nóng
 */
void Bridge::onKeyboardReport(const uint8_t *data, size_t length) {
  if (length < sizeof(hid_keyboard_input_report_boot_t))
    return;

  hid_keyboard_input_report_boot_t *kb_report =
      (hid_keyboard_input_report_boot_t *)data;

  // [SỬA ĐỔI]: Gọi hàm kiểm tra phím nóng kèm theo byte phím bổ trợ modifier
  if (checkDeviceSwitchCombo(kb_report->key, kb_report->modifier.val)) {
    return; // Chặn phím nóng, không chuyển tiếp lên máy chủ
  }

  // [GIỮ NGUYÊN TỪ GỐC]: In log mã phím ra Serial Monitor
  Serial.printf("[KB] mod:0x%02X keys:[%02X %02X %02X %02X %02X %02X]\n",
                kb_report->modifier.val, kb_report->key[0], kb_report->key[1],
                kb_report->key[2], kb_report->key[3], kb_report->key[4],
                kb_report->key[5]);

  // [GIỮ NGUYÊN TỪ GỐC]: Chuyển tiếp gói tin qua Bluetooth
  _bleManager.sendKeyboardReport(kb_report->key, kb_report->modifier.val);
}

/**
 * @brief [SỬA ĐỔI HOÀN TOÀN LOGIC] Phát hiện tổ hợp phím tắt chuyển Slot
 * Code gốc Bridge1.cpp kiểm tra phím Scroll Lock trong mảng keys[].
 * Code sửa đổi kiểm tra mặt nạ bit phím Alt (0x44) kết hợp với các phím số 1, 2, 3.
 */
bool Bridge::checkDeviceSwitchCombo(const uint8_t *keys, uint8_t modifiers) {
  if (!ENABLE_DEVICE_SWITCHING)
    return false;

  // [SỬA ĐỔI]: Bit 2 (0x04) là Left Alt, Bit 6 (0x40) là Right Alt
  bool hasAlt = (modifiers & 0x44) != 0;
  uint8_t numberKey = 0;

  // [GIỮ NGUYÊN TỪ GỐC]: Quét mảng 6 phím để tìm phím số từ 1 đến 3
  for (int i = 0; i < 6; i++) {
    if (keys[i] >= HID_KEY_1 && keys[i] <= HID_KEY_3) {
      numberKey = keys[i] - HID_KEY_1 + 1;
    }
  }

  // [SỬA ĐỔI]: Kích hoạt khi giữ Alt kết hợp phím số 1, 2 hoặc 3
  if (hasAlt && numberKey > 0 && numberKey <= NUM_DEVICE_SLOTS) {
    Serial.printf("[Switch] Alt + %d detected -> Chuyen Slot %d\n", numberKey, numberKey);
    switchToSlot(numberKey - 1);
    return true; // Trả về true để triệt tiêu gói tin phím này
  }

  return false;
}
