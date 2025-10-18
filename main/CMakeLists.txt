idf_component_register(SRCS "display_manager.c" "main.c" "wifi_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_lcd esp_lvgl_port esp_wifi esp_netif esp_event nvs_flash)
