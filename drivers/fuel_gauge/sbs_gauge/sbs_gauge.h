/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SBS_GAUGE_H_
#define ZEPHYR_DRIVERS_SENSOR_SBS_GAUGE_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

/** Standard Commands */
enum sbs_gauge_cmds {
	SBS_GAUGE_CMD_MANUFACTURER_ACCESS = 0x00, /* ManufacturerAccess */
	SBS_GAUGE_CMD_REM_CAPACITY_ALARM = 0x01,  /* LowCapacityAlarmThreshold */
	SBS_GAUGE_CMD_REM_TIME_ALARM = 0x02,      /* RemainingTimeToEmptyThreshold */
	SBS_GAUGE_CMD_BATTERY_MODE = 0x03,        /* BatteryOperatingMode */
	SBS_GAUGE_CMD_AR = 0x04,                  /* AtRate */
	SBS_GAUGE_CMD_ARTTF = 0x05,               /* AtRateTimeToFull */
	SBS_GAUGE_CMD_ARTTE = 0x06,               /* AtRateTimeToEmpty */
	SBS_GAUGE_CMD_AROK = 0x07,                /* AtRateOK */
	SBS_GAUGE_CMD_TEMP = 0x08,                /* Temperature */
	SBS_GAUGE_CMD_VOLTAGE = 0x09,             /* Voltage */
	SBS_GAUGE_CMD_CURRENT = 0x0A,             /* Current */
	SBS_GAUGE_CMD_AVG_CURRENT = 0x0B,         /* AverageCurrent */
	SBS_GAUGE_CMD_MAX_ERROR = 0x0C,           /* MaxError */
	SBS_GAUGE_CMD_RSOC = 0x0D,                /* RelativeStateOfCharge */
	SBS_GAUGE_CMD_ASOC = 0x0E,                /* AbsoluteStateOfCharge */
	SBS_GAUGE_CMD_REM_CAPACITY = 0x0F,        /* RemainingCapacity */
	SBS_GAUGE_CMD_FULL_CAPACITY = 0x10,       /* FullChargeCapacity */
	SBS_GAUGE_CMD_RUNTIME2EMPTY = 0x11,       /* RunTimeToEmpty */
	SBS_GAUGE_CMD_AVG_TIME2EMPTY = 0x12,      /* AverageTimeToEmpty */
	SBS_GAUGE_CMD_AVG_TIME2FULL = 0x13,       /* AverageTimeToFull */
	SBS_GAUGE_CMD_CHG_CURRENT = 0x14,         /* ChargeCurrent */
	SBS_GAUGE_CMD_CHG_VOLTAGE = 0x15,         /* ChargeVoltage */
	SBS_GAUGE_CMD_FLAGS = 0x16,               /* BatteryStatus */
	SBS_GAUGE_CMD_CYCLE_COUNT = 0x17,         /* CycleCount */
	SBS_GAUGE_CMD_NOM_CAPACITY = 0x18,        /* DesignCapacity */
	SBS_GAUGE_CMD_DESIGN_VOLTAGE = 0x19,      /* DesignVoltage */
	SBS_GAUGE_CMD_SPECS_INFO = 0x1A,          /* SpecificationInfo */
	SBS_GAUGE_CMD_MANUFACTURER_DATE = 0x1B,   /* ManufacturerDate */
	SBS_GAUGE_CMD_SN = 0x1C,                  /* SerialNumber */
	SBS_GAUGE_CMD_MANUFACTURER_NAME = 0x20,   /* ManufacturerName */
	SBS_GAUGE_CMD_DEVICE_NAME = 0x21,         /* DeviceName */
	SBS_GAUGE_CMD_DEVICE_CHEMISTRY = 0x22,    /* DeviceChemistry */
	SBS_GAUGE_CMD_MANUFACTURER_DATA = 0x23,   /* ManufacturerData */
};

/*
 * Nearly all cutoff payloads are actually a singular value that must be written twice to the fuel
 * gauge. For the case where it's a singular value that must only be written to the fuel gauge only
 * once, retransmitting the duplicate write has no significant negative consequences.
 *
 * Why not devicetree: Finding the maximum length of all the battery cutoff payloads in a devicetree
 * at compile-time would require labyrinthine amount of macro-batics.
 *
 * Why not compute at runtime: It's not worth the memory given having more than a single fuel gauge
 * is rare, and most will have a payload size of 2.
 *
 * This is validated as a BUILD_ASSERT in the driver.
 */
#define SBS_GAUGE_CUTOFF_PAYLOAD_MAX_SIZE 2

#endif
