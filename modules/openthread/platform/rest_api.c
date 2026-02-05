/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/client.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/cli.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rest_api.h"

LOG_MODULE_REGISTER(rest_api, LOG_LEVEL_INF);

/* Buffer for receiving POST data */
static char post_data_buffer[1024];
static size_t post_data_len = 0;

/* Helper function to get OpenThread instance */
static otInstance *get_ot_instance(void)
{
    return openthread_get_default_instance();
}

/* Helper function to parse URL-encoded parameter */
static bool get_url_param(const char *data, const char *param_name, char *output, size_t output_size)
{
    char search_str[64];
    snprintf(search_str, sizeof(search_str), "%s=", param_name);
    
    const char *start = strstr(data, search_str);
    if (!start) {
        return false;
    }
    
    start += strlen(search_str);
    const char *end = strchr(start, '&');
    size_t len;
    
    if (end) {
        len = end - start;
    } else {
        len = strlen(start);
    }
    
    if (len >= output_size) {
        len = output_size - 1;
    }
    
    memcpy(output, start, len);
    output[len] = '\0';
    
    return true;
}

/* Helper function to parse JSON string value */
static bool get_json_string_value(const char *json, const char *key, char *output, size_t output_size)
{
    char search_str[64];
    snprintf(search_str, sizeof(search_str), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_str);
    if (!key_pos) {
        LOG_DBG("Key '%s' not found in JSON", key);
        return false;
    }
    
    // Chercher le ':' après la clé
    const char *colon = strchr(key_pos, ':');
    if (!colon) {
        LOG_DBG("No colon found after key '%s'", key);
        return false;
    }
    
    // Sauter les espaces et trouver le '"' de début
    const char *start = colon + 1;
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    
    if (*start != '"') {
        LOG_DBG("No opening quote found for key '%s'", key);
        return false;
    }
    start++; // Sauter le '"'
    
    // Trouver le '"' de fin (non échappé)
    const char *end = start;
    while (*end && *end != '"') {
        if (*end == '\\' && *(end + 1) == '"') {
            end += 2; // Sauter les guillemets échappés
        } else {
            end++;
        }
    }
    
    if (*end != '"') {
        LOG_DBG("No closing quote found for key '%s'", key);
        return false;
    }
    
    size_t len = end - start;
    if (len >= output_size) {
        len = output_size - 1;
    }
    
    memcpy(output, start, len);
    output[len] = '\0';
    
    LOG_DBG("Extracted '%s' = '%s'", key, output);
    return true;
}

/* Helper function to convert hex string to bytes */
static int hex_to_bytes(const char *hex_str, uint8_t *bytes, size_t max_bytes)
{
    size_t len = strlen(hex_str);
    if (len % 2 != 0 || len / 2 > max_bytes) {
        return -1;
    }
    
    for (size_t i = 0; i < len / 2; i++) {
        char byte_str[3] = {hex_str[i * 2], hex_str[i * 2 + 1], '\0'};
        bytes[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }
    
    return len / 2;
}

/* REST API: GET /api/status */
int rest_api_status_handler(struct http_client_ctx *client, enum http_transaction_status status,
                           const struct http_request_ctx *request_ctx,
                           struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[1024];
    
    LOG_DBG("Status API called, status: %d", status);
    
    if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            openthread_mutex_lock();
            otDeviceRole role = otThreadGetDeviceRole(instance);
            const char *role_str = "unknown";
            openthread_mutex_unlock();
            
            switch (role) {
                case OT_DEVICE_ROLE_DISABLED: role_str = "disabled"; break;
                case OT_DEVICE_ROLE_DETACHED: role_str = "detached"; break;
                case OT_DEVICE_ROLE_CHILD: role_str = "child"; break;
                case OT_DEVICE_ROLE_ROUTER: role_str = "router"; break;
                case OT_DEVICE_ROLE_LEADER: role_str = "leader"; break;
                default:break;
            }

            openthread_mutex_lock();
            uint16_t rloc16 = otThreadGetRloc16(instance);
            openthread_mutex_unlock();
            openthread_mutex_lock();
            uint32_t partition_id = otThreadGetPartitionId(instance);
            openthread_mutex_unlock();
            openthread_mutex_lock();
            uint8_t router_id = otThreadGetLeaderRouterId(instance);
            openthread_mutex_unlock();

            openthread_mutex_lock();
            snprintf(json_response, sizeof(json_response),
                    "{\"status\":\"ok\",\"role\":\"%s\",\"rloc16\":\"0x%04x\",\"partition_id\":%u,\"router_id\":%u,\"thread_enabled\":%s}",
                    role_str, rloc16, partition_id, router_id,
                    otThreadGetDeviceRole(instance) != OT_DEVICE_ROLE_DISABLED ? "true" : "false");
            openthread_mutex_unlock();
        }
        
        response_ctx->body = (const uint8_t *)json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
        response_ctx->status = HTTP_200_OK;
    }
    
    return 0;
}

