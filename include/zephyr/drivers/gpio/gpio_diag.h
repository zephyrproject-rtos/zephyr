/*
 * Copyright (c) 2025 Baylibre SAS
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_DIAG_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_DIAG_H_

enum gpio_diag {
	/* Common diagnostic options */
	GPIO_DIAG_WIRE_BREAK = 0, /* Wire break */
	GPIO_DIAG_THRM_SHUTD,     /* Thermal shutdown */
	GPIO_DIAG_OVER_LOAD,      /* Over load - OVL */
	GPIO_DIAG_CURR_LIM,       /* Current limit */
	GPIO_DIAG_OVER_TEMP,      /* Over temperature */
	GPIO_DIAG_TEMP_ALRM,      /* Temperature alarm */

	/* Power supply diagnostics */
	GPIO_DIAG_LOSS_GND,   /* Missing ground */
	GPIO_DIAG_LOSS_VDD,   /* Missing ground */
	GPIO_DIAG_BAD_VDD,    /* Bad supply voltage */
	GPIO_DIAG_OVER_VDD,   /* Supply voltage too high */
	GPIO_DIAG_ABOVE_VDD,  /* Supply voltage too low */
	GPIO_DIAG_SHORT_VDD,  /* Short to VDD fault */
	GPIO_DIAG_SUPPLY_ERR, /* Power supply generic err */

	GPIO_DIAG_TERM_OVL,   /* Thermal overload */
	GPIO_DIAG_DE_MAG_FLT, /* Safe demagnitization for inductive loads */
	GPIO_DIAG_SYNC_ERR,   /* Sync error detected */
	GPIO_DIAG_COM_ERR,    /* Communication error */
	GPIO_DIAG_WDOG_ERR,   /* Watchdog error */
	GPIO_DIAG_CRC_ERR,    /* Communication CRC error */
	GPIO_DIAG_POR,        /* Power on reset error */
	GPIO_DIAG_IRQ,        /* Interrupt */

	/*
	 * MAX14906
	 */
	GPIO_DIAG_MAX14906_VINT_UV, /* Identical to POR */

	/* END of MAX14906 */

	/** MAX22190 */

	/* FAULT1 register for MAX22190 */
	GPIO_DIAG_MAX22190_TEMP_ALARM, /* Temperature ALRMT1 & ALRMT2 */
	GPIO_DIAG_MAX22190_FAULT2,     /* FAULT2 register */

	/* FAULT2 register for MAX22190 */
	GPIO_DIAG_MAX22190_RFWBS,    /* Ref WB pin short */
	GPIO_DIAG_MAX22190_RFWBO,    /* Ref WB pin open wire */
	GPIO_DIAG_MAX22190_RFDIS,    /* Ref DI pin short */
	GPIO_DIAG_MAX22190_RFDIO,    /* Ref DI pin open wire */
	GPIO_DIAG_MAX22190_FAULT8CK, /* FAULT8CK */

	/** All channels. */
	GPIO_DIAG_ALL,

	/**
	 * Number of all common sensor channels.
	 */
	GPIO_DIAG_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	GPIO_DIAG_PRIV_START = GPIO_DIAG_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor channel type.
	 */
	GPIO_DIAG_MAX = INT16_MAX,
};

#endif
