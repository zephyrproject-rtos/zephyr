/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui_manager.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <lvgl_zephyr.h>
#include <string.h>

LOG_MODULE_REGISTER(ui_manager, LOG_LEVEL_INF);

/* Color definitions - Minimalistic theme */
#define COLOR_WHITE          lv_color_hex(0xFFFFFF)   /* White background */
#define COLOR_BLACK          lv_color_hex(0x000000)   /* Black text */
#define COLOR_GRAY           lv_color_hex(0x666666)   /* Gray text */
#define COLOR_BUTTON         lv_color_hex(0x007BFF)   /* Modern blue button */
#define COLOR_BUTTON_TEXT    lv_color_hex(0xFFFFFF)   /* White button text */
#define COLOR_SUCCESS        lv_color_hex(0x28A745)   /* Green for success */
#define COLOR_ERROR          lv_color_hex(0xDC3545)   /* Red for error */
#define COLOR_WARNING        lv_color_hex(0xFFC107)   /* Orange for warning */

/* UI Objects */
static lv_obj_t *screen_main;
static lv_obj_t *screen_ip_display;

/* Main Screen Objects */
static lv_obj_t *btn_device_ip;
static lv_obj_t *label_device_ip;
static lv_obj_t *label_status;

/* IP Display Screen Objects */
static lv_obj_t *label_ip_title;
static lv_obj_t *label_ip_address;
static lv_obj_t *btn_back;

/* Current screen state */
static ui_screen_t current_screen = UI_SCREEN_MAIN;
static bool wifi_connected = false;
static char device_ip_str[16] = "Not Connected";
static char wifi_ssid[33] = "";  /* WiFi SSID storage (max 32 chars + null terminator) */

/* Event queue */
#define UI_EVENT_QUEUE_SIZE 10
static struct k_msgq ui_event_queue;
static char ui_event_buffer[UI_EVENT_QUEUE_SIZE * sizeof(ui_event_t)];
bool ui_initialized = false;

/* UI thread */
static struct k_thread ui_thread;
static K_THREAD_STACK_DEFINE(ui_thread_stack, 2048);

/* Forward declarations */
static void create_main_screen(void);
static void create_ip_display_screen(void);
static void show_screen(ui_screen_t screen);
static void ui_thread_entry(void *p1, void *p2, void *p3);
static void enable_backlight(void);

/* Backlight GPIO */
#define BACKLIGHT_NODE DT_PATH(leds, lcd_backlight)
#define GT911_NODE DT_NODELABEL(gt911)
static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(BACKLIGHT_NODE, gpios);

/* Touch input variables with debouncing */
static int16_t touch_x = -1;
static int16_t touch_y = -1;
static bool touch_pressed = false;
static int64_t last_touch_time = 0;
static bool touch_handled = false;

#define TOUCH_DEBOUNCE_MS 300  /* 300ms debounce time */

static void enable_backlight(void)
{
    LOG_INF("Configuring backlight...");

    if (!gpio_is_ready_dt(&backlight)) {
        LOG_ERR("Backlight GPIO not ready");
        return;
    }

    int ret = gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure backlight GPIO: %d", ret);
        return;
    }

    gpio_pin_set_dt(&backlight, 1);
    LOG_INF("Backlight enabled successfully");
}

