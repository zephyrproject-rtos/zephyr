/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

int init_peripheral(uint8_t iterations);

void main(void)
{
	(void)init_peripheral(CONFIG_SAMPLE_CONN_ITERATIONS);
}
