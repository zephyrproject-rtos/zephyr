/*
 * Copyright 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/haptics.h>

static inline int z_vrfy_haptics_calibrate(const struct device *dev, const uint32_t routine)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, calibrate));

	return z_impl_haptics_calibrate(dev, routine);
}

#include <syscalls/haptics_calibrate_mrsh.c>

static inline int z_vrfy_haptics_monitor_get(const struct device *dev,
					     const enum haptics_monitor monitor,
					     const enum haptics_monitor_type type,
					     struct sensor_value *const val)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, monitor_get));

	K_OOPS(K_SYSCALL_MEMORY_WRITE(val, sizeof(*val)));

	return z_impl_haptics_monitor_get(dev, monitor, type, val);
}

#include <syscalls/haptics_monitor_get_mrsh.c>

static inline int z_vrfy_haptics_monitor_set(const struct device *dev,
					     const enum haptics_monitor monitor, const bool enable)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, monitor_set));

	return z_impl_haptics_monitor_set(dev, monitor, enable);
}

#include <syscalls/haptics_monitor_set_mrsh.c>

static inline int z_vrfy_haptics_select_source(const struct device *dev,
					       const enum haptics_source src,
					       const union haptics_config *const cfg)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, select_source));

	if (cfg != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));
	}

	return z_impl_haptics_select_source(dev, src, cfg);
}

#include <syscalls/haptics_select_source_mrsh.c>

static inline int z_vrfy_haptics_set_level(const struct device *dev, const enum haptics_source src,
					   const union haptics_config *const cfg,
					   const uint8_t level)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, set_level));

	if (cfg != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));
	}

	return z_impl_haptics_set_level(dev, src, cfg, level);
}

#include <syscalls/haptics_set_level_mrsh.c>

static inline int z_vrfy_haptics_start_output(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, start_output));

	return z_impl_haptics_start_output(dev);
}

#include <syscalls/haptics_start_output_mrsh.c>

static inline int z_vrfy_haptics_stop_output(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, stop_output));

	return z_impl_haptics_stop_output(dev);
}

#include <syscalls/haptics_stop_output_mrsh.c>

static inline int z_vrfy_haptics_stream_samples(const struct device *dev,
						const uint8_t *const samples, const size_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_HAPTICS(dev, stream_samples));

	K_OOPS(K_SYSCALL_MEMORY_READ(samples, len));

	return z_impl_haptics_stream_samples(dev, samples, len);
}

#include <syscalls/haptics_stream_samples_mrsh.c>
