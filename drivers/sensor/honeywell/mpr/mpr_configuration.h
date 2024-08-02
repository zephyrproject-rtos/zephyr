/*
 * Copyright (c) 2020 Sven Herrmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPR_CONFIGURATION_H_
#define ZEPHYR_DRIVERS_SENSOR_MPR_CONFIGURATION_H_

/*
 * Pressure Range
 *
 * MIN is always 0
 */
#define MPR_P_MIN (0)

#if defined(CONFIG_MPR_PRESSURE_RANGE_0001)
#define MPR_P_MAX (1)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_01_6)
#define MPR_P_MAX (1.6)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_02_5)
#define MPR_P_MAX (2.5)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0015)
#define MPR_P_MAX (15)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0025)
#define MPR_P_MAX (25)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0030)
#define MPR_P_MAX (30)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0060)
#define MPR_P_MAX (60)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0100)
#define MPR_P_MAX (100)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0160)
#define MPR_P_MAX (160)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0250)
#define MPR_P_MAX (250)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0400)
#define MPR_P_MAX (400)

#elif defined(CONFIG_MPR_PRESSURE_RANGE_0600)
#define MPR_P_MAX (600)

#else
#error "MPR: Unknown pressure range."
#endif

/*
 * Pressure Unit
 */
#if defined(CONFIG_MPR_PRESSURE_UNIT_P)
/* psi to kPa conversion factor: 6.894757 * 10^6 */
#define MPR_CONVERSION_FACTOR (6894757)

#elif defined(CONFIG_MPR_PRESSURE_UNIT_K)
/* kPa to kPa conversion factor: 1 * 10^6 */
#define MPR_CONVERSION_FACTOR (1000000)

#elif defined(CONFIG_MPR_PRESSURE_UNIT_B)
/* bar to kPa conversion factor: 100 * 10^6 */
#define MPR_CONVERSION_FACTOR (100000000)

#elif defined(CONFIG_MPR_PRESSURE_UNIT_M)
/* mbar to kPa conversion factor: 0.1 * 10^6 */
#define MPR_CONVERSION_FACTOR (100000)

#else
#error "MPR: Unknown pressure unit."
#endif

/*
 * Transfer function
 */
#if defined(CONFIG_MPR_TRANSFER_FUNCTION_A)
#define MPR_OUTPUT_MIN (0x19999A) /* 10% of 2^24 */
#define MPR_OUTPUT_MAX (0xE66666) /* 90% of 2^24 */
#elif defined(CONFIG_MPR_TRANSFER_FUNCTION_B)
#define MPR_OUTPUT_MIN (0x66666) /* 2.5% of 2^24 */
#define MPR_OUTPUT_MAX (0x399999) /* 22.5% of 2^24 */
#elif defined(CONFIG_MPR_TRANSFER_FUNCTION_C)
#define MPR_OUTPUT_MIN (0x333333) /* 20% of 2^24 */
#define MPR_OUTPUT_MAX (0xCCCCCC) /* 80% of 2^24 */
#else
#error "MPR: Unknown pressure reference."
#endif

#define MPR_OUTPUT_RANGE (MPR_OUTPUT_MAX - MPR_OUTPUT_MIN)

#endif /* ZEPHYR_DRIVERS_SENSOR_MPR_CONFIGURATION_H_ */
