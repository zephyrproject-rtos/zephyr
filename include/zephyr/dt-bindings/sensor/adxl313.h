/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX313_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX313_H_

/**
 * @defgroup ADXL313 ADI DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ADXL313_ODR Output Rate options
 * @{
 */
#define ADXL313_DT_ODR_6_25 6  /**< ODR 6.25Hz setting for ADXL313 */
#define ADXL313_DT_ODR_12_5 7  /**< ODR 12.5Hz setting for ADXL313 */
#define ADXL313_DT_ODR_25   8  /**< ODR 25Hz setting for ADXL313 */
#define ADXL313_DT_ODR_50   9  /**< ODR 50Hz setting for ADXL313 */
#define ADXL313_DT_ODR_100  10 /**< ODR 100Hz setting for ADXL313 */
#define ADXL313_DT_ODR_200  11 /**< ODR 200Hz setting for ADXL313 */
#define ADXL313_DT_ODR_400  12 /**< ODR 400Hz setting for ADXL313 */
#define ADXL313_DT_ODR_800  13 /**< ODR 800Hz setting for ADXL313 */
#define ADXL313_DT_ODR_1600 14 /**< ODR 1600Hz setting for ADXL313 */
#define ADXL313_DT_ODR_3200 15 /**< ODR 3200Hz setting for ADXL313 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX313_H_ */
