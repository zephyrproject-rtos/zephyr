/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample application for timeaware GPIO.
 * This example demonstrates the following
 * a. How to generate pulses based on ART time on an output pin
 * b. How to timestamp a pulse on an input pin
 */

/* Local Includes */
#include <zephyr/sys/util.h>
#include <zephyr/drivers/misc/timeaware_gpio/timeaware_gpio.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TGPIO_LABEL	DT_NODELABEL(tgpio)
#define TGPIO_PIN_IN	0
#define TGPIO_PIN_OUT	1

int main(void)
{
	const struct device *tgpio_dev;
	uint64_t tm, ts, ec, ret;
	uint32_t cycles;

	/* Get the device handle for Timeaware-GPIO instance  */
	tgpio_dev = DEVICE_DT_GET(TGPIO_LABEL);
	if (!device_is_ready(tgpio_dev)) {
		printk("[TGPIO] Bind failed\n");
		return -EINVAL;
	}

	printk("[TGPIO] Bind Success\n");

	tgpio_port_get_time(tgpio_dev, &tm);
	printk("[TGPIO] Time now: %016llx\n", tm);

	tgpio_port_get_cycles_per_second(tgpio_dev, &cycles);
	printk("[TGPIO] Running rate: %d\n", cycles);

	/* Configure start time of first pulse, time has to be in future */
	tm += cycles;
	printk("[TGPIO] Periodic pulses start at: %016llx\n", tm);

	ret = tgpio_pin_periodic_output(tgpio_dev, TGPIO_PIN_OUT,
					 tm, cycles, true);
	if (ret) {
		printk("[TGPIO] periodic output configuration failed\n");
		return -EINVAL;
	}

	/* Configure external timestamp for input pulses */
	ret = tgpio_pin_config_ext_timestamp(tgpio_dev, TGPIO_PIN_IN, 0);
	if (ret) {
		printk("[TGPIO] external timestamp configuration failed\n");
		return -EINVAL;
	}

	while (1) {
		/* Read timestamp and event counter values */
		tgpio_pin_read_ts_ec(tgpio_dev, TGPIO_PIN_IN, &ts, &ec);
		printk("[TGPIO] timestamp: %016llx, event count: %016llx\n", ts, ec);
		k_sleep(K_MSEC(500));
	}
	return 0;
}
