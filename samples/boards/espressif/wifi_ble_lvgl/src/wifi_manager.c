/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi_manager.h"
#include "nvs_storage.h"
#include "ui_manager.h"
#include "ble_gatt.h"
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <string.h>

LOG_MODULE_REGISTER(wifi_manager, LOG_LEVEL_INF);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct k_sem wifi_connected;
static bool is_connected = false;
static char device_ip[16] = {0};

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status) {
        LOG_ERR("Connection request failed (%d)", status->status);
        is_connected = false;
        ui_send_event(UI_EVENT_WIFI_FAILED, NULL);

        /* Send failure notification via BLE */
        const char *response = "{\"status\":\"failed\",\"message\":\"WiFi connection failed\"}";
        ble_send_notification(response, strlen(response));
    } else {
        LOG_INF("Connected to WiFi");
        is_connected = true;
        ui_send_event(UI_EVENT_WIFI_CONNECTED, NULL);

        /* Send success notification via BLE */
        const char *response = "{\"status\":\"connected\",\"message\":\"WiFi connected successfully\"}";
        ble_send_notification(response, strlen(response));

        LOG_INF("WiFi connected - UI event sent");
    }
    k_sem_give(&wifi_connected);
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status) {
        LOG_ERR("Disconnection request failed (%d)", status->status);
    } else {
        LOG_INF("Disconnected from WiFi");
        is_connected = false;
        memset(device_ip, 0, sizeof(device_ip));
        /* Don't call UI functions directly from network event context */
        LOG_INF("WiFi disconnected - UI will be updated by status thread");
    }
}

static void handle_ipv4_result(struct net_if *iface)
{
    int i = 0;

    for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        char buf[NET_IPV4_ADDR_LEN];
        struct net_if_addr_ipv4 *unicast = &iface->config.ip.ipv4->unicast[i];

        if (!unicast->ipv4.is_used || unicast->ipv4.addr_state != NET_ADDR_PREFERRED) {
            continue;
        }

        net_addr_ntop(AF_INET, &unicast->ipv4.address.in_addr, buf, sizeof(buf));
        strncpy(device_ip, buf, sizeof(device_ip) - 1);
        device_ip[sizeof(device_ip) - 1] = '\0';

        LOG_INF("IPv4 address: %s", device_ip);
        ui_update_ip_address(device_ip);
        LOG_INF("IP address obtained and UI updated");
        break;
    }
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        handle_wifi_connect_result(cb);
        break;
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        handle_wifi_disconnect_result(cb);
        break;
    case NET_EVENT_IPV4_ADDR_ADD:
        handle_ipv4_result(iface);
        break;
    default:
        break;
    }
}

int wifi_manager_init(void)
{
    k_sem_init(&wifi_connected, 0, 1);

    net_mgmt_init_event_callback(&wifi_cb,
                                wifi_mgmt_event_handler,
                                NET_EVENT_WIFI_CONNECT_RESULT |
                                NET_EVENT_WIFI_DISCONNECT_RESULT);

    net_mgmt_init_event_callback(&ipv4_cb,
                                wifi_mgmt_event_handler,
                                NET_EVENT_IPV4_ADDR_ADD);

    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_add_event_callback(&ipv4_cb);

    LOG_INF("WiFi manager initialized");
    return 0;
}

int wifi_connect(const char *ssid, const char *password)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params wifi_params = { 0 };

    if (!ssid || !password) {
        return -EINVAL;
    }

    wifi_params.ssid = ssid;
    wifi_params.ssid_length = strlen(ssid);
    wifi_params.psk = password;
    wifi_params.psk_length = strlen(password);
    wifi_params.channel = WIFI_CHANNEL_ANY;
    wifi_params.security = WIFI_SECURITY_TYPE_PSK;
    wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ;
    wifi_params.mfp = WIFI_MFP_OPTIONAL;

    LOG_INF("Connecting to WiFi SSID: %s", ssid);

    /* Set the SSID in UI manager for display */
    ui_set_wifi_ssid(ssid);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                 &wifi_params, sizeof(struct wifi_connect_req_params))) {
        LOG_ERR("WiFi connection request failed");
        return -EIO;
    }

    LOG_INF("WiFi connection request submitted successfully");
    return 0;
}

int wifi_disconnect(void)
{
    struct net_if *iface = net_if_get_default();

    if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0)) {
        LOG_ERR("WiFi disconnection request failed");
        return -EIO;
    }

    is_connected = false;
    LOG_INF("WiFi disconnected");
    return 0;
}

bool wifi_is_connected(void)
{
    return is_connected;
}

int wifi_get_status(void)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status status = { 0 };

    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
                 sizeof(struct wifi_iface_status))) {
        LOG_ERR("WiFi status request failed");
        return -EIO;
    }

    LOG_INF("WiFi Status:");
    LOG_INF("State: %s", wifi_state_txt(status.state));
    if (status.state >= WIFI_STATE_ASSOCIATED) {
        LOG_INF("Interface Mode: %s", wifi_mode_txt(status.iface_mode));
        LOG_INF("Link Mode: %s", wifi_link_mode_txt(status.link_mode));
        LOG_INF("SSID: %s", status.ssid);
        LOG_INF("BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                status.bssid[0], status.bssid[1], status.bssid[2],
                status.bssid[3], status.bssid[4], status.bssid[5]);
        LOG_INF("Band: %s", wifi_band_txt(status.band));
        LOG_INF("Channel: %d", status.channel);
        LOG_INF("Security: %s", wifi_security_txt(status.security));
        LOG_INF("RSSI: %d", status.rssi);
    }

    return 0;
}

int wifi_connect_stored(void)
{
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWORD_LEN];
    int ret;

    if (!nvs_has_wifi_credentials()) {
        LOG_INF("No stored WiFi credentials found");
        return -ENOENT;
    }

    ret = nvs_read_wifi_credentials(ssid, password);
    if (ret < 0) {
        LOG_ERR("Failed to read stored credentials: %d", ret);
        return ret;
    }

    LOG_INF("Attempting to connect using stored credentials");
    ret = wifi_connect(ssid, password);
    if (ret < 0) {
        LOG_ERR("Failed to connect using stored credentials: %d", ret);
        return ret;
    }

    LOG_INF("Successfully connected using stored credentials");
    return 0;
}

int wifi_get_ip_address(char *ip_str, size_t len)
{
    if (!ip_str || len == 0) {
        return -EINVAL;
    }

    if (!is_connected || strlen(device_ip) == 0) {
        return -ENOTCONN;
    }

    strncpy(ip_str, device_ip, len - 1);
    ip_str[len - 1] = '\0';
    return 0;
}
