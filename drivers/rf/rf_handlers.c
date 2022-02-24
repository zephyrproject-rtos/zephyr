/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/rf.h>
#include <syscall_handler.h>

static inline int z_vrfy_rf_device_set(const struct device *rf,
					enum rf_device_arg arg,
					void *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, rf_device_set));
	return z_impl_rf_device_set((const struct device *)dev, cfg, val);
}
#include <syscalls/rf_device_set_mrsh.c>

static inline int z_vrfy_rf_device_get(const struct device *rf,
					enum rf_device_arg arg,
					void *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, rf_device_get));
	return z_impl_rf_device_get((const struct device *)dev, cfg, val);
}
#include <syscalls/rf_device_get_mrsh.c>

static inline int z_vrfy_rf_send(const struct device *rf, union rf_packet *pkt);
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, rf_send));
	return z_impl_rf_send((const struct device *)dev, (union rf_packet *)pkt);
}
#include <syscalls/rf_send_mrsh.c>

static inline int z_vrfy_rf_recv(const struct device *rf, union rf_packet *pkt);
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, rf_recv));
	return z_impl_rf_recv((const struct device *)dev, (union rf_packet *)pkt);
}
#include <syscalls/rf_recv_mrsh.c>
