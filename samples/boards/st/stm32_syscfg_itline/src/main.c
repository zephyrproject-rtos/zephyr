/**
 * Copyright (c) 2024, Etienne Raquin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#define SLEEP_TIME K_MSEC(1000)

int main(void)
{
	printf("Start\n");

	while (1) {
		printf("Loop it\n");
		k_sleep(SLEEP_TIME);
	}
}
