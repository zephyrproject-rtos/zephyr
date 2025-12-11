/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <stddef.h>
#include <stdint.h>

extern void run_central_sample(bt_gatt_notify_func_t cb);

int main(void)
{
	run_central_sample(NULL);
	return 0;
}
