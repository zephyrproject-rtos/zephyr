/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name apds9253 resolution channel references
 * @{
 */

#define APDS9253_RESOLUTION_20BIT_400MS 0
#define APDS9253_RESOLUTION_19BIT_200MS BIT(4)
#define APDS9253_RESOLUTION_18BIT_100MS BIT(5) /* default */
#define APDS9253_RESOLUTION_17BIT_50MS  (BIT(5) | BIT(4))
#define APDS9253_RESOLUTION_16BIT_25MS  BIT(6)
#define APDS9253_RESOLUTION_13BIT_3MS   (BIT(6) | BIT(4))

/** @} */

/**
 * @name apds9253 measurement rate
 * @{
 */

#define APDS9253_MEASUREMENT_RATE_2000MS (BIT(2) | BIT(1) | BIT(0))
#define APDS9253_MEASUREMENT_RATE_1000MS (BIT(2) | BIT(0))
#define APDS9253_MEASUREMENT_RATE_500MS  BIT(2)
#define APDS9253_MEASUREMENT_RATE_200MS  (BIT(1) | BIT(0))
#define APDS9253_MEASUREMENT_RATE_100MS  BIT(1) /* default */
#define APDS9253_MEASUREMENT_RATE_50MS   BIT(0)
#define APDS9253_MEASUREMENT_RATE_25MS   0

/** @} */

/**
 * @name apds9253 gain range
 * @{
 */

#define APDS9253_GAIN_RANGE_18 BIT(2)
#define APDS9253_GAIN_RANGE_9  (BIT(1) | BIT(0))
#define APDS9253_GAIN_RANGE_6  BIT(1)
#define APDS9253_GAIN_RANGE_3  BIT(0) /* default */
#define APDS9253_GAIN_RANGE_1  0

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_APDS9253_H_*/
