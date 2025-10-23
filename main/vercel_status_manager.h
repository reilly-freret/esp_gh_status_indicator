#pragma once

#include "esp_err.h"
#include "sdkconfig.h"

// Vercel API endpoint
#define VERCEL_DEPLOYMENTS_BASE "https://api.vercel.com/v6/deployments"

// Pre-built URLs for each environment
#define VERCEL_ENVIRONMENTS                                                    \
  X(production)                                                                \
  X(staging)

// Generate environment URL lookup table at build time
#define X(env)                                                                 \
  {#env, VERCEL_DEPLOYMENTS_BASE "?projectId=" CONFIG_VERCEL_PROJECT_ID        \
                                 "&teamId=" CONFIG_VERCEL_TEAM_ID              \
                                 "&target=" #env "&limit=1"},
static const struct {
  const char *name;
  const char *url;
} vercel_environment_urls[] = {
    VERCEL_ENVIRONMENTS{NULL, NULL} // Sentinel
};
#undef X

// Response buffer size (sufficient for single deployment response)
#define MAX_VERCEL_RESPONSE_SIZE 2048

// Function declarations
esp_err_t vercel_check_deployment_status(const char *environment, char *status,
                                         size_t status_size);
