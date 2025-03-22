/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define SAMPLE_AUDIOPLL_NODE DT_NODELABEL(audiopll)

#define SAMPLE_SET_FREQ_VAL            2000
#define SAMPLE_SET_PRESCALER_VAL       NRF_CLOCK_CONTROL_AUDIOPLL_DIV_4
#define SAMPLE_SET_FREQ_INC_INC_VAL    10
#define SAMPLE_SET_FREQ_INC_PERIOD_VAL 100

static const struct device *audiopll = DEVICE_DT_GET(SAMPLE_AUDIOPLL_NODE);

int main(void)
{
	int ret;

	ret = nrf_clock_control_audiopll_set_freq(audiopll, SAMPLE_SET_FREQ_VAL);
	if (ret) {
		printk("failed to set audiopll frequency fraction\n");
		return 0;
	}

	ret = nrf_clock_control_audiopll_set_prescaler(audiopll, SAMPLE_SET_PRESCALER_VAL);
	if (ret) {
		printk("failed to set audiopll prescaler divider\n");
		return 0;
	}

	ret = nrf_clock_control_audiopll_set_freq_inc(audiopll,
						      SAMPLE_SET_FREQ_INC_INC_VAL,
						      SAMPLE_SET_FREQ_INC_PERIOD_VAL);
	if (ret) {
		printk("failed to set audiopll freq inc\n");
		return 0;
	}

	ret = clock_control_on(audiopll, NULL);
	if (ret) {
		printk("failed to turn on audiopll\n");
		return 0;
	}

	k_msleep(1000);

	ret = clock_control_off(audiopll, NULL);
	if (ret) {
		printk("failed to turn off audiopll\n");
		return 0;
	}

	printk("sample complete\n");
	return 0;
}
