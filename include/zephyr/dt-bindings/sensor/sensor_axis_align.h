/*
 * Copyright 2025 CogniPilot Foundation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_AXIS_ALIGN_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_AXIS_ALIGN_H_

/**
 * @defgroup SENSOR_AXIS_ALIGN DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup SENSOR_AXIS_ALIGN_INDEX_DT Axis description for sensor alignment
 * @{
 */
#define SENSOR_AXIS_ALIGN_DT_X		0
#define SENSOR_AXIS_ALIGN_DT_Y		1
#define SENSOR_AXIS_ALIGN_DT_Z		2
/** @} */

/**
 * @defgroup SENSOR_AXIS_ALIGN_DT Axis description for sensor alignment
 * @{
 */
#define SENSOR_AXIS_ALIGN_DT_NEG		0
#define SENSOR_AXIS_ALIGN_DT_POS		2
/** @} */


/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_AXIS_ALIGN_H_ */
