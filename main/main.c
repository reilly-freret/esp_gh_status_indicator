#include "display_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gh_status_manager.h"
#include "vercel_status_manager.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "wifi_manager.h"
#include "utils.h"

static const char *TAG = "github_status";

void write_status_to_display(const char *environment, const char *status) {
  display_manager_write_text(environment);
  int r = 255;
  int g = 255;
  int b = 0; // yellow
  if (strcasecmp(status, "success") == 0) {
    r = 0;
    g = 255;
    b = 0; // green
  } else if (strcasecmp(status, "failure") == 0 || strcasecmp(status, "error") == 0) {
    r = 255;
    g = 0;
    b = 0; // red
  } else if (strcasecmp(status, "pending") == 0 ||
             strcasecmp(status, "in_progress") == 0 || strcasecmp(status, "in progress") == 0  ||
             strcasecmp(status, "queued") == 0 || strcasecmp(status, "inactive") == 0 || strcasecmp(status, "unknown") == 0) {
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
  #ifdef CONFIG_USE_VERCEL
    ESP_LOGI(TAG, "  Using Vercel API");
    ESP_LOGI(TAG, "  Vercel Team ID: %s", CONFIG_VERCEL_TEAM_ID);
    ESP_LOGI(TAG, "  Vercel Project ID: %s", CONFIG_VERCEL_PROJECT_ID);
  #else
    ESP_LOGI(TAG, "  Using GitHub API");
    ESP_LOGI(TAG, "  GitHub Username: %s", CONFIG_GITHUB_USERNAME);
    ESP_LOGI(TAG, "  GitHub Repository: %s", CONFIG_GITHUB_REPO);
  #endif
  ESP_LOGI(TAG, "  Status Check Interval: %d seconds",
           CONFIG_STATUS_CHECK_INTERVAL);
  ESP_LOGI(TAG, "  WiFi SSID: %s", CONFIG_WIFI_SSID);
  ESP_LOGI(TAG, "  WiFi Password: %s", CONFIG_WIFI_PASSWORD);

  display_manager_init();

  // Small delay to ensure display is ready
  vTaskDelay(pdMS_TO_TICKS(100));
  display_manager_clear();
  display_manager_set_bg_color(0, 255, 255);

  display_manager_write_text_color("init wifi...", 0, 0, 0);
  esp_err_t err = wifi_manager_init();
  if (err != ESP_OK) {
    display_manager_set_bg_color(255, 0, 0);
    display_manager_write_text_color("  failed to connect", 0, 0, 0);
    vTaskDelay(portMAX_DELAY);
  }
  display_manager_write_text_color("  wifi connected", 0, 0, 0);
  vTaskDelay(pdMS_TO_TICKS(500));

  // Initialize utils module (SNTP)
  display_manager_write_text_color("init time sync...", 0, 0, 0);
  esp_err_t time_err = utils_init();
  if (time_err != ESP_OK) {
    display_manager_set_bg_color(255, 0, 0);
    display_manager_write_text_color("  time sync failed", 0, 0, 0);
    vTaskDelay(portMAX_DELAY);
  }
  display_manager_write_text_color("  time synced", 0, 0, 0);
  vTaskDelay(pdMS_TO_TICKS(500));

  display_manager_write_text_color("init complete", 0, 0, 0);
  vTaskDelay(pdMS_TO_TICKS(500));

  display_manager_clear();
  display_manager_set_bg_color(0, 0, 0);
  display_manager_write_text("checking status...");
  vTaskDelay(pdMS_TO_TICKS(1000));

  while (1) {
    // Generate status variables and check deployment status
    #ifdef CONFIG_USE_VERCEL
      // Use Vercel API
      #define CHECK_ENV(env) char env##_status[32]; \
                             vercel_check_deployment_status(#env, env##_status, sizeof(env##_status));
      CHECK_ENV(production)
      CHECK_ENV(staging)
      CHECK_ENV(preview)
      #undef CHECK_ENV
    #else
      // Use GitHub API
      #define CHECK_ENV(env) char env##_status[32]; \
                             gh_check_deployment_status(#env, env##_status, sizeof(env##_status));
      CHECK_ENV(production)
      CHECK_ENV(staging)
      CHECK_ENV(preview)
      #undef CHECK_ENV
    #endif

    display_manager_clear();
    
    // Generate display calls
    #ifdef CONFIG_USE_VERCEL
      write_status_to_display("production", production_status);
      write_status_to_display("staging", staging_status);
      write_status_to_display("preview", preview_status);
    #else
      write_status_to_display("production", production_status);
      write_status_to_display("staging", staging_status);
      write_status_to_display("preview", preview_status);
    #endif

    char time_str[9];
    get_human_real_time(time_str);
    char last_checked_str[32];
    snprintf(last_checked_str, sizeof(last_checked_str), "checked: %s", time_str);
    display_manager_write_text_bottom(last_checked_str);

    vTaskDelay(pdMS_TO_TICKS(CONFIG_STATUS_CHECK_INTERVAL * 1000));
  }
}
