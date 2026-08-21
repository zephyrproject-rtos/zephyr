/*
 * Copyright (c) 2026 Ana Clara Forcelli <ana.forcelli@lsitec.org.br>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwaccel.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_accel_set_callback(const struct device *dev, hwaccel_irq_callback_user_data_t cb, void *user_data)
{
	K_OOPS(K_SYSCALL_DRIVER_HWACCEL(dev, set_callback));
	return z_impl_accel_set_callback(dev, cb, user_data);
}
#include <zephyr/syscalls/accel_set_callback_mrsh.c>

static inline int z_vrfy_accel_start(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_HWACCEL(dev, start));
	return z_impl_accel_start(dev);
}
#include <zephyr/syscalls/accel_start_mrsh.c>

static inline int z_vrfy_accel_abort(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_HWACCEL(dev, abort));
	return z_impl_accel_abort(dev);
}
#include <zephyr/syscalls/accel_abort_mrsh.c>

static inline int z_vrfy_accel_query_hw_caps(const struct device *dev, accel_hw_caps_t *caps)
{
	K_OOPS(K_SYSCALL_DRIVER_HWACCEL(dev, query_hw_caps));
	K_OOPS(K_SYSCALL_MEMORY_READ(caps, sizeof(accel_hw_caps_t)));
	K_OOPS(K_SYSCALL_MEMORY_READ(caps->hw_caps, sizeof(uint32_t)));
	K_OOPS(K_SYSCALL_MEMORY_READ(caps->fmt_caps, sizeof(uint32_t)));
	K_OOPS(K_SYSCALL_MEMORY_READ(caps->max_input_buffers, sizeof(uint32_t)));
	K_OOPS(K_SYSCALL_MEMORY_READ(caps->max_chan_dimension, sizeof(uint32_t)));
	return z_impl_accel_query_hw_caps(dev, caps);
}
#include <zephyr/syscalls/accel_query_hw_caps_mrsh.c>

static inline int z_vrfy_accel_set_buffers(const struct device *dev, accel_buffer_t **in_bufs,
					   int nbufs, accel_buffer_t *out_buf)
{
	K_OOPS(K_SYSCALL_DRIVER_HWACCEL(dev, set_buffers));
	return z_impl_accel_set_buffers(dev, in_bufs, nbufs, out_buf);
}
#include <zephyr/syscalls/accel_set_buffers_mrsh.c>

static inline int z_vrfy_accel_configure_ops(const struct device *dev, accel_hw_ops_t *ops_series,
					     int nops)
{
	K_OOPS(K_SYSCALL_DRIVER_HWACCEL(dev, configure_ops));
	return z_impl_accel_configure_ops(dev, ops_series, nops);
}
#include <zephyr/syscalls/accel_configure_ops_mrsh.c>
