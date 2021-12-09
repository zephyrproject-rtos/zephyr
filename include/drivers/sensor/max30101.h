/*
 * Copyright (c) 2021, Silvano Cortesi
 * Copyright (c) 2021, ETH Zürich
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Maxim Integrated MAX30101
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30101_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30101_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

#ifdef CONFIG_MAX30101_TRIGGER
/**
 * @brief Read all data from the FIFO.
 *
 * Function to read all data of the MAX30101 FIFO (without triggering a new measurement).
 * Sample is stored inside device data struct.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int max30101_readout_batch(const struct device *dev);
#endif /* CONFIG_MAX30101_TRIGGER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30101_ */
