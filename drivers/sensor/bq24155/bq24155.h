/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2021  Filip Primozic <filip.primozic@greyp.com>
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ24155_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ24155_H_

#include <stdint.h>
#include <kernel.h>

#define BQ24155_DEFAULT_VENDER_CODE (0x02)

/* timeout, in seconds, for resetting chip timer */
#define BQ24155_TIMER_RESET_RATE (10)

/* used for assert; size in bits */
#define BQ24155_REGISTER_SIZE (8)

/* bq24155 registers*/
#define BQ24155_STATUS_REGISTER (0x00)
#define BQ24155_CONTROL_REGISTER (0x01)
#define BQ24155_VOLTAGE_REGISTER (0x02)
#define BQ24155_PART_REVISION_REGISTER (0x03)
#define BQ24155_CURRENT_REGISTER (0x04)

/* reset state for all registers */
#define BQ24155_RESET_STATUS BIT(6)
#define BQ24155_RESET_CONTROL (BIT(4) | BIT(5))
#define BQ24155_RESET_VOLTAGE (BIT(1) | BIT(3))
#define BQ24155_RESET_CURRENT (BIT(0) | BIT(3) | BIT(7))

/* status register */
#define BQ24155_BIT_TMR_RST (7)
#define BQ24155_BIT_ISEL (7)
#define BQ24155_BIT_EN_STAT (6)
#define BQ24155_MASK_STAT (BIT(4) | BIT(5))
#define BQ24155_SHIFT_STAT (4)
/* N/A					BIT(3) */
#define BQ24155_MASK_FAULT (BIT(0) | BIT(1) | BIT(2))
#define BQ24155_SHIFT_FAULT (0)

/* control register */
#define BQ24155_MASK_LIMIT (BIT(6) | BIT(7))
#define BQ24155_SHIFT_LIMIT (6)
#define BQ24155_MASK_VLOWV (BIT(4) | BIT(5))
#define BQ24155_SHIFT_VLOWV (4)
#define BQ24155_BIT_TE (3)
#define BQ24155_BIT_CE (2)
#define BQ24155_BIT_HZ_MODE (1)
#define BQ24155_BIT_OPA_MODE (0)

/* voltage register */
#define BQ24155_MASK_VO (BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define BQ24155_SHIFT_VO (2)
/* N/A					BIT(1) */
/* N/A					BIT(0) */

/* vender/part/revision register */
#define BQ24155_MASK_VENDER (BIT(5) | BIT(6) | BIT(7))
#define BQ24155_SHIFT_VENDER (5)
#define BQ24155_MASK_PN (BIT(3) | BIT(4))
#define BQ24155_SHIFT_PN (3)
#define BQ24155_MASK_REVISION (BIT(0) | BIT(1) | BIT(2))
#define BQ24155_SHIFT_REVISION (0)

/* current register */
#define BQ24155_MASK_RESET BIT(7)
#define BQ24155_MASK_VI_CHRG (BIT(4) | BIT(5) | BIT(6))
#define BQ24155_SHIFT_VI_CHRG (4)
/* N/A					BIT(3) */
#define BQ24155_MASK_VI_TERM (BIT(0) | BIT(1) | BIT(2))
#define BQ24155_SHIFT_VI_TERM (0)

enum bq24155_command {
	BQ24155_TIMER_RESET,
	BQ24155_ISEL_STATUS,
	BQ24155_STAT_PIN_STATUS,
	BQ24155_STAT_PIN_ENABLE,
	BQ24155_STAT_PIN_DISABLE,
	BQ24155_CHARGE_STATUS,
	BQ24155_FAULT_STATUS,

	BQ24155_CHARGE_TERMINATION_STATUS,
	BQ24155_CHARGE_TERMINATION_ENABLE,
	BQ24155_CHARGE_TERMINATION_DISABLE,
	BQ24155_CHARGER_STATUS,
	BQ24155_CHARGER_ENABLE,
	BQ24155_CHARGER_DISABLE,
	BQ24155_HIGH_IMPEDANCE_STATUS,
	BQ24155_HIGH_IMPEDANCE_ENABLE,
	BQ24155_HIGH_IMPEDANCE_DISABLE,

	BQ24155_VENDER_CODE,
	BQ24155_PART_NUMBER,
	BQ24155_REVISION
};

/*
 * Configuration data structure containing bq24155 chip operating
 * parameters.
 *
 * Resistor sense value must be defined in device tree and is used to
 * determine charge and termination currents. Inappropriate values will
 * lead to unstable/indeterminate behaviour of the system.
 */
struct bq24155_config {
	char *bus_name;
	uint16_t i2c_addr;
	uint16_t input_current;         /* mA */
	uint16_t weak_voltage;          /* mV */
	uint16_t regulation_voltage;    /* mV */
	uint16_t charge_current;        /* mA */
	uint16_t termination_current;   /* mA */
	uint16_t resistor_sense;        /* mOhm */
};


struct bq24155_data {
	const struct device *i2c;
	const struct device *dev;
	struct k_work_delayable dwork_timer_reset;
	int32_t charge_status;
	int32_t fault_status;
};

#endif
