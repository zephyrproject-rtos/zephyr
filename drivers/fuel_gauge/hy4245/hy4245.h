/*
 * Copyright (c) 2025, Linumiz GmbH
 * Copyright (c) 2026, Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUEL_GAUGE_HY4245_HY4245_H_
#define ZEPHYR_DRIVERS_FUEL_GAUGE_HY4245_HY4245_H_

#define HY4245_CHIPID 0x4245

/* Standard commands */
#define HY4245_CMD_CTRL                   0x00
#define HY4245_CMD_TEMPERATURE            0x06
#define HY4245_CMD_VOLTAGE                0x08
#define HY4245_CMD_FLAGS                  0x0a
#define HY4245_CMD_CURRENT                0x0c
#define HY4245_CMD_CAPACITY_REM           0x10
#define HY4245_CMD_CAPACITY_FULL          0x12
#define HY4245_CMD_AVG_CURRENT            0x14
#define HY4245_CMD_TIME_TO_EMPTY          0x16
#define HY4245_CMD_TIME_TO_FULL           0x18
#define HY4245_CMD_RELATIVE_STATE_OF_CHRG 0x2c
#define HY4245_CMD_STATE_OF_HEALTH        0x2e
#define HY4245_CMD_CHRG_VOLTAGE           0x30
#define HY4245_CMD_CHRG_CURRENT           0x32
#define HY4245_CMD_CAPACITY_FULL_AVAIL    0x78

/* Control() subcommands */
#define HY4245_SUBCMD_CTRL_STATUS             0x0000
#define HY4245_SUBCMD_CTRL_DF_CHECKSUM        0x0004
#define HY4245_SUBCMD_CTRL_DF_VERSION         0x000c
#define HY4245_SUBCMD_CTRL_SET_UPD_EN         0x001f
#define HY4245_SUBCMD_CTRL_CHIPID             0x0055
#define HY4245_SUBCMD_CTRL_FW_VERSION         0x0082
#define HY4245_SUBCMD_CTRL_OPERATION_CFG_A    0x0098
#define HY4245_SUBCMD_CTRL_SAFETY_STATUS      0x009d
#define HY4245_SUBCMD_CTRL_LIFETIME_OVER_TEMP 0x00d8

#endif /* ZEPHYR_DRIVERS_FUEL_GAUGE_HY4245_HY4245_H_ */
