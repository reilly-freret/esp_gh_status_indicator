#include "display_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gh_status_manager.h"
#include "sdkconfig.h"
#include "wifi_manager.h"

static const char *TAG = "github_status";

void write_status_to_display(const char *environment, const char *status) {
  display_manager_write_text(environment);
  int r = 255;
  int g = 255;
  int b = 0; // yellow
  if (strcmp(status, "success") == 0) {
    r = 0;
    g = 255;
    b = 0; // green
  } else if (strcmp(status, "failure") == 0 || strcmp(status, "error") == 0) {
    r = 255;
    g = 0;
    b = 0; // red
  } else if (strcmp(status, "pending") == 0 ||
             strcmp(status, "in_progress") == 0 ||
             strcmp(status, "queued") == 0 || strcmp(status, "inactive") == 0) {
    r = 128;
    g = 128;
    b = 128; // gray
  }
  text_config_t config = {
      .color =
          {
              .r = r,
              .g = g,
              .b = b,
          },
      .size = TEXT_SIZE_22,
  };
  display_manager_write_text_custom(status, config);
}

void app_main(void) {
  ESP_LOGI(TAG, "Starting...");

  ESP_LOGI(TAG, "ESP-IDF version: %s", esp_get_idf_version());
  ESP_LOGI(TAG, "Configuration:");
  ESP_LOGI(TAG, "  GitHub Username: %s", CONFIG_GITHUB_USERNAME);
  ESP_LOGI(TAG, "  GitHub Repository: %s", CONFIG_GITHUB_REPO);
  ESP_LOGI(TAG, "  Status Check Interval: %d seconds",
           CONFIG_STATUS_CHECK_INTERVAL);
  ESP_LOGI(TAG, "  WiFi SSID: %s", CONFIG_WIFI_SSID);
  ESP_LOGI(TAG, "  WiFi Password: %s", CONFIG_WIFI_PASSWORD);

  ESP_ERROR_CHECK(display_manager_init());

  // Small delay to ensure display is ready
  vTaskDelay(pdMS_TO_TICKS(100));
  ESP_ERROR_CHECK(display_manager_clear());
  ESP_ERROR_CHECK(display_manager_set_bg_color(0, 255, 255));

  ESP_ERROR_CHECK(display_manager_write_text_color("init wifi...", 0, 0, 0));
  ESP_ERROR_CHECK(wifi_manager_init());
  ESP_ERROR_CHECK(
      display_manager_write_text_color("  wifi connected", 0, 0, 0));
  vTaskDelay(pdMS_TO_TICKS(500));

  ESP_ERROR_CHECK(display_manager_write_text_color("init complete", 0, 0, 0));
  vTaskDelay(pdMS_TO_TICKS(500));

  display_manager_clear();
  display_manager_set_bg_color(0, 0, 0);
  display_manager_write_text("checking status...");

  while (1) {
    // Generate status variables and check deployment status
    #define X(env) char env##_status[32]; \
                   gh_check_deployment_status(#env, env##_status, sizeof(env##_status));
    ENVIRONMENTS
    #undef X

    display_manager_clear();
    
    // Generate display calls
    #define X(env) write_status_to_display(#env, env##_status);
    ENVIRONMENTS
    #undef X

    vTaskDelay(pdMS_TO_TICKS(CONFIG_STATUS_CHECK_INTERVAL * 1000));
  }
}
