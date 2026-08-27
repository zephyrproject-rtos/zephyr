/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SBS_GAUGE_H_
#define ZEPHYR_DRIVERS_SENSOR_SBS_GAUGE_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

/*** Standard Commands ***/
#define SBS_GAUGE_CMD_MANUFACTURER_ACCESS   0x00 /* ManufacturerAccess */
#define SBS_GAUGE_CMD_REM_CAPACITY_ALARM    0x01 /* LowCapacityAlarmThreshold */
#define SBS_GAUGE_CMD_REM_TIME_ALARM        0x02 /* RemainingTimeToEmptyThreshold */
#define SBS_GAUGE_CMD_BATTERY_MODE          0x03 /* BatteryOperatingMode */
#define SBS_GAUGE_CMD_AR                    0x04 /* AtRate */
#define SBS_GAUGE_CMD_ARTTF                 0x05 /* AtRateTimeToFull */
#define SBS_GAUGE_CMD_ARTTE                 0x06 /* AtRateTimeToEmpty */
#define SBS_GAUGE_CMD_AROK                  0x07 /* AtRateOK */
#define SBS_GAUGE_CMD_TEMP                  0x08 /* Temperature */
#define SBS_GAUGE_CMD_VOLTAGE               0x09 /* Voltage */
#define SBS_GAUGE_CMD_CURRENT               0x0A /* Current */
#define SBS_GAUGE_CMD_AVG_CURRENT           0x0B /* AverageCurrent */
#define SBS_GAUGE_CMD_MAX_ERROR             0x0C /* MaxError */
#define SBS_GAUGE_CMD_RSOC                  0x0D /* RelativeStateOfCharge */
#define SBS_GAUGE_CMD_ASOC                  0x0E /* AbsoluteStateOfCharge */
#define SBS_GAUGE_CMD_REM_CAPACITY          0x0F /* RemainingCapacity */
#define SBS_GAUGE_CMD_FULL_CAPACITY         0x10 /* FullChargeCapacity */
#define SBS_GAUGE_CMD_RUNTIME2EMPTY         0x11 /* RunTimeToEmpty */
#define SBS_GAUGE_CMD_AVG_TIME2EMPTY        0x12 /* AverageTimeToEmpty */
#define SBS_GAUGE_CMD_AVG_TIME2FULL         0x13 /* AverageTimeToFull */
#define SBS_GAUGE_CMD_CHG_CURRENT           0x14 /* ChargeCurrent */
#define SBS_GAUGE_CMD_CHG_VOLTAGE           0x15 /* ChargeVoltage */
#define SBS_GAUGE_CMD_FLAGS                 0x16 /* BatteryStatus */
#define SBS_GAUGE_CMD_CYCLE_COUNT           0x17 /* CycleCount */
#define SBS_GAUGE_CMD_NOM_CAPACITY          0x18 /* DesignCapacity */
#define SBS_GAUGE_CMD_DESIGN_VOLTAGE        0x19 /* DesignVoltage */
#define SBS_GAUGE_CMD_SPECS_INFO            0x1A /* SpecificationInfo */
#define SBS_GAUGE_CMD_MANUFACTURER_DATE     0x1B /* ManufacturerDate */
#define SBS_GAUGE_CMD_SN                    0x1C /* SerialNumber */
#define SBS_GAUGE_CMD_MANUFACTURER_NAME     0x20 /* ManufacturerName */
#define SBS_GAUGE_CMD_DEVICE_NAME           0x21 /* DeviceName */
#define SBS_GAUGE_CMD_DEVICE_CHEM           0x22 /* DeviceChemistry */
#define SBS_GAUGE_CMD_MANUFACTURER_DATA     0x23 /* ManufacturerData */
#define SBS_GAUGE_CMD_DESIGN_MAX_POWER      0x24 /* DesignMaxPower */
#define SBS_GAUGE_CMD_START_TIME            0x25 /* StartTime */
#define SBS_GAUGE_CMD_TOTAL_RUNTIME         0x26 /* TotalRuntime */
#define SBS_GAUGE_CMD_FC_TEMP               0x27 /* FCTemp */
#define SBS_GAUGE_CMD_FC_STATUS             0x28 /* FCStatus */
#define SBS_GAUGE_CMD_FC_MODE               0x29 /* FCMode */
#define SBS_GAUGE_CMD_AUTO_SOFT_OFF         0x2A /* AutoSoftOff */
#define SBS_GAUGE_CMD_AUTHENTICATE          0x2F /* Authenticate */
#define SBS_GAUGE_CMD_CELL_V4               0x3C /* CellVoltage4 */
#define SBS_GAUGE_CMD_CELL_V3               0x3D /* CellVoltage3 */
#define SBS_GAUGE_CMD_CELL_V2               0x3E /* CellVoltage2 */
#define SBS_GAUGE_CMD_CELL_V1               0x3F /* CellVoltage1 */

#define SBS_GAUGE_DELAY                     1000

struct sbs_gauge_data {
	uint16_t voltage;
	int16_t avg_current;
	uint16_t state_of_charge;
	uint16_t internal_temperature;
	uint16_t full_charge_capacity;
	uint16_t remaining_charge_capacity;
	uint16_t nom_avail_capacity;
	uint16_t full_avail_capacity;
	uint16_t time_to_empty;
	uint16_t time_to_full;
	uint16_t cycle_count;
};

struct sbs_gauge_config {
	struct i2c_dt_spec i2c;
};

#endif
