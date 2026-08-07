/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/usb/usb_bc12.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_bc12_set_role(const struct device *dev, enum bc12_role role)
{
	K_OOPS(K_SYSCALL_DRIVER_BC12(dev, set_role));

	return z_impl_bc12_set_role(dev, role);
}

static inline int z_vrfy_bc12_set_result_cb(const struct device *dev, bc12_callback_t cb,
					    void *user_data)
{
	K_OOPS(K_SYSCALL_DRIVER_BC12(dev, set_result_cb));
	K_OOPS(K_SYSCALL_VERIFY_MSG(cb == NULL, "callbacks may not be set from user mode"));

	return z_impl_bc12_set_result_cb(dev, cb, user_data);
}
