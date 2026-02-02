/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/reboot.h>

#include "nvs_storage.h"
#include "wifi_manager.h"
#include "ble_gatt.h"
#include "ui_manager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Button configuration for factory reset */
#define SW0_NODE DT_ALIAS(sw0)
#if DT_NODE_EXISTS(SW0_NODE)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button_cb_data;
#define BUTTON_AVAILABLE 1
#else
#define BUTTON_AVAILABLE 0
#endif

static bool factory_reset_requested = false;

#ifdef BUTTON_AVAILABLE
#if BUTTON_AVAILABLE
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    static int64_t last_press_time = 0;
    static int press_count = 0;
    int64_t current_time = k_uptime_get();

    /* Reset counter if more than 5 seconds since last press */
    if (current_time - last_press_time > 5000) {
        press_count = 0;
    }

    press_count++;
    last_press_time = current_time;

    LOG_INF("Button pressed %d times", press_count);

    /* Factory reset on 5 consecutive presses within 5 seconds */
    if (press_count >= 5) {
        LOG_WRN("Factory reset requested!");
        factory_reset_requested = true;
        press_count = 0;
    }
}

static int init_button(void)
{
    int ret;

    if (!gpio_is_ready_dt(&button)) {
        LOG_ERR("Button device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure button pin: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button interrupt: %d", ret);
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    LOG_INF("Button initialized - press 5 times quickly for factory reset");
    return 0;
}
#endif
#endif

static void factory_reset(void)
{
    LOG_WRN("Performing factory reset...");
    
    /* Clear WiFi credentials */
    int ret = nvs_clear_wifi_credentials();
    if (ret < 0) {
        LOG_ERR("Failed to clear WiFi credentials: %d", ret);
    } else {
        LOG_INF("WiFi credentials cleared");
    }

    /* Disconnect from WiFi if connected */
    if (wifi_is_connected()) {
        wifi_disconnect();
    }

    LOG_INF("Factory reset complete, rebooting...");
    k_sleep(K_SECONDS(2));
    sys_reboot(SYS_REBOOT_COLD);
}

static void status_thread(void)
{
    char ip_str[16];
    bool last_wifi_status = false;
    
    while (1) {
        if (factory_reset_requested) {
            factory_reset();
        }

        /* Check for WiFi status changes and update UI safely */
        bool current_wifi_status = wifi_is_connected();
        if (current_wifi_status != last_wifi_status) {
            LOG_INF("WiFi status changed: %s", current_wifi_status ? "Connected" : "Disconnected");
            /* Don't call ui_set_wifi_status here as it's handled by WiFi manager callbacks */
            last_wifi_status = current_wifi_status;
        }

        /* Update IP address if connected */
        if (current_wifi_status) {
            if (wifi_get_ip_address(ip_str, sizeof(ip_str)) == 0) {
                /* IP address update is handled by WiFi manager callbacks */
            }
        }

        k_sleep(K_SECONDS(5)); /* Check less frequently to reduce conflicts */
    }
}

K_THREAD_DEFINE(status_tid, 1536, status_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
    int ret;

    LOG_INF("WiFi BLE Provisioning with LVGL UI Example");
    LOG_INF("Board: %s", CONFIG_BOARD);

#if BUTTON_AVAILABLE
    /* Initialize button */
    ret = init_button();
    if (ret < 0) {
        LOG_ERR("Failed to initialize button: %d", ret);
    }
#endif

    /* Initialize UI manager first */
    ret = ui_manager_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize UI manager: %d", ret);
        return ret;
    }

    /* Initialize NVS storage */
    ret = nvs_storage_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize NVS storage: %d", ret);
        return ret;
    }

    /* Initialize WiFi manager */
    ret = wifi_manager_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize WiFi manager: %d", ret);
        return ret;
    }

    /* Check for stored WiFi credentials and try to connect */
    if (nvs_has_wifi_credentials()) {
        LOG_INF("Found stored WiFi credentials, attempting to connect...");
        ret = wifi_connect_stored();
        if (ret == 0) {
            LOG_INF("Successfully connected using stored credentials");
            ui_set_wifi_status(true);
        } else {
            LOG_WRN("Failed to connect using stored credentials, starting BLE provisioning");
            ui_set_wifi_status(false);
        }
    } else {
        LOG_INF("No stored WiFi credentials found, starting BLE provisioning");
        ui_set_wifi_status(false);
    }

    /* Initialize and start BLE GATT service */
    ret = ble_gatt_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize BLE GATT: %d", ret);
        return ret;
    }

    ret = ble_start_advertising();
    if (ret < 0) {
        LOG_ERR("Failed to start BLE advertising: %d", ret);
        return ret;
    }

    LOG_INF("System initialized successfully");
    LOG_INF("Device is ready for WiFi provisioning via BLE");
    LOG_INF("Use the LVGL UI to navigate through the provisioning process");
    LOG_INF("Connect with a BLE client and send WiFi credentials in JSON format:");
    LOG_INF("{\"ssid\":\"YourWiFiSSID\",\"password\":\"YourWiFiPassword\",\"save_credentials\":true}");

    /* Main loop */
    while (1) {
        /* Print status every 30 seconds */
        if (wifi_is_connected()) {
            LOG_INF("WiFi Status: Connected");
            wifi_get_status();
        } else {
            LOG_INF("WiFi Status: Disconnected - BLE provisioning active");
        }
        
        k_sleep(K_SECONDS(30));
    }

    return 0;
}
