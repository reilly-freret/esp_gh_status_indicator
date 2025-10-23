#pragma once

#include "esp_err.h"
#include "sdkconfig.h"

// Vercel API endpoints - build-time optimized
#define VERCEL_API_BASE "https://api.vercel.com/v6/deployments"
#define VERCEL_DEPLOYMENTS_BASE "https://api.vercel.com/v6/deployments"

// Pre-built URLs for each environment
#define VERCEL_ENVIRONMENTS \
    X(production) \
    X(staging) \
    X(preview)

// Generate environment URL lookup table at build time
// Note: Vercel uses 'target' parameter and 'development' maps to 'staging'
#define X(env) {#env, VERCEL_DEPLOYMENTS_BASE "?projectId=" CONFIG_VERCEL_PROJECT_ID "&teamId=" CONFIG_VERCEL_TEAM_ID "&target=" #env "&limit=1"},
static const struct {
    const char* name;
    const char* url;
} vercel_environment_urls[] = {
    VERCEL_ENVIRONMENTS
    {NULL, NULL} // Sentinel
};
#undef X

// Special handling for staging (maps to development in Vercel)
#define X(env) {#env, VERCEL_DEPLOYMENTS_BASE "?projectId=" CONFIG_VERCEL_PROJECT_ID "&teamId=" CONFIG_VERCEL_TEAM_ID "&target=development&limit=1"},
static const struct {
    const char* name;
    const char* url;
} vercel_staging_url = {
    "staging", VERCEL_DEPLOYMENTS_BASE "?projectId=" CONFIG_VERCEL_PROJECT_ID "&teamId=" CONFIG_VERCEL_TEAM_ID "&target=development&limit=1"
};

// Response buffer size (increased for Vercel API responses)
#define MAX_VERCEL_RESPONSE_SIZE 2048
#define MAX_VERCEL_URL_SIZE 256

// Function declarations
esp_err_t vercel_check_deployment_status(const char *environment, char *status, size_t status_size);