static void input_callback(struct input_event *evt, void *user_data)
{
    switch (evt->code) {
    case INPUT_ABS_X:
        touch_x = evt->value;
        break;

    case INPUT_ABS_Y:
        touch_y = evt->value;
        break;

    case INPUT_BTN_TOUCH:
        if (evt->value == 1) {
            touch_pressed = true;
            touch_handled = false;  /* Reset handled flag on new touch */
        } else {
            touch_pressed = false;
            touch_x = -1;
            touch_y = -1;
        }
        break;

    default:
        break;
    }

    /* Handle touch events with debouncing */
    if (evt->sync && touch_pressed && touch_x >= 0 && touch_y >= 0 && !touch_handled) {
        int64_t current_time = k_uptime_get();

        /* Check debounce time */
        if (current_time - last_touch_time < TOUCH_DEBOUNCE_MS) {
            return;  /* Ignore touch within debounce period */
        }

        LOG_INF("Touch detected at (%d, %d)", touch_x, touch_y);
        last_touch_time = current_time;
        touch_handled = true;  /* Mark this touch as handled */

        /* Check which screen is active and handle touch accordingly */
        if (current_screen == UI_SCREEN_MAIN && btn_device_ip && wifi_connected) {
            lv_area_t btn_area;
            lv_obj_get_coords(btn_device_ip, &btn_area);

            LOG_INF("Device IP button area: (%d,%d) to (%d,%d)",
                    btn_area.x1, btn_area.y1, btn_area.x2, btn_area.y2);

            if (touch_x >= btn_area.x1 && touch_x <= btn_area.x2 &&
                touch_y >= btn_area.y1 && touch_y <= btn_area.y2) {
                LOG_INF("Device IP button touched!");
                ui_send_event(UI_EVENT_DEVICE_IP_PRESSED, NULL);
            }
        } else if (current_screen == UI_SCREEN_IP_DISPLAY && btn_back) {
            lv_area_t btn_area;
            lv_obj_get_coords(btn_back, &btn_area);

            LOG_INF("Back button area: (%d,%d) to (%d,%d)",
                    btn_area.x1, btn_area.y1, btn_area.x2, btn_area.y2);

            if (touch_x >= btn_area.x1 && touch_x <= btn_area.x2 &&
                touch_y >= btn_area.y1 && touch_y <= btn_area.y2) {
                LOG_INF("Back button touched!");
                show_screen(UI_SCREEN_MAIN);
            }
        } else {
            /* No action needed for other screens or conditions */
        }
    }
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(GT911_NODE), input_callback, NULL);

/* Button event handlers */
static void device_ip_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        LOG_INF("Device IP button clicked");
        ui_send_event(UI_EVENT_DEVICE_IP_PRESSED, NULL);
    }
}

static void back_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        LOG_INF("Back button clicked");
        show_screen(UI_SCREEN_MAIN);
    }
}

