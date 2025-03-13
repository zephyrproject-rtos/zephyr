/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/counter.h>
#include <stdio.h>

int main(void)
{
	uint64_t cycles = 0;
	uint64_t delta_ns = 0;
	int err;

	while (true) {
		k_sleep(K_MSEC(1000));

		err = sensor_clock_get_cycles(&cycles);
		if (err) {
			printf("Failed to get sensor clock cycles, error: %d\n", err);
			continue;
		}

		printf("Cycles: %llu\n", cycles);

		delta_ns = sensor_clock_cycles_to_ns(cycles);

		printf("Nanoseconds: %llu\n", delta_ns);
	}

	return 0;
}
