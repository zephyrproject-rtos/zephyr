/*
 * Copyright(c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ274XX_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ274XX_H_

#include <logging/log.h>
#include <drivers/gpio.h>
LOG_MODULE_REGISTER(bq274xx, CONFIG_SENSOR_LOG_LEVEL);

/*** General Constant ***/
#define BQ274XX_UNSEAL_KEY 0x8000       /* Secret code to unseal the BQ274XX */
#define BQ274XX_BLOCKDATA_SIZE 32       /* Number of bytes in a single block of data memory */

/*** Standard Commands ***/
#define BQ274XX_COMMAND_CONTROL 0x00            /* Control() */
#define BQ274XX_COMMAND_TEMP 0x02               /* Temperature() */
#define BQ274XX_COMMAND_VOLTAGE 0x04            /* Voltage() */
#define BQ274XX_COMMAND_FLAGS 0x06              /* Flags() */
#define BQ274XX_COMMAND_NOM_CAPACITY 0x08       /* NominalAvailableCapacity() */
#define BQ274XX_COMMAND_AVAIL_CAPACITY 0x0A     /* FullAvailableCapacity() */
#define BQ274XX_COMMAND_REM_CAPACITY 0x0C       /* RemainingCapacity() */
#define BQ274XX_COMMAND_FULL_CAPACITY 0x0E      /* FullChargeCapacity() */
#define BQ274XX_COMMAND_AVG_CURRENT 0x10        /* AverageCurrent() */
#define BQ274XX_COMMAND_STDBY_CURRENT 0x12      /* StandbyCurrent() */
#define BQ274XX_COMMAND_MAX_CURRENT 0x14        /* MaxLoadCurrent() */
#define BQ274XX_COMMAND_AVG_POWER 0x18          /* AveragePower() */
#define BQ274XX_COMMAND_SOC 0x1C                /* StateOfCharge() */
#define BQ274XX_COMMAND_INT_TEMP 0x1E           /* InternalTemperature() */
#define BQ274XX_COMMAND_SOH 0x20                /* StateOfHealth() */
#define BQ274XX_COMMAND_REM_CAP_UNFL 0x28       /* RemainingCapacityUnfiltered() */
#define BQ274XX_COMMAND_REM_CAP_FIL 0x2A        /* RemainingCapacityFiltered() */
#define BQ274XX_COMMAND_FULL_CAP_UNFL 0x2C      /* FullChargeCapacityUnfiltered() */
#define BQ274XX_COMMAND_FULL_CAP_FIL 0x2E       /* FullChargeCapacityFiltered() */
#define BQ274XX_COMMAND_SOC_UNFL 0x30           /* StateOfChargeUnfiltered() */

/*** Control Sub-Commands ***/
#define BQ274XX_CONTROL_CONTROL_STATUS 0x0000
#define BQ274XX_CONTROL_DEVICE_TYPE 0x0001
#define BQ274XX_CONTROL_FW_VERSION 0x0002
#define BQ274XX_CONTROL_DM_CODE 0x0004
#define BQ274XX_CONTROL_PREV_MACWRITE 0x0007
#define BQ274XX_CONTROL_CHEM_ID 0x0008
#define BQ274XX_CONTROL_BAT_INSERT 0x000C
#define BQ274XX_CONTROL_BAT_REMOVE 0x000D
#define BQ274XX_CONTROL_SET_HIBERNATE 0x0011
#define BQ274XX_CONTROL_CLEAR_HIBERNATE 0x0012
#define BQ274XX_CONTROL_SET_CFGUPDATE 0x0013
#define BQ274XX_CONTROL_SHUTDOWN_ENABLE 0x001B
#define BQ274XX_CONTROL_SHUTDOWN 0x001C
#define BQ274XX_CONTROL_SEALED 0x0020
#define BQ274XX_CONTROL_PULSE_SOC_INT 0x0023
#define BQ274XX_CONTROL_CHEM_A 0x0030
#define BQ274XX_CONTROL_CHEM_B 0x0031
#define BQ274XX_CONTROL_CHEM_C 0x0032
#define BQ274XX_CONTROL_RESET 0x0041
#define BQ274XX_CONTROL_SOFT_RESET 0x0042
#define BQ274XX_CONTROL_EXIT_CFGUPDATE 0x0043
#define BQ274XX_CONTROL_EXIT_RESIM 0x0044

