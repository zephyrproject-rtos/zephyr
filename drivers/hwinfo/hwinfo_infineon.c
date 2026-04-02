/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>

#include <cy_syslib.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint64_t uid = sys_cpu_to_be64(Cy_SysLib_GetUniqueId());

	length = MIN(length, sizeof(uid));
	memcpy(buffer, &uid, length);

	return length;
}
