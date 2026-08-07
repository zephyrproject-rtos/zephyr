/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32H2_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32H2_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL      0U
#define ESP32_CPU_CLK_SRC_PLL       1U
#define ESP32_CLK_SRC_RC_FAST       2U
#define ESP32_CPU_CLK_SRC_FLASH_PLL 3U

/* Supported CPU frequencies */
#define ESP32_CLK_CPU_PLL_48M       48000000
#define ESP32_CLK_CPU_FLASH_PLL_64M 64000000
#define ESP32_CLK_CPU_PLL_96M       96000000
#define ESP32_CLK_CPU_RC_FAST_FREQ  8500000

/* Supported XTAL Frequencies */
#define ESP32_CLK_XTAL_32M 32000000

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 0
#define ESP32_RTC_FAST_CLK_SRC_XTAL_D2 1

/* Supported RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW 0
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K 1
#define ESP32_RTC_SLOW_CLK_SRC_RC32K   2
#define ESP32_RTC_SLOW_CLK_32K_EXT_OSC 9

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ 136000
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ 32768
#define ESP32_RTC_SLOW_CLK_SRC_RC32K_FREQ   32768

/* Shared module IDs - must match shared_periph_module_t enum in periph_defs.h
 * These are used by the clock control driver to identify peripheral modules.
 */
#define ESP32_TIMG0_MODULE               0 /**< Timer group 0 module */
#define ESP32_TIMG1_MODULE               1 /**< Timer group 1 module */
#define ESP32_UHCI0_MODULE               2 /**< UHCI0 module */
#define ESP32_SYSTIMER_MODULE            3 /**< System timer module */
/* Peripherals clock managed by the modem_clock driver must be listed last */
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
#define ESP32_USB_DEVICE_MODULE   103 /**< USB device module */
#define ESP32_I2C0_MODULE         104 /**< I2C0 module */
#define ESP32_I2C1_MODULE         105 /**< I2C1 module */
#define ESP32_I2S1_MODULE         106 /**< I2S1 module */
#define ESP32_RMT_MODULE          107 /**< RMT module */
#define ESP32_PCNT_MODULE         108 /**< PCNT module */
#define ESP32_SPI_MODULE          109 /**< SPI1 module */
#define ESP32_SPI2_MODULE         110 /**< SPI2 module */
#define ESP32_TWAI0_MODULE        111 /**< TWAI0 module */
#define ESP32_RNG_MODULE          112 /**< RNG module */
#define ESP32_GDMA_MODULE         113 /**< GDMA module */
#define ESP32_MCPWM0_MODULE       114 /**< MCPWM0 module */
#define ESP32_ETM_MODULE          115 /**< ETM module */
#define ESP32_PARLIO_MODULE       116 /**< Parallel IO module */
#define ESP32_TEMPSENSOR_MODULE   117 /**< Temperature sensor module */
#define ESP32_REGDMA_MODULE       118 /**< REGDMA module */
#define ESP32_SARADC_MODULE       119 /**< SAR ADC module */
#define ESP32_RSA_MODULE          120 /**< RSA module */
#define ESP32_AES_MODULE          121 /**< AES module */
#define ESP32_SHA_MODULE          122 /**< SHA module */
#define ESP32_ECC_MODULE          123 /**< ECC module */
#define ESP32_HMAC_MODULE         124 /**< HMAC module */
#define ESP32_DS_MODULE           125 /**< Digital signature module */
#define ESP32_ECDSA_MODULE        126 /**< ECDSA module */
#define ESP32_ASSIST_DEBUG_MODULE 127 /**< Assist debug module */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32H2_H_ */
