/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SMART_BATTERY_H_
#define ZEPHYR_DRIVERS_SENSOR_SMART_BATTERY_H_

#include <stdint.h>

/*** Standard Commands ***/
#define SMART_BATTERY_COMMAND_MANUFACTURER_ACCESS 0x00 /* ManufacturerAccess() */
#define SMART_BATTERY_COMMAND_REM_CAPACITY_ALARM 0x01 /* LowCapacityAlarmThreshold() */
#define SMART_BATTERY_COMMAND_REM_TIME_ALARM 0x02 /* RemainingTimeToEmptyThreshold() */
#define SMART_BATTERY_COMMAND_BATTERY_MODE 0x03 /* BatteryOperatingMode() */
#define SMART_BATTERY_COMMAND_AR 0x04 /* AtRate() */
#define SMART_BATTERY_COMMAND_ARTTF 0x05 /* AtRateTimeToFull() */
#define SMART_BATTERY_COMMAND_ARTTE 0x06 /* AtRateTimeToEmpty() */
#define SMART_BATTERY_COMMAND_AROK 0x07 /* AtRateOK()*/
#define SMART_BATTERY_COMMAND_TEMP 0x08 /* Temperature() */
#define SMART_BATTERY_COMMAND_VOLTAGE 0x09 /* Voltage() */
#define SMART_BATTERY_COMMAND_CURRENT 0x0A /* Current() */
#define SMART_BATTERY_COMMAND_AVG_CURRENT 0x0B /* AverageCurrent() */
#define SMART_BATTERY_COMMAND_MAX_ERROR 0x0C /* MaxError() */
#define SMART_BATTERY_COMMAND_RSOC 0x0D /* RelativeStateOfCharge() */
#define SMART_BATTERY_COMMAND_ASOC 0x0E /* AbsoluteStateOfCharge() */
#define SMART_BATTERY_COMMAND_REM_CAPACITY 0x0F /* RemainingCapacity() */
#define SMART_BATTERY_COMMAND_FULL_CAPACITY 0x10 /* FullChargeCapacity() */
#define SMART_BATTERY_COMMAND_RUNTIME2EMPTY 0x11 /* RunTimeToEmpty() */
#define SMART_BATTERY_COMMAND_AVG_TIME2EMPTY 0x12 /* AverageTimeToEmpty() */
#define SMART_BATTERY_COMMAND_AVG_TIME2FULL 0x13 /* AverageTimeToFull() */
#define SMART_BATTERY_COMMAND_CHG_CURRENT 0x14 /* ChargeCurrent() */
#define SMART_BATTERY_COMMAND_CHG_VOLTAGE 0x15 /* ChargeVoltage() */
#define SMART_BATTERY_COMMAND_FLAGS 0x16 /* BatteryStatus() */
#define SMART_BATTERY_COMMAND_CYCLE_COUNT 0x17 /* CycleCount() */
#define SMART_BATTERY_COMMAND_NOM_CAPACITY 0x18 /* DesignCapacity() */
#define SMART_BATTERY_COMMAND_DESIGN_VOLTAGE 0x19 /* DesignVoltage() */
#define SMART_BATTERY_COMMAND_SPECS_INFO 0x1A /* SpecificationInfo() */
#define SMART_BATTERY_COMMAND_MANUFACTURER_DATE 0x1B /* ManufacturerDate() */
#define SMART_BATTERY_COMMAND_SN 0x1C /* SerialNumber() */
#define SMART_BATTERY_COMMAND_MANUFACTURER_NAME 0x20 /* ManufacturerName() */
#define SMART_BATTERY_COMMAND_DEVICE_NAME 0x21 /* DeviceName() */
#define SMART_BATTERY_COMMAND_DEVICE_CHEM 0x22 /* DeviceChemistry() */
#define SMART_BATTERY_COMMAND_MANUFACTURER_DATA 0x23 /* ManufacturerData() */
#define SMART_BATTERY_COMMAND_DESIGN_MAX_POWER 0x24 /* DesignMaxPower() */
#define SMART_BATTERY_COMMAND_START_TIME 0x25 /* StartTime() */
#define SMART_BATTERY_COMMAND_TOTAL_RUNTIME 0x26 /* TotalRuntime() */
#define SMART_BATTERY_COMMAND_FC_TEMP 0x27 /* FCTemp() */
#define SMART_BATTERY_COMMAND_FC_STATUS 0x28 /* FCStatus() */
#define SMART_BATTERY_COMMAND_FC_MODE 0x29 /* FCMode() */
#define SMART_BATTERY_COMMAND_AUTO_SOFT_OFF 0x2A /* AutoSoftOff() */
#define SMART_BATTERY_COMMAND_AUTHENTICATE 0x2F /* Authenticate() */
#define SMART_BATTERY_COMMAND_CELL_V4 0x3C /* CellVoltage4() */
#define SMART_BATTERY_COMMAND_CELL_V3 0x3D /* CellVoltage3() */
#define SMART_BATTERY_COMMAND_CELL_V2 0x3E /* CellVoltage2() */
#define SMART_BATTERY_COMMAND_CELL_V1 0x3F /* CellVoltage1() */

#define SMART_BATTERY_DELAY 1000

#define BYTE_SHIFT 8

struct smartbattery_data {
	const struct device *i2c;
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

struct smartbattery_config {
	const struct device *i2c_dev;
	uint8_t i2c_addr;
};

#endif