/* REST API: GET /api/dataset */
int rest_api_dataset_get_handler(struct http_client_ctx *client, enum http_transaction_status status,
                                 const struct http_request_ctx *request_ctx,
                                 struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[2048];
    
    LOG_DBG("Dataset GET API called, status: %d", status);
    
    if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            otOperationalDataset dataset;
            openthread_mutex_lock();
            otError error = otDatasetGetActive(instance, &dataset);
            openthread_mutex_unlock();
            
            if (error != OT_ERROR_NONE) {
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to get dataset\",\"error_code\":%d}", error);
            } else {
                char network_name[OT_NETWORK_NAME_MAX_SIZE + 1] = {0};
                char pan_id_str[16] = "null";
                char extpanid_str[128] = "null";
                char channel_str[16] = "null";
                char network_key_str[128] = "null";
                
                if (dataset.mComponents.mIsNetworkNamePresent) {
                    size_t name_len = strnlen((const char *)dataset.mNetworkName.m8, OT_NETWORK_NAME_MAX_SIZE);
                    memcpy(network_name, dataset.mNetworkName.m8, name_len);
                    network_name[name_len] = '\0';
                }
                
                if (dataset.mComponents.mIsPanIdPresent) {
                    snprintf(pan_id_str, sizeof(pan_id_str), "\"0x%04x\"", dataset.mPanId);
                }
                
                if (dataset.mComponents.mIsExtendedPanIdPresent) {
                    char extpanid_hex[17];
                    for (int i = 0; i < OT_EXT_PAN_ID_SIZE; i++) {
                        snprintf(&extpanid_hex[i * 2], 3, "%02x", dataset.mExtendedPanId.m8[i]);
                    }
                    snprintf(extpanid_str, sizeof(extpanid_str), "\"%s\"", extpanid_hex);
                }
                
                if (dataset.mComponents.mIsChannelPresent) {
                    snprintf(channel_str, sizeof(channel_str), "%u", dataset.mChannel);
                }
                
                if (dataset.mComponents.mIsNetworkKeyPresent) {
                    char key_hex[65];
                    for (int i = 0; i < OT_NETWORK_KEY_SIZE; i++) {
                        snprintf(&key_hex[i * 2], 3, "%02x", dataset.mNetworkKey.m8[i]);
                    }
                    snprintf(network_key_str, sizeof(network_key_str), "\"%s\"", key_hex);
                }
                
                snprintf(json_response, sizeof(json_response),
                        "{\"status\":\"ok\",\"network_name\":\"%s\",\"pan_id\":%s,\"extpanid\":%s,\"channel\":%s,\"network_key\":%s}",
                        network_name, pan_id_str, extpanid_str, channel_str, network_key_str);
            }
        }
        
        response_ctx->body = (const uint8_t *)json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
        response_ctx->status = HTTP_200_OK;
    }
    
    return 0;
}

