/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	otpc_registers_t *const otpc = (void *)DT_REG_ADDR(DT_INST(0, microchip_sama7g5_otpc));

	if (length > 16) {
		length = 16;
	}
	memcpy(buffer, (void *)&otpc->OTPC_UID0R, length);

	return length;
}
