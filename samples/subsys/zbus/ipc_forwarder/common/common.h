/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_H
#define COMMON_H

#include <zephyr/zbus/zbus.h>

/* Sample data structures for request and response */
struct request_data {
	int request_id;
	int min_value;
	int max_value;
};

struct response_data {
	int response_id;
	int value;
};

/* Conditional compilation for device-specific code, needed if channels should be included on a
 * subset of applications devices
 */
#if defined(ZBUS_DEVICE_A) || defined(ZBUS_DEVICE_B)
#define include_on_device_a_b 1
#else
#define include_on_device_a_b 0
#endif

/* Define shared channels for request and response
 * request_channel is  master on device A and shadow on device B
 * response_channel is shadow on device A and master on device B
 */
ZBUS_MULTIDOMAIN_CHAN_DEFINE(request_channel, struct request_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
			     ZBUS_MSG_INIT(0), IS_ENABLED(ZBUS_DEVICE_A), include_on_device_a_b);

ZBUS_MULTIDOMAIN_CHAN_DEFINE(response_channel, struct response_data, NULL, NULL,
			     ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0), IS_ENABLED(ZBUS_DEVICE_B),
			     include_on_device_a_b);

#endif /* COMMON_H */
