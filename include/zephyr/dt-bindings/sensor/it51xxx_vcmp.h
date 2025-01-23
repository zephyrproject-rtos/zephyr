/*
 * Copyright (c) 2025 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT51XXX_VCMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT51XXX_VCMP_H_

/**
 * @name it51xxx voltage comparator channel references
 * @{
 */

#define VCMP_CHANNEL_0   0
#define VCMP_CHANNEL_1   1
#define VCMP_CHANNEL_2   2
#define VCMP_CHANNEL_CNT 3

/** @} */

/**
 * @name it51xxx voltage comparator interrupt trigger mode
 * @{
 */

#define IT51XXX_VCMP_LESS_OR_EQUAL 0
#define IT51XXX_VCMP_GREATER       1
#define IT51XXX_VCMP_UNDEFINED     0xffff

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT51XXX_VCMP_H_ */
