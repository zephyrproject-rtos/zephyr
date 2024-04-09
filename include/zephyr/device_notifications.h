/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_NOTIFICATIONS_H_
#define ZEPHYR_INCLUDE_DEVICE_NOTIFICATIONS_H_

#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_CHANNEL_GET(node_id) _CONCAT(__device_channel_, node_id)

#ifdef CONFIG_DEVICE_NOTIFICATIONS
#define DEVICE_CHANNEL_DEFINE(node_id)                             \
	ZBUS_CHAN_DEFINE(DEVICE_CHANNEL_GET(node_id),              \
			 struct device_notification,               \
			 NULL, NULL, ZBUS_OBSERVERS_EMPTY,         \
			 ZBUS_MSG_INIT(0));                        \
	\


#else
#define DEVICE_CHANNEL_DEFINE(node_id)
#endif

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_DEVICE_NOTIFICATIONS_H_ */
