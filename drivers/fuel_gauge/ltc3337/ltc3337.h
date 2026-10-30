/*
 * Copyright (c) 2026 Federico Borgo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUEL_GAUGE_LTC3337_LTC3337_H_
#define ZEPHYR_DRIVERS_FUEL_GAUGE_LTC3337_LTC3337_H_

#include <zephyr/sys/util.h>

/*
 * Every register is a 16-bit word, least significant byte first, selected by a
 * one byte sub-address (datasheet Table 2).
 */
enum ltc3337_regs {
	/* Prescaler, clear interrupt, counter check, shutdown, ADC trigger, alarm level (W) */
	LTC3337_REG_A = 0x01,
	/* Accumulated charge (R/W, only bits 15:8 are writable) */
	LTC3337_REG_B = 0x02,
	/* Status flags, IPEAK pin strap and die temperature (R) */
	LTC3337_REG_C = 0x03,
	/* BAT_IN voltage with IPEAK on (R) */
	LTC3337_REG_D = 0x04,
	/* BAT_IN voltage with IPEAK off (R) */
	LTC3337_REG_E = 0x05,
	/* BAT_OUT voltage with IPEAK on (R) */
	LTC3337_REG_F = 0x06,
	/* BAT_OUT voltage with IPEAK off (R) */
	LTC3337_REG_G = 0x07,
	/* Cold and hot die temperature alarm levels (W) */
	LTC3337_REG_H = 0x08,
};

#define LTC3337_NUM_REGS (LTC3337_REG_H + 1)

/* Register A */
#define LTC3337_A_PRESCALER_MASK GENMASK(3, 0)
#define LTC3337_A_CLEAR_INT      BIT(4) /* self clearing */
#define LTC3337_A_COUNTER_CHECK  BIT(5)
#define LTC3337_A_SHUTDOWN       BIT(6)
#define LTC3337_A_ADC_TRIGGER    BIT(7) /* self clearing */
#define LTC3337_A_ALARM_MASK     GENMASK(15, 8)
#define LTC3337_A_POR            0xFF00

/* Register B: only the upper byte can be written */
#define LTC3337_B_WRITABLE_MASK GENMASK(15, 8)

/* Register C */
#define LTC3337_C_OVERFLOW   BIT(0)
#define LTC3337_C_ALARM_TRIP BIT(1)
#define LTC3337_C_COLD_ALARM BIT(2)
#define LTC3337_C_HOT_ALARM  BIT(3)
#define LTC3337_C_ADC_READY  BIT(4)
#define LTC3337_C_ALARMS     GENMASK(3, 0)
#define LTC3337_C_IPK_MASK   GENMASK(7, 5)
#define LTC3337_C_STATUS     GENMASK(7, 0)
#define LTC3337_C_TEMP_MASK  GENMASK(15, 8)

/* Registers D to G */
#define LTC3337_VOLTAGE_MASK GENMASK(11, 0)

/* Register H */
#define LTC3337_H_COLD_MASK GENMASK(7, 0)
#define LTC3337_H_HOT_MASK  GENMASK(15, 8)
#define LTC3337_H_POR       0xFF00

/* Bits of FUEL_GAUGE_CC_CONFIG: the settable part of register A */
#define LTC3337_CC_PRESCALER_MASK LTC3337_A_PRESCALER_MASK
#define LTC3337_CC_COUNTER_CHECK  LTC3337_A_COUNTER_CHECK
#define LTC3337_CC_SHUTDOWN       LTC3337_A_SHUTDOWN

#define LTC3337_CC_MASK (LTC3337_CC_PRESCALER_MASK | LTC3337_CC_COUNTER_CHECK | LTC3337_CC_SHUTDOWN)

#define LTC3337_PRESCALER_MAX 15

/* Typical values from the electrical characteristics table */
#define LTC3337_VOLTAGE_LSB_UV     1465
#define LTC3337_TEMP_LSB_MDEGC     784
#define LTC3337_TEMP_ZERO_MDEGC    (-41000)
#define LTC3337_MDEGC_PER_DK       100
#define LTC3337_KELVIN_ZERO_MDEGC  (-273150)
#define LTC3337_DIE_TEMP_FULLSCALE 255

#endif /* ZEPHYR_DRIVERS_FUEL_GAUGE_LTC3337_LTC3337_H_ */
