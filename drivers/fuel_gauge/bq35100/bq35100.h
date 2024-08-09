/*
 * Copyright (c) 2024 Orgatex GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EOX_SDK_DRIVERS_FUEL_GAUGE_BQ35100_H_
#define EOX_SDK_DRIVERS_FUEL_GAUGE_BQ35100_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>

// Register addresses
#define BQ35100_REG_CONTROL_STATUS              0x00
#define BQ35100_REG_ACCUMULATED_CAPACITY        0x02
#define BQ35100_REG_TEMPERATURE                 0x06
#define BQ35100_REG_VOLTAGE                     0x08
#define BQ35100_REG_BATTERY_STATUS              0x0A
#define BQ35100_REG_BATTERY_ALERT               0x0B
#define BQ35100_REG_CURRENT                     0x0C
#define BQ35100_REG_SCALED_R                    0x16
#define BQ35100_REG_MEASURED_Z                  0x22
#define BQ35100_REG_INTERNAL_TEMPERATURE        0x28
#define BQ35100_REG_STATE_OF_HEALTH             0x2E
#define BQ35100_REG_DESIGN_CAPACITY             0x3C
#define BQ35100_REG_MANUFACTURER_ACCESS_CONTROL 0x3E
#define BQ35100_REG_MAC_DATA                    0x40
#define BQ35100_REG_MAC_DATA_SUM                0x60
#define BQ35100_REG_MAC_DATA_LEN                0x61
#define BQ35100_REG_CAL_COUNT                   0x79
#define BQ35100_REG_CAL_CURRENT                 0x7A
#define BQ35100_REG_CAL_VOLTAGE                 0x7C
#define BQ35100_REG_CAL_TEMPERATURE             0x7E

/* MAC commands */
#define BQ35100_MAC_CMD_CONTROL_STATUS     0x0000
#define BQ35100_MAC_CMD_DEVICETYPE         0x0001
#define BQ35100_MAC_CMD_FIRMWAREVERSION    0x0002
#define BQ35100_MAC_CMD_HARDWAREVERSION    0x0003
#define BQ35100_MAC_CMD_STATIC_CHEM_CHKSUM 0x0005
#define BQ35100_MAC_CMD_CHEMID             0x0006
#define BQ35100_MAC_CMD_PREV_MACWRITE      0x0007
#define BQ35100_MAC_CMD_BOARD_OFFSET       0x0009
#define BQ35100_MAC_CMD_CC_OFFSET          0x000A
#define BQ35100_MAC_CMD_CC_OFFSET_SAVE     0x000B
#define BQ35100_MAC_CMD_GAUGE_START        0x0011
#define BQ35100_MAC_CMD_GAUGE_STOP         0x0012
#define BQ35100_MAC_CMD_SEALED             0x0020
#define BQ35100_MAC_CMD_CAL_ENABLE         0x002D
#define BQ35100_MAC_CMD_LT_ENABLE          0x002E
#define BQ35100_MAC_CMD_RESET              0x0041
#define BQ35100_MAC_CMD_EXIT_CAL           0x0080
#define BQ35100_MAC_CMD_ENTER_CAL          0x0081
#define BQ35100_MAC_CMD_NEW_BATTERY        0xA613

struct bq35100_config {
	struct i2c_dt_spec i2c;
};

#endif // EOX_SDK_DRIVERS_FUEL_GAUGE_BQ35100_H_
