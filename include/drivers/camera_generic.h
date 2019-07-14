/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for camera drivers
 */

#ifndef ZEPHYR_INCLUDE_CAMERA_GENERIC_H_
#define ZEPHYR_INCLUDE_CAMERA_GENERIC_H_

#include <zephyr/types.h>
#include <device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(CONFIG_IMAGE_SENSOR_INIT_PRIO) &&\
	defined(CONFIG_CAMERA_INIT_PRIO))
#if (CONFIG_IMAGE_SENSOR_INIT_PRIO >= CONFIG_CAMERA_INIT_PRIO)
#error Image sensor initalization must be earlier than
#error camera initalization.
#endif
#endif

/*In order to support multiple cameras on device.*/
#define CAMERA_MAX_NUMBER 2

/*For multiple cameras on board, the dts LABEL of
 * primary camera should be named "primary".
 */
#define CAMERA_PRIMARY_LABEL "primary"

/*For multiple cameras on board, the dts LABEL of
 * secondary camera should be named "secondary".
 */
#define CAMERA_SECONDARY_LABEL "secondary"

/**
 * @enum camera_id
 * @brief Enumeration with camera id to identify
 * if the camera is promary camera or secondary camera.
 *
 */
enum camera_id {
	CAMERA_NULL_ID = 0,
	CAMERA_PRIMARY_ID = 1,
	CAMERA_SECONDARY_ID = 2
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CAMERA_GENERIC_H_*/
