/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_clock_pic32ck_sg_gc.h
 * @brief Clock control header file for Microchip pic32ck_sg_gc family.
 *
 * This file provides clock driver interface definitions and structures
 * for pic32ck_sg_gc family
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CK_SG_GC_H_
#define INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CK_SG_GC_H_

#include <zephyr/dt-bindings/clock/mchp_pic32ck_sg_gc_clock.h>

/** @brief XOSC configuration structure */
struct clock_mchp_subsys_xosc_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;
};

/** @brief GCLK generator numbers */
enum clock_mchp_gclkgen {
	CLOCK_MCHP_GCLKGEN_GEN0,
	CLOCK_MCHP_GCLKGEN_GEN1,
	CLOCK_MCHP_GCLKGEN_GEN2,
	CLOCK_MCHP_GCLKGEN_GEN3,
	CLOCK_MCHP_GCLKGEN_GEN4,
	CLOCK_MCHP_GCLKGEN_GEN5,
	CLOCK_MCHP_GCLKGEN_GEN6,
	CLOCK_MCHP_GCLKGEN_GEN7,
	CLOCK_MCHP_GCLKGEN_GEN8,
	CLOCK_MCHP_GCLKGEN_GEN9,
	CLOCK_MCHP_GCLKGEN_GEN10,
	CLOCK_MCHP_GCLKGEN_GEN11,
};

struct clock_mchp_subsys_dfll48m_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;

	/** @brief Enable open-loop operation */
	bool closed_loop_en;

	/** @brief Reference source clock selection */
	enum clock_mchp_gclkgen src;

	/** @brief Determines the ratio of the CLK_DFLL48M output frequency to the CLK_DFLL48M_REF
	 * input frequency (0 - 65535)
	 */
	uint16_t multiply_factor;
};

/** @brief PLL source clocks */
enum clock_mchp_pll_src_clock {
	/** @brief GCLK generator 0 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK0,
	/** @brief GCLK generator 1 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK1,
	/** @brief GCLK generator 2 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK2,
	/** @brief GCLK generator 3 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK3,
	/** @brief GCLK generator 4 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK4,
	/** @brief GCLK generator 5 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK5,
	/** @brief GCLK generator 6 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK6,
	/** @brief GCLK generator 7 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK7,
	/** @brief GCLK generator 8 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK8,
	/** @brief GCLK generator 9 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK9,
	/** @brief GCLK generator 10 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK10,
	/** @brief GCLK generator 11 as PLL source */
	CLOCK_MCHP_PLL_SRC_GCLK11,
	/** @brief External oscillator as PLL source */
	CLOCK_MCHP_PLL_SRC_XOSC,
	/** @brief DFLL48M as PLL source */
	CLOCK_MCHP_PLL_SRC_DFLL48M,
	/** @brief Maximum PLL source value */
	CLOCK_MCHP_PLL_SRC_MAX = CLOCK_MCHP_PLL_SRC_DFLL48M
};

/** @brief PLL configuration structure */
struct clock_mchp_subsys_pll_config {
	/** @brief ratio of PLL's VCO output frequency to Reference input frequency */
	uint16_t feedback_divider_factor;

	/** @brief Determines division factor of PLL input reference
	 * frequency (1 ≤ REFDIV ≤ 63)
	 */
	uint8_t ref_division_factor;

	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;

	/** @brief Reference source clock selection */
	enum clock_mchp_pll_src_clock src;
};

/** @brief PLL output configuration structure */
struct clock_mchp_subsys_pll_out_config {
	/** @brief Determines the division factor of PLL VCO freq output 1 ≤ POSTDIV ≤ 63 */
	uint8_t output_division_factor;
};

/** @brief RTC source clocks */
enum clock_mchp_rtc_src_clock {
	/** @brief Ultra low power 1kHz oscillator as RTC source */
	CLOCK_MCHP_RTC_SRC_OSCULP1K = OSC32KCTRL_CLKSELCTRL_RTCSEL_ULP1K_Val,
	/** @brief Ultra low power 32kHz oscillator as RTC source */
	CLOCK_MCHP_RTC_SRC_OSCULP32K = OSC32KCTRL_CLKSELCTRL_RTCSEL_ULP32K_Val,
	/** @brief External 1kHz oscillator as RTC source */
	CLOCK_MCHP_RTC_SRC_XOSC1K = OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC1K_Val,
	/** @brief External 32kHz oscillator as RTC source */
	CLOCK_MCHP_RTC_SRC_XOSC32K = OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC32K_Val
};

struct clock_mchp_subsys_rtc_config {
	/** @brief RTC source clock selection */
	enum clock_mchp_rtc_src_clock src;
};

struct clock_mchp_subsys_xosc32k_config {
	/** @brief configure oscillator to ON, when a peripheral is requesting it as a source */
	bool on_demand_en;
};

/** @brief Gclk Generator source clocks */
enum clock_mchp_gclk_src_clock {
	/** @brief External oscillator as GCLK source */
	CLOCK_MCHP_GCLK_SRC_XOSC,
	/** @brief GCLK pin as GCLK source */
	CLOCK_MCHP_GCLK_SRC_GCLKPIN,
	/** @brief GCLK generator 1 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_GCLKGEN1,
	/** @brief Ultra low power 32kHz oscillator as GCLK source */
	CLOCK_MCHP_GCLK_SRC_OSCULP32K,
	/** @brief External 32kHz oscillator as GCLK source */
	CLOCK_MCHP_GCLK_SRC_XOSC32K,
	/** @brief DFLL48M as GCLK source */
	CLOCK_MCHP_GCLK_SRC_DFLL48M,
	/** @brief PLL0 clock output 0 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT0,
	/** @brief PLL0 clock output 1 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT1,
	/** @brief PLL0 clock output 2 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT2,
	/** @brief PLL0 clock output 3 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT3,
	/** @brief PLL0 clock output 4 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT4,
	/** @brief PLL0 clock output 5 as GCLK source */
	CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT5,
	/** @brief Maximum GCLK source value */
	CLOCK_MCHP_GCLK_SRC_MAX = CLOCK_MCHP_GCLK_SRC_PLL0_CLKOUT5
};

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

struct clock_mchp_subsys_gclkperiph_config {
	/** @brief gclk generator source of a peripheral clock */
	enum clock_mchp_gclkgen src;
};

/** @brief division ratio of mclk prescaler for CPU */
enum clock_mchp_mclk_cpu_div {
	CLOCK_MCHP_MCLK_CPU_DIV_1 = 1,
	CLOCK_MCHP_MCLK_CPU_DIV_2 = 2,
	CLOCK_MCHP_MCLK_CPU_DIV_4 = 4,
	CLOCK_MCHP_MCLK_CPU_DIV_8 = 8,
	CLOCK_MCHP_MCLK_CPU_DIV_16 = 16,
	CLOCK_MCHP_MCLK_CPU_DIV_32 = 32,
	CLOCK_MCHP_MCLK_CPU_DIV_64 = 64,
	CLOCK_MCHP_MCLK_CPU_DIV_128 = 128
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

#endif /* INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CK_SG_GC_H_ */
