/*
 * Copyright (c) 2020 Paratronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/rtc.h>

static inline int z_vrfy_rtc_get_time(struct device *dev,
				      struct timespec *tp)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RTC));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(tp, sizeof(*tp)));
	return z_impl_rtc_get_time(dev, tp);
}
#include <syscalls/rtc_get_time_mrsh.c>

static inline int z_vrfy_rtc_set_time(struct device *dev,
				      const struct timespec *tp)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, set_time));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(tp, sizeof(*tp)));
	return z_impl_rtc_set_time(dev, tp);
}
#include <syscalls/rtc_set_time_mrsh.c>
