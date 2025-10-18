#include "display_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "wifi_manager.h"

static const char *TAG = "github_status";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting...");

    ESP_LOGI(TAG, "ESP-IDF version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Configuration:");
    ESP_LOGI(TAG, "  GitHub Username: %s", CONFIG_GITHUB_USERNAME);
    ESP_LOGI(TAG, "  GitHub Repository: %s", CONFIG_GITHUB_REPO);
    ESP_LOGI(TAG, "  Status Check Interval: %d seconds", CONFIG_STATUS_CHECK_INTERVAL);
    ESP_LOGI(TAG, "  WiFi SSID: %s", CONFIG_WIFI_SSID);
    ESP_LOGI(TAG, "  WiFi Password: %s", CONFIG_WIFI_PASSWORD);

    ESP_ERROR_CHECK(display_manager_init());
    
    // Small delay to ensure display is ready
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(display_manager_clear());
    ESP_ERROR_CHECK(display_manager_set_bg_color(0, 255, 255));
    ESP_ERROR_CHECK(display_manager_write_text("starting up..."));
    
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(display_manager_write_text("  wifi connected"));
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_ERROR_CHECK(display_manager_write_text("  another label"));

    
    while (1) {
        ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Checking status for %s/%s every %d seconds", 
                 CONFIG_GITHUB_USERNAME, CONFIG_GITHUB_REPO, CONFIG_STATUS_CHECK_INTERVAL);
        
        vTaskDelay(pdMS_TO_TICKS(CONFIG_STATUS_CHECK_INTERVAL * 1000));
    }
}
