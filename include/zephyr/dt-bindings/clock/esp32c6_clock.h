/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C6_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C6_H_

/* Supported CPU clock Sources */
#define ESP32_CPU_CLK_SRC_XTAL 0U
#define ESP32_CPU_CLK_SRC_PLL  1U
#define ESP32_CLK_SRC_RC_FAST  2U

/* Supported CPU frequencies */
#define ESP32_CLK_CPU_PLL_80M      80000000
#define ESP32_CLK_CPU_PLL_160M     160000000
#define ESP32_CLK_CPU_RC_FAST_FREQ 17500000

/* Supported XTAL Frequencies */
#define ESP32_CLK_XTAL_32M 32000000
#define ESP32_CLK_XTAL_40M 40000000

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

/* Modules IDs
 * These IDs are actually offsets in CLK and RST Control registers.
 * These IDs shouldn't be changed unless there is a Hardware change
 * from Espressif.
 *
 * Basic Modules
 * Registers: DPORT_PERIP_CLK_EN_REG, DPORT_PERIP_RST_EN_REG
 */
#define ESP32_LEDC_MODULE                0
#define ESP32_UART0_MODULE               1
#define ESP32_UART1_MODULE               2
#define ESP32_USB_MODULE                 3
#define ESP32_I2C0_MODULE                4
#define ESP32_I2S1_MODULE                5
#define ESP32_TIMG0_MODULE               6
#define ESP32_TIMG1_MODULE               7
#define ESP32_UHCI0_MODULE               8
#define ESP32_RMT_MODULE                 9
#define ESP32_PCNT_MODULE                10
#define ESP32_SPI_MODULE                 11
#define ESP32_SPI2_MODULE                12
#define ESP32_TWAI0_MODULE               13
#define ESP32_TWAI1_MODULE               14
#define ESP32_RNG_MODULE                 15
#define ESP32_RSA_MODULE                 16
#define ESP32_AES_MODULE                 17
#define ESP32_SHA_MODULE                 18
#define ESP32_ECC_MODULE                 19
#define ESP32_HMAC_MODULE                20
#define ESP32_DS_MODULE                  21
#define ESP32_SDIO_SLAVE_MODULE          22
#define ESP32_GDMA_MODULE                23
#define ESP32_MCPWM0_MODULE              24
#define ESP32_ETM_MODULE                 25
#define ESP32_PARLIO_MODULE              26
#define ESP32_SYSTIMER_MODULE            27
#define ESP32_SARADC_MODULE              28
#define ESP32_TEMPSENSOR_MODULE          29
#define ESP32_ASSIST_DEBUG_MODULE        30
/* LP peripherals */
#define ESP32_LP_I2C0_MODULE             31
#define ESP32_LP_UART0_MODULE            32
/* Peripherals clock managed by the modem_clock driver must be listed last */
#define ESP32_WIFI_MODULE                33
#define ESP32_BT_MODULE                  34
#define ESP32_IEEE802154_MODULE          35
#define ESP32_COEX_MODULE                36
#define ESP32_PHY_MODULE                 37
#define ESP32_ANA_I2C_MASTER_MODULE      38
#define ESP32_MODEM_ETM_MODULE           39
#define ESP32_MODEM_ADC_COMMON_FE_MODULE 40
#define ESP32_MODULE_MAX                 41

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32C6_H_ */
