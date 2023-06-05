/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C6_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C6_H_

/* System Clock Source */
#define ESP32_CLK_SRC_XTAL             0U
#define ESP32_CLK_SRC_PLL              1U
#define ESP32_CLK_SRC_RC_FAST          2U

/* Supported CPU Frequencies */
#define ESP32_CLK_CPU_80M              80000000
#define ESP32_CLK_CPU_160M             160000000

/* Supported XTAL Frequencies */
#define ESP32_CLK_XTAL_32M             32
#define ESP32_CLK_XTAL_40M             40

/* Supported RTC fast clock frequencies */
#define ESP32_RTC_FAST_CLK_FREQ_8M     8500000U
#define ESP32_RTC_FAST_CLK_FREQ_APPROX ESP32_RTC_FAST_CLK_FREQ_8M

/* Supported RTC slow clock frequencies */
#define ESP32_RTC_SLOW_CLK_FREQ_90K    90000U
#define ESP32_RTC_SLOW_CLK_FREQ_8MD256 (ESP32_RTC_FAST_CLK_FREQ_APPROX / 256)
#define ESP32_RTC_SLOW_CLK_FREQ_32K    32768U

/* Modules IDs
 * These IDs are actually offsets in CLK and RST Control registers.
 * These IDs shouldn't be changed unless there is a Hardware change
 * from Espressif.
 *
 * Basic Modules
 * Registers: DPORT_PERIP_CLK_EN_REG, DPORT_PERIP_RST_EN_REG
 */
#define ESP32_LEDC_MODULE              0
#define ESP32_UART0_MODULE             1
#define ESP32_UART1_MODULE             2
#define ESP32_USB_MODULE               3
#define ESP32_I2C0_MODULE              4
#define ESP32_I2S1_MODULE              5
#define ESP32_TIMG0_MODULE             6
#define ESP32_TIMG1_MODULE             7
#define ESP32_UHCI0_MODULE             8
#define ESP32_RMT_MODULE               9
#define ESP32_PCNT_MODULE             10
#define ESP32_SPI_MODULE              11
#define ESP32_SPI2_MODULE             12
#define ESP32_TWAI0_MODULE            13
#define ESP32_TWAI1_MODULE            14
#define ESP32_RNG_MODULE              15
#define ESP32_RSA_MODULE              16
#define ESP32_AES_MODULE              17
#define ESP32_SHA_MODULE              18
#define ESP32_ECC_MODULE              19
#define ESP32_HMAC_MODULE             20
#define ESP32_DS_MODULE               21
#define ESP32_SDIO_SLAVE_MODULE       22
#define ESP32_GDMA_MODULE             23
#define ESP32_MCPWM0_MODULE           24
#define ESP32_ETM_MODULE              25
#define ESP32_PARLIO_MODULE           26
#define ESP32_SYSTIMER_MODULE         27
#define ESP32_SARADC_MODULE           28
#define ESP32_TEMPSENSOR_MODULE       29
#define ESP32_REGDMA_MODULE           30
#define ESP32_LP_I2C0_MODULE          31
/* Peripherals clock managed by the modem_clock driver must be listed last */
#define ESP32_WIFI_MODULE             32
#define ESP32_BT_MODULE               33
#define ESP32_IEEE802154_MODULE       34
#define ESP32_COEX_MODULE             35
#define ESP32_PHY_MODULE              36
#define ESP32_MODULE_MAX              37

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C6_H_ */