/*** Control().CONTROL_STATUS bit definitions ***/
#define BQ274XX_CONTROL_STATUS_SHUTDOWNEN 15
#define BQ274XX_CONTROL_STATUS_WDRESET 14
#define BQ274XX_CONTROL_STATUS_SS 13
#define BQ274XX_CONTROL_STATUS_CALMODE 12
#define BQ274XX_CONTROL_STATUS_CCA 11
#define BQ274XX_CONTROL_STATUS_BCA 10
#define BQ274XX_CONTROL_STATUS_QMAX_UP 9
#define BQ274XX_CONTROL_STATUS_RES_UP 8
#define BQ274XX_CONTROL_STATUS_INITCOMP 7
#define BQ274XX_CONTROL_STATUS_HIBERNATE 6
#define BQ274XX_CONTROL_STATUS_SLEEP 4
#define BQ274XX_CONTROL_STATUS_LDMD 3
#define BQ274XX_CONTROL_STATUS_RUP_DIS 2
#define BQ274XX_CONTROL_STATUS_VOK 1
#define BQ274XX_CONTROL_STATUS_CHEM_CHANGE 0

/*** Flags() bit definitions ***/
#define BQ274XX_FLAGS_OT 15
#define BQ274XX_FLAGS_UT 14
#define BQ274XX_FLAGS_EEFAIL 10
#define BQ274XX_FLAGS_FC 9
#define BQ274XX_FLAGS_CHG 8
#define BQ274XX_FLAGS_OCVTAKEN 7
#define BQ274XX_FLAGS_DODCORRECT 6
#define BQ274XX_FLAGS_ITPOR 5
#define BQ274XX_FLAGS_CFGUPMODE 4
#define BQ274XX_FLAGS_BAT_DET 3
#define BQ274XX_FLAGS_SOC1 2
#define BQ274XX_FLAGS_SOCF 1
#define BQ274XX_FLAGS_DSG 0

/*** Extended Data Commands ***/
#define BQ274XX_EXTENDED_OPCONFIG 0x3A          /* OpConfig() */
#define BQ274XX_EXTENDED_CAPACITY 0x3C          /* DesignCapacity() */
#define BQ274XX_EXTENDED_DATA_CLASS 0x3E        /* DataClass() */
#define BQ274XX_EXTENDED_DATA_BLOCK 0x3F        /* DataBlock() */
#define BQ274XX_EXTENDED_BLOCKDATA_START 0x40   /* BlockData_start() */
#define BQ274XX_EXTENDED_BLOCKDATA_END 0x5F     /* BlockData_end() */
#define BQ274XX_EXTENDED_CHECKSUM 0x60          /* BlockDataCheckSum() */
#define BQ274XX_EXTENDED_DATA_CONTROL 0x61      /* BlockDataControl() */

/*** Extended Data Subclasses ***/
#define BQ274XX_SUBCLASS_INVALID 0
#define BQ274XX_SUBCLASS_SAFETY 2
#define BQ274XX_SUBCLASS_CHARGE_TERMINATION 36
#define BQ274XX_SUBCLASS_DISCHARGE 49
#define BQ274XX_SUBCLASS_REGISTERS 64
#define BQ274XX_SUBCLASS_IT_CFG 80
#define BQ274XX_SUBCLASS_CURRENT_THRESHOLDS 81
#define BQ274XX_SUBCLASS_STATE 82
#define BQ274XX_SUBCLASS_CHEM_DATA 109

struct bq274xx_blockdata_address {
	uint8_t subclass;
	uint8_t offset;
};

struct bq274xx_blockdata_addresses {
	struct bq274xx_blockdata_address design_cap;
	struct bq274xx_blockdata_address design_enr;
	struct bq274xx_blockdata_address terminate_voltage;
	struct bq274xx_blockdata_address taper_rate;
	struct bq274xx_blockdata_address taper_current;
};

#define MAX_BLOCKDATA_ENTRIES                                                                      \
	sizeof(struct bq274xx_blockdata_addresses) / sizeof(struct bq274xx_blockdata_address)

enum bq274xx_part {
	BQ27411,
	BQ27421,
	BQ27425,
	BQ27426,
	BQ27441,
};

enum bq274xx_chemistry {
	CHEM_A,
	CHEM_B,
	CHEM_C,
	CHEM_DEFAULT,
};

struct bq274xx_data {
	const struct device *i2c;
#ifdef CONFIG_BQ274XX_LAZY_CONFIGURE
	bool lazy_loaded;
#endif
	uint16_t voltage;
	int16_t avg_current;
	int16_t stdby_current;
	int16_t max_load_current;
	int16_t avg_power;
	uint16_t state_of_charge;
	int16_t state_of_health;
	uint16_t internal_temperature;
	uint16_t full_charge_capacity;
	uint16_t remaining_charge_capacity;
	uint16_t nom_avail_capacity;
	uint16_t full_avail_capacity;
};

struct bq274xx_config {
	char *bus_name;
	uint16_t reg_addr;
	enum bq274xx_part part;
	const struct bq274xx_blockdata_addresses *blockdata_addresses;
	uint16_t design_voltage;
	uint16_t design_capacity;
	uint16_t taper_current;
	uint16_t terminate_voltage;
#ifdef CONFIG_PM_DEVICE
	struct gpio_dt_spec int_gpios;
#endif
	enum bq274xx_chemistry chemistry;
};

#endif
