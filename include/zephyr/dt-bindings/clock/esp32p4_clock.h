/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock module identifiers for Espressif ESP32-P4
 *
 * Defines clock source and peripheral module IDs for use in
 * devicetree clock bindings.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32P4_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32P4_H_

/* Supported CPU clock Sources
 * Derived from components/soc/esp32p4/include/soc/clk_tree_defs.h
 */
#define ESP32_CPU_CLK_SRC_XTAL 0U /**< CPU clock source: XTAL */
#define ESP32_CPU_CLK_SRC_PLL  1U /**< CPU clock source: PLL */
#define ESP32_CLK_SRC_RC_FAST  2U /**< CPU clock source: RC fast */

/* Supported CPU frequencies (from CPLL 400MHz, ECO5+) */
#define ESP32_CLK_CPU_PLL_100M     100000000 /**< CPU PLL clock 100 MHz */
#define ESP32_CLK_CPU_PLL_200M     200000000 /**< CPU PLL clock 200 MHz */
#define ESP32_CLK_CPU_PLL_400M     400000000 /**< CPU PLL clock 400 MHz */
#define ESP32_CLK_CPU_RC_FAST_FREQ 17500000  /**< CPU RC fast clock 17.5 MHz */

/* Supported XTAL Frequency */
#define ESP32_CLK_XTAL_40M 40000000 /**< XTAL clock 40 MHz */

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 0 /**< RTC fast clock source: RC fast */
#define ESP32_RTC_FAST_CLK_SRC_XTAL    1 /**< RTC fast clock source: XTAL */
#define ESP32_RTC_FAST_CLK_SRC_LP_PLL  2 /**< RTC fast clock source: LP PLL */

/* RTC fast clock frequencies */
#define ESP32_RTC_FAST_CLK_SRC_LP_PLL_FREQ 8000000 /**< LP PLL clock frequency 8 MHz */

/* Supported RTC slow clock sources */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW 0 /**< RTC slow clock source: RC slow */
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K 1 /**< RTC slow clock source: XTAL 32 KHz */
#define ESP32_RTC_SLOW_CLK_SRC_RC32K   2 /**< RTC slow clock source: RC 32 KHz */
#define ESP32_RTC_SLOW_CLK_32K_EXT_OSC 9 /**< RTC slow clock source: external 32 KHz OSC */

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ 136000 /**< RC slow clock frequency 136 KHz */
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ 32768  /**< XTAL 32 KHz clock frequency */
#define ESP32_RTC_SLOW_CLK_SRC_RC32K_FREQ   32768  /**< RC 32 KHz clock frequency */

/* Shared module IDs - must match shared_periph_module_t enum in periph_defs.h */
#define ESP32_TIMG0_MODULE    1 /**< Timer group 0 module */
#define ESP32_TIMG1_MODULE    2 /**< Timer group 1 module */
#define ESP32_SYSTIMER_MODULE 3 /**< System timer module */
#define ESP32_MODULE_MAX      4 /**< Shared module count */

/* Non-shared peripherals - values start at 100 */
#define ESP32_LEDC_MODULE         100 /**< LEDC module */
#define ESP32_UART0_MODULE        101 /**< UART0 module */
#define ESP32_UART1_MODULE        102 /**< UART1 module */
#define ESP32_UART2_MODULE        103 /**< UART2 module */
#define ESP32_UART3_MODULE        104 /**< UART3 module */
#define ESP32_UART4_MODULE        105 /**< UART4 module */
#define ESP32_USB_MODULE          106 /**< USB module */
#define ESP32_I2C0_MODULE         107 /**< I2C0 module */
#define ESP32_I2C1_MODULE         108 /**< I2C1 module */
#define ESP32_I2S0_MODULE         109 /**< I2S0 module */
#define ESP32_I2S1_MODULE         110 /**< I2S1 module */
#define ESP32_I2S2_MODULE         111 /**< I2S2 module */
#define ESP32_RMT_MODULE          112 /**< RMT module */
#define ESP32_PCNT_MODULE         113 /**< PCNT module */
#define ESP32_SPI2_MODULE         114 /**< SPI2 module */
#define ESP32_SPI3_MODULE         115 /**< SPI3 module */
#define ESP32_TWAI0_MODULE        116 /**< TWAI0 module */
#define ESP32_TWAI1_MODULE        117 /**< TWAI1 module */
#define ESP32_TWAI2_MODULE        118 /**< TWAI2 module */
#define ESP32_RNG_MODULE          119 /**< RNG module */
#define ESP32_GDMA_MODULE         120 /**< GDMA module */
#define ESP32_MCPWM0_MODULE       121 /**< MCPWM0 module */
#define ESP32_MCPWM1_MODULE       122 /**< MCPWM1 module */
#define ESP32_ETM_MODULE          123 /**< ETM module */
#define ESP32_PARLIO_MODULE       124 /**< Parallel IO module */
#define ESP32_TEMPSENSOR_MODULE   125 /**< Temperature sensor module */
#define ESP32_SARADC_MODULE       126 /**< SAR ADC module */
#define ESP32_RSA_MODULE          127 /**< RSA module */
#define ESP32_AES_MODULE          128 /**< AES module */
#define ESP32_SHA_MODULE          129 /**< SHA module */
#define ESP32_ECC_MODULE          130 /**< ECC module */
#define ESP32_HMAC_MODULE         131 /**< HMAC module */
#define ESP32_DS_MODULE           132 /**< Digital signature module */
#define ESP32_ASSIST_DEBUG_MODULE 133 /**< Assist debug module */
#define ESP32_EMAC_MODULE         134 /**< Ethernet MAC module */
#define ESP32_USB_OTG_MODULE      135 /**< USB OTG module */
#define ESP32_SDMMC_MODULE        136 /**< SDMMC module */
/* LP peripherals */
#define ESP32_LP_I2C0_MODULE      137 /**< LP I2C0 module */
#define ESP32_LP_UART0_MODULE     138 /**< LP UART0 module */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32P4_H_ */
