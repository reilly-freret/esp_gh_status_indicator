#include "display_manager.h"
#include "config.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "misc/lv_color.h"

static const char *TAG = "display_manager";
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_t * disp_handle = NULL;

esp_err_t display_manager_set_bg_color(uint8_t r, uint8_t g, uint8_t b) {
    if (lvgl_port_lock(0)) {
        static lv_style_t style_scr;
        lv_style_init(&style_scr);
        lv_style_set_bg_color(&style_scr, lv_color_make(r, g, b));
        lv_obj_t *scr = lv_scr_act();
        lv_obj_add_style(scr, &style_scr, LV_STATE_DEFAULT);
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_clear(void) {
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_write_text(const char *text) {
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_scr_act();
        
        // Set up flex layout if not already done
        lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        
        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, text);
        
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_test(void) {
    ESP_LOGI(TAG, "Test display");
    if (lvgl_port_lock(0)) {
        static lv_style_t style_scr;
        lv_style_init(&style_scr);
        lv_style_set_bg_color(&style_scr, lv_color_make(0, 0, 255));
    
        static lv_style_t style_label;
        lv_style_init(&style_label);
        lv_style_set_text_color(&style_label, lv_color_white());

        lv_obj_t *scr = lv_scr_act();
    
        lv_obj_clean(scr);
        lv_obj_set_size(scr, lv_disp_get_hor_res(disp_handle), lv_disp_get_ver_res(disp_handle));
        lv_obj_add_style(scr, &style_scr, LV_STATE_DEFAULT);

        lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_flex_main_place(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_STATE_DEFAULT);

        lv_obj_t *label1 = lv_label_create(scr);
        lv_label_set_text(label1, "Label");
        lv_obj_center(label1);
        lv_obj_set_width(label1, lv_pct(100));
        lv_obj_add_style(label1, &style_label, LV_STATE_DEFAULT);

        lvgl_port_unlock();
    }
    return ESP_OK;
}

esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initialize display manager");

    const gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_TFT_BL,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    spi_bus_config_t buscfg = {
        .sclk_io_num = BOARD_SPI_SCK,
        .mosi_io_num = BOARD_SPI_MOSI,
        .miso_io_num = BOARD_SPI_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = AMOLED_HEIGHT * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BOARD_TFT_DC,
        .cs_gpio_num = BOARD_TFT_CS,
        .pclk_hz = 27 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) 1, &io_config, &io_handle));

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_TFT_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 52, 40));

    ESP_ERROR_CHECK(gpio_set_level(BOARD_TFT_BL, 1));

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 12,
        .task_stack = 8192,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISPLAY_BUFFER_SIZE,
        .double_buffer = true,
        .hres = AMOLED_WIDTH,
        .vres = AMOLED_HEIGHT,
        .monochrome = false,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .full_refresh = DISPLAY_FULLRESH,
            .swap_bytes = true,
        }
    };

    disp_handle = lvgl_port_add_disp(&disp_cfg);

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    return ESP_OK;
}