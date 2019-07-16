/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_H__
#define LED_H__

enum led_idx {
	LED1 = 0,
	LED2,
	LED3,
	LED4,
	LED_MAX
};

int led_init(void);

int led_set(enum led_idx idx, bool status);
int led_set_time(enum led_idx idx, size_t ms);

#endif /* LED_H__ */
