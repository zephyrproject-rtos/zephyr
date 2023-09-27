/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/kscan.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_kscan_config(const struct device *dev,
				      kscan_callback_t callback_isr)
{
	K_OOPS(K_SYSCALL_DRIVER_KSCAN(dev, config));
	K_OOPS(K_SYSCALL_VERIFY_MSG(callback_isr == 0,
				    "callback cannot be set from user mode"));
	return z_impl_kscan_config((const struct device *)dev, callback_isr);
}
#include <syscalls/kscan_config_mrsh.c>

static inline int z_vrfy_kscan_disable_callback(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_KSCAN(dev, disable_callback));

	return z_impl_kscan_disable_callback((const struct device *)dev);
}
#include <syscalls/kscan_disable_callback_mrsh.c>

static int z_vrfy_kscan_enable_callback(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_KSCAN(dev, enable_callback));

	return z_impl_kscan_enable_callback((const struct device *)dev);
}
#include <syscalls/kscan_enable_callback_mrsh.c>
