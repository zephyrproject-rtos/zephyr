/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <string.h>
#include <stdio.h>
#include "rest_api.h"

LOG_MODULE_REGISTER(rest_api, LOG_LEVEL_DBG);

/* Helper function to get OpenThread instance */
static otInstance *get_ot_instance(void)
{
    return openthread_get_default_instance();
}

/* REST API: GET /api/status */
int rest_api_status_handler(struct http_client_ctx *client, enum http_data_status status,
                           const struct http_request_ctx *request_ctx,
                           struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[1024];

    LOG_DBG("Status API called, status: %d", status);
    if (status == HTTP_SERVER_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            otDeviceRole role = otThreadGetDeviceRole(instance);
            const char *role_str = "unknown";
            
            switch (role) {
                case OT_DEVICE_ROLE_DISABLED: role_str = "disabled"; break;
                case OT_DEVICE_ROLE_DETACHED: role_str = "detached"; break;
                case OT_DEVICE_ROLE_CHILD: role_str = "child"; break;
                case OT_DEVICE_ROLE_ROUTER: role_str = "router"; break;
                case OT_DEVICE_ROLE_LEADER: role_str = "leader"; break;
            }

            uint16_t rloc16 = otThreadGetRloc16(instance);
            uint32_t partition_id = otThreadGetPartitionId(instance);
            uint8_t router_id = otThreadGetLeaderRouterId(instance);

            snprintf(json_response, sizeof(json_response),
                    "{\"status\":\"ok\",\"role\":\"%s\",\"rloc16\":\"0x%04x\",\"partition_id\":%u,\"router_id\":%u,\"thread_enabled\":%s}",
                    role_str, rloc16, partition_id, router_id,
                    otThreadGetDeviceRole(instance) != OT_DEVICE_ROLE_DISABLED ? "true" : "false");
        }
        response_ctx->body = json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
    }
    return 0;
}

/* REST API: GET /api/thread/start */
int rest_api_thread_start_handler(struct http_client_ctx *client, enum http_data_status status,
                                 const struct http_request_ctx *request_ctx,
                                 struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[256];

    LOG_DBG("Thread start API called, status: %d", status);
    if (status == HTTP_SERVER_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            otError error = otThreadSetEnabled(instance, true);
            if (error != OT_ERROR_NONE) {
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to start Thread\",\"error_code\":%d}", error);
            } else {
                snprintf(json_response, sizeof(json_response),
                        "{\"status\":\"success\",\"message\":\"Thread network started\"}");
            }
        }
        response_ctx->body = json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
    }
    return 0;
}

/* REST API: GET /api/thread/stop */
int rest_api_thread_stop_handler(struct http_client_ctx *client, enum http_data_status status,
                                const struct http_request_ctx *request_ctx,
                                struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[256];

    LOG_DBG("Thread stop API called, status: %d", status);
    if (status == HTTP_SERVER_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            otError error = otThreadSetEnabled(instance, false);
            if (error != OT_ERROR_NONE) {
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to stop Thread\",\"error_code\":%d}", error);
            } else {
                snprintf(json_response, sizeof(json_response),
                        "{\"status\":\"success\",\"message\":\"Thread network stopped\"}");
            }
        }
        response_ctx->body = json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
    }
    return 0;
}

int rest_api_init(void)
{
    LOG_INF("REST API initialized with OpenThread integration");
    return 0;
}
