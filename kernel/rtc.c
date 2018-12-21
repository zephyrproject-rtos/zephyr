/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtc.h>

void _impl_rtc_enable(struct device *dev)
{
	const struct rtc_driver_api *api = dev->driver_api;

	api->enable(dev);
}

void _impl_rtc_disable(struct device *dev)
{
	const struct rtc_driver_api *api = dev->driver_api;

	api->disable(dev);
}

u32_t _impl_rtc_read(struct device *dev)
{
	const struct rtc_driver_api *api = dev->driver_api;

	return api->read(dev);
}

int _impl_rtc_set_alarm(struct device *dev,
			const u32_t alarm_val)
{
	const struct rtc_driver_api *api = dev->driver_api;

	return api->set_alarm(dev, alarm_val);
}

int _impl_rtc_get_pending_int(struct device *dev)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}
