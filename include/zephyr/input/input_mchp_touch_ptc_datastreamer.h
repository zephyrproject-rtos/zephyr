/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This DataStreamer API is specifically designed
 * to visualize touch parameters using the Microchip Data Visualizer software.
 */

#ifndef INPUT_MCHP_TOUCH_PTC_DATASTREAMER_H
#define INPUT_MCHP_TOUCH_PTC_DATASTREAMER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the microchip datastreamer.
 *
 * @param dev Pointer to the UART device.
 *
 * @retval 0 If the device is ready for use
 * @retval ENODEV If the device is not ready for use or if a NULL device pointer
 * is passed as argument.
 */
int datastreamer_init(const struct device *dev);

/**
 * @brief It constructs the packet using touch parameters
 * and transmits it through the UART driver.
 *
 * @param dev Pointer to the PTC device.
 */
void datastreamer_output(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_MCHP_TOUCH_PTC_DATASTREAMER_H */
