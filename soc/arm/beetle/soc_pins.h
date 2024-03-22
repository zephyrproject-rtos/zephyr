/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC Pin Out definition for the ARM LTD Beetle SoC.
 *
 */
#define PIN_UART_0_RX     0
#define PIN_UART_0_TX     1
#define PIN_SPI_0_CS      10
#define PIN_SPI_0_MOSI    11
#define PIN_SPI_0_MISO    12
#define PIN_SPI_0_SCLK    13
#define PIN_I2C_0_SCL     14
#define PIN_I2C_0_SDA     15
#define PIN_UART_1_RX     16
#define PIN_UART_1_TX     17
#define PIN_SPI_1_CS      18
#define PIN_SPI_1_MOSI    19
#define PIN_SPI_1_MISO    20
#define PIN_SPI_1_SCK     21
#define PIN_I2C_1_SDA     22
#define PIN_I2C_1_SCL     23

/* CMSDK APB Universal Asynchronous Receiver-Transmitter (UART) */
#define CMSDK_APB_UART_FUN_SEL 1
#define CMSDK_APB_UART_0_RX PIN_UART_0_RX
#define CMSDK_APB_UART_0_TX PIN_UART_0_TX
#define CMSDK_APB_UART_1_RX PIN_UART_1_RX
#define CMSDK_APB_UART_1_TX PIN_UART_1_TX
