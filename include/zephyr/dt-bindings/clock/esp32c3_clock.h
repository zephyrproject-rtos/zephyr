/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C3_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C3_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL 0U
#define ESP32_CPU_CLK_SRC_PLL  1U
#define ESP32_CLK_SRC_RC_FAST  2U

/* Supported CPU frequencies */
#define ESP32_CLK_CPU_PLL_80M     80000000
#define ESP32_CLK_CPU_PLL_160M    160000000
#define ESP32_CLK_CPU_RC_FAST_FREQ 17500000

/* Supported XTAL frequencies */
#define ESP32_CLK_XTAL_32M 32000000
#define ESP32_CLK_XTAL_40M 40000000

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_XTAL_D2 0
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 1

/* Supported RTC slow clock sources */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW      0
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K      1
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256 2
#define ESP32_RTC_SLOW_CLK_32K_EXT_OSC      9

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ      136000
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ      32768
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256_FREQ 68359

/* Modules IDs
 * These are used by the clock control driver to identify peripheral modules.
 */
#define ESP32_UART1_MODULE           0  /**< UART1 module */
#define ESP32_TIMG0_MODULE           1  /**< Timer group 0 module */
#define ESP32_TIMG1_MODULE           2  /**< Timer group 1 module */
#define ESP32_UHCI0_MODULE           3  /**< UHCI0 module */
#define ESP32_RNG_MODULE             4  /**< RNG module */
#define ESP32_WIFI_MODULE            5  /**< Wi-Fi module */
#define ESP32_BT_MODULE              6  /**< Bluetooth module */
#define ESP32_WIFI_BT_COMMON_MODULE  7  /**< Wi-Fi/BT common module */
#define ESP32_SYSTIMER_MODULE        8  /**< System timer module */
#define ESP32_PHY_CALIBRATION_MODULE 9  /**< PHY calibration module */
#define ESP32_MODULE_MAX             10 /**< Module count */

/* Non-shared peripherals - these have dedicated clock control in their drivers
 * and don't use periph_module_enable(). Values start after MODULE_MAX.
 */
#define ESP32_LEDC_MODULE        100 /**< LEDC module */
#define ESP32_UART0_MODULE       101 /**< UART0 module */
#define ESP32_USB_MODULE         102 /**< USB module */
#define ESP32_I2C0_MODULE        103 /**< I2C0 module */
#define ESP32_I2S1_MODULE        104 /**< I2S1 module */
#define ESP32_RMT_MODULE         105 /**< RMT module */
#define ESP32_SPI_MODULE         106 /**< SPI1 module */
#define ESP32_SPI2_MODULE        107 /**< SPI2 module */
#define ESP32_TWAI_MODULE        108 /**< TWAI module */
#define ESP32_BT_BASEBAND_MODULE 109 /**< BT baseband module */
#define ESP32_BT_LC_MODULE       110 /**< BT link controller module */
#define ESP32_RSA_MODULE         111 /**< RSA module */
#define ESP32_AES_MODULE         112 /**< AES module */
#define ESP32_SHA_MODULE         113 /**< SHA module */
#define ESP32_HMAC_MODULE        114 /**< HMAC module */
#define ESP32_DS_MODULE          115 /**< Digital signature module */
#define ESP32_GDMA_MODULE        116 /**< GDMA module */
#define ESP32_SARADC_MODULE      117 /**< SAR ADC module */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C3_H_ */
