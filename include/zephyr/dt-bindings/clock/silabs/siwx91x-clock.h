/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock identifiers for Silicon Labs SiWx91x devices.
 * @ingroup clock_control_dt_silabs_siwx91x
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SILABS_SIWX91X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SILABS_SIWX91X_CLOCK_H_

/**
 * @defgroup clock_control_dt_silabs_siwx91x Silicon Labs SiWx91x clock identifiers
 * @ingroup clock_control_interface
 *
 * @details Clock identifiers for use with the SiWx91x AON/HP/ULP clock managers.
 * Reference clock names such as @c HP_REF_ULP mean an ULP reference sourced from
 * the HP domain.
 * @{
 */

/** @name HP (High Power) clocks */
/** @{ */
#define SIWX91X_CLK_UART0          0  /**< UART0 clock. */
#define SIWX91X_CLK_UART1          1  /**< UART1 clock. */
#define SIWX91X_CLK_I2C0           2  /**< I2C0 clock. */
#define SIWX91X_CLK_I2C1           3  /**< I2C1 clock. */
#define SIWX91X_CLK_UDMA           4  /**< UDMA clock. */
#define SIWX91X_CLK_PWM            5  /**< PWM clock. */
#define SIWX91X_CLK_GSPI           6  /**< GSPI clock. */
#define SIWX91X_CLK_QSPI           7  /**< QSPI clock. */
#define SIWX91X_CLK_QSPI2          8  /**< QSPI2 clock. */
#define SIWX91X_CLK_I2S            9  /**< I2S clock. */
#define SIWX91X_CLK_STATIC_I2S     10 /**< Static I2S external master-clock input. */
#define SIWX91X_CLK_GPDMA          11 /**< GPDMA clock. */
#define SIWX91X_CLK_RNG            12 /**< RNG clock. */
#define SIWX91X_CLK_GPIO           13 /**< GPIO clock. */
#define SIWX91X_CLK_SSI            14 /**< SSI clock. */
#define SIWX91X_CLK_PIN_OUT        15 /**< Pin clock output (OUT_CLK). */
#define SIWX91X_CLK_ICACHE         16 /**< Instruction cache clock. */

#define SIWX91X_CLK_CPU            17 /**< CPU / processor clock (PROC). */
#define SIWX91X_CLK_CPU_LP         18 /**< CPU low-power clock (MCU_LP); not supported. */

#define SIWX91X_CLK_HP_REF_ULP     19 /**< ULP reference from HP domain (HP_ULP). */
#define SIWX91X_CLK_HP_REF_PLL     20 /**< PLL reference clock (PLL_REF). */

#define SIWX91X_CLK_PLL_SOC        21 /**< SoC PLL (SOC_PLL). */
#define SIWX91X_CLK_PLL_INTF       22 /**< Interface PLL (INTF_PLL). */
#define SIWX91X_CLK_PLL_I2S        23 /**< I2S PLL (I2S_PLL). */
/** @} */

/** @name ULP (Ultra Low Power) clocks */
/** @{ */
#define SIWX91X_CLK_ULP_UART       24 /**< ULP UART clock. */
#define SIWX91X_CLK_ULP_I2C        25 /**< ULP I2C clock. */
#define SIWX91X_CLK_ULP_UDMA       26 /**< ULP UDMA clock. */
#define SIWX91X_CLK_ULP_I2S        27 /**< ULP I2S clock. */
#define SIWX91X_CLK_ULP_STATIC_I2S 28 /**< ULP static I2S external master-clock input. */
#define SIWX91X_CLK_ULP_ADC        29 /**< ULP ADC clock. */
#define SIWX91X_CLK_ULP_SSI        30 /**< ULP SSI clock. */
#define SIWX91X_CLK_ULP_TIMER      31 /**< ULP timer clock. */
#define SIWX91X_CLK_ULP_GPIO       32 /**< ULP GPIO clock. */

#define SIWX91X_CLK_ULP_REF_AON    33 /**< AON reference from ULP domain (SLP_SENSOR). */
#define SIWX91X_CLK_ULP_REF_CPU    34 /**< CPU reference from ULP domain (ULP_PROC). */
/** @} */

/** @name AON (Always-On) clocks */
/** @{ */
#define SIWX91X_CLK_XTAL_MHZ       35 /**< High-frequency crystal oscillator. */
#define SIWX91X_CLK_XTAL_KHZ       36 /**< Low-frequency crystal oscillator. */
#define SIWX91X_CLK_RC_MHZ         37 /**< High-frequency RC oscillator. */
#define SIWX91X_CLK_RC_KHZ         38 /**< Low-frequency RC oscillator. */

#define SIWX91X_CLK_WATCHDOG       39 /**< Watchdog clock. */
#define SIWX91X_CLK_RTC            40 /**< RTC clock. */
#define SIWX91X_CLK_SYSRTC         41 /**< SYSRTC clock. */
#define SIWX91X_CLK_UULP_GPIO      42 /**< UULP GPIO clock. */

#define SIWX91X_CLK_REF_HP         43 /**< HP domain reference (HP_REF). */
#define SIWX91X_CLK_REF_ULP        44 /**< ULP domain reference (ULP_REF). */
#define SIWX91X_CLK_AON_REF_HF     45 /**< AON high-frequency reference (UULP_HF_REF). */
#define SIWX91X_CLK_AON_REF_LF     46 /**< AON low-frequency reference (UULP_LF_REF). */
/** @} */

/** Gated / disabled clock source. */
#define SIWX91X_CLK_GATED          47

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SILABS_SIWX91X_CLOCK_H_ */
