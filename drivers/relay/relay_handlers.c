/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/relay/relay.h>

static inline int z_vrfy_relay_set_state(const struct device *dev, bool on)
{
	K_OOPS(K_SYSCALL_DRIVER_RELAY(dev, set_state));
	return z_impl_relay_set_state(dev, on);
}
#include <zephyr/syscalls/relay_set_state_mrsh.c>

static inline int z_vrfy_relay_get_state(const struct device *dev, bool *on)
{
	K_OOPS(K_SYSCALL_DRIVER_RELAY(dev, get_state));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(on, sizeof(*on)));
	return z_impl_relay_get_state(dev, on);
}
#include <zephyr/syscalls/relay_get_state_mrsh.c>
