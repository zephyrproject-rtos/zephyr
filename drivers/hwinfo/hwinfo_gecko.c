/*
 * Copyright (c) 2021 Sun Amar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <em_system.h>
#include <drivers/hwinfo.h>
#include <string.h>
#include <sys/byteorder.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint64_t unique_id = sys_cpu_to_be64(SYSTEM_GetUnique());

	if (length > sizeof(unique_id)) {
		length = sizeof(unique_id);
	}

	memcpy(buffer, &unique_id, length);

	return length;
}
