/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/sys/util.h>

#define EFUSE_DEV DEVICE_DT_GET_ONE(sifli_sf32lb_efuse)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t uid[16];
	int ret;

	if (buffer == NULL) {
		return -EINVAL;
	}

	if (length == 0U) {
		return 0;
	}

	if (length > sizeof(uid)) {
		length = sizeof(uid);
	}

	if (!device_is_ready(EFUSE_DEV)) {
		return -ENODEV;
	}

	ret = otp_read(EFUSE_DEV, 0, uid, sizeof(uid));
	if (ret < 0) {
		return ret;
	}

	memcpy(buffer, uid, length);

	return (ssize_t)length;
}

