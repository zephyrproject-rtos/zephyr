/*
 * Copyright (c) 2025 Gielen De Maesschalck <gielen.de.maesschalck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;

#define LED_STRIP_NODE DT_NODELABEL(rgb_ledstrip)
#define AMOUNT_OF_LEDS DT_PROP(LED_STRIP_NODE, chain_length)

ZTEST_USER(led_strip_api, test_led_strip_device_is_ready)
{
    const struct device *dev = DEVICE_DT_GET(LED_STRIP_NODE);
    zassert_true(device_is_ready(dev), "LED strip device is not ready");
}

ZTEST_USER(led_strip_api, test_led_strip_set_color)
{
    const struct device *dev = DEVICE_DT_GET(LED_STRIP_NODE);
    struct led_rgb colors[AMOUNT_OF_LEDS];
    int ret;

    for (size_t i = 0; i < AMOUNT_OF_LEDS; i++) {
        colors[i].r = (i + 1) * 0x20;
        colors[i].g = (i + 1) * 0x10;
        colors[i].b = (i + 1) * 0x08;
    }

    ret = led_strip_update_rgb(dev, colors, AMOUNT_OF_LEDS);
    zassert_equal(ret, 0, "Failed to set LED strip colors: %d", ret);
}

ZTEST_USER(led_strip_api, test_led_strip_update_channels_no_channels_present)
{
    const struct device *dev = DEVICE_DT_GET(LED_STRIP_NODE);
    uint8_t channels[AMOUNT_OF_LEDS * 3];
    int ret;

    for (size_t i = 0; i < AMOUNT_OF_LEDS; i++) {
        channels[i * 3 + 0] = (i + 1) * 0x20; // R
        channels[i * 3 + 1] = (i + 1) * 0x10; // G
        channels[i * 3 + 2] = (i + 1) * 0x08; // B
    }

    ret = led_strip_update_channels(dev, channels, sizeof(channels));
    zassert_equal(ret, -ENOSYS, "Expected -ENOSYS for update_channels, got: %d", ret);
}

ZTEST_USER(led_strip_api, test_led_strip_length)
{
    const struct device *dev = DEVICE_DT_GET(LED_STRIP_NODE);
    size_t length;

    length = led_strip_length(dev);
    zassert_equal(length, AMOUNT_OF_LEDS, "LED strip length mismatch: expected %d, got %d",
                  AMOUNT_OF_LEDS, length);
}

ZTEST_SUITE(led_strip_api, NULL, NULL, NULL, NULL, NULL);



