/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/renesas_lvd/renesas_lvd.h>
#include <zephyr/internal/syscall_handler.h>

int z_vrfy_renesas_lvd_get_status(const struct device *dev, renesas_lvd_status_t *status)
{
	renesas_lvd_status_t status_copy;

	K_OOPS(K_SYSCALL_DRIVER_RENESAS_LVD(dev, get_status));
	K_OOPS(k_usermode_from_copy(&status_copy, status, sizeof(status_copy)));

	int ret = z_impl_renesas_lvd_get_status(dev, &status_copy);

	K_OOPS(k_usermode_to_copy(status, &status_copy, sizeof(*status)));

	return ret;
}
#include <zephyr/syscalls/renesas_lvd_get_status_mrsh.c>

int z_vrfy_renesas_lvd_clear_status(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_LVD(dev, clear_status));
	return z_impl_renesas_lvd_clear_status(dev);
}
#include <zephyr/syscalls/renesas_lvd_clear_status_mrsh.c>

static int z_vrfy_renesas_lvd_callback_set(const struct device *dev,
					   renesas_lvd_callback_t callback, void *user_data)
{
	K_OOPS(K_SYSCALL_DRIVER_RENESAS_LVD(dev, callback_set));
	return z_impl_renesas_lvd_callback_set(dev, callback, user_data);
}
#include <zephyr/syscalls/renesas_lvd_callback_set_mrsh.c>
