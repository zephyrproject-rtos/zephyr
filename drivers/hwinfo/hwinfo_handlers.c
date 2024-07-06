/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/hwinfo.h>

int z_vrfy_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, length));

	return z_impl_hwinfo_get_device_id((uint8_t *)buffer, (size_t)length);
}
#include <zephyr/syscalls/hwinfo_get_device_id_mrsh.c>

int z_vrfy_hwinfo_get_device_eui64(uint8_t *buffer)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, 8));

	return z_impl_hwinfo_get_device_eui64((uint8_t *)buffer);
}
#include <zephyr/syscalls/hwinfo_get_device_eui64_mrsh.c>

int z_vrfy_hwinfo_get_reset_cause(uint32_t *cause)
{
	int ret;
	uint32_t cause_copy;

	ret = z_impl_hwinfo_get_reset_cause(&cause_copy);
	K_OOPS(k_usermode_to_copy(cause, &cause_copy, sizeof(uint32_t)));

	return ret;
}
#include <zephyr/syscalls/hwinfo_get_reset_cause_mrsh.c>


int z_vrfy_hwinfo_clear_reset_cause(void)
{
	return z_impl_hwinfo_clear_reset_cause();
}
#include <zephyr/syscalls/hwinfo_clear_reset_cause_mrsh.c>

int z_vrfy_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	int ret;
	uint32_t supported_copy;

	ret = z_impl_hwinfo_get_supported_reset_cause(&supported_copy);
	K_OOPS(k_usermode_to_copy(supported, &supported_copy, sizeof(uint32_t)));

	return ret;
}
#include <zephyr/syscalls/hwinfo_get_supported_reset_cause_mrsh.c>
