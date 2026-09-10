/*
 * Copyright (c) 2020 Mohamed ElShahawi
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL 0U
#define ESP32_CPU_CLK_SRC_PLL  1U
#define ESP32_CLK_SRC_RC_FAST  2U
#define ESP32_CLK_SRC_APLL_CLK 3U

/* Supported PLL CPU frequencies */
#define ESP32_CLK_CPU_PLL_80M     80000000
#define ESP32_CLK_CPU_PLL_160M    160000000
#define ESP32_CLK_CPU_PLL_240M    240000000
#define ESP32_CLK_CPU_RC_FAST_FREQ 1062500

/* Supported XTAL frequencies */
#define ESP32_CLK_XTAL_24M             24000000
#define ESP32_CLK_XTAL_26M             26000000
#define ESP32_CLK_XTAL_40M             40000000

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_XTAL_D4 0
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 1

/* Supported RTC slow clock sources */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW      0
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K      1
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256 2
#define ESP32_RTC_SLOW_CLK_32K_EXT_OSC      9

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ      150000
#define ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ      32768
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256_FREQ 33203

/* Modules IDs
 * These are used by the clock control driver to identify peripheral modules.
 */
#define ESP32_UART1_MODULE           0  /**< UART1 module */
#define ESP32_UART2_MODULE           1  /**< UART2 module */
#define ESP32_I2S0_MODULE            2  /**< I2S0 module */
#define ESP32_TIMG0_MODULE           3  /**< Timer group 0 module */
#define ESP32_TIMG1_MODULE           4  /**< Timer group 1 module */
#define ESP32_UHCI0_MODULE           5  /**< UHCI0 module */
#define ESP32_SPI_MODULE             6  /**< SPI1 module */
#define ESP32_HSPI_MODULE            7  /**< HSPI module */
#define ESP32_VSPI_MODULE            8  /**< VSPI module */
#define ESP32_RNG_MODULE             9  /**< RNG module */
#define ESP32_WIFI_MODULE            10 /**< Wi-Fi module */
#define ESP32_BT_MODULE              11 /**< Bluetooth module */
#define ESP32_WIFI_BT_COMMON_MODULE  12 /**< Wi-Fi/BT common module */
#define ESP32_BT_BASEBAND_MODULE     13 /**< BT baseband module */
#define ESP32_PHY_CALIBRATION_MODULE 14 /**< PHY calibration module */
#define ESP32_MODULE_MAX             15 /**< Module count */

/* Non-shared peripherals - these have dedicated clock control in their drivers
 * and don't use periph_module_enable(). Values start after MODULE_MAX.
 */
#define ESP32_LEDC_MODULE       100               /**< LEDC module */
#define ESP32_UART0_MODULE      101               /**< UART0 module */
#define ESP32_I2C0_MODULE       102               /**< I2C0 module */
#define ESP32_I2C1_MODULE       103               /**< I2C1 module */
#define ESP32_I2S1_MODULE       104               /**< I2S1 module */
#define ESP32_PWM0_MODULE       105               /**< PWM0 module */
#define ESP32_PWM1_MODULE       106               /**< PWM1 module */
#define ESP32_UHCI1_MODULE      107               /**< UHCI1 module */
#define ESP32_RMT_MODULE        108               /**< RMT module */
#define ESP32_PCNT_MODULE       109               /**< PCNT module */
#define ESP32_SPI_DMA_MODULE    110               /**< SPI DMA module */
#define ESP32_SDMMC_MODULE      111               /**< SDMMC module */
#define ESP32_SDIO_SLAVE_MODULE 112               /**< SDIO slave module */
#define ESP32_TWAI_MODULE       113               /**< TWAI module */
#define ESP32_CAN_MODULE        ESP32_TWAI_MODULE /**< CAN module (alias) */
#define ESP32_EMAC_MODULE       114               /**< EMAC module */
#define ESP32_AES_MODULE        115               /**< AES module */
#define ESP32_SHA_MODULE        116               /**< SHA module */
#define ESP32_RSA_MODULE        117               /**< RSA module */
#define ESP32_SARADC_MODULE     118               /**< SAR ADC module */
#define ESP32_BT_LC_MODULE      119               /**< BT link controller module */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32_H_ */
