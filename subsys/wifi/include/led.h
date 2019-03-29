/*
 * @file
 * @brief LED lighting header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_LED_H_
#define _WIFIMGR_LED_H_

#include <led.h>

#ifdef CONFIG_WIFIMGR_LED
int wifimgr_led_on(const char *name, int pin);
int wifimgr_led_off(const char *name, int pin);
#ifdef CONFIG_WIFIMGR_STA
#define wifimgr_sta_led_on(...) \
	wifimgr_led_on(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_STA)

#define wifimgr_sta_led_off(...) \
	wifimgr_led_off(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_STA)
#endif
#ifdef CONFIG_WIFIMGR_AP
#define wifimgr_ap_led_on(...) \
	wifimgr_led_on(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_AP)

#define wifimgr_ap_led_off(...) \
	wifimgr_led_off(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_AP)
#endif
#else
#define wifimgr_sta_led_on(...)
#define wifimgr_sta_led_off(...)
#define wifimgr_ap_led_on(...)
#define wifimgr_ap_led_off(...)
#endif

#endif
