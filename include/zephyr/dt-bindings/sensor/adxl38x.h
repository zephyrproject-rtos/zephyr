/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common devicetree bindings for the Analog Devices ADXL380/ADXL382
 *        accelerometers.
 *
 * The ADXL380 and ADXL382 share one register map and bitfield layout; only the
 * measurement range and sensitivity differ. The per-part range macros live in
 * adxl380.h / adxl382.h, which include this file for the shared options below.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL38X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL38X_H_

/**
 * @defgroup adxl38x ADXL380/ADXL382 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup adxl38x_op_mode Operating mode options
 *
 * Values map directly to OP_MODE (0x26) bits [3:0]. HP/RBW/LP use the 16-bit
 * DSM signal path; HS/ULP/VLP use the 12-bit SAR path.
 * @{
 */
/** Standby mode (no measurement) */
#define ADXL38X_DT_OP_STANDBY 0
/** Heart Sound mode (single-axis SAR) */
#define ADXL38X_DT_OP_HS      1
/** Ultra Low Power mode (SAR, 50 Hz) */
#define ADXL38X_DT_OP_ULP     2
/** Very Low Power mode (SAR, 500 Hz) */
#define ADXL38X_DT_OP_VLP     3
/** Low Power mode (DSM) */
#define ADXL38X_DT_OP_LP      4
/** Low Power + ULP on serial port */
#define ADXL38X_DT_OP_LP_ULP  6
/** Low Power + VLP on serial port */
#define ADXL38X_DT_OP_LP_VLP  7
/** Reduced Bandwidth mode (DSM) */
#define ADXL38X_DT_OP_RBW     8
/** RBW + ULP on serial port */
#define ADXL38X_DT_OP_RBW_ULP 10
/** RBW + VLP on serial port */
#define ADXL38X_DT_OP_RBW_VLP 11
/** High Performance mode (DSM) */
#define ADXL38X_DT_OP_HP      12
/** HP + ULP on serial port */
#define ADXL38X_DT_OP_HP_ULP  14
/** HP + VLP on serial port */
#define ADXL38X_DT_OP_HP_VLP  15
/** @} */

/**
 * @defgroup adxl38x_hpf_corner High Pass Filter corner options
 *
 * Values map directly to FILTER (0x50) bits [2:0]. Corner is expressed as a
 * fraction of the resulting ODR.
 * @{
 */
/** High-pass filter disabled */
#define ADXL38X_DT_HPF_OFF       0
/** High-pass filter corner at 24.7e-4 times ODR */
#define ADXL38X_DT_HPF_24_7e_4   1
/** High-pass filter corner at 6.2084e-4 times ODR */
#define ADXL38X_DT_HPF_6_2084e_4 2
/** High-pass filter corner at 1.5545e-4 times ODR */
#define ADXL38X_DT_HPF_1_5545e_4 3
/** High-pass filter corner at 0.3862e-4 times ODR */
#define ADXL38X_DT_HPF_0_3862e_4 4
/** High-pass filter corner at 0.0954e-4 times ODR */
#define ADXL38X_DT_HPF_0_0954e_4 5
/** High-pass filter corner at 0.0238e-4 times ODR */
#define ADXL38X_DT_HPF_0_0238e_4 6
/** @} */

/**
 * @defgroup adxl38x_int_pol Interrupt polarity options
 *
 * Maps to the INTx pad register (0x5D/0x5E) bit 7.
 * @{
 */
/** Interrupt pin active high (device default) */
#define ADXL38X_DT_INT_POL_ACTIVE_HIGH 0
/** Interrupt pin active low */
#define ADXL38X_DT_INT_POL_ACTIVE_LOW  1
/** @} */

/**
 * @defgroup adxl38x_double_speed Double-speed (ODR doubling) options
 *
 * Maps to DIG_EN (0x4B) bit 2. ADXL380 only: runs the digital path from a
 * 1024 kHz system clock instead of the 512 kHz default, doubling the ODR. The
 * ADXL382 is already clocked at 1024 kHz internally, so this option has no
 * effect there.
 * @{
 */
/** Double-speed disabled: 512 kHz system clock (device default) */
#define ADXL38X_DT_DOUBLE_SPEED_DISABLED 0
/** Double-speed enabled: 1024 kHz system clock, ODR doubled */
#define ADXL38X_DT_DOUBLE_SPEED_ENABLED  1
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL38X_H_ */
