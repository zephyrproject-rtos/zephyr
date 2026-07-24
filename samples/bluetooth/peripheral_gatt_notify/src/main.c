/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

extern uint32_t peripheral_gatt_notify(uint32_t count);

int main(void)
{
	(void)peripheral_gatt_notify(0U);
	return 0;
}
