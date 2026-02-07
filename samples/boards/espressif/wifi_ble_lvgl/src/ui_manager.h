/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <zephyr/kernel.h>
#include <lvgl.h>

/* UI Screen States */
typedef enum {
    UI_SCREEN_MAIN,         /* Main screen with Device IP button */
    UI_SCREEN_IP_DISPLAY,   /* IP display screen */
} ui_screen_t;

/* UI Events */
typedef enum {
    UI_EVENT_DEVICE_IP_PRESSED,
    UI_EVENT_WIFI_CONNECTING,
    UI_EVENT_WIFI_CONNECTED,
    UI_EVENT_WIFI_FAILED,
    UI_EVENT_BLE_RECEIVING,
    UI_EVENT_RESET_STATUS,
} ui_event_t;

/**
 * @brief Initialize UI manager and LVGL
 * @return 0 on success, negative error code on failure
 */
int ui_manager_init(void);

/**
 * @brief Send event to UI manager
 * @param event UI event to process
 * @param data Optional event data
 */
void ui_send_event(ui_event_t event, void *data);

/**
 * @brief Update device IP address display
 * @param ip_address IP address string
 */
void ui_update_ip_address(const char *ip_address);

/**
 * @brief Set WiFi connection status
 * @param connected true if connected, false otherwise
 */
void ui_set_wifi_status(bool connected);

/**
 * @brief Set WiFi SSID for display
 * @param ssid WiFi network name
 */
void ui_set_wifi_ssid(const char *ssid);

/* External UI initialization status */
extern bool ui_initialized;

#endif /* UI_MANAGER_H */
