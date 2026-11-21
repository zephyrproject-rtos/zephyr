/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 hwinfo driver.
 * Reads unique device ID from OTP via the Zephyr OTP API.
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/device.h>

/*
 * CPU ID location within OTP non-secure region (byte offset, size).
 * U-Boot: CFG_CPUID_OFFSET = 0x7, CPUID_LEN = 0x10
 * Linux:  otp_id: id@7 { reg = <0x07 0x10>; }
 */
#define RK3588_CPUID_OFFSET 0x07
#define RK3588_CPUID_LEN    16

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(rk3588_otp));
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (length > RK3588_CPUID_LEN) {
		length = RK3588_CPUID_LEN;
	}

	ret = otp_read(dev, RK3588_CPUID_OFFSET, buffer, length);
	if (ret < 0) {
		return ret;
	}

	return length;
}
