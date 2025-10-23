#include "utils.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include <sys/time.h>
#include <time.h>

static const char *TAG = "utils";

// Event group bits for SNTP synchronization
#define SNTP_SYNC_BIT BIT0

static EventGroupHandle_t sntp_event_group;
static bool sntp_synchronized = false;

/**
 * @brief SNTP notification callback
 */
static void sntp_notification_cb(struct timeval *tv) {
  ESP_LOGI(TAG, "SNTP notification received");
  sntp_synchronized = true;
  xEventGroupSetBits(sntp_event_group, SNTP_SYNC_BIT);
}

esp_err_t utils_init(void) {
  ESP_LOGI(TAG, "Initializing utils module");

  // Create event group for SNTP synchronization
  sntp_event_group = xEventGroupCreate();
  if (sntp_event_group == NULL) {
    ESP_LOGE(TAG, "Failed to create event group");
    return ESP_FAIL;
  }

  // Initialize SNTP
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org"); // Use default server for now
  esp_sntp_set_time_sync_notification_cb(sntp_notification_cb);
  esp_sntp_init();

  ESP_LOGI(TAG, "SNTP initialized with server pool.ntp.org, waiting for time "
                "synchronization...");

  // Wait for SNTP synchronization (with timeout)
  EventBits_t bits =
      xEventGroupWaitBits(sntp_event_group, SNTP_SYNC_BIT, pdFALSE, pdTRUE,
                          pdMS_TO_TICKS(10000)); // 10 second timeout

  if (bits & SNTP_SYNC_BIT) {
    ESP_LOGI(TAG, "Time synchronized successfully");
    sntp_synchronized = true;
    return ESP_OK;
  } else {
    ESP_LOGW(TAG, "Time synchronization timeout");
    return ESP_ERR_TIMEOUT;
  }
}

time_t get_real_time(void) {
  if (!sntp_synchronized) {
    ESP_LOGW(TAG, "SNTP not synchronized, returning 0");
    return 0;
  }

  time_t now = 0;
  struct timeval tv = {0};

  if (gettimeofday(&tv, NULL) == 0) {
    now = tv.tv_sec;
  } else {
    ESP_LOGE(TAG, "Failed to get time");
    return 0;
  }

  return now;
}

bool utils_is_time_synchronized(void) { return sntp_synchronized; }

esp_err_t get_human_real_time(char *timestamp) {
  if (timestamp == NULL) {
    ESP_LOGE(TAG, "Timestamp buffer is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  if (!sntp_synchronized) {
    ESP_LOGW(TAG, "SNTP not synchronized, returning error");
    return ESP_ERR_INVALID_STATE;
  }

  time_t now = get_real_time();
  if (now == 0) {
    ESP_LOGE(TAG, "Failed to get real time");
    return ESP_FAIL;
  }

  // adjust to local time
  now += CONFIG_TZ_OFFSET * 3600;

  struct tm timeinfo;
  if (localtime_r(&now, &timeinfo) == NULL) {
    ESP_LOGE(TAG, "Failed to convert time to local time");
    return ESP_FAIL;
  }

  // Format as 12-hour HH:MM:SS
  int hour_12 = timeinfo.tm_hour % 12;
  if (hour_12 == 0)
    hour_12 = 12; // Convert 0 to 12 for 12-hour format

  snprintf(timestamp, 16, "%02d:%02d:%02d", hour_12, timeinfo.tm_min,
           timeinfo.tm_sec);

  return ESP_OK;
}
