/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_clock_pic32cz_ca.h
 * @brief Clock control header file for Microchip pic32cz_ca family.
 *
 * This file provides clock driver interface definitions and structures
 * for pic32cz_ca family
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CZ_CA_H_
#define INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CZ_CA_H_

#include <zephyr/dt-bindings/clock/mchp_pic32cz_ca_clock.h>

/** @brief External Crystal Oscillator configuration structure */
struct clock_mchp_subsys_xosc_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;
};

/** @brief Gclk generator numbers */
enum clock_mchp_gclkgen {
	CLOCK_MCHP_GCLKGEN_GEN0,  /**< Gclk Generator 0 */
	CLOCK_MCHP_GCLKGEN_GEN1,  /**< Gclk Generator 1 */
	CLOCK_MCHP_GCLKGEN_GEN2,  /**< Gclk Generator 2 */
	CLOCK_MCHP_GCLKGEN_GEN3,  /**< Gclk Generator 3 */
	CLOCK_MCHP_GCLKGEN_GEN4,  /**< Gclk Generator 4 */
	CLOCK_MCHP_GCLKGEN_GEN5,  /**< Gclk Generator 5 */
	CLOCK_MCHP_GCLKGEN_GEN6,  /**< Gclk Generator 6 */
	CLOCK_MCHP_GCLKGEN_GEN7,  /**< Gclk Generator 7 */
	CLOCK_MCHP_GCLKGEN_GEN8,  /**< Gclk Generator 8 */
	CLOCK_MCHP_GCLKGEN_GEN9,  /**< Gclk Generator 9 */
	CLOCK_MCHP_GCLKGEN_GEN10, /**< Gclk Generator 10 */
	CLOCK_MCHP_GCLKGEN_GEN11, /**< Gclk Generator 11 */
	CLOCK_MCHP_GCLKGEN_GEN12, /**< Gclk Generator 12 */
	CLOCK_MCHP_GCLKGEN_GEN13, /**< Gclk Generator 13 */
	CLOCK_MCHP_GCLKGEN_GEN14, /**< Gclk Generator 14 */
	CLOCK_MCHP_GCLKGEN_GEN15  /**< Gclk Generator 15 */
};

/** @brief DFLL 48MHz configuration structure */
struct clock_mchp_subsys_dfll48m_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;

	/** @brief Enable closed-loop operation */
	bool closed_loop_en;

	/** @brief Reference source clock selection */
	enum clock_mchp_gclkgen src;

	/** @brief Determines the ratio of the CLK_DFLL48M output frequency to the CLK_DFLL48M_REF
	 * input frequency (0 - 65535)
	 */
	uint16_t multiply_factor;
};

/** @brief DPLL source clocks */
enum clock_mchp_dpll_src_clock {
	CLOCK_MCHP_DPLL_SRC_GCLK0,   /**< Gclk Generator 0 */
	CLOCK_MCHP_DPLL_SRC_GCLK1,   /**< Gclk Generator 1 */
	CLOCK_MCHP_DPLL_SRC_GCLK2,   /**< Gclk Generator 2 */
	CLOCK_MCHP_DPLL_SRC_GCLK3,   /**< Gclk Generator 3 */
	CLOCK_MCHP_DPLL_SRC_GCLK4,   /**< Gclk Generator 4 */
	CLOCK_MCHP_DPLL_SRC_GCLK5,   /**< Gclk Generator 5 */
	CLOCK_MCHP_DPLL_SRC_GCLK6,   /**< Gclk Generator 6 */
	CLOCK_MCHP_DPLL_SRC_GCLK7,   /**< Gclk Generator 7 */
	CLOCK_MCHP_DPLL_SRC_GCLK8,   /**< Gclk Generator 8 */
	CLOCK_MCHP_DPLL_SRC_GCLK9,   /**< Gclk Generator 9 */
	CLOCK_MCHP_DPLL_SRC_GCLK10,  /**< Gclk Generator 10 */
	CLOCK_MCHP_DPLL_SRC_GCLK11,  /**< Gclk Generator 11 */
	CLOCK_MCHP_DPLL_SRC_GCLK12,  /**< Gclk Generator 12 */
	CLOCK_MCHP_DPLL_SRC_GCLK13,  /**< Gclk Generator 13 */
	CLOCK_MCHP_DPLL_SRC_GCLK14,  /**< Gclk Generator 14 */
	CLOCK_MCHP_DPLL_SRC_GCLK15,  /**< Gclk Generator 15 */
	CLOCK_MCHP_DPLL_SRC_XOSC,    /**< External crystal oscillator (XOSC) */
	CLOCK_MCHP_DPLL_SRC_DFLL48M, /**< Internal 48 MHz DFLL clock */

	CLOCK_MCHP_DPLL_SRC_MAX =
		CLOCK_MCHP_DPLL_SRC_DFLL48M /**< Maximum valid DPLL source clock */
};

/** @brief DPLL configuration structure */
struct clock_mchp_subsys_dpll_config {
	/** @brief ratio of PLL's VCO output frequency to Reference input frequency */
	uint16_t feedback_divider_factor;

	/** @brief Determines division factor of PLL input reference freq (1 ≤ REFDIV ≤ 63) */
	uint8_t ref_division_factor;

	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;

	/** @brief Reference source clock selection */
	enum clock_mchp_dpll_src_clock src;
};

