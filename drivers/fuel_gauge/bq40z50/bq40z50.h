/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUELGAUGE_BQ40Z50_GAUGE_H_
#define ZEPHYR_DRIVERS_FUELGAUGE_BQ40Z50_GAUGE_H_

#include <zephyr/drivers/i2c.h>

// Register Definitions
#define BQ40Z50_MANUFACTURERACCESS      0x00 // R/W
#define BQ40Z50_REMAININGCAPACITYALARM  0X01 // R/W, Unit: mAh/cWh, Range: 0..700
#define BQ40Z50_REMAININGTIMEALARM      0X02 // R/W, Unit: minutes, Range: 0..30
#define BQ40Z50_BATTERYMODE             0x03 // R/W, Unit: ---, Range: 0x0000..0xFFFF
#define BQ40Z50_ATRATE                  0x04 // R/W, Unit: mA, Range: -32768..32767
#define BQ40Z50_ATRATETIMETOFULL        0x05 // R/O, Unit: minutes, Range: 0..65535
#define BQ40Z50_ATRATETIMETOEMPTY       0x06 // R/O, Unit: minutes, Range: 0..65535
#define BQ40Z50_ATRATEOK                0x07 // R/O, Unit: ---, Range: 0..65535
#define BQ40Z50_TEMPERATURE             0x08 // R/O, Unit: 0.1 K, Range: 0..65535
#define BQ40Z50_VOLTAGE                 0x09 // R/O, Unit: mv, Range: 0..65535
#define BQ40Z50_CURRENT                 0x0A // R/O, Unit: mA, Range: -32768..32767
#define BQ40Z50_AVERAGECURRENT          0x0B // R/O, Unit: mA, Range: -32768..32767
#define BQ40Z50_RELATIVESTATEOFCHARGE   0x0D // R/O, Unit: percent, Range: 0..100
#define BQ40Z50_ABSOLUTESTATEOFCHARGE   0x0E // R/O, Unit: percent, Range: 0..100
#define BQ40Z50_REMAININGCAPACITY       0x0F // R/O, Unit: mAh, Range: 0..65535
#define BQ40Z50_FULLCHARGECAPACITY      0x10 // R/O, Unit: mAh, Range: 0..65535
#define BQ40Z50_RUNTIMETOEMPTY          0x11 // R/O, Unit: minutes, Range: 0..65535
#define BQ40Z50_AVERAGETIMETOEMPTY      0x12 // R/O, Unit: minutes, Range: 0..65535
#define BQ40Z50_CHARGINGCURRENT         0x14 // R/O, Unit: mA, Range: 0..65535
#define BQ40Z50_CHARGINGVOLTAGE         0x15 // R/O, Unit: mV, Range: 0..65535
#define BQ40Z50_BATTERYSTATUS           0x16 // R/O, Unit: ---, Range: ---
#define BQ40Z50_CYCLECOUNT              0x17 // R/O, Unit: cycles, Range: 0..65535
#define BQ40Z50_DESIGNCAPACITY          0x18 // R/O, Unit: mAh, Range: 0..65535
#define BQ40Z50_DESIGNVOLTAGE           0x19 // R/O, Unit: mV, Range: 7000..18000
#define BQ40Z50_MANUFACTURERDATE        0x1B // R/O, Unit: ---, Range: 0..65535
#define BQ40Z50_SERIALNUMBER            0x1C // R/O, Unit: ---, Range: 0..65535
#define BQ40Z50_MANUFACTURERNAME        0X20 // R/O, Unit: ASCII, Range: ---
#define BQ40Z50_DEVICENAME              0X21 // R/O, Unit: ASCII, Range: ---
#define BQ40Z50_DEVICECHEMISTRY         0x22 // R/O, Unit: ASCII, Range: ---
#define BQ40Z50_MANUFACTURERDATA        0x23 // R/O, Unit: ---, Range: ---
#define BQ40Z50_AUTHENTICATE            0x2F // R/W, Unit: ---, Range: ---
#define BQ40Z50_MANUFACTURERBLOCKACCESS 0x44 // R/W
#define BQ40Z50_BTPDISCHARGE            0x4A // R/W, Unit: mAh, Range: 150..65535
#define BQ40Z50_BTPCHARGE               0x4B // R/W, Unit: mAh, Range: 175..65535
#define BQ40Z50_PFSTATUS                0x53 // Cannot read in Sealed Mode
#define BQ40Z50_OPERATIONSTATUS         0x54 // Cannot read in Sealed Mode
#define BQ40Z50_CHARGINGSTATUS          0x55 // Cannot read in Sealed Mode
#define BQ40Z50_GAUGINGSSTATUS          0x55 // Cannot read in Sealed Mode
#define BQ40Z50_MAXTURBOPWR             0x59 // R/W, Unit: cW, Range: ---
#define BQ40Z50_SUSTURBOPWR             0x5A // R/W, Unit: cW, Range: ---
#define BQ40Z50_MAXTURBOCURR            0x5E // R/W, Unit: mA, Range: ---
#define BQ40Z50_SUSTURBOCURR            0x5F // R/W, Unit: mA, Range: ---

/* MAC commands */
#define BQ40Z50_MAC_CMD_DEVICE_TYPE  0X0001
#define BQ40Z50_MAC_CMD_FIRMWARE_VER 0X0002
#define BQ40Z50_MAC_CMD_SHUTDOWNMODE 0X0010
#define BQ40Z50_MAC_CMD_SLEEPMODE    0X0011
#define BQ40Z50_MAC_CMD_GAUGING      0X0021

// first byte is the length of the data received from the block access and the next two bytes are
// the cmd.
#define BQ40Z50_MAC_META_DATA_LEN    3
#define BQ40Z50_FIRMWARE_VERSION_LEN 11

#define BQ40Z50_LEN_BYTE      1
#define BQ40Z50_LEN_HALF_WORD 2
#define BQ40Z50_LEN_WORD      4

// Bit 14 of Operational status (0x54) is XCHG (charging disabled)
#define BQ40Z50_OPERATION_STATUS_XCHG_BIT 14

struct bq40z50_config {
	struct i2c_dt_spec i2c;
};

struct bq40z50_data {
	uint8_t major_version;
	uint8_t minor_version;
};

#endif
