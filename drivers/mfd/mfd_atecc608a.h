/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MFD_MFD_ATECC608A_H_
#define ZEPHYR_DRIVERS_MFD_MFD_ATECC608A_H_

/* I2C word addresses, ATECC608A datasheet. */
#define ATECC608A_WA_RESET   0x00U
#define ATECC608A_WA_SLEEP   0x01U
#define ATECC608A_WA_IDLE    0x02U
#define ATECC608A_WA_COMMAND 0x03U

/* Opcodes used by in-tree children, ATECC608A datasheet. */
#define ATECC608A_OP_INFO   0x30U
#define ATECC608A_OP_RANDOM 0x1BU

#define ATECC608A_INFO_REVISION 0x00U

#define ATECC608A_STATUS_SUCCESS 0x00U
#define ATECC608A_STATUS_WAKE    0x11U

#define ATECC608A_INFO_LEN     4U
#define ATECC608A_RANDOM_LEN   32U
#define ATECC608A_DEVTYPE_608  0x60U

#endif /* ZEPHYR_DRIVERS_MFD_MFD_ATECC608A_H_ */
