// ==============================================================================
// Nguồn: https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE/tree/main (BLEManager1.cpp)
// Giấy phép: MIT License
// Mức độ sử dụng: Sửa đổi
// Nội dung nhóm đã sửa:
//   - Giữ nguyên: Cấu trúc lớp BLEManager, hàm isConnected() và hàm sendKeyboardReport().
//   - Thay đổi: Thay thế cơ chế setUniqueMac() thủ công bằng cấu hình chuẩn NimBLE Security:
//     setSecurityAuth(bonding=true, mitm=false, sc=true) và IO Capability NO_INPUT_OUTPUT
//     để thiết bị ghép đôi mượt mà (Just Works) và tự động ghi nhớ liên kết vào NVS Flash.
//   - Bổ sung: Khởi chạy quảng bá Scan Response rõ ràng thông qua NimBLEAdvertising.
// ==============================================================================

#include "BLEManager.h"
#include "Config.h"
#include <esp_mac.h>
#include <NimBLEDevice.h>

// [GIỮ NGUYÊN TỪ GỐC] Constructor khởi tạo con trỏ _bleCombo là nullptr
BLEManager::BLEManager() : _bleCombo(nullptr) {}

/**
 * @brief [SỬA ĐỔI] Khởi tạo ngăn xếp BLE HID và thiết lập tham số bảo mật ghép đôi NimBLE
 */
void BLEManager::begin(uint8_t slot, const char *deviceName) {
  Serial.printf("[BLE] Initializing: '%s'\n", deviceName);

  // [GIỮ NGUYÊN TỪ GỐC]: Khởi tạo đối tượng BleCombo
  _bleCombo = new BleCombo(deviceName, DEVICE_MANUFACTURER, BATTERY_LEVEL);
  _bleCombo->begin();

  // [SỬA ĐỔI - BỔ SUNG CẤU HÌNH BẢO MẬT NIMBLE THAY CHO SETUNIQUEMAC THỦ CÔNG]
  // Bật tính năng lưu khóa ghép đôi (bonding) và mã hóa an toàn LE Secure Connections
  NimBLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/false, /*sc=*/true);
  // Không yêu cầu màn hình nhập mã PIN (chế độ ghép đôi tự động Just Works)
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  // Cấu hình phân phối khóa mã hóa bảo mật 2 chiều
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

  // [SỬA ĐỔI]: Kích hoạt quảng bá có kèm Scan Response để máy tính nhận diện tên đầy đủ
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Kiểm tra xem máy tính/điện thoại đã kết nối BLE hay chưa
 */
bool BLEManager::isConnected() {
  return (_bleCombo != nullptr && _bleCombo->isConnected());
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Đóng gói và phát bản tin HID 8-byte qua BLE
 */
void BLEManager::sendKeyboardReport(const uint8_t *keys, uint8_t modifiers) {
  if (!isConnected())
    return;

  KeyReport report;
  report.modifiers = modifiers; // Gán byte phím bổ trợ (Ctrl, Shift, Alt, GUI)
  report.reserved = 0;          // Byte dành riêng theo chuẩn USB HID
  memcpy(report.keys, keys, 6); // Sao chép 6 mã quét Usage ID
  _bleCombo->sendReport(&report);
}
