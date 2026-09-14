/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP32-C61 clock definitions for device tree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C61_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C61_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL 0U /**< CPU clock source: XTAL */
#define ESP32_CPU_CLK_SRC_PLL  1U /**< CPU clock source: PLL */
#define ESP32_CLK_SRC_RC_FAST  2U /**< CPU clock source: RC fast */

/* Supported CPU frequencies */
#define ESP32_CLK_CPU_PLL_80M      80000000  /**< CPU PLL clock 80 MHz */
#define ESP32_CLK_CPU_PLL_160M     160000000 /**< CPU PLL clock 160 MHz */
#define ESP32_CLK_CPU_RC_FAST_FREQ 17500000  /**< CPU RC fast clock 17.5 MHz */

/* Supported XTAL Frequencies */
#define ESP32_CLK_XTAL_40M 40000000 /**< XTAL clock 40 MHz */

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 0 /**< RTC fast clock source: RC fast */
#define ESP32_RTC_FAST_CLK_SRC_XTAL_D2 1 /**< RTC fast clock source: XTAL/2 */
#define ESP32_RTC_FAST_CLK_SRC_XTAL    2 /**< RTC fast clock source: XTAL */

/* Supported RTC slow clock sources */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW  0 /**< RTC slow clock source: RC slow */
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K  1 /**< RTC slow clock source: XTAL 32 KHz */
#define ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW 3 /**< RTC slow clock source: OSC slow */
#define ESP32_RTC_SLOW_CLK_32K_EXT_OSC  9 /**< RTC slow clock source: external 32 KHz OSC */

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ  136000 /**< RC slow clock frequency 136 KHz */
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ  32768  /**< XTAL 32 KHz clock frequency */
#define ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW_FREQ 32768  /**< OSC slow clock frequency 32.768 KHz */

/* Shared module IDs - must match shared_periph_module_t enum in periph_defs.h
 * These are used by the clock control driver to identify peripheral modules.
 */
#define ESP32_TIMG0_MODULE               0 /**< Timer group 0 module */
#define ESP32_TIMG1_MODULE               1 /**< Timer group 1 module */
#define ESP32_SYSTIMER_MODULE            2 /**< System timer module */
/* Peripherals clock managed by the modem_clock driver must be listed last */
#define ESP32_WIFI_MODULE                3  /**< Wi-Fi module */
#define ESP32_BT_MODULE                  4  /**< Bluetooth module */
#define ESP32_IEEE802154_MODULE          5  /**< IEEE 802.15.4 module */
#define ESP32_COEX_MODULE                6  /**< Coexistence module */
#define ESP32_PHY_MODULE                 7  /**< PHY module */
#define ESP32_ANA_I2C_MASTER_MODULE      8  /**< Analog I2C master module */
#define ESP32_MODEM_ETM_MODULE           9  /**< Modem ETM module */
#define ESP32_MODEM_ADC_COMMON_FE_MODULE 10 /**< Modem ADC common FE module */
#define ESP32_PHY_CALIBRATION_MODULE     11 /**< PHY calibration module */
#define ESP32_MODULE_MAX                 12 /**< Module count */

/* Non-shared peripherals - these have dedicated clock control in their drivers
 * and don't use periph_module_enable(). Values start at 100.
 */
#define ESP32_LEDC_MODULE         100 /**< LEDC module */
#define ESP32_UART0_MODULE        101 /**< UART0 module */
#define ESP32_UART1_MODULE        102 /**< UART1 module */
#define ESP32_UART2_MODULE        103 /**< UART2 module */
#define ESP32_USB_MODULE          104 /**< USB module */
#define ESP32_I2C0_MODULE         105 /**< I2C0 module */
#define ESP32_I2S0_MODULE         106 /**< I2S0 module */
#define ESP32_SPI_MODULE          107 /**< SPI1 module */
#define ESP32_SPI2_MODULE         108 /**< SPI2 module */
#define ESP32_RNG_MODULE          109 /**< RNG module */
#define ESP32_GDMA_MODULE         110 /**< GDMA module */
#define ESP32_ETM_MODULE          111 /**< ETM module */
#define ESP32_TEMPSENSOR_MODULE   112 /**< Temperature sensor module */
#define ESP32_SARADC_MODULE       113 /**< SAR ADC module */
#define ESP32_SHA_MODULE          116 /**< SHA module */
#define ESP32_ECC_MODULE          117 /**< ECC module */
#define ESP32_SDIO_SLAVE_MODULE   120 /**< SDIO slave module */
#define ESP32_ASSIST_DEBUG_MODULE 121 /**< Assist debug module */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C61_H_ */
