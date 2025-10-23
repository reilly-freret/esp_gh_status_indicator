idf_component_register(SRCS "display_manager.c" "main.c" "wifi_manager.c" "gh_status_manager.c" "vercel_status_manager.c" "utils.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_lcd esp_lvgl_port esp_wifi esp_netif esp_event esp_http_client nvs_flash)
