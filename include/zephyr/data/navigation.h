/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DATA_NAVIGATION_H_
#define ZEPHYR_INCLUDE_DATA_NAVIGATION_H_

#include <zephyr/types.h>

/**
 * @brief Navigation utilities
 * @defgroup navigation Navigation
 * @ingroup utilities
 * @{
 */

/**
 * @brief Navigation data structure
 *
 * @details The structure describes the momentary navigation details of a
 * point relative to a sphere (commonly Earth)
 */
struct navigation_data {
	/** Latitudal position in nanodegrees (0 to +-180E9) */
	int64_t latitude;
	/** Longitudal position in nanodegrees (0 to +-180E9) */
	int64_t longitude;
	/** Bearing angle in millidegrees (0 to 360E3) */
	uint32_t bearing;
	/** Speed in millimeters per second */
	uint32_t speed;
	/** Altitude in millimeters */
	int32_t altitude;
};

/**
 * @brief Calculate the distance between two navigation points along the
 * surface of the sphere they are relative to.
 *
 * @param distance Destination for calculated distance in millimeters
 * @param p1 First navigation point
 * @param p2 Second navigation point
 *
 * @return 0 if successful
 * @return -EINVAL if either navigation point is invalid
 */
int navigation_distance(uint64_t *distance, const struct navigation_data *p1,
			const struct navigation_data *p2);

/**
 * @brief Calculate the bearing from one navigation point to another
 *
 * @param bearing Destination for calculated bearing angle in millidegrees
 * @param from First navigation point
 * @param to Second navigation point
 *
 * @return 0 if successful
 * @return -EINVAL if either navigation point is invalid
 */
int navigation_bearing(uint32_t *bearing, const struct navigation_data *from,
		       const struct navigation_data *to);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DATA_NAVIGATION_H_ */
