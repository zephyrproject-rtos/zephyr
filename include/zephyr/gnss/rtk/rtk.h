/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_GNSS_RTK_RTK_H_
#define ZEPHYR_INCLUDE_GNSS_RTK_RTK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct gnss_rtk_data {
	const uint8_t *data;
	size_t len;
};

typedef void (*gnss_rtk_data_callback_t)(const struct device *dev,
					 const struct gnss_rtk_data *data);

struct gnss_rtk_data_callback {
	const struct device *dev;
	gnss_rtk_data_callback_t callback;
};

#if CONFIG_GNSS_RTK
#define GNSS_RTK_DATA_CALLBACK_DEFINE(_dev, _callback)						   \
	static const STRUCT_SECTION_ITERABLE(gnss_rtk_data_callback,				   \
					     _gnss_rtk_data_callback__##_callback) = {		   \
		.dev = _dev,									   \
		.callback = _callback,								   \
	}

#define GNSS_DT_RTK_DATA_CALLBACK_DEFINE(_node_id, _callback)                                      \
	static const STRUCT_SECTION_ITERABLE(                                                      \
		gnss_rtk_data_callback,                                                            \
		_CONCAT_4(_gnss_rtk_data_callback_, DT_DEP_ORD(_node_id), _, _callback)) = {       \
		.dev = DEVICE_DT_GET(_node_id),                                                    \
		.callback = _callback,                                                             \
	}
#else
#define GNSS_RTK_DATA_CALLBACK_DEFINE(_dev, _callback)
#define GNSS_DT_RTK_DATA_CALLBACK_DEFINE(_node_id, _callback)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_GNSS_RTK_RTK_H_ */
