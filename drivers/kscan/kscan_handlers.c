/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/kscan.h>
#include <syscall_handler.h>

static inline int z_vrfy_kscan_config(struct device *dev,
				      kscan_callback_t callback_isr)
{
	Z_OOPS(Z_SYSCALL_DRIVER_KSCAN(dev, config));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(callback == 0,
				    "callback cannot be set from user mode"));
	return z_impl_kscan_config((struct device *)dev, callback);
}
#include <syscalls/kscan_config_mrsh.c>

static inline int z_vrfy_kscan_disable_callback(struct device *dev);
{
	return z_impl_kscan_disable_callback((struct device *)dev);
}
#include <syscalls/kscan_disable_callback_mrsh.c>

static int z_vrfy_kscan_enable_callback(struct device *dev);
{
	return z_impl_kscan_enable_callback((struct device *)dev);
}
#include <syscalls/kscan_enable_callback_mrsh.c>
