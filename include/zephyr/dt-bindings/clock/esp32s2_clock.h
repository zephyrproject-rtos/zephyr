/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32S2_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32S2_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL 0U
#define ESP32_CPU_CLK_SRC_PLL  1U
#define ESP32_CLK_SRC_RC_FAST  2U
#define ESP32_CLK_SRC_APLL_CLK 3U

/* Supported PLL CPU frequencies */
#define ESP32_CLK_CPU_PLL_80M     80000000
#define ESP32_CLK_CPU_PLL_160M    160000000
#define ESP32_CLK_CPU_PLL_240M    240000000
#define ESP32_CLK_CPU_RC_FAST_FREQ 8500000

/* Supported XTAL frequencies */
#define ESP32_CLK_XTAL_40M 40000000

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_XTAL_D4 0
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 1

/* Supported RTC slow clock sources */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW      0
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K      1
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256 2
#define ESP32_RTC_SLOW_CLK_32K_EXT_OSC      9

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ      90000
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ      32768
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256_FREQ 33203

/* Modules IDs
 * These are used by the clock control driver to identify peripheral modules.
 */
#define ESP32_TIMG0_MODULE           0  /**< Timer group 0 module */
#define ESP32_TIMG1_MODULE           1  /**< Timer group 1 module */
#define ESP32_UHCI0_MODULE           2  /**< UHCI0 module */
#define ESP32_SPI2_DMA_MODULE        3  /**< SPI2 DMA module */
#define ESP32_SPI3_DMA_MODULE        4  /**< SPI3 DMA module */
#define ESP32_RNG_MODULE             5  /**< RNG module */
#define ESP32_WIFI_MODULE            6  /**< Wi-Fi module */
#define ESP32_WIFI_BT_COMMON_MODULE  7  /**< Wi-Fi/BT common module */
#define ESP32_SYSTIMER_MODULE        8  /**< System timer module */
#define ESP32_PHY_CALIBRATION_MODULE 9  /**< PHY calibration module */
#define ESP32_MODULE_MAX             10 /**< Module count */

/* Non-shared peripherals - these have dedicated clock control in their drivers
 * and don't use periph_module_enable(). Values start after MODULE_MAX.
 */
#define ESP32_LEDC_MODULE          100 /**< LEDC module */
#define ESP32_UART0_MODULE         101 /**< UART0 module */
#define ESP32_UART1_MODULE         102 /**< UART1 module */
#define ESP32_USB_MODULE           103 /**< USB module */
#define ESP32_I2C0_MODULE          104 /**< I2C0 module */
#define ESP32_I2C1_MODULE          105 /**< I2C1 module */
#define ESP32_I2S0_MODULE          106 /**< I2S0 module */
#define ESP32_UHCI1_MODULE         107 /**< UHCI1 module */
#define ESP32_RMT_MODULE           108 /**< RMT module */
#define ESP32_PCNT_MODULE          109 /**< PCNT module */
#define ESP32_SPI_MODULE           110 /**< SPI1 module */
#define ESP32_FSPI_MODULE          111 /**< FSPI module */
#define ESP32_HSPI_MODULE          112 /**< HSPI module */
#define ESP32_TWAI_MODULE          113 /**< TWAI module */
#define ESP32_AES_MODULE           114 /**< AES module */
#define ESP32_SHA_MODULE           115 /**< SHA module */
#define ESP32_RSA_MODULE           116 /**< RSA module */
#define ESP32_CRYPTO_DMA_MODULE    117 /**< Crypto DMA module */
#define ESP32_AES_DMA_MODULE       118 /**< AES DMA module */
#define ESP32_SHA_DMA_MODULE       119 /**< SHA DMA module */
#define ESP32_DEDIC_GPIO_MODULE    120 /**< Dedicated GPIO module */
#define ESP32_PERIPH_SARADC_MODULE 121 /**< SAR ADC module */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32S2_H_ */
