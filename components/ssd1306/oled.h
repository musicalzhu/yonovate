// oled.h - 轻量 SSD1306 驱动
#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <esp_err.h>

// OLED 尺寸
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_PAGES   (OLED_HEIGHT / 8)

// I2C 设置
#define OLED_I2C_ADDR  0x3D  // 用户确认使用0x3D地址

// 函数声明
esp_err_t oled_init(void);
esp_err_t oled_clear(void);
esp_err_t oled_display_string(int line, const char* str);
esp_err_t oled_refresh(void);

#ifdef __cplusplus
}
#endif

#endif