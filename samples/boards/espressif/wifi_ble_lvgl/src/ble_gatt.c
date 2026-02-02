/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_gatt.h"
#include "wifi_manager.h"
#include "nvs_storage.h"
#include "ui_manager.h"
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_gatt, LOG_LEVEL_INF);

/* External reference to UI initialization status */
extern bool ui_initialized;

/* Work queue for WiFi operations */
static struct k_work_q wifi_work_q;
static K_THREAD_STACK_DEFINE(wifi_work_stack, 3072);

/* Work item for WiFi connection */
static struct k_work wifi_connect_work;
static char pending_ssid[64];
static char pending_password[64];
static bool pending_save_credentials;

/* WiFi Provisioning Service UUID: 12345678-1234-1234-1234-123456789abc */
#define BT_UUID_WIFI_PROV_SERVICE BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abc))

/* WiFi Credentials Characteristic UUID: 12345678-1234-1234-1234-123456789abd */
#define BT_UUID_WIFI_CREDS_CHAR BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abd))

/* WiFi Status Characteristic UUID: 12345678-1234-1234-1234-123456789abe */
#define BT_UUID_WIFI_STATUS_CHAR BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abe))

static struct bt_conn *current_conn;
static bool notify_enabled = false;

/* JSON parsing structures */
struct wifi_credentials {
    const char *ssid;
    const char *password;
    bool save_credentials;
};

static void wifi_connect_work_handler(struct k_work *work)
{
    int ret;
    
    LOG_INF("WiFi work handler: Starting connection process");
    
    /* Validate input parameters */
    if (strlen(pending_ssid) == 0) {
        LOG_ERR("Invalid SSID");
        ui_send_event(UI_EVENT_WIFI_FAILED, NULL);
        return;
    }
    
    if (strlen(pending_password) == 0) {
        LOG_ERR("Invalid password");
        ui_send_event(UI_EVENT_WIFI_FAILED, NULL);
        return;
    }
    
    LOG_INF("WiFi work handler: Attempting connection to SSID: %s", pending_ssid);
    
    /* Send connecting event to UI */
    ui_send_event(UI_EVENT_WIFI_CONNECTING, NULL);
    
    /* Try to connect to WiFi - this is now non-blocking */
    ret = wifi_connect(pending_ssid, pending_password);
    if (ret != 0) {
        LOG_ERR("WiFi connection request failed with error: %d", ret);
        ui_send_event(UI_EVENT_WIFI_FAILED, NULL);
        
        /* Send failure notification */
        const char *response = "{\"status\":\"failed\",\"message\":\"WiFi connection request failed\"}";
        ret = ble_send_notification(response, strlen(response));
        if (ret < 0) {
            LOG_WRN("Failed to send failure notification: %d", ret);
        }
        return;
    }
    
    LOG_INF("WiFi connection request submitted successfully");
    
    /* Save credentials if requested - do this immediately since connection is async */
    if (pending_save_credentials) {
        ret = nvs_save_wifi_credentials(pending_ssid, pending_password);
        if (ret < 0) {
            LOG_ERR("Failed to save credentials to NVS: %d", ret);
        } else {
            LOG_INF("Credentials saved to NVS successfully");
        }
    }
    
    LOG_INF("WiFi work handler: Completed - waiting for connection result");
}

static const struct json_obj_descr wifi_creds_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct wifi_credentials, ssid, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct wifi_credentials, password, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct wifi_credentials, save_credentials, JSON_TOK_TRUE),
};

