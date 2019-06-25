/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/hwinfo.h>

ssize_t __weak z_impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	return -ENOTSUP;
}
