#include "display_manager.h"
#include "config.h"
#include "display/lv_display.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "font/lv_font.h"
#include "misc/lv_color.h"
#include "sdkconfig.h"

static const char *TAG = "display_manager";
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_t * disp_handle = NULL;
static lv_obj_t * main_content_container = NULL;

static void create_main_content_container(void) {
    if (main_content_container) {
        lv_obj_del(main_content_container);
    }
    
    lv_obj_t *scr = lv_scr_act();
    main_content_container = lv_obj_create(scr);
    lv_obj_remove_style_all(main_content_container);
    lv_obj_set_size(main_content_container, lv_disp_get_hor_res(disp_handle), lv_disp_get_ver_res(disp_handle));
    lv_obj_set_pos(main_content_container, 0, 0);
    
    // Set up flex layout for main content
    lv_obj_set_layout(main_content_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_content_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_content_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
}

const lv_font_t *get_font(enum text_size size) {
    switch (size) {
        #ifdef CONFIG_LV_FONT_MONTSERRAT_8
        case TEXT_SIZE_8:
            return &lv_font_montserrat_8;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_10
        case TEXT_SIZE_10:
            return &lv_font_montserrat_10;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_12
        case TEXT_SIZE_12:
            return &lv_font_montserrat_12;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_14
        case TEXT_SIZE_14:
            return &lv_font_montserrat_14;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_16
        case TEXT_SIZE_16:
            return &lv_font_montserrat_16;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_18
        case TEXT_SIZE_18:
            return &lv_font_montserrat_18;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_20
        case TEXT_SIZE_20:
            return &lv_font_montserrat_20;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_22
        case TEXT_SIZE_22:
            return &lv_font_montserrat_22;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_24
        case TEXT_SIZE_24:
            return &lv_font_montserrat_24;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_26
        case TEXT_SIZE_26:
            return &lv_font_montserrat_26;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_28
        case TEXT_SIZE_28:
            return &lv_font_montserrat_28;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_30
        case TEXT_SIZE_30:
            return &lv_font_montserrat_30;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_32
        case TEXT_SIZE_32:
            return &lv_font_montserrat_32;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_34
        case TEXT_SIZE_34:
            return &lv_font_montserrat_34;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_36
        case TEXT_SIZE_36:
            return &lv_font_montserrat_36;
        #endif
        #ifdef CONFIG_LV_FONT_MONTSERRAT_38
        case TEXT_SIZE_38:
            return &lv_font_montserrat_38;
        #endif
        default:
            return lv_font_get_default();
    }
}

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
        
        // Recreate the main content container
        create_main_content_container();
        
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_write_text(const char *text) {
    if (lvgl_port_lock(0)) {
        // Ensure main content container exists
        if (!main_content_container) {
            create_main_content_container();
        }
        
        lv_obj_t *label = lv_label_create(main_content_container);
        lv_label_set_text(label, text);
        
        // Set text color to white
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_write_text_color(const char *text, int16_t r, int16_t g, int16_t b) {
    if (lvgl_port_lock(0)) {
        // Ensure main content container exists
        if (!main_content_container) {
            create_main_content_container();
        }
        
        lv_obj_t *label = lv_label_create(main_content_container);
        lv_label_set_text(label, text);
        
        // Set text color using provided RGB values
        lv_color_t text_color = lv_color_make(r, g, b);
        lv_obj_set_style_text_color(label, text_color, 0);
        
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_write_text_bottom(const char *text) {
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_scr_act();
        
        // Get screen dimensions
        lv_coord_t screen_width = lv_disp_get_hor_res(disp_handle);
        lv_coord_t screen_height = lv_disp_get_ver_res(disp_handle);
        
        // Create a container for the bottom text that bypasses flex layout
        lv_obj_t *bottom_container = lv_obj_create(scr);
        lv_obj_remove_style_all(bottom_container);
        lv_obj_set_size(bottom_container, screen_width, CONFIG_BOTTOM_TEXT_HEIGHT); // Fixed height for bottom text
        
        // Position container at the very bottom using absolute coordinates
        lv_obj_set_pos(bottom_container, 0, screen_height - CONFIG_BOTTOM_TEXT_HEIGHT);
        
        // Create a label inside the container
        lv_obj_t *label = lv_label_create(bottom_container);
        lv_label_set_text(label, text);
        lv_obj_set_width(label, screen_width);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        
        // Set text color to white
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t display_manager_write_text_custom(const char *text, text_config_t config) {
    if (lvgl_port_lock(0)) {
        // Ensure main content container exists
        if (!main_content_container) {
            create_main_content_container();
        }
        
        lv_obj_t *label = lv_label_create(main_content_container);
        lv_label_set_text(label, text);
        
        // Set text color using provided RGB values
        lv_color_t text_color = lv_color_make(config.color.r, config.color.g, config.color.b);
        lv_obj_set_style_text_color(label, text_color, 0);
        
        // Set text size using the get_font function
        const lv_font_t *font = get_font(config.size);
        lv_obj_set_style_text_font(label, font, 0);
        
        lvgl_port_unlock();
        return ESP_OK;
    }
    return ESP_FAIL;
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
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, X_OFFSET, Y_OFFSET));

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

    #if CONFIG_MIKES_WAY
    lv_disp_set_rotation(disp_handle, LV_DISPLAY_ROTATION_180);
    #endif

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    return ESP_OK;
}