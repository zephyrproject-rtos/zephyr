/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gap/device_name.h>

#define ADV_SLOW_INT                                                           \
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,                               \
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX

#define ADV_FAST_INT                                                           \
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,                             \
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2

#if defined(CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC)
#define DEVICE_NAME_SIZE CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC_MAX
#else
#define DEVICE_NAME_SIZE BT_GAP_DEVICE_NAME_MAX_SIZE
#endif /* CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC */
