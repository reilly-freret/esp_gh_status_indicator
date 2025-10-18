#pragma once
#include "esp_err.h"
#include "sdkconfig.h"

enum text_size {
    #ifdef CONFIG_LV_FONT_MONTSERRAT_8
        TEXT_SIZE_8 = 8,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_10
        TEXT_SIZE_10 = 10,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_12
        TEXT_SIZE_12 = 12,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_14
        TEXT_SIZE_14 = 14,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_16
        TEXT_SIZE_16 = 16,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_18
        TEXT_SIZE_18 = 18,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_20
        TEXT_SIZE_20 = 20,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_22
        TEXT_SIZE_22 = 22,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_24
        TEXT_SIZE_24 = 24,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_26
        TEXT_SIZE_26 = 26,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_28
        TEXT_SIZE_28 = 28,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_30
        TEXT_SIZE_30 = 30,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_32
        TEXT_SIZE_32 = 32,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_34
        TEXT_SIZE_34 = 34,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_36
        TEXT_SIZE_36 = 36,
    #endif
    #ifdef CONFIG_LV_FONT_MONTSERRAT_38
        TEXT_SIZE_38 = 38,
    #endif
};

typedef struct {
    struct {
        int16_t r;
        int16_t g;
        int16_t b;
    } color;
    enum text_size size;
} text_config_t;

esp_err_t display_manager_init(void);
esp_err_t display_manager_test(void);
esp_err_t display_manager_write_text(const char *text);
esp_err_t display_manager_write_text_color(const char *text, int16_t r, int16_t g, int16_t b);
esp_err_t display_manager_write_text_custom(const char *text, text_config_t config);
esp_err_t display_manager_set_bg_color(uint8_t r, uint8_t g, uint8_t b);
esp_err_t display_manager_clear(void);