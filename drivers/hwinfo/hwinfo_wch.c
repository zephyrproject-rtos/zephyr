/*
 * SPDX-FileCopyrightText: Copyright Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

#include <hal_ch32fun.h>

#define DT_DRV_COMPAT wch_esig

/*
 * ch32fun uses different names for the typedefs and UID registers across variants.
 */
#define HWINFO_WCH_UID_OFFSET 0x08

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const uint8_t *uid = (const uint8_t *)DT_INST_REG_ADDR(0) + HWINFO_WCH_UID_OFFSET;

	length = MIN(length, 12);
	memcpy(buffer, uid, length);

	return length;
}
