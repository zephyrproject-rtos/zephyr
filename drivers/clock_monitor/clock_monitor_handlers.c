/*
 * Copyright (c) 2026 NXP
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
	K_OOPS(k_usermode_from_copy(&cfg_copy, cfg, sizeof(cfg_copy)));
	K_OOPS(K_SYSCALL_VERIFY_MSG(cfg_copy.callback == NULL,
				    "callbacks may not be set from user mode"));
	return z_impl_clock_monitor_configure(dev, &cfg_copy);
}
#include <zephyr/syscalls/clock_monitor_configure_mrsh.c>

static inline int z_vrfy_clock_monitor_check(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, check));
	return z_impl_clock_monitor_check(dev);
}
#include <zephyr/syscalls/clock_monitor_check_mrsh.c>

static inline int z_vrfy_clock_monitor_stop(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, stop));
	return z_impl_clock_monitor_stop(dev);
}
#include <zephyr/syscalls/clock_monitor_stop_mrsh.c>

static inline int z_vrfy_clock_monitor_measure(const struct device *dev,
					       uint32_t *rate_hz,
					       k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, measure));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(rate_hz, sizeof(*rate_hz)));
	return z_impl_clock_monitor_measure(dev, rate_hz, timeout);
}
#include <zephyr/syscalls/clock_monitor_measure_mrsh.c>

static inline int z_vrfy_clock_monitor_get_events(const struct device *dev,
						  uint32_t *events)
{
	K_OOPS(K_SYSCALL_DRIVER_CLOCK_MONITOR(dev, get_events));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(events, sizeof(*events)));
	return z_impl_clock_monitor_get_events(dev, events);
}
#include <zephyr/syscalls/clock_monitor_get_events_mrsh.c>
