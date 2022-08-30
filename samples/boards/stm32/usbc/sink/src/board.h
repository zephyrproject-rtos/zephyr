/*
 * Copyright (c) 2022 The Chromium OS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_H__
#define __BOARD_H__

/**
 * @brief Configure the board
 */
int board_config(void);

/**
 * @brief Control a board specific led
 *
 * @param val LED is off when 0, else on
 */
void board_led(int val);

/**
 * @brief Measure VBUS in a board specific way
 *
 * @param mv pointer where VBUS, in millivolts, is stored
 *
 * @return 0 on success
 * @return -EINVAL on failure
 */
int board_vbus_meas(int *mv);

/**
 * @brief Discharge VBUS in a board specific way
 *
 * @param en VBUS is discharged when true, else it is not
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOTSUP if not supported
 */
int board_discharge_vbus(int en);

#endif /* __BOARD_H__ */
