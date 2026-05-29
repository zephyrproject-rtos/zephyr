/*
 * Copyright (c) 2025 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX345_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX345_H_

/**
 * @defgroup adxl345 ADXL345 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup adxl345_odr Output Rate options
 * @{
 */
#define ADXL345_DT_ODR_12_5		7
#define ADXL345_DT_ODR_25		8
#define ADXL345_DT_ODR_50		9
#define ADXL345_DT_ODR_100		10
#define ADXL345_DT_ODR_200		11
#define ADXL345_DT_ODR_400		12
/** @} */

/**
 * @defgroup adxl345_range Select range options
 * @{
 */
#define ADXL345_DT_RANGE_2G	    0
#define ADXL345_DT_RANGE_4G	    1
#define ADXL345_DT_RANGE_8G	    2
#define ADXL345_DT_RANGE_16G    3
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX345_H_ */
