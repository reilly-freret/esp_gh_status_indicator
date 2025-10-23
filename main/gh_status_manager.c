#include "gh_status_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "string.h"
#include <stdio.h>

static const char *TAG = "GH_STATUS";

// Static buffers to avoid dynamic allocation
static char response_buffer[MAX_RESPONSE_SIZE];
static char url_buffer[MAX_URL_SIZE];

// Helper function to get pre-built URL for environment
static const char *get_deployments_url(const char *environment) {
  for (int i = 0; environment_urls[i].name != NULL; i++) {
    if (strcmp(environment, environment_urls[i].name) == 0) {
      return environment_urls[i].url;
    }
  }
  return NULL; // Unknown environment
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
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
      size_t buffer_len = strlen(response_buffer);

      // Prevent buffer overflow
      if (buffer_len + data_len >= MAX_RESPONSE_SIZE) {
        data_len = MAX_RESPONSE_SIZE - buffer_len - 1;
      }

      if (data_len > 0) {
        strncat(response_buffer, evt->data, data_len);
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

static esp_err_t parse_json_field(const char *json, const char *field_name,
                                  char *value, size_t value_size) {
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

static esp_err_t get_deployment_id(const char *environment, char *deployment_id,
                                   size_t id_size) {
  if (!environment || !deployment_id || id_size == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  // Clear response buffer
  memset(response_buffer, 0, sizeof(response_buffer));

  // Get pre-built URL for environment
  const char *url = get_deployments_url(environment);
  if (!url) {
    ESP_LOGE(TAG, "Unknown environment: %s", environment);
    return ESP_ERR_INVALID_ARG;
  }

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_GET,
      .event_handler = http_event_handler,
      .buffer_size = MAX_RESPONSE_SIZE,
      .buffer_size_tx = MAX_RESPONSE_SIZE,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return ESP_ERR_NO_MEM;
  }

  // Set GitHub token header
  esp_http_client_set_header(client, "Authorization",
                             "token " CONFIG_GITHUB_AUTH_TOKEN);
  esp_http_client_set_header(client, "User-Agent", "ESP32-GitHub-Status");
  esp_http_client_set_header(client, "Accept",
                             "application/vnd.github.v3+json");

  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status: %d", status_code);

    if (status_code == 200) {
      err = parse_json_field(response_buffer, "id", deployment_id, id_size);
      if (err == ESP_OK) {
        ESP_LOGI(TAG, "Found deployment ID: %s", deployment_id);
      } else {
        ESP_LOGE(TAG, "Failed to parse deployment ID");
      }
    } else {
      ESP_LOGE(TAG, "HTTP request failed with status %d", status_code);
      err = ESP_FAIL;
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  return err;
}

static esp_err_t get_deployment_status(const char *deployment_id, char *status,
                                       size_t status_size) {
  if (!deployment_id || !status || status_size == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  // Clear response buffer
  memset(response_buffer, 0, sizeof(response_buffer));

  // Build URL for statuses endpoint
  snprintf(url_buffer, sizeof(url_buffer), "%s/%s/statuses?per_page=1",
           GITHUB_STATUSES_BASE, deployment_id);

  esp_http_client_config_t config = {
      .url = url_buffer,
      .method = HTTP_METHOD_GET,
      .event_handler = http_event_handler,
      .buffer_size = MAX_RESPONSE_SIZE,
      .buffer_size_tx = MAX_RESPONSE_SIZE,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return ESP_ERR_NO_MEM;
  }

  // Set GitHub token header
  esp_http_client_set_header(client, "Authorization",
                             "token " CONFIG_GITHUB_AUTH_TOKEN);
  esp_http_client_set_header(client, "User-Agent", "ESP32-GitHub-Status");
  esp_http_client_set_header(client, "Accept",
                             "application/vnd.github.v3+json");

  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status: %d", status_code);

    if (status_code == 200) {
      err = parse_json_field(response_buffer, "state", status, status_size);
      if (err == ESP_OK) {
        ESP_LOGI(TAG, "Found deployment status: %s", status);
      } else {
        ESP_LOGE(TAG, "Failed to parse deployment status");
      }
    } else {
      ESP_LOGE(TAG, "HTTP request failed with status %d", status_code);
      err = ESP_FAIL;
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  return err;
}

esp_err_t gh_check_deployment_status(const char *environment, char *status,
                                     size_t status_size) {
  if (!environment || !status || status_size == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  char deployment_id[32];

  // Step 1: Get deployment ID
  esp_err_t err =
      get_deployment_id(environment, deployment_id, sizeof(deployment_id));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get deployment ID: %s", esp_err_to_name(err));
    snprintf(status, status_size, "unknown");
    return err;
  }

  // Step 2: Get deployment status
  err = get_deployment_status(deployment_id, status, status_size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get deployment status: %s", esp_err_to_name(err));
    snprintf(status, status_size, "unknown");
    return err;
  }

  return ESP_OK;
}
