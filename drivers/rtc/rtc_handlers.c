/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <rtc.h>

Z_SYSCALL_HANDLER(rtc_read, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, read));
	return _impl_rtc_read((struct device *)dev);
}

Z_SYSCALL_HANDLER(rtc_enable, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, enable));
	_impl_rtc_enable((struct device *)dev);
	return 0;
}

Z_SYSCALL_HANDLER(rtc_disable, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, disable));
	_impl_rtc_disable((struct device *)dev);
	return 0;
}

Z_SYSCALL_HANDLER(rtc_set_alarm, dev, alarm_val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, set_alarm));
	return _impl_rtc_set_alarm((struct device *)dev, alarm_val);
}

Z_SYSCALL_HANDLER(rtc_get_pending_int, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RTC(dev, get_pending_int));
	return _impl_rtc_get_pending_int((struct device *)dev);
}
