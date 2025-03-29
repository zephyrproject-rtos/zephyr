/*
 * Copyright (c) 2025, Yongci zhuyongci@hotmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(clock, LOG_LEVEL_INF);

const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static int set_date_time(const struct device *rtc)
{
    int ret = 0;
    struct rtc_time tm = {
        .tm_year = 2025 - 1900,
        .tm_mon = 3 - 1,
        .tm_mday = 29,
        .tm_hour = 16,
        .tm_min = 30,
        .tm_sec = 0,
    };
 
    ret = rtc_set_time(rtc, &tm);
    if (ret < 0) {
        printk("Cannot write date time: %d\n", ret);
        return ret;
    }
    return ret;
}
 
static int get_date_time(const struct device *rtc, char *time_str, size_t len)
{
    int ret = 0;
    struct rtc_time tm;
 
    ret = rtc_get_time(rtc, &tm);
    if (ret < 0) {
        printk("Cannot read date time: %d\n", ret);
        return ret;
    }
 
    snprintf(time_str, len, "%02d:%02d:%02d", 
             tm.tm_hour, tm.tm_min, tm.tm_sec);
 
    return ret;
}

static void display_time(const struct device *display, const char *time_str)
{
    if (!device_is_ready(display)) {
        LOG_ERR("Display not ready");
        return;
    }

    cfb_framebuffer_set_font(display, 1);
    cfb_framebuffer_clear(display, false);
    cfb_print(display, time_str, 5, 20);
    cfb_framebuffer_finalize(display);
}


int setup(void)
{
    if (!device_is_ready(rtc)) {
        printk("RTC device is not ready\n");
        return -1;
    }

    if (!device_is_ready(display)) {
        printk("Display device is not ready\n");
        return -1;
    }

    if (cfb_framebuffer_init(display)) {
        LOG_ERR("CFB framebuffer initialization failed");
        return -1;
    }

    cfb_framebuffer_clear(display, true);
    cfb_framebuffer_finalize(display);
    return 0;
}

int main(void)
{
    if (setup() < 0) {
        return -1;
    }

    set_date_time(rtc);
    char time_str[32];
    char prev_time_str[32] = "";
 
    while (1) {
        if (get_date_time(rtc, time_str, sizeof(time_str)) == 0) {
            if (strcmp(time_str, prev_time_str) != 0) {  // Only update if time changed
                display_time(display, time_str);
                strcpy(prev_time_str, time_str);
            }
        }
        k_sleep(K_MSEC(200));  // Faster refresh to avoid visible flicker
    }
    return 0;
}
