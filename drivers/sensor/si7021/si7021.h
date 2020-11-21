/*
 * Copyright (c) 2020 Nikolaus Huber, Uppsala University
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SI7021_H
#define _SI7021_H

/* List of available commands */
#define SI7021_MEAS_RH_HOLD_MASTER          0xE5
#define SI7021_MEAS_RH_NO_HOLD_MASTER       0xF5
#define SI7021_MEAS_TEMP_HOLD_MASTER        0xE3
#define SI7021_MEAS_TEMP_NO_HOLD_MASTER     0xF3
#define SI7021_READ_TEMP_FROM_PREV_RH       0xE0
#define SI7021_RESET                        0xFE
#define SI7021_WRITE_USER_REGISTER          0xE6
#define SI7021_READ_USER_REGISTER           0xE7
#define SI7021_WRITE_HEATER_CNTRL_REG       0x51
#define SI7021_READ_HEATER_CNTRL_REG        0x11
#define SI7021_READ_ID_LOW_0                0xFA
#define SI7021_READ_ID_LOW_1                0x0F
#define SI7021_READ_ID_HIGH_0               0xFC
#define SI7021_READ_ID_HIGH_1               0xC9
#define SI7021_FIRMWARE_0                   0x84
#define SI7021_FIRMWARE_1                   0xB8

/* Buffer sizes */
#define SI7021_READ_NO_CHECKSUM_SIZE        0x02
#define SI7021_READ_WITH_CHECKSUM_SIZE      0x03

/* Checksum */
#define SI7021_CRC_POLY                     0x31
#define SI7021_CRC_SIZE                     sizeof(uint8_t)
#define SI7021_CRC_INIT                     0x0
#define SI7021_DATA_SIZE                    (sizeof(uint8_t) * 2)

#endif /* _SI7021_H */
