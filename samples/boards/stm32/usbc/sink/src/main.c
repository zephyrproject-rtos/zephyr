/*
 * Copyright (c) 2022 The Chromium OS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "usbc_snk.h"
#include "board.h"

/* 1 s = 200 * 5 msec */
#define LED_TOGGLE_TIME 200

void main(void)
{
	int ret;
	int loop_interval;
	int timer;
	int led_is_on;

	/* Config the board */
	ret = board_config();
	if (ret) {
		LOG_ERR("Could not configure board");
		return;
	}

	/* Initialize the Sink State Machine */
	ret = sink_init();
	if (ret) {
		LOG_ERR("Could not initialize sink state machine\n");
		return;
	}

	led_is_on = 0;
	timer = 0;

	/* Run Sink State Machine on 5 msec loop interval */
	while (1) {
		/* Run Sink State Machine */
		loop_interval = sink_sm();

		/* Update LED toggle timer */
		timer++;

		/* Toggle Board LED */
		if (timer == LED_TOGGLE_TIME) {
			timer = 0;
			board_led(led_is_on);
			led_is_on = !led_is_on;
		}

		k_msleep(loop_interval);
	}
}
