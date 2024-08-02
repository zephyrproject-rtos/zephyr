/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

extern uint32_t peripheral_gatt_write(uint32_t count);

int main(void)
{
	(void)peripheral_gatt_write(0U);
	return 0;
}
