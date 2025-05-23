/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RTK_RTK_H_
#define ZEPHYR_INCLUDE_RTK_RTK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct rtk_data {
	const uint8_t *data;
	size_t len;
};

typedef void (*rtk_data_callback_t)(const struct device *dev, const struct rtk_data *data);

struct rtk_data_callback {
	const struct device *dev;
	rtk_data_callback_t callback;
};

#if CONFIG_GNSS_RTK
#define RTK_DATA_CALLBACK_DEFINE(_dev, _callback)						   \
	static const STRUCT_SECTION_ITERABLE(rtk_data_callback,					   \
					     _rtk_data_callback__##_callback) = {		   \
		.dev = _dev,									   \
		.callback = _callback,								   \
	}
#else
#define RTK_DATA_CALLBACK_DEFINE(_dev, _callback)
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_RTK_RTK_H_ */
