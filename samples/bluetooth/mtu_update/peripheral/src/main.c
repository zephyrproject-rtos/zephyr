/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

extern void run_peripheral_sample(uint8_t *notify_data, size_t notify_data_size, uint16_t seconds);

int main(void)
{
	uint8_t notify_data[100] = {};

	notify_data[13] = 0x7f;
	notify_data[99] = 0x55;

	run_peripheral_sample(notify_data, sizeof(notify_data), 0);
	return 0;
}
