/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C2_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C2_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL 0U
#define ESP32_CPU_CLK_SRC_PLL  1U
#define ESP32_CLK_SRC_RC_FAST  2U

/* Supported CPU frequencies */
#define ESP32_CLK_CPU_PLL_40M      40000000
#define ESP32_CLK_CPU_PLL_60M      60000000
#define ESP32_CLK_CPU_PLL_80M      80000000
#define ESP32_CLK_CPU_PLL_120M    120000000
#define ESP32_CLK_CPU_RC_FAST_FREQ  8750000

/* Supported XTAL frequencies */
#define ESP32_CLK_XTAL_26M 26000000
#define ESP32_CLK_XTAL_32M 32000000
#define ESP32_CLK_XTAL_40M 40000000

/* Supported RTC fast clock sources */
#define ESP32_RTC_FAST_CLK_SRC_XTAL_D2 0
#define ESP32_RTC_FAST_CLK_SRC_RC_FAST 1

/* Supported RTC slow clock sources */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW      0
#define ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW     1
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256 2

/* RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ      136000
#define ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW_FREQ      32768
#define ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256_FREQ  68359

/* Modules IDs
 * These IDs are actually offsets in CLK and RST Control registers.
 * These IDs shouldn't be changed unless there is a Hardware change
 * from Espressif.
 *
 * Basic Modules
 * Registers: DPORT_PERIP_CLK_EN_REG, DPORT_PERIP_RST_EN_REG
 */
#define ESP32_LEDC_MODULE               0
#define ESP32_UART0_MODULE              1
#define ESP32_UART1_MODULE              2
#define ESP32_I2C0_MODULE               3
#define ESP32_TIMG0_MODULE              4
#define ESP32_TIMG1_MODULE              5  /* No timg1 on esp32c2, TODO: IDF-3825 */
#define ESP32_UHCI0_MODULE              6
#define ESP32_SPI_MODULE                7  /* SPI1 */
#define ESP32_SPI2_MODULE               8  /* SPI2 */
#define ESP32_RNG_MODULE                9
#define ESP32_WIFI_MODULE               10
#define ESP32_BT_MODULE                 11
#define ESP32_WIFI_BT_COMMON_MODULE     12
#define ESP32_BT_BASEBAND_MODULE        13
#define ESP32_BT_LC_MODULE              14
#define ESP32_AES_MODULE                15
#define ESP32_SHA_MODULE                16
#define ESP32_ECC_MODULE                17
#define ESP32_GDMA_MODULE               18
#define ESP32_SYSTIMER_MODULE           19
#define ESP32_SARADC_MODULE             20
#define ESP32_TEMPSENSOR_MODULE         21
#define ESP32_MODEM_RPA_MODULE          22
#define ESP32_MODULE_MAX                23

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C2_H_ */
