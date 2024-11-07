/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "board_version.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>

#include "board.h"
#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board_version, CONFIG_BOARD_VERSION_LOG_LEVEL);

#define BOARD_ID DT_NODELABEL(board_id)

static const struct adc_dt_spec adc = ADC_DT_SPEC_GET(BOARD_ID);
static const struct gpio_dt_spec power_gpios = GPIO_DT_SPEC_GET(BOARD_ID, power_gpios);

/* We allow the ADC register value to deviate by N points in either direction */
#define BOARD_VERSION_TOLERANCE	  70
#define VOLTAGE_STABILIZE_TIME_US 5

static int16_t sample_buffer;

static struct adc_sequence sequence = {
	.buffer = &sample_buffer,
	.buffer_size = sizeof(sample_buffer),
};

/* @brief Enable board version voltage divider and trigger ADC read */
static int divider_value_get(void)
{
	int ret;

	ret = gpio_pin_set_dt(&power_gpios, 1);
	if (ret) {
		return ret;
	}

	/* Wait for voltage to stabilize */
	k_busy_wait(VOLTAGE_STABILIZE_TIME_US);

	ret = adc_read(adc.dev, &sequence);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_set_dt(&power_gpios, 0);
	if (ret) {
		return ret;
	}

	return 0;
}

/**@brief Traverse all defined versions and get the one with the
 * most similar value. Check tolerances.
 */
static int version_search(int reg_value, uint32_t tolerance, struct board_version *board_rev)
{
	uint32_t diff;
	uint32_t smallest_diff = UINT_MAX;
	uint8_t smallest_diff_idx = UCHAR_MAX;

	for (uint8_t i = 0; i < (uint8_t)ARRAY_SIZE(BOARD_VERSION_ARR); i++) {
		diff = abs(BOARD_VERSION_ARR[i].adc_reg_val - reg_value);

		if (diff < smallest_diff) {
			smallest_diff = diff;
			smallest_diff_idx = i;
		}
	}

	if (smallest_diff >= tolerance) {
		LOG_ERR("Board ver search failed. ADC reg read: %d", reg_value);
		return -ESPIPE; /* No valid board_rev found */
	}

	*board_rev = BOARD_VERSION_ARR[smallest_diff_idx];
	LOG_DBG("Board ver search OK!. ADC reg val: %d", reg_value);
	return 0;
}

/* @brief Internal init routine */
static int board_version_init(void)
{
	int ret;
	static bool initialized;

	if (initialized) {
		return 0;
	}

	if (!gpio_is_ready_dt(&power_gpios)) {
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&power_gpios, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	if (!device_is_ready(adc.dev)) {
		LOG_ERR("ADC not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&adc);
	if (ret) {
		return ret;
	}

	(void)adc_sequence_init_dt(&adc, &sequence);

	initialized = true;
	return 0;
}

int board_version_get(struct board_version *board_rev)
{
	int ret;

	ret = board_version_init();
	if (ret) {
		return ret;
	}

	ret = divider_value_get();
	if (ret) {
		return ret;
	}

	ret = version_search(sample_buffer, BOARD_VERSION_TOLERANCE, board_rev);
	if (ret) {
		return ret;
	}

	return 0;
}

int board_version_valid_check(void)
{
	int ret;
	struct board_version board_rev;

	ret = board_version_get(&board_rev);
	if (ret) {
		LOG_ERR("Unable to get any board version");
		return ret;
	}

	if (BOARD_VERSION_VALID_MSK & (board_rev.mask)) {
		LOG_INF(COLOR_GREEN "Compatible board/HW version found: %s" COLOR_RESET,
			board_rev.name);
	} else {
		LOG_ERR("Invalid board found, rev: %s Valid mask: 0x%x valid mask: 0x%lx",
			board_rev.name, board_rev.mask, BOARD_VERSION_VALID_MSK);
		return -EPERM;
	}

	return 0;
}
