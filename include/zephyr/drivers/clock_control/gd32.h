/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_GD32_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_GD32_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

/** Devicetree node representing GD32 clock controller. */
#define GD32_CLOCK_CONTROLLER_NODE DT_NODELABEL(rcu)

/** Common clock controller device for GD32 SoCs */
#define GD32_CLOCK_CONTROLLER DEVICE_DT_GET(GD32_CLOCK_CONTROLLER_NODE)

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_GD32_H_ */