static ssize_t write_wifi_creds(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               const void *buf, uint16_t len, uint16_t offset,
                               uint8_t flags)
{
    char json_buf[256];
    struct wifi_credentials creds = {0};
    int ret;

    LOG_INF("=== BLE Write Handler Called ===");
    LOG_INF("Length: %d, Offset: %d, Flags: 0x%02x", len, offset, flags);

    if (offset != 0) {
        LOG_ERR("Write with offset not supported (offset=%d)", offset);
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len == 0) {
        LOG_ERR("Empty write request");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (len >= sizeof(json_buf)) {
        LOG_ERR("Data too large: %d >= %d", len, (int)sizeof(json_buf));
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    /* Copy and null-terminate the data */
    memcpy(json_buf, buf, len);
    json_buf[len] = '\0';

    LOG_INF("Received data (%d bytes): '%s'", len, json_buf);

    /* Send BLE receiving event to UI immediately when we get data */
    ui_send_event(UI_EVENT_BLE_RECEIVING, NULL);
    LOG_INF("BLE receiving event sent to UI");

    /* Parse JSON */
    ret = json_obj_parse(json_buf, len, wifi_creds_descr,
                        ARRAY_SIZE(wifi_creds_descr), &creds);
    if (ret < 0) {
        LOG_ERR("JSON parsing failed: %d", ret);
        LOG_ERR("Raw data: %s", json_buf);
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (!creds.ssid || strlen(creds.ssid) == 0) {
        LOG_ERR("Missing or empty SSID");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (!creds.password || strlen(creds.password) == 0) {
        LOG_ERR("Missing or empty password");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    LOG_INF("Parsed credentials successfully:");
    LOG_INF("  SSID: '%s' (len=%d)", creds.ssid, (int)strlen(creds.ssid));
    LOG_INF("  Password: '%s' (len=%d)", creds.password, (int)strlen(creds.password));
    LOG_INF("  Save: %s", creds.save_credentials ? "true" : "false");

    /* Store credentials for work queue processing */
    memset(pending_ssid, 0, sizeof(pending_ssid));
    memset(pending_password, 0, sizeof(pending_password));
    
    strncpy(pending_ssid, creds.ssid, sizeof(pending_ssid) - 1);
    pending_ssid[sizeof(pending_ssid) - 1] = '\0';
    
    strncpy(pending_password, creds.password, sizeof(pending_password) - 1);
    pending_password[sizeof(pending_password) - 1] = '\0';
    
    pending_save_credentials = creds.save_credentials;

    LOG_INF("Stored credentials - SSID: %s, Password length: %d", 
            pending_ssid, (int)strlen(pending_password));

    /* Submit work to dedicated work queue - no UI calls from here */
    LOG_INF("Submitting WiFi connection work to work queue");
    k_work_submit_to_queue(&wifi_work_q, &wifi_connect_work);

    LOG_INF("=== Write Handler Complete ===");
    return len;
}

static ssize_t read_wifi_status(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               void *buf, uint16_t len, uint16_t offset)
{
    char status_buf[128];
    const char *status_msg;

    LOG_INF("GATT Read operation on status characteristic");

    if (wifi_is_connected()) {
        status_msg = "{\"status\":\"connected\",\"message\":\"WiFi is connected\"}";
    } else {
        status_msg = "{\"status\":\"disconnected\",\"message\":\"WiFi is not connected\"}";
    }

    strcpy(status_buf, status_msg);
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, status_buf, strlen(status_buf));
}

static void wifi_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("WiFi status notifications %s", notify_enabled ? "enabled" : "disabled");
}

/* WiFi Provisioning Service Definition */
BT_GATT_SERVICE_DEFINE(wifi_prov_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_WIFI_PROV_SERVICE),
    
    /* WiFi Credentials Characteristic - Support both write types */
    BT_GATT_CHARACTERISTIC(BT_UUID_WIFI_CREDS_CHAR,
                          BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                          BT_GATT_PERM_WRITE,
                          NULL, write_wifi_creds, NULL),
    
    /* WiFi Status Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_WIFI_STATUS_CHAR,
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_READ,
                          read_wifi_status, NULL, NULL),
    BT_GATT_CCC(wifi_status_ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "ESP32_S3_BOX3_BLE", 24),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abc)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to %s (%u)", addr, err);
        return;
    }

    LOG_INF("Connected %s", addr);
    current_conn = bt_conn_ref(conn);
    
    /* Log connection parameters */
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) == 0) {
        LOG_INF("Connection info - Type: %d, Role: %d", info.type, info.role);
        if (info.type == BT_CONN_TYPE_LE) {
            LOG_INF("LE connection - Latency: %d, Timeout: %d", 
                    info.le.latency, info.le.timeout);
        }
    }
    
    /* Try to get MTU information */
    uint16_t mtu = bt_gatt_get_mtu(conn);
    LOG_INF("Current GATT MTU: %d bytes", mtu);
    
    LOG_INF("BLE connected - ready to receive credentials");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected from %s (reason 0x%02x)", addr, reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    notify_enabled = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int ble_gatt_init(void)
{
    int err;

    /* Initialize work queue for WiFi operations */
    k_work_queue_init(&wifi_work_q);
    k_work_queue_start(&wifi_work_q, wifi_work_stack,
                       K_THREAD_STACK_SIZEOF(wifi_work_stack),
                       K_PRIO_COOP(3), NULL);
    
    LOG_INF("WiFi work queue initialized with stack size %d", 
            (int)K_THREAD_STACK_SIZEOF(wifi_work_stack));
    
    /* Initialize work item */
    k_work_init(&wifi_connect_work, wifi_connect_work_handler);

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");
    LOG_INF("GATT service registered with UUIDs:");
    LOG_INF("  Service: 12345678-1234-1234-1234-123456789abc");
    LOG_INF("  Credentials: 12345678-1234-1234-1234-123456789abd");
    LOG_INF("  Status: 12345678-1234-1234-1234-123456789abe");
    return 0;
}

int ble_start_advertising(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising successfully started");
    return 0;
}

int ble_stop_advertising(void)
{
    int err;

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Advertising failed to stop (err %d)", err);
        return err;
    }

    LOG_INF("Advertising stopped");
    return 0;
}

int ble_send_notification(const void *data, uint16_t len)
{
    if (!current_conn || !notify_enabled) {
        LOG_WRN("No connection or notifications not enabled");
        return -ENOTCONN;
    }

    return bt_gatt_notify(current_conn, &wifi_prov_svc.attrs[4], data, len);
}
