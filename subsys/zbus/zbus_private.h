/*
 * Copyright (c) 2026 Embeint Pty Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION)

/**
 * @brief Private iterator for runtime registered channels
 *
 * see @ref zbus_iterate_over_channels
 */
bool zbus_runtime_iterate_over_channels(bool (*iterator_func)(const struct zbus_channel *chan));

/**
 * @brief Private iterator for runtime registered channels with user data
 *
 * see @ref zbus_iterate_over_channels_with_user_data
 */
bool zbus_runtime_iterate_over_channels_with_user_data(
	bool (*iterator_func)(const struct zbus_channel *chan, void *user_data), void *user_data);

#endif /* CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION */
