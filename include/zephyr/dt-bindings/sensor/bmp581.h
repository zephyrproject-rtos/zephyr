/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_BOSCH_BMP581_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_BOSCH_BMP581_H_

/**
 * @defgroup bmp581 Bosch BMP581 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup BMP581_POWER_MODES Sensor power modes
 * @{
 */
#define BMP581_DT_MODE_NORMAL		1
#define BMP581_DT_MODE_FORCED		2
#define BMP581_DT_MODE_CONTINUOUS	3
/** @} */

/**
 * @defgroup BMP581_OUTPUT_DATA_RATE Output data rate options
 * @{
 */
#define BMP581_DT_ODR_240_HZ		0x00
#define BMP581_DT_ODR_218_5_HZ		0x01
#define BMP581_DT_ODR_199_1_HZ		0x02
#define BMP581_DT_ODR_179_2_HZ		0x03
#define BMP581_DT_ODR_160_HZ		0x04
#define BMP581_DT_ODR_149_3_HZ		0x05
#define BMP581_DT_ODR_140_HZ		0x06
#define BMP581_DT_ODR_129_8_HZ		0x07
#define BMP581_DT_ODR_120_HZ		0x08
#define BMP581_DT_ODR_110_1_HZ		0x09
#define BMP581_DT_ODR_100_2_HZ		0x0A
#define BMP581_DT_ODR_89_6_HZ		0x0B
#define BMP581_DT_ODR_80_HZ		0x0C
#define BMP581_DT_ODR_70_HZ		0x0D
#define BMP581_DT_ODR_60_HZ		0x0E
#define BMP581_DT_ODR_50_HZ		0x0F
#define BMP581_DT_ODR_45_HZ		0x10
#define BMP581_DT_ODR_40_HZ		0x11
#define BMP581_DT_ODR_35_HZ		0x12
#define BMP581_DT_ODR_30_HZ		0x13
#define BMP581_DT_ODR_25_HZ		0x14
#define BMP581_DT_ODR_20_HZ		0x15
#define BMP581_DT_ODR_15_HZ		0x16
#define BMP581_DT_ODR_10_HZ		0x17
#define BMP581_DT_ODR_5_HZ		0x18
#define BMP581_DT_ODR_4_HZ		0x19
#define BMP581_DT_ODR_3_HZ		0x1A
#define BMP581_DT_ODR_2_HZ		0x1B
#define BMP581_DT_ODR_1_HZ		0x1C
#define BMP581_DT_ODR_0_5_HZ		0x1D
#define BMP581_DT_ODR_0_250_HZ		0x1E
#define BMP581_DT_ODR_0_125_HZ		0x1F
/** @} */

/**
 * @defgroup BMP581_OVERSAMPLING Oversampling options. Valid for temperature and pressure.
 * @{
 */
#define BMP581_DT_OVERSAMPLING_1X	0x00
#define BMP581_DT_OVERSAMPLING_2X	0x01
#define BMP581_DT_OVERSAMPLING_4X	0x02
#define BMP581_DT_OVERSAMPLING_8X	0x03
#define BMP581_DT_OVERSAMPLING_16X	0x04
#define BMP581_DT_OVERSAMPLING_32X	0x05
#define BMP581_DT_OVERSAMPLING_64X	0x06
#define BMP581_DT_OVERSAMPLING_128X	0x07
/** @} */

/**
 * @defgroup BMP581_IIR_FILTER IIR Filter options. Valid for temperature and pressure.
 * @{
 */
#define BMP581_DT_IIR_FILTER_BYPASS	0x00
#define BMP581_DT_IIR_FILTER_COEFF_1	0x01
#define BMP581_DT_IIR_FILTER_COEFF_3	0x02
#define BMP581_DT_IIR_FILTER_COEFF_7	0x03
#define BMP581_DT_IIR_FILTER_COEFF_15	0x04
#define BMP581_DT_IIR_FILTER_COEFF_31	0x05
#define BMP581_DT_IIR_FILTER_COEFF_63	0x06
#define BMP581_DT_IIR_FILTER_COEFF_127	0x07
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_BOSCH_BMP581_H_*/
