/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/hwinfo.h>

ssize_t z_vrfy_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buffer, length));

	return z_impl_hwinfo_get_device_id((uint8_t *)buffer, (size_t)length);
}
#include <syscalls/hwinfo_get_device_id_mrsh.c>

int z_vrfy_hwinfo_get_reset_cause(uint32_t *cause)
{
	int ret;
	uint32_t cause_copy;

	ret = z_impl_hwinfo_get_reset_cause(&cause_copy);
	Z_OOPS(z_user_to_copy(cause, &cause_copy, sizeof(uint32_t)));

	return ret;
}
#include <syscalls/hwinfo_get_reset_cause_mrsh.c>


int z_vrfy_hwinfo_clear_reset_cause(void)
{
	return z_impl_hwinfo_clear_reset_cause();
}
#include <syscalls/hwinfo_clear_reset_cause_mrsh.c>

int z_vrfy_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	int ret;
	uint32_t supported_copy;

	ret = z_impl_hwinfo_get_supported_reset_cause(&supported_copy);
	Z_OOPS(z_user_to_copy(supported, &supported_copy, sizeof(uint32_t)));

	return ret;
}
#include <syscalls/hwinfo_get_supported_reset_cause_mrsh.c>
