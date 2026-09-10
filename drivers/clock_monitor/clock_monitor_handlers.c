/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_clock_monitor_configure(const struct device *dev,
						 const struct clock_monitor_config *cfg)
{
	struct clock_monitor_config cfg_copy;

	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, configure));
	K_OOPS(k_usermode_from_copy(&cfg_copy, cfg, sizeof(*cfg)));
	K_OOPS(K_SYSCALL_VERIFY_MSG(cfg_copy.callback == NULL,
				    "callbacks may not be set from user mode"));
	return z_impl_clock_monitor_configure(dev, &cfg_copy);
}
#include <zephyr/syscalls/clock_monitor_configure_mrsh.c>

static inline int z_vrfy_clock_monitor_start(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, start));
	return z_impl_clock_monitor_start(dev);
}
#include <zephyr/syscalls/clock_monitor_start_mrsh.c>

static inline int z_vrfy_clock_monitor_stop(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, stop));
	return z_impl_clock_monitor_stop(dev);
}
#include <zephyr/syscalls/clock_monitor_stop_mrsh.c>

static inline int z_vrfy_clock_monitor_get_rate(const struct device *dev,
						uint32_t *rate_hz)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, get_rate));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(rate_hz, sizeof(*rate_hz)));
	return z_impl_clock_monitor_get_rate(dev, rate_hz);
}
#include <zephyr/syscalls/clock_monitor_get_rate_mrsh.c>

static inline int z_vrfy_clock_monitor_set_source(const struct device *dev, uint32_t reference,
						  uint32_t target)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, set_source));
	return z_impl_clock_monitor_set_source(dev, reference, target);
}
#include <zephyr/syscalls/clock_monitor_set_source_mrsh.c>
