/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	bsp_unique_id_t const *unique_id = R_BSP_UniqueIdGet();
	size_t len = MIN(length, sizeof(unique_id->unique_id_bytes));

	memcpy(buffer, &unique_id->unique_id_bytes[0], len);

	return len;
}
