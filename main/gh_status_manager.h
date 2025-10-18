#pragma once

#include "esp_err.h"
#include "sdkconfig.h"

// GitHub API endpoints - build-time optimized
#define GITHUB_API_BASE "https://api.github.com"
#define GITHUB_DEPLOYMENTS_BASE "https://api.github.com/repos/" CONFIG_GITHUB_USERNAME "/" CONFIG_GITHUB_REPO "/deployments"
#define GITHUB_STATUSES_BASE "https://api.github.com/repos/" CONFIG_GITHUB_USERNAME "/" CONFIG_GITHUB_REPO "/deployments"

// Pre-built URLs for each environment
#define ENVIRONMENTS \
    X(production) \
    X(staging) \
    X(preview)

// Generate environment URL lookup table at build time
#define X(env) {#env, GITHUB_DEPLOYMENTS_BASE "?environment=" #env "&per_page=1"},
static const struct {
    const char* name;
    const char* url;
} environment_urls[] = {
    ENVIRONMENTS
    {NULL, NULL} // Sentinel
};
#undef X

// Response buffer size (keep under 1KB total)
#define MAX_RESPONSE_SIZE 512
#define MAX_URL_SIZE 256

// Function declarations
esp_err_t gh_check_deployment_status(const char *environment, char *status, size_t status_size);