/* REST API: POST /api/network/config */
int rest_api_network_config_handler(struct http_client_ctx *client, enum http_transaction_status status,
                                    const struct http_request_ctx *request_ctx,
                                    struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[512];
    
    LOG_INF("Network config API called, status: %d", status);
    
    if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
        post_data_len = 0;
        memset(post_data_buffer, 0, sizeof(post_data_buffer));
        return 0;
    }
    
    if (status == HTTP_SERVER_REQUEST_DATA_MORE) {
        size_t remaining = sizeof(post_data_buffer) - post_data_len - 1;
        size_t to_copy = request_ctx->data_len < remaining ? request_ctx->data_len : remaining;
        
        LOG_DBG("Receiving more data: %d bytes (total so far: %d)", to_copy, post_data_len);
        
        memcpy(post_data_buffer + post_data_len, request_ctx->data, to_copy);
        post_data_len += to_copy;
        post_data_buffer[post_data_len] = '\0';
        
        return 0;
    }
    
    if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
        if (request_ctx->data_len > 0) {
            size_t remaining = sizeof(post_data_buffer) - post_data_len - 1;
            size_t to_copy = request_ctx->data_len < remaining ? request_ctx->data_len : remaining;
            
            LOG_DBG("Receiving final data: %d bytes (total before: %d)", to_copy, post_data_len);
            
            memcpy(post_data_buffer + post_data_len, request_ctx->data, to_copy);
            post_data_len += to_copy;
            post_data_buffer[post_data_len] = '\0';
        }
        
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
            goto send_response;
        }
        
        LOG_INF("Received POST data (%d bytes): %s", post_data_len, post_data_buffer);
        LOG_HEXDUMP_DBG(post_data_buffer, post_data_len, "POST data (hex):");
        
        if (post_data_len == 0) {
            LOG_ERR("No POST data received");
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"No data received\"}");
            goto send_response;
        }
        
        char network_name[OT_NETWORK_NAME_MAX_SIZE + 1] = {0};
        char pan_id_str[16] = {0};
        char extpanid_str[65] = {0};
        char channel_str[16] = {0};
        char network_key_str[65] = {0};
        
        bool has_name = false;
        bool has_pan = false;
        bool has_extpanid = false;
        bool has_channel = false;
        bool has_key = false;
        
        bool is_json = (post_data_buffer[0] == '{');
        
        if (is_json) {
            LOG_INF("Detected JSON format");            // Parser le JSON
            has_name = get_json_string_value(post_data_buffer, "network_name", network_name, sizeof(network_name));
            has_pan = get_json_string_value(post_data_buffer, "pan_id", pan_id_str, sizeof(pan_id_str));
            has_extpanid = get_json_string_value(post_data_buffer, "extpanid", extpanid_str, sizeof(extpanid_str));
            has_channel = get_json_string_value(post_data_buffer, "channel", channel_str, sizeof(channel_str));
            has_key = get_json_string_value(post_data_buffer, "network_key", network_key_str, sizeof(network_key_str));
        } else {
            LOG_INF("Detected URL-encoded format");
            has_name = get_url_param(post_data_buffer, "network_name", network_name, sizeof(network_name));
            has_pan = get_url_param(post_data_buffer, "pan_id", pan_id_str, sizeof(pan_id_str));
            has_extpanid = get_url_param(post_data_buffer, "extpanid", extpanid_str, sizeof(extpanid_str));
            has_channel = get_url_param(post_data_buffer, "channel", channel_str, sizeof(channel_str));
            has_key = get_url_param(post_data_buffer, "network_key", network_key_str, sizeof(network_key_str));
        }
        
        LOG_DBG("===== PARSED PARAMETERS =====");
        LOG_DBG("has_name: %d, value: '%s', len: %d", has_name, network_name, has_name ? strlen(network_name) : 0);
        LOG_DBG("has_pan: %d, value: '%s', len: %d", has_pan, pan_id_str, has_pan ? strlen(pan_id_str) : 0);
        LOG_DBG("has_extpanid: %d, value: '%s', len: %d", has_extpanid, extpanid_str, has_extpanid ? strlen(extpanid_str) : 0);
        LOG_DBG("has_channel: %d, value: '%s', len: %d", has_channel, channel_str, has_channel ? strlen(channel_str) : 0);
        LOG_DBG("has_key: %d, value: '%s', len: %d", has_key, network_key_str, has_key ? strlen(network_key_str) : 0);
        
        otOperationalDataset dataset;
        otOperationalDatasetTlvs datasetTlvs;
        
        memset(&dataset, 0, sizeof(dataset));
        
        LOG_DBG("Creating dataset with UI values and random generated values");
        
        /* Override with user-provided values (only the 5 params from web UI) */
        if (has_name && strlen(network_name) > 0) {
            size_t name_len = strlen(network_name);
            if (name_len > OT_NETWORK_NAME_MAX_SIZE) {
                name_len = OT_NETWORK_NAME_MAX_SIZE;
            }
            memcpy(dataset.mNetworkName.m8, network_name, name_len);
            if (name_len < OT_NETWORK_NAME_MAX_SIZE) {
                dataset.mNetworkName.m8[name_len] = '\0';
            }
            dataset.mComponents.mIsNetworkNamePresent = true;
            LOG_DBG("✓ Set network name: '%s' (len=%d)", network_name, name_len);
        } else {
            LOG_WRN("✗ Network name NOT set");
        }
        
        if (has_pan && strlen(pan_id_str) > 0) {
            dataset.mPanId = (uint16_t)strtol(pan_id_str, NULL, 0);
            dataset.mComponents.mIsPanIdPresent = true;
            LOG_DBG("✓ Set PAN ID: 0x%04x (from '%s')", dataset.mPanId, pan_id_str);
        } else {
            LOG_WRN("✗ PAN ID NOT set");
        }
        
        if (has_extpanid && strlen(extpanid_str) > 0) {
            int extpanid_len = hex_to_bytes(extpanid_str, dataset.mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
            if (extpanid_len == OT_EXT_PAN_ID_SIZE) {
                dataset.mComponents.mIsExtendedPanIdPresent = true;
                LOG_DBG("✓ Set Extended PAN ID (from '%s', converted %d bytes)", extpanid_str, extpanid_len);
                LOG_HEXDUMP_DBG(dataset.mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE, "  ExtPanID:");
            } else {
                LOG_WRN("✗ Invalid Extended PAN ID length: %d (expected %d)", extpanid_len, OT_EXT_PAN_ID_SIZE);
            }
        } else {
            LOG_WRN("✗ Extended PAN ID NOT set");
        }
        
        if (has_channel && strlen(channel_str) > 0) {
            dataset.mChannel = (uint16_t)atoi(channel_str);
            dataset.mComponents.mIsChannelPresent = true;
            LOG_DBG("✓ Set channel: %u (from '%s')", dataset.mChannel, channel_str);
        } else {
            LOG_WRN("✗ Channel NOT set");
        }
        
        if (has_key && strlen(network_key_str) > 0) {
            int key_len = hex_to_bytes(network_key_str, dataset.mNetworkKey.m8, OT_NETWORK_KEY_SIZE);
            if (key_len == OT_NETWORK_KEY_SIZE) {
                dataset.mComponents.mIsNetworkKeyPresent = true;
                LOG_DBG("✓ Set network key (from '%s', converted %d bytes)", network_key_str, key_len);
                LOG_HEXDUMP_DBG(dataset.mNetworkKey.m8, OT_NETWORK_KEY_SIZE, "  Network Key:");
            } else {
                LOG_WRN("✗ Invalid network key length: %d (expected %d)", key_len, OT_NETWORK_KEY_SIZE);
            }
        } else {
            LOG_WRN("✗ Network key NOT set");
        }
        
        otError error = OT_ERROR_NONE;
        
        LOG_DBG("===== GENERATING RANDOM VALUES =====");
        
        openthread_mutex_lock();
        otPlatCryptoRandomGet(dataset.mPskc.m8, OT_PSKC_MAX_SIZE);
        openthread_mutex_unlock();
        dataset.mComponents.mIsPskcPresent = true;
        LOG_HEXDUMP_DBG(dataset.mPskc.m8, OT_PSKC_MAX_SIZE, "PSKc:");
        
        dataset.mMeshLocalPrefix.m8[0] = 0xfd;
        openthread_mutex_lock();
        otPlatCryptoRandomGet(&dataset.mMeshLocalPrefix.m8[1], 7);
        openthread_mutex_unlock();
        dataset.mComponents.mIsMeshLocalPrefixPresent = true;
        LOG_HEXDUMP_DBG(dataset.mMeshLocalPrefix.m8, OT_IP6_PREFIX_SIZE, "Mesh Local Prefix:");
        
        dataset.mSecurityPolicy.mRotationTime = 672;
        dataset.mSecurityPolicy.mObtainNetworkKeyEnabled = true;
        dataset.mSecurityPolicy.mNativeCommissioningEnabled = true;
        dataset.mSecurityPolicy.mRoutersEnabled = true;
        dataset.mSecurityPolicy.mExternalCommissioningEnabled = true;
        dataset.mSecurityPolicy.mCommercialCommissioningEnabled = false;
        dataset.mSecurityPolicy.mAutonomousEnrollmentEnabled = false;
        dataset.mSecurityPolicy.mNetworkKeyProvisioningEnabled = false;
        dataset.mSecurityPolicy.mNonCcmRoutersEnabled = false;
        dataset.mComponents.mIsSecurityPolicyPresent = true;
        
        dataset.mActiveTimestamp.mSeconds = 1;
        dataset.mActiveTimestamp.mTicks = 0;
        dataset.mActiveTimestamp.mAuthoritative = false;
        dataset.mComponents.mIsActiveTimestampPresent = true;
        
        dataset.mChannelMask = 0x07FFF800;
        dataset.mComponents.mIsChannelMaskPresent = true;
        
       LOG_DBG("===== COMPLETE DATASET =====");
        LOG_DBG("Network Name: '%s'", dataset.mNetworkName.m8);
        LOG_DBG("Channel: %d", dataset.mChannel);
        LOG_DBG("PAN ID: 0x%04x", dataset.mPanId);
        LOG_HEXDUMP_DBG(dataset.mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE, "Extended PAN ID:");
        LOG_HEXDUMP_DBG(dataset.mNetworkKey.m8, OT_NETWORK_KEY_SIZE, "Network Key:");
        
        LOG_DBG("Dataset created with UI values and generated random values");
        
        openthread_mutex_lock();
        otDatasetConvertToTlvs(&dataset, &datasetTlvs);
        openthread_mutex_unlock();
        LOG_DBG("otDatasetConvertToTlvs Done");
        
        if (error != OT_ERROR_NONE) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"Failed to create new dataset\",\"error_code\":%d}", error);
            goto send_response;
        }
        
        LOG_INF("Created new dataset with random values");
        
        /* Set the active dataset */
        openthread_mutex_lock();
        error = otDatasetSetActive(instance, &dataset);
        openthread_mutex_unlock();
        if (error != OT_ERROR_NONE) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"Failed to set dataset\",\"error_code\":%d}", error);
        } else {
            snprintf(json_response, sizeof(json_response),
                    "{\"status\":\"success\",\"message\":\"Network configuration created successfully\"}");
            LOG_INF("Dataset configured and activated with otDatasetSetActive");
        }
        
        send_response:
        post_data_len = 0;
        memset(post_data_buffer, 0, sizeof(post_data_buffer));
        response_ctx->body = (const uint8_t *)json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
        response_ctx->status = HTTP_200_OK;
    }
    
    return 0;
}

