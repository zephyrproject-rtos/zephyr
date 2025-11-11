/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <soc.h>
#include <wrap_max32_sys.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

static struct k_spinlock device_id_lock;
static bool initialized;
static uint8_t usn[MXC_SYS_USN_LEN];

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&device_id_lock);

	if (!initialized) {
		ret = Wrap_MXC_SYS_GetUSN(usn);

		if (ret != E_NO_ERROR) {
			/* Error reading USN */
			goto exit;
		}

		initialized = true;
	}

	if (length > sizeof(usn)) {
		length = sizeof(usn);
	}

	/* Provide device ID in big endian */
	sys_memcpy_swap(buffer, usn, length);
	ret = length;

exit:
	k_spin_unlock(&device_id_lock, key);
	return ret;
}