static void create_main_screen(void)
{
    /* Create main screen with white background */
    screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_main, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(screen_main, LV_OPA_COVER, 0);

    /* Status label - centered */
    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "WiFi BLE Provisioning Ready\n\nUse provision_wifi.py to send\nWiFi credentials via Bluetooth\n\n(Check README.rst for details)");
    lv_obj_set_style_text_color(label_status, COLOR_BLACK, 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_status, 280);
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 0);

    /* Device IP button (initially hidden) - centered */
    btn_device_ip = lv_btn_create(screen_main);
    lv_obj_set_size(btn_device_ip, 160, 50);
    lv_obj_align(btn_device_ip, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(btn_device_ip, COLOR_BUTTON, 0);
    lv_obj_set_style_border_width(btn_device_ip, 0, 0);
    lv_obj_set_style_radius(btn_device_ip, 12, 0);
    lv_obj_set_style_shadow_width(btn_device_ip, 4, 0);
    lv_obj_set_style_shadow_color(btn_device_ip, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(btn_device_ip, LV_OPA_20, 0);
    lv_obj_add_event_cb(btn_device_ip, device_ip_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(btn_device_ip, LV_OBJ_FLAG_HIDDEN);

    /* Device IP button label - centered */
    label_device_ip = lv_label_create(btn_device_ip);
    lv_label_set_text(label_device_ip, "DEVICE IP");
    lv_obj_set_style_text_color(label_device_ip, COLOR_BUTTON_TEXT, 0);
    lv_obj_set_style_text_font(label_device_ip, &lv_font_montserrat_16, 0);
    lv_obj_center(label_device_ip);

    LOG_INF("Main screen created");
}

static void create_ip_display_screen(void)
{
    /* Create IP display screen with white background */
    screen_ip_display = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_ip_display, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(screen_ip_display, LV_OPA_COVER, 0);

    /* IP title label - centered */
    label_ip_title = lv_label_create(screen_ip_display);
    lv_label_set_text(label_ip_title, "Device IP Address");
    lv_obj_set_style_text_color(label_ip_title, COLOR_BLACK, 0);
    lv_obj_set_style_text_font(label_ip_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(label_ip_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_ip_title, LV_ALIGN_CENTER, 0, -50);

    /* IP address label - centered */
    label_ip_address = lv_label_create(screen_ip_display);
    lv_label_set_text(label_ip_address, device_ip_str);
    lv_obj_set_style_text_color(label_ip_address, COLOR_BLACK, 0);
    lv_obj_set_style_text_font(label_ip_address, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(label_ip_address, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_ip_address, LV_ALIGN_CENTER, 0, -10);

    /* Back button - centered */
    btn_back = lv_btn_create(screen_ip_display);
    lv_obj_set_size(btn_back, 120, 45);
    lv_obj_align(btn_back, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn_back, COLOR_BUTTON, 0);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_set_style_radius(btn_back, 12, 0);
    lv_obj_set_style_shadow_width(btn_back, 4, 0);
    lv_obj_set_style_shadow_color(btn_back, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(btn_back, LV_OPA_20, 0);
    lv_obj_add_event_cb(btn_back, back_btn_event_handler, LV_EVENT_CLICKED, NULL);

    /* Back button label - centered */
    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "BACK");
    lv_obj_set_style_text_color(label_back, COLOR_BUTTON_TEXT, 0);
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_14, 0);
    lv_obj_center(label_back);

    LOG_INF("IP display screen created");
}

static void show_screen(ui_screen_t screen)
{
    switch (screen) {
    case UI_SCREEN_MAIN:
        lv_scr_load(screen_main);
        current_screen = UI_SCREEN_MAIN;
        LOG_INF("Showing main screen");
        break;

    case UI_SCREEN_IP_DISPLAY:
        /* Update IP address label before showing the screen */
        if (label_ip_address) {
            lv_label_set_text(label_ip_address, device_ip_str);
        }
        lv_scr_load(screen_ip_display);
        current_screen = UI_SCREEN_IP_DISPLAY;
        LOG_INF("Showing IP display screen with IP: %s", device_ip_str);
        break;

    default:
        LOG_WRN("Unknown screen requested: %d", screen);
        break;
    }
}

static void handle_ui_event(ui_event_t event)
{
    switch (event) {
    case UI_EVENT_DEVICE_IP_PRESSED:
        LOG_INF("Handling DEVICE_IP_PRESSED event");
        show_screen(UI_SCREEN_IP_DISPLAY);
        break;


    case UI_EVENT_RESET_STATUS:
        LOG_INF("Handling RESET_STATUS event");
        if (current_screen == UI_SCREEN_MAIN && !wifi_connected) {
            lv_label_set_text(label_status, "WiFi BLE Provisioning Ready");
            lv_obj_set_style_text_color(label_status, COLOR_BLACK, 0);
        }
        break;

    case UI_EVENT_WIFI_CONNECTING:
        LOG_INF("Handling WIFI_CONNECTING event");
        if (current_screen == UI_SCREEN_MAIN) {
            lv_label_set_text(label_status, "Connecting to WiFi...");
            lv_obj_set_style_text_color(label_status, COLOR_BLACK, 0);
            /* Center the connecting message */
            lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 0);
        }
        break;

    case UI_EVENT_WIFI_CONNECTED:
        LOG_INF("Handling WIFI_CONNECTED event");
        wifi_connected = true;
        if (current_screen == UI_SCREEN_MAIN) {
            char success_msg[80];
            if (strlen(wifi_ssid) > 0) {
                snprintf(success_msg, sizeof(success_msg), "WiFi Connected Successfully\nto %s", wifi_ssid);
            } else {
                snprintf(success_msg, sizeof(success_msg), "WiFi Connected Successfully!");
            }
            lv_label_set_text(label_status, success_msg);
            lv_obj_set_style_text_color(label_status, COLOR_BLACK, 0);
            /* Move status to higher position for connected state only */
            lv_obj_align(label_status, LV_ALIGN_CENTER, 0, -40);
            lv_obj_clear_flag(btn_device_ip, LV_OBJ_FLAG_HIDDEN);
            LOG_INF("Device IP button is now visible");
        }
        break;

    case UI_EVENT_WIFI_FAILED:
        LOG_INF("Handling WIFI_FAILED event");
        if (current_screen == UI_SCREEN_MAIN) {
            lv_label_set_text(label_status, "WiFi Connection Failed!");
            lv_obj_set_style_text_color(label_status, COLOR_BLACK, 0);
            /* Center the failed message */
            lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 0);
        }
        break;

    default:
        LOG_WRN("Unknown UI event: %d", event);
        break;
    }
}

static void ui_thread_entry(void *p1, void *p2, void *p3)
{
    ui_event_t event;

    LOG_INF("UI thread started");

    while (1) {
        /* Process UI events */
        if (k_msgq_get(&ui_event_queue, &event, K_NO_WAIT) == 0) {
            handle_ui_event(event);
        }

        /* Handle LVGL tasks */
        lv_timer_handler();

        /* Small delay to prevent busy waiting */
        k_sleep(K_MSEC(10));
    }
}

int ui_manager_init(void)
{
    const struct device *display_dev;
    const struct device *gt911_dev;

    LOG_INF("Initializing UI manager");

    /* Check if GT911 touch is ready */
    gt911_dev = DEVICE_DT_GET(GT911_NODE);
    if (!device_is_ready(gt911_dev)) {
        LOG_ERR("GT911 touch controller not ready!");
        return -ENODEV;
    }
    LOG_INF("GT911 touch controller ready");

    /* Get display device */
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -ENODEV;
    }
    LOG_INF("Display device is ready");

    /* Enable backlight */
    enable_backlight();

    /* Turn on display */
    LOG_INF("Turning on display...");
    display_blanking_off(display_dev);

    /* Wait for display to stabilize */
    k_sleep(K_MSEC(200));

    /* Initialize LVGL */
    LOG_INF("Initializing LVGL...");
    lv_init();
    LOG_INF("LVGL core initialized");

    /* Initialize LVGL display driver */
    int ret = lvgl_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize LVGL display driver: %d", ret);
        return ret;
    }
    LOG_INF("LVGL display driver initialized successfully");

    /* Initialize event queue */
    k_msgq_init(&ui_event_queue, ui_event_buffer, sizeof(ui_event_t), UI_EVENT_QUEUE_SIZE);
    ui_initialized = true;

    /* Create all screens */
    create_main_screen();
    create_ip_display_screen();

    /* Show initial screen */
    show_screen(UI_SCREEN_MAIN);

    /* Start UI thread */
    k_thread_create(&ui_thread, ui_thread_stack,
                    K_THREAD_STACK_SIZEOF(ui_thread_stack),
                    ui_thread_entry, NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&ui_thread, "ui_thread");

    LOG_INF("UI manager initialized successfully");
    return 0;
}

