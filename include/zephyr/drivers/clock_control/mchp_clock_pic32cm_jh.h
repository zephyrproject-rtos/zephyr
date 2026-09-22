/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control header file for the Microchip PIC32CM JH family.
 *
 * This file provides clock driver interface definitions and structures
 * for the PIC32CM JH family.
 * @ingroup clock_control_mchp_pic32cm_jh
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CM_JH_H_
#define INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CM_JH_H_

#include <zephyr/dt-bindings/clock/mchp_pic32cm_jh_clock.h>

/**
 * @defgroup clock_control_mchp_pic32cm_jh Microchip PIC32CM JH
 * @ingroup clock_control_mchp
 * @{
 */

/** @brief External oscillator (XOSC) configuration. */
struct clock_mchp_subsys_xosc_config {
	/** @brief Turn the oscillator ON when a peripheral requests it as a source. */
	bool on_demand_en;

	/** @brief Keep the oscillator ON in standby sleep mode, unless on_demand_en is set. */
	bool run_in_standby_en;
};

/** @brief Control the OSC48M oscillator frequency range by adjusting the division ratio. */
enum clock_mchp_osc48m_divider_freq {
	CLOCK_MCHP_DIVIDER_48_MHZ,   /**< 48 MHz output. */
	CLOCK_MCHP_DIVIDER_24_MHZ,   /**< 24 MHz output. */
	CLOCK_MCHP_DIVIDER_16_MHZ,   /**< 16 MHz output. */
	CLOCK_MCHP_DIVIDER_12_MHZ,   /**< 12 MHz output. */
	CLOCK_MCHP_DIVIDER_9_6_MHZ,  /**< 9.6 MHz output. */
	CLOCK_MCHP_DIVIDER_8_MHZ,    /**< 8 MHz output. */
	CLOCK_MCHP_DIVIDER_6_86_MHZ, /**< 6.86 MHz output. */
	CLOCK_MCHP_DIVIDER_6_MHZ,    /**< 6 MHz output. */
	CLOCK_MCHP_DIVIDER_5_33_MHZ, /**< 5.33 MHz output. */
	CLOCK_MCHP_DIVIDER_4_8_MHZ,  /**< 4.8 MHz output. */
	CLOCK_MCHP_DIVIDER_4_36_MHZ, /**< 4.36 MHz output. */
	CLOCK_MCHP_DIVIDER_4_MHZ,    /**< 4 MHz output. */
	CLOCK_MCHP_DIVIDER_3_69_MHZ, /**< 3.69 MHz output. */
	CLOCK_MCHP_DIVIDER_3_43_MHZ, /**< 3.43 MHz output. */
	CLOCK_MCHP_DIVIDER_3_2_MHZ,  /**< 3.2 MHz output. */
	CLOCK_MCHP_DIVIDER_3_MHZ     /**< 3 MHz output. */
};

/** @brief Internal 48 MHz oscillator (OSC48M) configuration. */
struct clock_mchp_subsys_osc48m_config {
	/** @brief Turn the oscillator ON when a peripheral requests it as a source. */
	bool on_demand_en;

	/** @brief Keep the oscillator ON in standby sleep mode, unless on_demand_en is set. */
	bool run_in_standby_en;

	/** @brief Control the oscillator frequency range by adjusting the division ratio. */
	enum clock_mchp_osc48m_divider_freq post_divider_freq;
};

