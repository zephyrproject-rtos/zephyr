/*
 * Copyright 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uaol.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_uaol_adjust_rate(const struct device *dev, int stream, bool increase)
{
	K_OOPS(K_SYSCALL_DRIVER_UAOL(dev, adjust_rate));

	return z_impl_uaol_adjust_rate(dev, stream, increase);
}
#include <zephyr/syscalls/uaol_adjust_rate_mrsh.c>

static inline int z_vrfy_uaol_interpret_feedback_value(const struct device *dev, int stream,
						       uint32_t feedback_value)
{
	K_OOPS(K_SYSCALL_DRIVER_UAOL(dev, interpret_feedback_value));

	return z_impl_uaol_interpret_feedback_value(dev, stream, feedback_value);
}
#include <zephyr/syscalls/uaol_interpret_feedback_value_mrsh.c>
