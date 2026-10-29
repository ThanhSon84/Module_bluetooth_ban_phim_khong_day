#include "BLEManager.h"
#include "Config.h"
#include <esp_mac.h>
#include <NimBLEDevice.h>

BLEManager::BLEManager() : _bleCombo(nullptr) {}

void BLEManager::begin(uint8_t slot, const char *deviceName) {
  Serial.printf("[BLE] Initializing: '%s'\n", deviceName);

  _bleCombo = new BleCombo(deviceName, DEVICE_MANUFACTURER, BATTERY_LEVEL);
  _bleCombo->begin();

  // Cấu hình bảo mật trực tiếp sau begin (không tạo server mới)
  NimBLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/false, /*sc=*/true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
}

bool BLEManager::isConnected() {
  return (_bleCombo != nullptr && _bleCombo->isConnected());
}

void BLEManager::sendKeyboardReport(const uint8_t *keys, uint8_t modifiers) {
  if (!isConnected())
    return;

  KeyReport report;
  report.modifiers = modifiers;
  report.reserved = 0;
  memcpy(report.keys, keys, 6);
  _bleCombo->sendReport(&report);
}