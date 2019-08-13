/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/rtc.h>

static inline u32_t z_vrfy_rtc_read(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, read));
	return z_impl_rtc_read((struct device *)dev);
}

static inline void z_vrfy_rtc_enable(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, enable));
	z_impl_rtc_enable((struct device *)dev);
	return 0;
}

static inline void z_vrfy_rtc_disable(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, disable));
	z_impl_rtc_disable((struct device *)dev);
	return 0;
}

static inline int z_vrfy_rtc_set_alarm(struct device *dev,
				      const u32_t alarm_val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, set_alarm));
	return z_impl_rtc_set_alarm((struct device *)dev, alarm_val);
}

static inline int z_vrfy_rtc_get_pending_int(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, get_pending_int));
	return z_impl_rtc_get_pending_int((struct device *)dev);
}

#include <syscalls/rtc_read_mrsh.c>
#include <syscalls/rtc_enable_mrsh.c>
#include <syscalls/rtc_disable_mrsh.c>
#include <syscalls/rtc_set_alarm_mrsh.c>
#include <syscalls/rtc_get_pending_int_mrsh.c>
