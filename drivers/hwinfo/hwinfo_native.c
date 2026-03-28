/*
 * Copyright (c) 2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmdline.h>
#include <nsi_host_trampolines.h>
#include <posix_native_task.h>
#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include "hwinfo_native_bottom.h"

static uint32_t native_hwinfo_device_id;
static bool native_hwinfo_device_id_set;
static uint32_t native_hwinfo_reset_cause;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{

	if (length > sizeof(native_hwinfo_device_id)) {
		length = sizeof(native_hwinfo_device_id);
	}

	sys_put_be(buffer, &native_hwinfo_device_id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	*cause = native_hwinfo_reset_cause;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	native_hwinfo_reset_cause = 0;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_POR | RESET_SOFTWARE;

	return 0;
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

static void native_hwinfo_get_reset_cause(void)
{
	/* If CONFIG_NATIVE_SIM_REBOOT was set, and a reboot was triggered, this
	 * environment variable would be set. Otherwise it is not expected to
	 * exist. Note this environment variable is not an stable API of any kind
	 */
	const char *cause = nsi_host_getenv("NATIVE_SIM_RESET_CAUSE");

	if (!cause) {
		/* Default to POR if not set */
		native_hwinfo_reset_cause = RESET_POR;
		return;
	}

	if (strcmp(cause, "SOFTWARE") == 0) {
		native_hwinfo_reset_cause = RESET_SOFTWARE;
	} else {
		posix_print_warning("NATIVE_SIM_RESET_CAUSE (%s) set to an unknown reset cause, "
				    "defaulting to POR\n",
				    cause);
		native_hwinfo_reset_cause = RESET_POR;
	}
}

NATIVE_TASK(native_hwinfo_add_options, PRE_BOOT_1, 10);
NATIVE_TASK(native_hwinfo_gethostid, PRE_BOOT_2, 10);
NATIVE_TASK(native_hwinfo_get_reset_cause, PRE_BOOT_2, 10);
