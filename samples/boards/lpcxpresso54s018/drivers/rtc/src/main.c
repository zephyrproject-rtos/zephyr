/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LPC54S018 RTC Sample
 * 
 * This sample demonstrates the use of the Real-Time Clock (RTC) peripheral
 * on the LPC54S018. It shows how to:
 * - Set and read the current time
 * - Set alarms
 * - Use the RTC for low-power timekeeping
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <time.h>
#include <stdio.h>

LOG_MODULE_REGISTER(rtc_sample, LOG_LEVEL_INF);

/* RTC device */
#define RTC_NODE DT_NODELABEL(rtc)

/* LED configuration */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Alarm configuration */
#define ALARM_CHANNEL_ID 0
#define ALARM_INTERVAL_SEC 10  /* Alarm every 10 seconds */

/* RTC alarm callback */
static void rtc_alarm_callback(const struct device *counter_dev,
                              uint8_t chan_id, uint32_t ticks,
                              void *user_data)
{
    struct tm time_info;
    time_t current_time = (time_t)ticks;
    
    /* Convert to broken-down time */
    gmtime_r(&current_time, &time_info);
    
    LOG_INF("RTC Alarm! Current time: %02d:%02d:%02d",
            time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    
    /* Toggle LED on alarm */
    gpio_pin_toggle_dt(&led);
    
    /* Set next alarm */
    struct counter_alarm_cfg alarm_cfg = {
        .flags = 0,
        .ticks = ticks + ALARM_INTERVAL_SEC,
        .callback = rtc_alarm_callback,
        .user_data = NULL
    };
    
    int ret = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &alarm_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to set next alarm");
    }
}

/* Format time for display */
static void format_time(uint32_t ticks, char *buffer, size_t size)
{
    struct tm time_info;
    time_t current_time = (time_t)ticks;
    
    gmtime_r(&current_time, &time_info);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             time_info.tm_year + 1900,
             time_info.tm_mon + 1,
             time_info.tm_mday,
             time_info.tm_hour,
             time_info.tm_min,
             time_info.tm_sec);
}

int main(void)
{
    const struct device *rtc_dev;
    struct counter_alarm_cfg alarm_cfg;
    uint32_t current_ticks;
    char time_str[32];
    int ret;
    
    LOG_INF("LPC54S018 RTC Sample");
    
    /* Initialize LED */
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED device not ready");
        return -1;
    }
    
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED pin");
        return -1;
    }
    
    /* Get RTC device */
    rtc_dev = DEVICE_DT_GET(RTC_NODE);
    if (!device_is_ready(rtc_dev)) {
        LOG_ERR("RTC device not ready");
        return -1;
    }
    
    /* Get RTC properties */
    uint32_t freq = counter_get_frequency(rtc_dev);
    uint32_t max_ticks = counter_get_max_relative_alarm(rtc_dev);
    
    LOG_INF("RTC frequency: %u Hz", freq);
    LOG_INF("Max relative alarm: %u ticks (%u seconds)", max_ticks, max_ticks / freq);
    
    /* Start RTC (if not already running) */
    ret = counter_start(rtc_dev);
    if (ret < 0 && ret != -EALREADY) {
        LOG_ERR("Failed to start RTC");
        return -1;
    }
    
    /* Get current RTC value */
    ret = counter_get_value(rtc_dev, &current_ticks);
    if (ret < 0) {
        LOG_ERR("Failed to read RTC");
        return -1;
    }
    
    /* If RTC is at zero, set initial time to 2024-01-01 00:00:00 */
    if (current_ticks == 0) {
        struct tm init_time = {
            .tm_year = 2024 - 1900,  /* Years since 1900 */
            .tm_mon = 0,              /* January (0-11) */
            .tm_mday = 1,             /* Day of month */
            .tm_hour = 0,
            .tm_min = 0,
            .tm_sec = 0
        };
        
        time_t init_ticks = mktime(&init_time);
        
        /* Set RTC to initial time */
        ret = counter_set_value(rtc_dev, (uint32_t)init_ticks);
        if (ret == 0) {
            LOG_INF("RTC initialized to 2024-01-01 00:00:00");
            current_ticks = (uint32_t)init_ticks;
        } else if (ret == -ENOTSUP) {
            LOG_WRN("RTC set not supported, using current value");
        } else {
            LOG_ERR("Failed to set RTC time");
        }
    }
    
    /* Display current time */
    format_time(current_ticks, time_str, sizeof(time_str));
    LOG_INF("Current RTC time: %s", time_str);
    
    /* Set up alarm */
    alarm_cfg.flags = 0;
    alarm_cfg.ticks = current_ticks + ALARM_INTERVAL_SEC;
    alarm_cfg.callback = rtc_alarm_callback;
    alarm_cfg.user_data = NULL;
    
    ret = counter_set_channel_alarm(rtc_dev, ALARM_CHANNEL_ID, &alarm_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to set alarm");
        return -1;
    }
    
    format_time(alarm_cfg.ticks, time_str, sizeof(time_str));
    LOG_INF("Alarm set for: %s", time_str);
    
    /* Main loop - display time every 5 seconds */
    while (1) {
        k_sleep(K_SECONDS(5));
        
        /* Read current RTC value */
        ret = counter_get_value(rtc_dev, &current_ticks);
        if (ret == 0) {
            format_time(current_ticks, time_str, sizeof(time_str));
            LOG_INF("Current time: %s", time_str);
        }
        
        /* Check if counter is still running */
        if (!counter_is_counting_up(rtc_dev)) {
            LOG_WRN("RTC has stopped!");
        }
    }
    
    return 0;
}