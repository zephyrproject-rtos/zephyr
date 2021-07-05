/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/qep.h>

static inline int z_vrfy_qep_config_device(const struct device *dev,
					   struct qep_config *config)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, config_device));

	return z_impl_qep_config_device((const struct device *)dev,
					(struct qep_config *)config);
}

#include <syscalls/qep_config_device_mrsh.c>

static inline int z_vrfy_qep_start_decode(const struct device *dev,
					  qep_callback_t cb,
					  void *cb_param)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, start_decode));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cb == NULL,
				    "Callbacks forbidden from user mode"));
	return z_impl_qep_start_decode((const struct device *)dev,
				       (qep_callback_t)cb, (void *)cb_param);
}

#include <syscalls/qep_start_decode_mrsh.c>

static inline int z_vrfy_qep_stop_decode(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, stop_decode));
	return z_impl_qep_stop_decode((const struct device *)dev);
}

#include <syscalls/qep_stop_decode_mrsh.c>

static inline int z_vrfy_qep_get_direction(const struct device *dev,
					   uint32_t *p_direction)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, get_direction));
	return z_impl_qep_get_direction((const struct device *)dev,
					(uint32_t *)p_direction);
}

#include <syscalls/qep_get_direction_mrsh.c>

static inline int z_vrfy_qep_get_position_count(const struct device *dev,
						uint32_t *p_current_count)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, get_position_count));
	return z_impl_qep_get_position_count((const struct device *)dev,
					     (uint32_t *)p_current_count);
}

#include <syscalls/qep_get_position_count_mrsh.c>

static inline int z_vrfy_qep_start_capture(const struct device *dev,
					   uint64_t *buffer, uint32_t count,
					   qep_callback_t cb, void *cb_param)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, start_capture));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buffer, (count << 3)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cb == NULL,
				    "Callbacks forbidden from user mode"));
	return z_impl_qep_start_capture((const struct device *)dev,
					(uint64_t *)buffer,
					(uint32_t)count,
					(qep_callback_t)cb,
					(void *)cb_param);
}

#include <syscalls/qep_start_capture_mrsh.c>

static inline int z_vrfy_qep_stop_capture(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, stop_capture));
	return z_impl_qep_stop_capture((const struct device *)dev);
}

#include <syscalls/qep_stop_capture_mrsh.c>

static inline int z_vrfy_qep_enable_event(const struct device *dev,
					  qep_event_t event)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, enable_event));
	return z_impl_qep_enable_event((const struct device *)dev,
				       (qep_event_t)event);
}

#include <syscalls/qep_enable_event_mrsh.c>

static inline int z_vrfy_qep_disable_event(const struct device *dev,
					   qep_event_t event)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, disable_event));
	return z_impl_qep_disable_event((const struct device *)dev,
					(qep_event_t)event);
}

#include <syscalls/qep_disable_event_mrsh.c>

static inline int z_vrfy_qep_get_phase_err_status(const struct device *dev,
						  uint32_t *p_phase_err)
{
	Z_OOPS(Z_SYSCALL_DRIVER_QEP(dev, get_phase_err_status));
	return z_impl_qep_get_phase_err_status((const struct device *)dev,
					       (uint32_t *)p_phase_err);
}

#include <syscalls/qep_get_phase_err_status_mrsh.c>

