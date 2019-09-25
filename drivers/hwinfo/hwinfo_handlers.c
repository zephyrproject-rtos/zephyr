/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/hwinfo.h>

ssize_t z_vrfy_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buffer, length));

	return z_impl_hwinfo_get_device_id((u8_t *)buffer, (size_t)length);
}
#include <syscalls/hwinfo_get_device_id_mrsh.c>