/** @brief DPLL Output configuration structure */
struct clock_mchp_subsys_dpll_out_config {
	/** @brief Determines the division factor of PLL VCO freq output 1 ≤ POSTDIV ≤ 63 */
	uint8_t output_division_factor;
};

/** @brief RTC source clocks */
enum clock_mchp_rtc_src_clock {
	/** Internal ultra-low-power 1 kHz oscillator */
	CLOCK_MCHP_RTC_SRC_ULP1K = OSC32KCTRL_CLKSELCTRL_RTCSEL_ULP1K_Val,

	/** Internal ultra-low-power 32 kHz oscillator */
	CLOCK_MCHP_RTC_SRC_ULP32K = OSC32KCTRL_CLKSELCTRL_RTCSEL_ULP32K_Val,

	/** External crystal oscillator divided to 1 kHz */
	CLOCK_MCHP_RTC_SRC_XOSC1K = OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC1K_Val,

	/** External 32.768 kHz crystal oscillator */
	CLOCK_MCHP_RTC_SRC_XOSC32K = OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC32K_Val
};

/** @brief RTC configuration structure */
struct clock_mchp_subsys_rtc_config {
	/** @brief RTC source clock selection */
	enum clock_mchp_rtc_src_clock src;
};

/** @brief External 32.768 kHz crystal oscillator configuration structure */
struct clock_mchp_subsys_xosc32k_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;
};

/** @brief Gclk Generator source clocks */
enum clock_mchp_gclk_src_clock {
	CLOCK_MCHP_GCLK_SRC_XOSC,              /**< External crystal oscillator (XOSC) */
	CLOCK_MCHP_GCLK_SRC_GCLKPIN,           /**< External clock input from Gclk I/O pin */
	CLOCK_MCHP_GCLK_SRC_GCLKGEN1,          /**< Output of Generic Clock Generator 1 */
	CLOCK_MCHP_GCLK_SRC_OSCULP32K,         /**< Internal ultra-low-power 32 kHz oscillator */
	CLOCK_MCHP_GCLK_SRC_XOSC32K,           /**< External 32.768 kHz crystal oscillator */
	CLOCK_MCHP_GCLK_SRC_DFLL48M,           /**< Internal 48 MHz DFLL clock */
	CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0,     /**< DPLL0 output clock 0 */
	CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT1,     /**< DPLL0 output clock 1 */
	CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT2,     /**< DPLL0 output clock 2 */
	CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT3,     /**< DPLL0 output clock 3 */
	CLOCK_MCHP_GCLK_SRC_DPLL1_FRC_CLKOUT0, /**< DPLL1 clock output 0 */
	CLOCK_MCHP_GCLK_SRC_DPLL1_FRC_CLKOUT1, /**< DPLL1 clock output 1 */
	CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT2,     /**< DPLL1 output clock 2 */
	CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT3,     /**< DPLL1 output clock 3 */

	CLOCK_MCHP_GCLK_SRC_MAX =
		CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT3 /**< Maximum valid Gclk source */
};

/** @brief Gclk Generator configuration structure */
struct clock_mchp_subsys_gclkgen_config {
	/** @brief Represent a division value for the corresponding Generator. The actual division
	 * factor is dependent on the state of div_select (gclk1 0 - 65535, others 0 - 255)
	 */
	uint16_t div_factor;

	/** @brief configure oscillator to ON in standby sleep mode, unless on_demand_en is set */
	bool run_in_standby_en;

	/** @brief Generator source clock selection */
	enum clock_mchp_gclk_src_clock src;
};

/** @brief Gclk Peripheral configuration structure */
struct clock_mchp_subsys_gclkperiph_config {
	/** @brief gclk generator source of a peripheral clock */
	enum clock_mchp_gclkgen src;
};

/** @brief division ratio of mclk prescaler for CPU */
enum clock_mchp_mclk_cpu_div {
	CLOCK_MCHP_MCLK_CPU_DIV_1 = 1,    /**< Divide by 1 (no division) */
	CLOCK_MCHP_MCLK_CPU_DIV_2 = 2,    /**< Divide by 2 */
	CLOCK_MCHP_MCLK_CPU_DIV_4 = 4,    /**< Divide by 4 */
	CLOCK_MCHP_MCLK_CPU_DIV_8 = 8,    /**< Divide by 8 */
	CLOCK_MCHP_MCLK_CPU_DIV_16 = 16,  /**< Divide by 16 */
	CLOCK_MCHP_MCLK_CPU_DIV_32 = 32,  /**< Divide by 32 */
	CLOCK_MCHP_MCLK_CPU_DIV_64 = 64,  /**< Divide by 64 */
	CLOCK_MCHP_MCLK_CPU_DIV_128 = 128 /**< Divide by 128 */
};

/** @brief MCLK configuration structure
 *
 * Used for CLOCK_MCHP_SUBSYS_TYPE_MCLKDOMAIN
 */
struct clock_mchp_subsys_mclkcpu_config {
	/** @brief division ratio of mclk prescaler for CPU */
	enum clock_mchp_mclk_cpu_div division_factor;
};

/** @brief clock rate datatype
 *
 * Used for setting a clock rate
 */
typedef uint32_t *clock_mchp_rate_t;

#endif /* INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CZ_CA_H_ */
