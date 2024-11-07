/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led.h"
#include "button_handler.h"
#include "button_assignments.h"
#include "nrfx_clock.h"
#include "board_version.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf5340_audio_dk, CONFIG_MODULE_NRF5340_AUDIO_DK_LOG_LEVEL);

static struct board_version board_rev;

static int hfclock_config_and_start(void)
{
	int ret;

	/* Use this to turn on 128 MHz clock for cpu_app */
	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);

	ret -= NRFX_ERROR_BASE_NUM;
	if (ret) {
		return ret;
	}

	nrfx_clock_hfclk_start();
	while (!nrfx_clock_hfclk_is_running()) {
	}

	return 0;
}

static int leds_set(void)
{
	int ret;

	/* Blink LED 3 to indicate that APP core is running */
	ret = led_blink(LED_APP_3_GREEN);
	if (ret) {
		return ret;
	}

	/* Default color is yellow */

	ret = led_on(LED_APP_RGB, LED_COLOR_YELLOW);
	if (ret) {
		return ret;
	}

	return 0;
}

int nrf5340_audio_dk_init(void)
{
	int ret;

	/* Might as well */
	ret = hfclock_config_and_start();
	if (ret) {
		return ret;
	}

	/* Might as well */
	ret = led_init();
	if (ret) {
		LOG_ERR("Failed to initialize LED module");
		return ret;
	}

	/* Might as well */
	ret = button_handler_init();
	if (ret) {
		LOG_ERR("Failed to initialize button handler");
		return ret;
	}

	/* Removed channel assignment as not used in this sample */

	ret = board_version_valid_check();
	if (ret) {
		return ret;
	}

	ret = board_version_get(&board_rev);
	if (ret) {
		return ret;
	}

	/* No need for sd card in this sample */

	/* Might as well */
	ret = leds_set();
	if (ret) {
		LOG_ERR("Failed to set LEDs");
		return ret;
	}

	/* Custom init */

	return 0;
}
