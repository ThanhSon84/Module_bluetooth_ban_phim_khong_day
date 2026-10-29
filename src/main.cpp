// ==============================================================================
// Nguồn: https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE/tree/main (main1.cpp)
// Giấy phép: MIT License
// Mức độ sử dụng: Giữ nguyên
// Nội dung nhóm đã sửa: Không sửa đổi cấu trúc logic. Giữ nguyên 100% luồng khởi động
//   setup() và vòng lặp loop() điều phối; bổ sung chú thích tiếng Việt chi tiết.
// ==============================================================================

#include "Bridge.h"
#include "Config.h"
#include <Arduino.h>

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Khởi tạo hệ thống
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  // In banner nhận diện hệ thống
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 USB to BLE Keyboard Bridge           ║");
  Serial.println("║  Supports keyboard + multi-device              ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();

  // Khởi tạo chân đèn LED báo hiệu trạng thái
  if (LED_FEEDBACK_PIN >= 0) {
    pinMode(LED_FEEDBACK_PIN, OUTPUT);
    digitalWrite(LED_FEEDBACK_PIN, LOW);
  }

  // Khởi chạy toàn bộ hệ thống qua lớp Bridge
  Bridge::begin();

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  READY - Connect USB devices via hub           ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Vòng lặp chính thực thi định kỳ
 */
void loop() {
  Bridge::loop();
  delay(10); // Tạm dừng 10ms nhường CPU cho các tác vụ FreeRTOS ngầm
}
