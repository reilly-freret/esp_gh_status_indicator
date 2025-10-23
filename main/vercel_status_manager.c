#include "vercel_status_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "string.h"
#include <stdio.h>

static const char *TAG = "VERCEL_STATUS";

// Static buffers to avoid dynamic allocation
static char vercel_response_buffer[MAX_VERCEL_RESPONSE_SIZE];

// Helper function to get pre-built URL for environment
static const char* get_vercel_deployments_url(const char* environment) {
    // Special case for staging (maps to development in Vercel)
    if (strcmp(environment, "staging") == 0) {
        return vercel_staging_url.url;
    }
    
    for (int i = 0; vercel_environment_urls[i].name != NULL; i++) {
        if (strcmp(environment, vercel_environment_urls[i].name) == 0) {
            return vercel_environment_urls[i].url;
        }
    }
    return NULL; // Unknown environment
}

static esp_err_t vercel_http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP headers sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                size_t data_len = evt->data_len;
                size_t buffer_len = strlen(vercel_response_buffer);
                
                // Prevent buffer overflow
                if (buffer_len + data_len >= MAX_VERCEL_RESPONSE_SIZE) {
                    data_len = MAX_VERCEL_RESPONSE_SIZE - buffer_len - 1;
                }
                
                if (data_len > 0) {
                    strncat(vercel_response_buffer, evt->data, data_len);
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP request finished");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP disconnected");
            break;
        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t parse_vercel_json_field(const char *json, const char *field_name, char *value, size_t value_size)
{
    if (!json || !field_name || !value || value_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", field_name);
    
    const char *field_start = strstr(json, search_pattern);
    if (!field_start) {
        return ESP_ERR_NOT_FOUND;
    }
    
    const char *value_start = strchr(field_start, ':');
    if (!value_start) {
        return ESP_ERR_NOT_FOUND;
    }
    
    value_start++; // Skip ':'
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++; // Skip whitespace
    }
    
    const char *value_end;
    if (*value_start == '"') {
        // String value
        value_start++; // Skip opening quote
        value_end = strchr(value_start, '"');
        if (!value_end) {
            return ESP_ERR_INVALID_RESPONSE;
        }
    } else {
        // Numeric value
        value_end = value_start;
        while ((*value_end >= '0' && *value_end <= '9') || *value_end == '.') {
            value_end++;
        }
        // Don't include trailing comma, bracket, or whitespace in the value
    }
    
    size_t value_len = value_end - value_start;
    if (value_len >= value_size) {
        value_len = value_size - 1;
    }
    
    strncpy(value, value_start, value_len);
    value[value_len] = '\0';
    
    return ESP_OK;
}

static esp_err_t parse_vercel_deployment_status(const char *json, char *status, size_t status_size)
{
    if (!json || !status || status_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Debug: Log the response for troubleshooting
    ESP_LOGI(TAG, "Vercel API response: %s", json);
    
    // Check if response looks complete (ends with } or ])
    size_t response_len = strlen(json);
    if (response_len > 0 && json[response_len - 1] != '}' && json[response_len - 1] != ']') {
        ESP_LOGW(TAG, "Response may be truncated - last char: %c", json[response_len - 1]);
    }
    
    // Vercel API returns: {"deployments": [...]}
    // First, look for the "deployments" key
    const char *deployments_key = strstr(json, "\"deployments\"");
    if (!deployments_key) {
        ESP_LOGE(TAG, "No 'deployments' key found in Vercel response");
        ESP_LOGE(TAG, "Response was: %s", json);
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Find the array after "deployments":
    const char *array_start = strstr(deployments_key, "[");
    if (!array_start) {
        ESP_LOGE(TAG, "No deployments array found in Vercel response");
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Check if array is empty
    const char *array_end = strstr(array_start, "]");
    if (!array_end) {
        ESP_LOGE(TAG, "Malformed array in Vercel response - response may be truncated");
        ESP_LOGE(TAG, "Array start: %s", array_start);
        // Check if response looks truncated (no closing bracket)
        if (strlen(array_start) > 100) {
            ESP_LOGE(TAG, "Response appears to be truncated - buffer too small");
        }
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Check if array is empty (just "[]")
    if (array_end - array_start <= 2) {
        ESP_LOGW(TAG, "Empty deployments array - no deployments found for this environment");
        strncpy(status, "NO DEPLOYMENTS", status_size - 1);
        status[status_size - 1] = '\0';
        return ESP_OK;
    }
    
    // Find the first object in the array
    const char *object_start = strstr(array_start, "{");
    if (!object_start) {
        ESP_LOGE(TAG, "No deployment object found in Vercel response");
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Parse the state field from the first deployment object
    esp_err_t err = parse_vercel_json_field(object_start, "state", status, status_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse deployment state from Vercel response");
        return err;
    }
    
    // Map Vercel states to display-friendly strings
    if (strcmp(status, "READY") == 0) {
        strncpy(status, "SUCCESS", status_size - 1);
        status[status_size - 1] = '\0';
    } else if (strcmp(status, "BUILDING") == 0) {
        strncpy(status, "IN PROGRESS", status_size - 1);
        status[status_size - 1] = '\0';
    } else if (strcmp(status, "ERROR") == 0) {
        strncpy(status, "FAILURE", status_size - 1);
        status[status_size - 1] = '\0';
    } else if (strcmp(status, "CANCELED") == 0) {
        strncpy(status, "CANCELED", status_size - 1);
        status[status_size - 1] = '\0';
    }
    // For any other states, keep the original value
    
    return ESP_OK;
}

esp_err_t vercel_check_deployment_status(const char *environment, char *status, size_t status_size)
{
    if (!environment || !status || status_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Clear response buffer
    memset(vercel_response_buffer, 0, sizeof(vercel_response_buffer));
    
    // Get pre-built URL for environment
    const char* url = get_vercel_deployments_url(environment);
    if (!url) {
        ESP_LOGE(TAG, "Unknown environment: %s", environment);
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = vercel_http_event_handler,
        .buffer_size = MAX_VERCEL_RESPONSE_SIZE,
        .buffer_size_tx = MAX_VERCEL_RESPONSE_SIZE,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_ERR_NO_MEM;
    }
    
    // Set Vercel token header
    esp_http_client_set_header(client, "Authorization", "Bearer " CONFIG_VERCEL_AUTH_TOKEN);
    esp_http_client_set_header(client, "User-Agent", "ESP32-Vercel-Status");
    esp_http_client_set_header(client, "Accept", "application/json");
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status: %d", status_code);
        
        if (status_code == 200) {
            err = parse_vercel_deployment_status(vercel_response_buffer, status, status_size);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Found Vercel deployment status: %s", status);
            } else {
                ESP_LOGE(TAG, "Failed to parse Vercel deployment status");
                snprintf(status, status_size, "unknown");
            }
        } else {
            ESP_LOGE(TAG, "Vercel HTTP request failed with status %d", status_code);
            snprintf(status, status_size, "unknown");
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "Vercel HTTP request failed: %s", esp_err_to_name(err));
        snprintf(status, status_size, "unknown");
    }
    
    esp_http_client_cleanup(client);
    return err;
}