/* REST API: GET /api/thread/start */
int rest_api_thread_start_handler(struct http_client_ctx *client, enum http_transaction_status status,
                                 const struct http_request_ctx *request_ctx,
                                 struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[256];

    LOG_DBG("Thread start API called, status: %d", status);
    if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            // First, enable the IPv6 interface
            openthread_mutex_lock();
            otError error = otIp6SetEnabled(instance, true);
            openthread_mutex_unlock();
            if (error != OT_ERROR_NONE) {
                LOG_ERR("Failed to enable IPv6 interface: %d", error);
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to enable IPv6 interface\",\"error_code\":%d}", error);
                goto send_response;
            }

            LOG_INF("IPv6 interface enabled");

            // Small delay to allow interface to stabilize
            k_sleep(K_MSEC(100));

            // Then enable Thread
            openthread_mutex_lock();
            error = otThreadSetEnabled(instance, true);
            openthread_mutex_unlock();
            if (error != OT_ERROR_NONE) {
                LOG_ERR("Failed to start Thread: %d", error);
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to start Thread\",\"error_code\":%d}", error);
            } else {
                LOG_INF("Thread network started successfully");
                snprintf(json_response, sizeof(json_response),
                        "{\"status\":\"success\",\"message\":\"Thread network started\"}");
            }
        }

