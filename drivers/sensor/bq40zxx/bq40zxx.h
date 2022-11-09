/*
 * Copyright(c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ40ZXX_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ40ZXX_H_

#include <logging/log.h>
LOG_MODULE_REGISTER(bq40zxx, CONFIG_SENSOR_LOG_LEVEL);

/*** General Constant ***/
#define BQ40ZXX_UNSEAL_KEY_1 0x0414 /* Secret code to unseal the BQ40Z50 */
#define BQ40ZXX_UNSEAL_KEY_2 0x3672 /* Secret code to unseal the BQ40Z50 */
#define BQ40ZXX_DEVICE_ID    0x4500 /* Default device ID */

/*** Standard Commands ***/
#define BQ40ZXX_COMMAND_MANUFACTURER_ACCESS 0x00 /* ManufacturerAccess() register */
#define BQ40ZXX_COMMAND_REMAINING_CAPACITY_ALARM 0x01 /* RemainingCapacityAlarm() register */
#define BQ40ZXX_COMMAND_REMAINING_TIME_ALARM 0x02 /* RemainingTimeAlarm() */
#define BQ40ZXX_COMMAND_BATTERY_MODE 0x03 /* BatteryMode() */
#define BQ40ZXX_COMMAND_AT_RATE 0x04 /* AtRate() */
#define BQ40ZXX_COMMAND_TEMP 0x08 /* Temperature() */
#define BQ40ZXX_COMMAND_VOLTAGE 0x09 /* Voltage() */
#define BQ40ZXX_COMMAND_AVG_CURRENT 0x0B /* AverageCurrent() */
#define BQ40ZXX_COMMAND_FLAGS 0x16 /* Flags() */
#define BQ40ZXX_COMMAND_TIME_TO_EMPTY 0x12 /* TimeToEmpty() */
#define BQ40ZXX_COMMAND_TIME_TO_FULL 0x13 /* TimeToFull() */
#define BQ40ZXX_COMMAND_FULL_CAPACITY 0x10 /* FullChargeCapacity() */
#define BQ40ZXX_COMMAND_REM_CAPACITY 0x0F /* RemainingCapacity() */
#define BQ40ZXX_COMMAND_CYCLE_COUNT 0x17 /* CycleCount() */
#define BQ40ZXX_COMMAND_SOC 0x0D /* StateOfCharge() */
#define BQ40ZXX_COMMAND_SOH 0x4F /* StateOfHealth() */
#define BQ40ZXX_COMMAND_DESIGN_CAPACITY 0x18 /* DesignCapacity() */

#define BQ40ZXX_COMMAND_SAFETY_ALERT 0x50 /* SafetyAlert() */
#define BQ40ZXX_COMMAND_SAFETY_STATUS 0x51 /* SafetyStatus() */
#define BQ40ZXX_COMMAND_PF_ALERT 0x52 /* PFAlert() */
#define BQ40ZXX_COMMAND_PF_STATUS 0x53 /* PFStatus() */
#define BQ40ZXX_COMMAND_OPERATION_STATUS 0x54 /* OperationStatus() */
#define BQ40ZXX_COMMAND_CHARGING_STATUS 0x55 /* ChargingStatus() */
#define BQ40ZXX_COMMAND_GAUGING_STATUS 0x56 /* GaugingStatus() */
#define BQ40ZXX_COMMAND_MANUFACTURING_STATUS 0x57 /* ManufacturingStatus() */
#define BQ40ZXX_COMMAND_MANUFACTURER_BLOCK_ACCESS 0x44 /* ManufacturerBlockAccess() */

/*** Control Sub-Commands ***/
#define BQ40ZXX_CONTROL_OP_STATUS   0x0000
#define BQ40ZXX_CONTROL_DEVICE_TYPE 0x0001
#define BQ40ZXX_CONTROL_FW_VERSION  0x0002
#define BQ40ZXX_CONTROL_HW_VERSION  0x0003
#define BQ40ZXX_CONTROL_IF_SIG      0x0004
#define BQ40ZXX_CONTROL_CHEM_ID     0x0006
#define BQ40ZXX_CONTROL_GAUGING     0x0021
#define BQ40ZXX_CONTROL_SEALED      0x0030
#define BQ40ZXX_CONTROL_DEV_RESET   0x0041

/*** Block data addresses */
#define BQ40ZXX_EXTENDED_BLOCKDATA_START   0x4000

#define BQ40ZXX_DELAY 1000

struct bq40zxx_data {
	struct device *i2c;
	uint16_t voltage;
	int16_t avg_current;
	uint16_t internal_temperature;
	uint16_t flags;
	uint16_t time_to_empty;
	uint16_t time_to_full;
	uint8_t state_of_charge;
	uint16_t cycle_count;
	uint16_t full_charge_capacity;
	uint16_t remaining_charge_capacity;
	uint16_t state_of_health;
	uint16_t design_capacity;
	uint16_t manufacturer_block_access;
	uint32_t safety_alert;
	uint32_t safety_status;
	uint32_t pf_alert;
	uint32_t pf_status;
	uint32_t op_status;
	uint32_t gauging_status;
	uint32_t ch_status;
	uint32_t mfg_status;
};

struct bq40zxx_config {
	char *bus_name;
	uint16_t design_voltage;
	uint16_t design_capacity;
	uint16_t taper_current;
	uint16_t terminate_voltage;
};

#endif
