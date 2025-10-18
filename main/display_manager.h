#pragma once
#include "esp_err.h"

esp_err_t display_manager_init(void);
esp_err_t display_manager_test(void);
esp_err_t display_manager_write_text(const char *text);
esp_err_t display_manager_set_bg_color(uint8_t r, uint8_t g, uint8_t b);
esp_err_t display_manager_clear(void);