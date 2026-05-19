/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_clock_pic32cm_pl.h
 * @brief Clock control header file for Microchip pic32cm_pl family.
 *
 * This file provides clock driver interface definitions and structures
 * for pic32cm_pl family
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CM_PL_H_
#define INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CM_PL_H_

#include <zephyr/dt-bindings/clock/mchp_pic32cm_pl_clock.h>

/** @brief OSCHF (high-frequency oscillator) subsystem configuration. */
struct clock_mchp_subsys_oschf_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;
};

/** @brief Common clock frequency constants (in Hz). */
enum clock_mchp_freq {
	/** 1 kHz nominal clock frequency (1024 Hz). */
	CLOCK_FREQ_1KHZ = 1024,
	/** 32 kHz clock frequency (32768 Hz). */
	CLOCK_FREQ_32KHZ = 32768,
	/** 1 MHz clock frequency. */
	CLOCK_FREQ_1MHZ = 1000000,
	/** 2 MHz clock frequency. */
	CLOCK_FREQ_2MHZ = 2000000,
	/** 3 MHz clock frequency. */
	CLOCK_FREQ_3MHZ = 3000000,
	/** 4 MHz clock frequency. */
	CLOCK_FREQ_4MHZ = 4000000,
	/** 8 MHz clock frequency. */
	CLOCK_FREQ_8MHZ = 8000000,
	/** 12 MHz clock frequency. */
	CLOCK_FREQ_12MHZ = 12000000,
	/** 20 MHz clock frequency. */
	CLOCK_FREQ_20MHZ = 20000000,
	/** 24 MHz clock frequency. */
	CLOCK_FREQ_24MHZ = 24000000,
};

/** @brief Gclk Generator source clocks */
enum clock_mchp_gclk_src_clock {
	/** High-frequency internal oscillator (OSCHF). */
	CLOCK_MCHP_GCLK_SRC_OSCHF = 0,
	/** Internal 32 kHz oscillator (OSC32K). */
	CLOCK_MCHP_GCLK_SRC_OSC32K,
	/** External 32 kHz crystal oscillator (XOSC32K). */
	CLOCK_MCHP_GCLK_SRC_XOSC32K,
	/** External clock input on GCLK pin (GCLKPIN). */
	CLOCK_MCHP_GCLK_SRC_GCLKPIN,
	/** Output of Generic Clock Generator 1 (GCLKGEN1). */
	CLOCK_MCHP_GCLK_SRC_GCLKGEN1,
	/** Number of GLCK SRC Available. */
	CLOCK_MCHP_GCLK_SRC_COUNT
};

/** @brief GCLK generator numbers */
enum clock_mchp_gclkgen {
	/** Generic Clock Generator 0. */
	CLOCK_MCHP_GCLKGEN_GEN0,
	/** Generic Clock Generator 1. */
	CLOCK_MCHP_GCLKGEN_GEN1,
	/** Generic Clock Generator 2. */
	CLOCK_MCHP_GCLKGEN_GEN2,
	/** Generic Clock Generator 3. */
	CLOCK_MCHP_GCLKGEN_GEN3,
	/** Number of supported GCLK generators (sentinel/max). */
	CLOCK_MCHP_GCLKGEN_MAX
};

/** @brief clock rate datatype
 *
 * Used for setting a clock rate
 */
typedef uint32_t *clock_mchp_rate_t;

#endif /* INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CM_PL_H_ */
