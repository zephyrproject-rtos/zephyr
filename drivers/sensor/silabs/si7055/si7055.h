/*
 * Copyright (c) 2020 Laird Connectivity
 * Copyright (c) 2019 Electronut Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SI7055_H
#define _SI7055_H

/* Si7055 register addresses */
#define SI7055_MEAS_TEMP_MASTER_MODE                    0xE3
#define SI7055_MEAS_TEMP_NO_MASTER_MODE                 0xF3
#define SI7055_RESET                                    0xFE
#define SI7055_READ_ID_LOW_0                            0xFA
#define SI7055_READ_ID_LOW_1                            0x0F
#define SI7055_READ_ID_HIGH_0                           0xFC
#define SI7055_READ_ID_HIGH_1                           0xC9
#define SI7055_FIRMWARE_0                               0x84
#define SI7055_FIRMWARE_1                               0xB8
/* Si7055 temperature conversion factors and constants */
#define SI7055_CONV_FACTOR_1                            17572
#define SI7055_CONV_FACTOR_2                            4685
#define SI7055_MULTIPLIER                               10000
#define SI7055_DIVIDER                                  1000000
/* Si7055 buffer sizes and locations */
#define SI7055_TEMPERATURE_READ_NO_CHECKSUM_SIZE        0x02
#define SI7055_TEMPERATURE_READ_WITH_CHECKSUM_SIZE      0x03
#define SI7055_TEMPERATURE_DATA_BYTE_0                  0x0
#define SI7055_TEMPERATURE_DATA_BYTE_1                  0x1
/* Si7055 Checksum constants */
#define SI7055_CRC_POLY                                 0x31
#define SI7055_CRC_SIZE                                 sizeof(uint8_t)
#define SI7055_CRC_INIT                                 0x0
#define SI7055_DATA_SIZE                                (sizeof(uint8_t) * 2)

#endif /* _SI7055_H */
