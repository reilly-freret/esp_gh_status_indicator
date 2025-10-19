#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the utils module (SNTP client)
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t utils_init(void);

/**
 * @brief Get the current real-time from NTP server
 * 
 * @return time_t Current Unix timestamp, or 0 if SNTP is not synchronized
 */
time_t get_real_time(void);

/**
 * @brief Check if SNTP is synchronized
 * 
 * @return true if synchronized, false otherwise
 */
bool utils_is_time_synchronized(void);

/**
 * @brief Get the current real-time in human-readable format (HH:MM:SS)
 * 
 * @param timestamp Buffer to store the formatted time string (must be at least 9 bytes)
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t get_human_real_time(char *timestamp);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H
