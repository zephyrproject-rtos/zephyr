/*
 * Copyright (c) 2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmdline.h>
#include <posix_native_task.h>
#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include "hwinfo_native_bottom.h"

static uint32_t native_hwinfo_device_id;
static bool native_hwinfo_device_id_set;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{

	if (length > sizeof(native_hwinfo_device_id)) {
		length = sizeof(native_hwinfo_device_id);
	}

	sys_put_be(buffer, &native_hwinfo_device_id, length);

	return length;
}

static void native_hwinfo_gethostid(void)
{
	if (!native_hwinfo_device_id_set) {
		native_hwinfo_device_id = native_hwinfo_gethostid_bottom();
	}
}

static void native_hwinfo_device_id_was_set(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);

	native_hwinfo_device_id_set = true;
}

static void native_hwinfo_add_options(void)
{
	static struct args_struct_t native_hwinfo_options[] = {
		{
			.option = "device_id",
			.name = "id",
			.type = 'u',
			.dest = (void *)&native_hwinfo_device_id,
			.call_when_found = native_hwinfo_device_id_was_set,
			.descript = "A 32-bit integer value to use as HWINFO device ID. "
				    "If not set, the host gethostid() output will be used.",
		},
		ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(native_hwinfo_options);
}

NATIVE_TASK(native_hwinfo_add_options, PRE_BOOT_1, 10);
NATIVE_TASK(native_hwinfo_gethostid, PRE_BOOT_2, 10);
