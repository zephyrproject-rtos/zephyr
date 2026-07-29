/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/zbus/zbus.h>

#include "zbus_private.h"

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

bool zbus_iterate_over_channels(bool (*iterator_func)(const struct zbus_channel *chan))
{
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		if (!(*iterator_func)(chan)) {
			return false;
		}
	}
#if defined(CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION)
	return zbus_runtime_iterate_over_channels(iterator_func);
#else
	return true;
#endif /* CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION */
}

bool zbus_iterate_over_channels_with_user_data(
	bool (*iterator_func)(const struct zbus_channel *chan, void *user_data), void *user_data)
{
	STRUCT_SECTION_FOREACH(zbus_channel, chan) {
		if (!(*iterator_func)(chan, user_data)) {
			return false;
		}
	}
#if defined(CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION)
	return zbus_runtime_iterate_over_channels_with_user_data(iterator_func, user_data);
#else
	return true;
#endif /* CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION */
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

bool zbus_iterate_over_observers_with_user_data(
	bool (*iterator_func)(const struct zbus_observer *obs, void *user_data), void *user_data)
{
	STRUCT_SECTION_FOREACH(zbus_observer, obs) {
		if (!(*iterator_func)(obs, user_data)) {
			return false;
		}
	}
	return true;
}
