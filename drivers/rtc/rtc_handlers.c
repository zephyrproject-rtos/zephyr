/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <rtc.h>

_SYSCALL_HANDLER1_SIMPLE(rtc_read, K_OBJ_DRIVER_RTC, struct device *);

_SYSCALL_HANDLER1_SIMPLE_VOID(rtc_enable, K_OBJ_DRIVER_RTC, struct device *);

_SYSCALL_HANDLER1_SIMPLE_VOID(rtc_disable, K_OBJ_DRIVER_RTC, struct device *);

_SYSCALL_HANDLER(rtc_set_alarm, dev, alarm_val)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RTC);
	return _impl_rtc_set_alarm((struct device *)dev, alarm_val);
}

_SYSCALL_HANDLER1_SIMPLE(rtc_get_pending_int, K_OBJ_DRIVER_RTC,
			 struct device *);
