/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ADV_SLOW_INT                                                           \
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,                               \
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX

#define ADV_FAST_INT                                                           \
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,                             \
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2

#define BT_DEVICE_NAME (IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC) ? \
			(const uint8_t *)bt_get_name() : \
			(const uint8_t *)CONFIG_BT_DEVICE_NAME)
#define BT_DEVICE_NAME_LEN (IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC) ? strlen(bt_get_name()) : \
			    (sizeof(CONFIG_BT_DEVICE_NAME) - 1))
