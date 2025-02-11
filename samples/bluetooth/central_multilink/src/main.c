/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

int init_central(uint8_t iterations);

int main(void)
{
	(void)init_central(CONFIG_SAMPLE_CONN_ITERATIONS);
	return 0;
}
