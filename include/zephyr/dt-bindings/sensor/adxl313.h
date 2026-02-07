/*
 * Copyright (c) 2025 Lothar Rubusch (l.rubusch@gmail.ch)
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
#define ADXL313_DT_ODR_6_25 6
#define ADXL313_DT_ODR_12_5 7
#define ADXL313_DT_ODR_25   8
#define ADXL313_DT_ODR_50   9
#define ADXL313_DT_ODR_100  10
#define ADXL313_DT_ODR_200  11
#define ADXL313_DT_ODR_400  12
#define ADXL313_DT_ODR_800  13
#define ADXL313_DT_ODR_1600 14
#define ADXL313_DT_ODR_3200 15
/** @} */

/**
 * @defgroup ADXL313_RANGE g range options
 * @{
 */
#define ADXL313_DT_RANGE_0_5G 0x00
#define ADXL313_DT_RANGE_1G   0x01
#define ADXL313_DT_RANGE_2G   0x02
#define ADXL313_DT_RANGE_4G   0x03
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX313_H_ */
