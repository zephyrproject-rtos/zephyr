/*
 * Copyright (c) 2020 Mohamed ElShahawi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32_H_

/* System Clock Source */
#define ESP32_CLK_SRC_XTAL             0U
#define ESP32_CLK_SRC_PLL              1U
#define ESP32_CLK_SRC_RTC8M            2U

/* Supported CPU Frequencies */
#define ESP32_CLK_CPU_26M              26U
#define ESP32_CLK_CPU_40M              40U
#define ESP32_CLK_CPU_80M              80U
#define ESP32_CLK_CPU_160M             160U
#define ESP32_CLK_CPU_240M             240U

/* Supported XTAL Frequencies */
#define ESP32_CLK_XTAL_40M             0U
#define ESP32_CLK_XTAL_26M             1U

/* Modules IDs
 * These IDs are actually offsets in CLK and RST Control registers.
 * These IDs shouldn't be changed unless there is a Hardware change
 * from Espressif.
 *
 * Basic Modules
 * Registers: DPORT_PERIP_CLK_EN_REG, DPORT_PERIP_RST_EN_REG
 */
#define	ESP32_TIMERS_MODULE            0
#define	ESP32_SPI1_MODULE              1
#define	ESP32_UART0_MODULE             2
#define	ESP32_WDG_MODULE               3
#define	ESP32_I2S0_MODULE              4
#define	ESP32_UART1_MODULE             5
#define	ESP32_SPI2_MODULE              6
#define	ESP32_I2C_EXT0_MODULE          7
#define	ESP32_UHCI0_MODULE             8
#define	ESP32_RMT_MODULE               9
#define	ESP32_PCNT_MODULE              10
#define	ESP32_LEDC_MODULE              11
#define	ESP32_UHCI1_MODULE             12
#define	ESP32_TIMERGROUP_MODULE        13
#define	ESP32_EFUSE_MODULE             14
#define	ESP32_TIMERGROUP1_MODULE       15
#define	ESP32_SPI3_MODULE              16
#define	ESP32_PWM0_MODULE              17
#define	ESP32_I2C_EXT1_MODULE          18
#define	ESP32_CAN_MODULE               19
#define	ESP32_PWM1_MODULE              20
#define	ESP32_I2S1_MODULE              21
#define	ESP32_SPI_DMA_MODULE           22
#define	ESP32_UART2_MODULE             23
#define	ESP32_UART_MEM_MODULE          24
#define	ESP32_PWM2_MODULE              25
#define	ESP32_PWM3_MODULE              26

/* HW Security Modules
 * Registers: DPORT_PERI_CLK_EN_REG, DPORT_PERI_RST_EN_REG
 */
#define	ESP32_AES_MODULE               32
#define	ESP32_SHA_MODULE               33
#define	ESP32_RSA_MODULE               34
#define	ESP32_SECUREBOOT_MODULE	       35	/* Secure boot reset will hold SHA & AES in reset */
#define	ESP32_DIGITAL_SIGNATURE_MODULE 36      /* Digital signature reset will hold AES & RSA in reset */

/* WiFi/BT
 * Registers: DPORT_WIFI_CLK_EN_REG, DPORT_CORE_RST_EN_REG
 */
#define	ESP32_SDMMC_MODULE             64
#define	ESP32_SDIO_SLAVE_MODULE        65
#define	ESP32_EMAC_MODULE              66
#define	ESP32_RNG_MODULE               67
#define	ESP32_WIFI_MODULE              68
#define	ESP32_BT_MODULE                69
#define	ESP32_WIFI_BT_COMMON_MODULE    70
#define	ESP32_BT_BASEBAND_MODULE       71
#define	ESP32_BT_LC_MODULE             72

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ESP32_H_ */
