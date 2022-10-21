/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEAS_H__
#define __MEAS_H__

/**
 * @brief Initializes the measurement module, sets up all the adc channels through the device tree binding
 *
 * @return 0 on success
 */
int meas_init(void);

/**
 * @brief Measure the voltage on VBUS
 *
 * @param v pointer where VBUS voltage, in millivolts, is stored
 *
 * @return 0 on success
 */
int meas_vbus_v(int32_t *v);

/**
 * @brief Measure the current on VBUS
 *
 * @param c pointer where VBUS current, in milliamperes, is stored
 *
 * @return 0 on success
 */
int meas_vbus_c(int32_t *c);

/**
 * @brief Measure the voltage on CC1
 *
 * @param v pointer where CC1 voltage, in millivolts, is stored
 *
 * @return 0 on success
 */
int meas_cc1_v(int32_t *v);

/**
 * @brief Measure the voltage on CC2
 *
 * @param v pointer where CC2 voltage, in millivolts, is stored
 *
 * @return 0 on success
 */
int meas_cc2_v(int32_t *v);

/**
 * @brief Measure the current on VCON
 *
 * @param c pointer where VCON current, in milliamperes, is stored
 *
 * @return 0 on success
 */
int meas_vcon_c(int32_t *c);

#endif
