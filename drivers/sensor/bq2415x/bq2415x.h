/* SPDX-License-Identifier: Apache-2.0 */
/*
 * bq2415x charger driver
 *
 * Copyright (C) 2011-2013  Pali Rohár <pali@kernel.org>
 * Copyright (C) 2021  Filip Primozic <filip.primozic@greyp.com>
 */

#ifndef BQ2415X_CHARGER_H
#define BQ2415X_CHARGER_H

#include <stdint.h>
#include <kernel.h>

#define BQ2415X_DEFAULT_VENDER_CODE 0x0A /* in binary */

/* timeout, in seconds, for resetting chip timer */
#define BQ2415X_TIMER_TIMEOUT 10

#define BQ2415X_REG_STATUS 0x00
#define BQ2415X_REG_CONTROL 0x01
#define BQ2415X_REG_VOLTAGE 0x02
#define BQ2415X_REG_VENDER 0x03
#define BQ2415X_REG_CURRENT 0x04

/* reset state for all registers */
#define BQ2415X_RESET_STATUS BIT(6)
#define BQ2415X_RESET_CONTROL (BIT(4) | BIT(5))
#define BQ2415X_RESET_VOLTAGE (BIT(1) | BIT(3))
#define BQ2415X_RESET_CURRENT (BIT(0) | BIT(3) | BIT(7))

/* status register */
#define BQ2415X_BIT_TMR_RST 7
#define BQ2415X_BIT_OTG 7
#define BQ2415X_BIT_EN_STAT 6
#define BQ2415X_MASK_STAT (BIT(4) | BIT(5))
#define BQ2415X_SHIFT_STAT 4
#define BQ2415X_BIT_BOOST 3
#define BQ2415X_MASK_FAULT (BIT(0) | BIT(1) | BIT(2))
#define BQ2415X_SHIFT_FAULT 0

/* control register */
#define BQ2415X_MASK_LIMIT (BIT(6) | BIT(7))
#define BQ2415X_SHIFT_LIMIT 6
#define BQ2415X_MASK_VLOWV (BIT(4) | BIT(5))
#define BQ2415X_SHIFT_VLOWV 4
#define BQ2415X_BIT_TE 3
#define BQ2415X_BIT_CE 2
#define BQ2415X_BIT_HZ_MODE 1
#define BQ2415X_BIT_OPA_MODE 0

/* voltage register */
#define BQ2415X_MASK_VO (BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define BQ2415X_SHIFT_VO 2
#define BQ2415X_BIT_OTG_PL 1
#define BQ2415X_BIT_OTG_EN 0

/* vender register */
#define BQ2415X_MASK_VENDER (BIT(5) | BIT(6) | BIT(7))
#define BQ2415X_SHIFT_VENDER 5
#define BQ2415X_MASK_PN (BIT(3) | BIT(4))
#define BQ2415X_SHIFT_PN 3
#define BQ2415X_MASK_REVISION (BIT(0) | BIT(1) | BIT(2))
#define BQ2415X_SHIFT_REVISION 0

/* current register */
#define BQ2415X_MASK_RESET BIT(7)
#define BQ2415X_MASK_VI_CHRG (BIT(4) | BIT(5) | BIT(6))
#define BQ2415X_SHIFT_VI_CHRG 4
/* N/A					BIT(3) */
#define BQ2415X_MASK_VI_TERM (BIT(0) | BIT(1) | BIT(2))
#define BQ2415X_SHIFT_VI_TERM 0

enum bq2415x_command {
	BQ2415X_TIMER_RESET,
	BQ2415X_OTG_STATUS,
	BQ2415X_STAT_PIN_STATUS,
	BQ2415X_STAT_PIN_ENABLE,
	BQ2415X_STAT_PIN_DISABLE,
	BQ2415X_CHARGE_STATUS,
	BQ2415X_BOOST_STATUS,
	BQ2415X_FAULT_STATUS,

	BQ2415X_CHARGE_TERMINATION_STATUS,
	BQ2415X_CHARGE_TERMINATION_ENABLE,
	BQ2415X_CHARGE_TERMINATION_DISABLE,
	BQ2415X_CHARGER_STATUS,
	BQ2415X_CHARGER_ENABLE,
	BQ2415X_CHARGER_DISABLE,
	BQ2415X_HIGH_IMPEDANCE_STATUS,
	BQ2415X_HIGH_IMPEDANCE_ENABLE,
	BQ2415X_HIGH_IMPEDANCE_DISABLE,
	BQ2415X_BOOST_MODE_STATUS,
	BQ2415X_BOOST_MODE_ENABLE,
	BQ2415X_BOOST_MODE_DISABLE,

	BQ2415X_OTG_LEVEL,
	BQ2415X_OTG_ACTIVATE_HIGH,
	BQ2415X_OTG_ACTIVATE_LOW,
	BQ2415X_OTG_PIN_STATUS,
	BQ2415X_OTG_PIN_ENABLE,
	BQ2415X_OTG_PIN_DISABLE,

	BQ2415X_VENDER_CODE,
	BQ2415X_PART_NUMBER,
	BQ2415X_REVISION,
};

enum bq2415x_chip {
	BQUNKNOWN,
	BQ24150,
	BQ24150A,
	BQ24151,
	BQ24151A,
	BQ24152,
	BQ24153,
	BQ24153A,
	BQ24155,
	BQ24156,
	BQ24156A,
	BQ24157S,
	BQ24158,
};

/*
 * This is platform data for bq2415x chip. It contains default board
 * voltages and currents.
 *
 * Value resistor_sense is needed for configuring charge and
 * termination current. If it is less or equal to zero, configuring charge
 * and termination current will not be possible.
 */
struct bq2415x_config {
	char *bus_name;
	uint16_t i2c_addr;
	uint16_t current_limit;         /* mA */
	uint16_t weak_voltage;          /* mV */
	uint16_t regulation_voltage;    /* mV */
	uint16_t charge_current;        /* mA */
	uint16_t termination_current;   /* mA */
	uint16_t resistor_sense;        /* m ohm */
};


struct bq2415x_device {
	const struct device *i2c;
	const struct device *dev;
	struct k_work_delayable dwork_timer_reset;
	enum bq2415x_chip chip;
	int32_t charge_status;
	int32_t fault_status;
};

#endif
