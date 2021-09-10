/*
 * Copyright (c) 2021, Silvano Cortesi
 * Copyright (c) 2021, ETH Zürich
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Maxim Integrated MAX30208
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30208_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30208_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

#ifdef CONFIG_MAX30208_TRIGGER
/**
 * @brief Read one sample from the FIFO.
 *
 * Function to read a sample of the MAX30208 FIFO (without triggering a new measurement).
 * Sample is stored inside device data struct.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int max30208_readout_sample(const struct device *dev);

/**
 * @brief Read all data from the FIFO.
 *
 * Function to read all data of the MAX30208 FIFO (without triggering a new measurement).
 * Sample is stored inside device data struct.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int max30208_readout_batch(const struct device *dev);
#endif /* CONFIG_MAX30208_TRIGGER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30208_ */