void ui_send_event(ui_event_t event, void *data)
{
    if (!ui_initialized) {
        LOG_WRN("UI not initialized, ignoring event %d", event);
        return;
    }

    int ret = k_msgq_put(&ui_event_queue, &event, K_NO_WAIT);
    if (ret != 0) {
        LOG_WRN("Failed to send UI event %d: %d", event, ret);
    }
}

void ui_update_ip_address(const char *ip_address)
{
    if (ip_address) {
        strncpy(device_ip_str, ip_address, sizeof(device_ip_str) - 1);
        device_ip_str[sizeof(device_ip_str) - 1] = '\0';
    } else {
        strcpy(device_ip_str, "Not Connected");
    }

    /* Always update IP display label if it exists */
    if (label_ip_address) {
        lv_label_set_text(label_ip_address, device_ip_str);
    }

    LOG_INF("IP address updated: %s", device_ip_str);
}

void ui_set_wifi_status(bool connected)
{
    wifi_connected = connected;

    if (connected) {
        LOG_INF("WiFi connected - sending UI event");
        ui_send_event(UI_EVENT_WIFI_CONNECTED, NULL);
    } else {
        /* Hide Device IP button when disconnected */
        if (current_screen == UI_SCREEN_MAIN && btn_device_ip) {
            lv_obj_add_flag(btn_device_ip, LV_OBJ_FLAG_HIDDEN);
        }
        ui_update_ip_address(NULL);
        /* Clear SSID when disconnected */
        wifi_ssid[0] = '\0';
    }
}

void ui_set_wifi_ssid(const char *ssid)
{
    if (ssid) {
        strncpy(wifi_ssid, ssid, sizeof(wifi_ssid) - 1);
        wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
        LOG_INF("WiFi SSID updated: %s", wifi_ssid);
    } else {
        wifi_ssid[0] = '\0';
        LOG_INF("WiFi SSID cleared");
    }
}
