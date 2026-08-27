/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_otp_uid

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one nxp,otp-uid node must be enabled");

/* OTP flash address and size (in bytes) of the unique chip identifier. */
#define UID_ADDR DT_INST_REG_ADDR(0)
#define UID_SIZE DT_INST_REG_SIZE(0)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	/*
	 * The unique chip identifier lives in OTP flash and is read directly as
	 * memory. Copy it into the caller's buffer most-significant byte first
	 * so the device id reads consistently regardless of the caller's word
	 * size, writing only as many bytes as were requested.
	 */
	const uint8_t *uid = (const uint8_t *)UID_ADDR;

	length = MIN(length, (size_t)UID_SIZE);

	for (size_t i = 0; i < length; i++) {
		buffer[i] = uid[UID_SIZE - 1 - i];
	}

	return length;
}
