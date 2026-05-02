/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CHARGER_NPM10XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_CHARGER_NPM10XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/charger.h>

/**
 * @file npm10xx.h
 * @brief Nordic's nPM10 Series PMIC charger driver custom properties and value enums
 * @defgroup charger_interface_npm10xx nPM10xx Charger interface
 * @ingroup charger_interface_ext
 * @{
 */

/** nPM10xx Charger custom properties. Extends enum charger_property  */
enum npm10xx_charger_prop {
	/** Charging termination current in percent of constant charging current */
	/** Value should be of type enum npm10xx_charger_iterm */
	NPM10XX_CHARGER_PROP_TERM_CURRENT = CHARGER_PROP_CUSTOM_BEGIN,
	/** Charging trickle current in percent of constant charging current */
	/** Value should be of type enum npm10xx_charger_itrickle */
	NPM10XX_CHARGER_PROP_TRICKLE_CURRENT,
	/**
	 * Charge throttling configuration. Value should consist of one of enum
	 * npm10xx_charger_ithrottle bitwise-ORed with one of enum npm10xx_charger_vthrottle.
	 * For example, `NPM10XX_CHARGER_ITHROTTLE_75P | NPM10XX_CHARGER_VTHROTTLE_100MV`
	 */
	NPM10XX_CHARGER_PROP_THROTTLE_LVL,
	/** Charge current target in µA for the NTC Warm region */
	NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_CURRENT_WARM_UA,
	/** Charge current target in µA for the NTC Cool region */
	NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_CURRENT_COOL_UA,
	/** Charge voltage regulation target in µV for the NTC Warm region */
	NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_WARM_UV,
	/** Charge voltage regulation target in µV for the NTC Cool region */
	NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_COOL_UV,
};

/** nPM10xx Charger termination current in percent */
enum npm10xx_charger_iterm {
	/** Termination current equals 12.5% of charge current */
	NPM10XX_CHARGER_ITERM_12P5,
	/** Termination current equals 100% of charge current */
	NPM10XX_CHARGER_ITERM_100P,
	/** Termination current equals 50% of charge current */
	NPM10XX_CHARGER_ITERM_50P,
	/** Termination current equals 25% of charge current */
	NPM10XX_CHARGER_ITERM_25P,
	/** Termination current equals 6.25% of charge current */
	NPM10XX_CHARGER_ITERM_6P25,
	/** Termination current equals 3.125% of charge current */
	NPM10XX_CHARGER_ITERM_3P125,
	/** Termination current equals 1.56% of charge current */
	NPM10XX_CHARGER_ITERM_1P56,
	/** Termination current equals 0.78% of charge current */
	NPM10XX_CHARGER_ITERM_0P78,
	/** Mark the end of valid enum values */
	NPM10XX_CHARGER_CURRENT_PERCENT_MAX,
};

/** nPM10xx Charger trickle current in percent */
enum npm10xx_charger_itrickle {
	/** Trickle current equals 12.5% of charge current */
	NPM10XX_CHARGER_ITRICKLE_12P5,
	/** Trickle current equals 100% of charge current */
	NPM10XX_CHARGER_ITRICKLE_100P,
	/** Trickle current equals 50% of charge current */
	NPM10XX_CHARGER_ITRICKLE_50P,
	/** Trickle current equals 25% of charge current */
	NPM10XX_CHARGER_ITRICKLE_25P,
	/** Trickle current equals 6.25% of charge current */
	NPM10XX_CHARGER_ITRICKLE_6P25,
	/** Trickle current equals 3.125% of charge current */
	NPM10XX_CHARGER_ITRICKLE_3P125,
	/** Trickle current equals 1.56% of charge current */
	NPM10XX_CHARGER_ITRICKLE_1P56,
	/** Trickle current equals 0.78% of charge current */
	NPM10XX_CHARGER_ITRICKLE_0P78,
};

/**
 * nPM10xx Charger throttle current in percent. Use in combination with
 * an enum npm10xx_charger_vthrottle value to configure charge throttling.
 */
enum npm10xx_charger_ithrottle {
	/** Throttle current equals 100% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_100P = 0x00U,
	/** Throttle current equals 87.5% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_87P5 = 0x10U,
	/** Throttle current equals 75% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_75P = 0x20U,
	/** Throttle current equals 62.5% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_62P5 = 0x30U,
	/** Throttle current equals 50% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_50P = 0x40U,
	/** Throttle current equals 37.5% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_37P5 = 0x50U,
	/** Throttle current equals 25% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_25P = 0x60U,
	/** Throttle current equals 12.5% of charge current */
	NPM10XX_CHARGER_ITHROTTLE_12P5 = 0x70U,
};

/**
 * nPM10xx Charger throttle voltage levels relative to termination voltage. Use in combination with
 * an enum npm10xx_charger_ithrottle value to configure charge throttling.
 */
enum npm10xx_charger_vthrottle {
	/** Throttle voltage is 25mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_25MV,
	/** Throttle voltage is 50mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_50MV,
	/** Throttle voltage is 75mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_75MV,
	/** Throttle voltage is 100mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_100MV,
	/** Throttle voltage is 125mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_125MV,
	/** Throttle voltage is 150mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_150MV,
	/** Throttle voltage is 175mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_175MV,
	/** Throttle voltage is 200mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_200MV,
	/** Throttle voltage is 225mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_225MV,
	/** Throttle voltage is 250mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_250MV,
	/** Throttle voltage is 275mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_275MV,
	/** Throttle voltage is 300mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_300MV,
	/** Throttle voltage is 325mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_325MV,
	/** Throttle voltage is 350mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_350MV,
	/** Throttle voltage is 375mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_375MV,
	/** Throttle voltage is 400mV below termination voltage */
	NPM10XX_CHARGER_VTHROTTLE_400MV,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CHARGER_NPM10XX_H_ */
