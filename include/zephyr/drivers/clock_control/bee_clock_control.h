/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Realtek BEE Clock Control Driver Header
 * @ingroup clock_control_bee
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BEE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BEE_H_

#include <zephyr/device.h>

/**
 * @defgroup clock_control_bee Realtek BEE
 * @ingroup clock_control_interface_ext
 * @{
 */

/**
 * @brief Global device handle for the BEE clock controller.
 *
 * This macro retrieves the device structure for the primary clock controller
 * defined in the devicetree with the label `cctl`.
 *
 * @return const struct device* Pointer to the clock controller device structure.
 */
#define BEE_CLOCK_CONTROLLER DEVICE_DT_GET(DT_NODELABEL(cctl))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_BEE_H_ */