/** @brief FDPLL source clocks. */
enum clock_mchp_fdpll_src_clock {
	CLOCK_MCHP_FDPLL_SRC_GCLK0,   /**< GCLK generator 0. */
	CLOCK_MCHP_FDPLL_SRC_GCLK1,   /**< GCLK generator 1. */
	CLOCK_MCHP_FDPLL_SRC_GCLK2,   /**< GCLK generator 2. */
	CLOCK_MCHP_FDPLL_SRC_GCLK3,   /**< GCLK generator 3. */
	CLOCK_MCHP_FDPLL_SRC_GCLK4,   /**< GCLK generator 4. */
	CLOCK_MCHP_FDPLL_SRC_GCLK5,   /**< GCLK generator 5. */
	CLOCK_MCHP_FDPLL_SRC_GCLK6,   /**< GCLK generator 6. */
	CLOCK_MCHP_FDPLL_SRC_GCLK7,   /**< GCLK generator 7. */
	CLOCK_MCHP_FDPLL_SRC_GCLK8,   /**< GCLK generator 8. */
	CLOCK_MCHP_FDPLL_SRC_XOSC32K, /**< 32 kHz external oscillator. */
	CLOCK_MCHP_FDPLL_SRC_XOSC,    /**< External oscillator. */

	CLOCK_MCHP_FDPLL_SRC_MAX = CLOCK_MCHP_FDPLL_SRC_XOSC /**< Highest valid source. */
};

/** @brief Fractional DPLL (FDPLL) configuration. */
struct clock_mchp_subsys_fdpll_config {
	/** @brief Turn the oscillator ON when a peripheral requests it as a source. */
	bool on_demand_en;

	/** @brief Keep the oscillator ON in standby sleep mode, unless on_demand_en is set. */
	bool run_in_standby_en;

	/** @brief Fractional part of the frequency multiplier (0 - 31). */
	uint32_t divider_ratio_frac;

	/** @brief Integer part of the frequency multiplier (0 - 4095). */
	uint32_t divider_ratio_int;

	/** @brief XOSC clock division factor (0 - 2047). */
	uint32_t xosc_clock_divider;

	/** @brief Reference source clock selection. */
	enum clock_mchp_fdpll_src_clock src;
};

/** @brief RTC source clocks. */
enum clock_mchp_rtc_src_clock {
	/** Ultra-low-power 1 kHz. */
	CLOCK_MCHP_RTC_SRC_ULP1K = OSC32KCTRL_RTCCTRL_RTCSEL_ULP1K,
	/** Ultra-low-power 32 kHz. */
	CLOCK_MCHP_RTC_SRC_ULP32K = OSC32KCTRL_RTCCTRL_RTCSEL_ULP32K,
	/** Internal 1 kHz. */
	CLOCK_MCHP_RTC_SRC_OSC1K = OSC32KCTRL_RTCCTRL_RTCSEL_OSC1K,
	/** Internal 32 kHz. */
	CLOCK_MCHP_RTC_SRC_OSC32K = OSC32KCTRL_RTCCTRL_RTCSEL_OSC32K,
	/** External 1 kHz. */
	CLOCK_MCHP_RTC_SRC_XOSC1K = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K,
	/** External 32 kHz. */
	CLOCK_MCHP_RTC_SRC_XOSC32K = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K
};

/** @brief RTC clock configuration. */
struct clock_mchp_subsys_rtc_config {
	/** @brief RTC source clock selection. */
	enum clock_mchp_rtc_src_clock src;
};

/** @brief 32 kHz external oscillator (XOSC32K) configuration. */
struct clock_mchp_subsys_xosc32k_config {
	/** @brief Turn the oscillator ON when a peripheral requests it as a source. */
	bool on_demand_en;

	/** @brief Keep the oscillator ON in standby sleep mode, unless on_demand_en is set. */
	bool run_in_standby_en;
};

/** @brief 32 kHz internal oscillator (OSC32K) configuration. */
struct clock_mchp_subsys_osc32k_config {
	/** @brief Turn the oscillator ON when a peripheral requests it as a source. */
	bool on_demand_en;

	/** @brief Keep the oscillator ON in standby sleep mode, unless on_demand_en is set. */
	bool run_in_standby_en;
};

