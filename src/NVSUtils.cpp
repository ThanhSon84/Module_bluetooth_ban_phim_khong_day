// ==============================================================================
// Nguồn: https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE/tree/main (NVSUtils1.cpp)
// Giấy phép: MIT License
// Mức độ sử dụng: Giữ nguyên
// Nội dung nhóm đã sửa: Không sửa đổi thuật toán. Giữ nguyên 100% cơ chế sao chép phân vùng
//   và đồng bộ khóa liên kết Bluetooth (Bonding Keys) NVS; bổ sung chú thích tiếng Việt chi tiết.
// ==============================================================================

#include "NVSUtils.h"
#include <Arduino.h>
#include <nvs.h>
#include <nvs_flash.h>

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Sao chép toàn bộ dữ liệu từ namespace nguồn sang namespace đích trong NVS
 */
void NVSUtils::copyNamespace(const char *src_ns, const char *dst_ns) {
  nvs_handle_t h_src, h_dst;
  esp_err_t err;

  // Thử mở namespace nguồn ở chế độ chỉ đọc
  err = nvs_open(src_ns, NVS_READONLY, &h_src);
  if (err != ESP_OK) {
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      // Nếu nguồn rỗng hoặc chưa có: Đảm bảo xóa sạch namespace đích
      err = nvs_open(dst_ns, NVS_READWRITE, &h_dst);
      if (err == ESP_OK) {
        nvs_erase_all(h_dst);
        nvs_commit(h_dst);
        nvs_close(h_dst);
      }
    }
    return;
  }

  // Mở namespace đích ở chế độ đọc/ghi
  err = nvs_open(dst_ns, NVS_READWRITE, &h_dst);
  if (err != ESP_OK) {
    nvs_close(h_src);
    return;
  }

  // Xóa sạch đích trước khi sao chép để đảm bảo đồng nhất
  nvs_erase_all(h_dst);

  // Khởi tạo con trỏ duyệt (iterator) qua tất cả các khóa trong namespace nguồn
  nvs_iterator_t it = NULL;
  err = nvs_entry_find("nvs", src_ns, NVS_TYPE_ANY, &it);
  while (err == ESP_OK) {
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);

    // Sao chép tương ứng theo từng kiểu dữ liệu
    switch (info.type) {
    case NVS_TYPE_U8: {
      uint8_t v;
      nvs_get_u8(h_src, info.key, &v);
      nvs_set_u8(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_I8: {
      int8_t v;
      nvs_get_i8(h_src, info.key, &v);
      nvs_set_i8(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_U16: {
      uint16_t v;
      nvs_get_u16(h_src, info.key, &v);
      nvs_set_u16(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_I16: {
      int16_t v;
      nvs_get_i16(h_src, info.key, &v);
      nvs_set_i16(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_U32: {
      uint32_t v;
      nvs_get_u32(h_src, info.key, &v);
      nvs_set_u32(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_I32: {
      int32_t v;
      nvs_get_i32(h_src, info.key, &v);
      nvs_set_i32(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_U64: {
      uint64_t v;
      nvs_get_u64(h_src, info.key, &v);
      nvs_set_u64(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_I64: {
      int64_t v;
      nvs_get_i64(h_src, info.key, &v);
      nvs_set_i64(h_dst, info.key, v);
      break;
    }
    case NVS_TYPE_STR: { // Kiểu chuỗi ký tự
      size_t len;
      if (nvs_get_str(h_src, info.key, NULL, &len) == ESP_OK) {
        char *v = (char *)malloc(len);
        if (v) {
          nvs_get_str(h_src, info.key, v, &len);
          nvs_set_str(h_dst, info.key, v);
          free(v);
        }
      }
      break;
    }
    case NVS_TYPE_BLOB: { // Kiểu dữ liệu nhị phân (chứa khóa LTK/IRK Bluetooth)
      size_t len;
      if (nvs_get_blob(h_src, info.key, NULL, &len) == ESP_OK) {
        void *v = malloc(len);
        if (v) {
          nvs_get_blob(h_src, info.key, v, &len);
          nvs_set_blob(h_dst, info.key, v, len);
          free(v);
        }
      }
      break;
    }
    }
    err = nvs_entry_next(&it);
  }
  nvs_release_iterator(it);

  // Xác nhận ghi dữ liệu xuống Flash
  nvs_commit(h_dst);
  nvs_close(h_src);
  nvs_close(h_dst);
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Khôi phục khóa bonding của Slot từ Flash vào namespace động của NimBLE
 */
void NVSUtils::loadSlotBonds(uint8_t slot) {
  char slot_ns[16];
  snprintf(slot_ns, sizeof(slot_ns), "ble_bond_%d", slot);
  Serial.printf("[System] Loading BLE bonds for slot %d from '%s'...\n",
                slot + 1, slot_ns);
  copyNamespace(slot_ns, "nimble_bond");
}

/**
 * @brief [GIỮ NGUYÊN TỪ GỐC] Sao lưu khóa bonding hiện tại vào namespace lưu trữ tĩnh của Slot
 */
void NVSUtils::saveSlotBonds(uint8_t slot) {
  char slot_ns[16];
  snprintf(slot_ns, sizeof(slot_ns), "ble_bond_%d", slot);
  Serial.printf("[System] Saving BLE bonds for slot %d to '%s'...\n", slot + 1,
                slot_ns);
  copyNamespace("nimble_bond", slot_ns);
}
