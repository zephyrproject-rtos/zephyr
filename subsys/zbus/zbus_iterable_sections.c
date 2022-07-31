/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

bool zbus_iterate_over_channels(bool (*iterator_func)(const struct zbus_channel *chan))
{
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		if (!(*iterator_func)(chan)) {
			return false;
		}
	}
	return true;
}

bool zbus_iterate_over_observers(bool (*iterator_func)(const struct zbus_observer *obs))
{
	STRUCT_SECTION_FOREACH(zbus_observer, obs) {
		if (!(*iterator_func)(obs)) {
			return false;
		}
	}
	return true;
}