/** @brief GCLK generator source clocks. */
enum clock_mchp_gclk_src_clock {
	CLOCK_MCHP_GCLK_SRC_XOSC,      /**< External oscillator. */
	CLOCK_MCHP_GCLK_SRC_GCLKPIN,   /**< GCLK input pin. */
	CLOCK_MCHP_GCLK_SRC_GCLKGEN1,  /**< GCLK generator 1. */
	CLOCK_MCHP_GCLK_SRC_OSCULP32K, /**< Ultra-low-power 32 kHz oscillator. */
	CLOCK_MCHP_GCLK_SRC_OSC32K,    /**< Internal 32 kHz oscillator. */
	CLOCK_MCHP_GCLK_SRC_XOSC32K,   /**< External 32 kHz oscillator. */
	CLOCK_MCHP_GCLK_SRC_OSC48M,    /**< Internal 48 MHz oscillator. */
	CLOCK_MCHP_GCLK_SRC_FDPLL,     /**< Fractional DPLL. */

	CLOCK_MCHP_GCLK_SRC_MAX = CLOCK_MCHP_GCLK_SRC_FDPLL /**< Highest valid source. */
};

/** @brief GCLK generator configuration. */
struct clock_mchp_subsys_gclkgen_config {
	/** @brief Division value for the generator. The actual division factor depends on the
	 * state of div_select (gclk1: 0 - 65535, others: 0 - 255).
	 */
	uint16_t div_factor;

	/** @brief Keep the generator running in standby sleep mode, unless on_demand_en is set. */
	bool run_in_standby_en;

	/** @brief Generator source clock selection. */
	enum clock_mchp_gclk_src_clock src;
};

/** @brief GCLK generator numbers. */
enum clock_mchp_gclkgen {
	CLOCK_MCHP_GCLKGEN_GEN0, /**< Generator 0. */
	CLOCK_MCHP_GCLKGEN_GEN1, /**< Generator 1. */
	CLOCK_MCHP_GCLKGEN_GEN2, /**< Generator 2. */
	CLOCK_MCHP_GCLKGEN_GEN3, /**< Generator 3. */
	CLOCK_MCHP_GCLKGEN_GEN4, /**< Generator 4. */
	CLOCK_MCHP_GCLKGEN_GEN5, /**< Generator 5. */
	CLOCK_MCHP_GCLKGEN_GEN6, /**< Generator 6. */
	CLOCK_MCHP_GCLKGEN_GEN7, /**< Generator 7. */
	CLOCK_MCHP_GCLKGEN_GEN8  /**< Generator 8. */
};

/** @brief Peripheral GCLK channel configuration. */
struct clock_mchp_subsys_gclkperiph_config {
	/** @brief GCLK generator that sources the peripheral clock. */
	enum clock_mchp_gclkgen src;
};

/** @brief Division ratio of the MCLK prescaler for the CPU. */
enum clock_mchp_mclk_cpu_div {
	CLOCK_MCHP_MCLK_CPU_DIV_1 = 1,     /**< Divide by 1. */
	CLOCK_MCHP_MCLK_CPU_DIV_2 = 2,     /**< Divide by 2. */
	CLOCK_MCHP_MCLK_CPU_DIV_4 = 4,     /**< Divide by 4. */
	CLOCK_MCHP_MCLK_CPU_DIV_8 = 8,     /**< Divide by 8. */
	CLOCK_MCHP_MCLK_CPU_DIV_16 = 16,   /**< Divide by 16. */
	CLOCK_MCHP_MCLK_CPU_DIV_32 = 32,   /**< Divide by 32. */
	CLOCK_MCHP_MCLK_CPU_DIV_64 = 64,   /**< Divide by 64. */
	CLOCK_MCHP_MCLK_CPU_DIV_128 = 128  /**< Divide by 128. */
};

/** @brief MCLK configuration structure.
 *
 * Used for CLOCK_MCHP_SUBSYS_TYPE_MCLKCPU.
 */
struct clock_mchp_subsys_mclkcpu_config {
	/** @brief Division ratio of the MCLK prescaler for the CPU. */
	enum clock_mchp_mclk_cpu_div division_factor;
};

/** @brief Clock rate datatype.
 *
 * Used for setting a clock rate.
 */
typedef uint32_t *clock_mchp_rate_t;

/** @} */

#endif /* INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_PIC32CM_JH_H_ */