send_response:
        response_ctx->body = (const uint8_t *)json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
        response_ctx->status = HTTP_200_OK;
    }

    return 0;
}

/* REST API: GET /api/thread/stop */
int rest_api_thread_stop_handler(struct http_client_ctx *client, enum http_transaction_status status,
                                const struct http_request_ctx *request_ctx,
                                struct http_response_ctx *response_ctx, void *user_data)
{
    static char json_response[256];

    LOG_DBG("Thread stop API called, status: %d", status);

    if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
        otInstance *instance = get_ot_instance();
        if (!instance) {
            snprintf(json_response, sizeof(json_response),
                    "{\"error\":\"OpenThread instance not available\"}");
        } else {
            // First disable Thread
            openthread_mutex_lock();
            otError error = otThreadSetEnabled(instance, false);
            openthread_mutex_unlock();
            if (error != OT_ERROR_NONE) {
                LOG_ERR("Failed to stop Thread: %d", error);
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to stop Thread\",\"error_code\":%d}", error);
                goto send_response;
            }

            LOG_INF("Thread network stopped");

            // Small delay
            k_sleep(K_MSEC(100));

            // Then disable IPv6 interface
            openthread_mutex_lock();
            error = otIp6SetEnabled(instance, false);
            openthread_mutex_unlock();
            if (error != OT_ERROR_NONE) {
                LOG_ERR("Failed to disable IPv6 interface: %d", error);
                snprintf(json_response, sizeof(json_response),
                        "{\"error\":\"Failed to disable IPv6 interface\",\"error_code\":%d}", error);
            } else {
                LOG_INF("IPv6 interface disabled successfully");
                snprintf(json_response, sizeof(json_response),
                        "{\"status\":\"success\",\"message\":\"Thread network stopped\"}");
            }
        }

send_response:
        response_ctx->body = (const uint8_t *)json_response;
        response_ctx->body_len = strlen(json_response);
        response_ctx->final_chunk = true;
        response_ctx->status = HTTP_200_OK;
    }

    return 0;
}

int rest_api_init(void)
{
    LOG_INF("REST API initialized with OpenThread integration");
    return 0;
}
