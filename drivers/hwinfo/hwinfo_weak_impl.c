/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>

ssize_t __weak z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	return -ENOSYS;
}

int __weak z_impl_hwinfo_get_device_eui64(uint8_t *buffer)
{
	return -ENOSYS;
}

int __weak z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	return -ENOSYS;
}

int __weak z_impl_hwinfo_clear_reset_cause(void)
{
	return -ENOSYS;
}

int __weak z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	return -ENOSYS;
}
