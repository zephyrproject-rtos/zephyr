/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZTN_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZTN_CLOCK_H_

/* RZ clock configuration values */
#define RZ_IP_MASK     0xFF0000UL
#define RZ_IP_SHIFT    16UL
#define RZ_IP_CH_MASK  0xFF00UL
#define RZ_IP_CH_SHIFT 8UL
#define RZ_CLOCK_MASK  0xFFUL
#define RZ_CLOCK_SHIFT 0UL

#define RZ_IP_BSC   0UL  /* Bus State Controller */
#define RZ_IP_XSPI  1UL  /* Expanded Serial Peripheral Interface */
#define RZ_IP_SCI   2UL  /* Serial Communications Interface */
#define RZ_IP_IIC   3UL  /* I2C Bus Interface */
#define RZ_IP_SPI   4UL  /* Serial Peripheral Interface */
#define RZ_IP_GPT   5UL  /* General PWM Timer */
#define RZ_IP_ADC12 6UL  /* 12-Bit A/D Converter */
#define RZ_IP_CMT   7UL  /* Compare Match Timer */
#define RZ_IP_CMTW  8UL  /* Compare Match Timer W */
#define RZ_IP_CANFD 9UL  /* Controller Area Network with Flexible Data Rate */
#define RZ_IP_GMAC  10UL /* Ethernet MAC */
#define RZ_IP_ETHSW 11UL /* Ethernet Switch */
#define RZ_IP_USBHS 12UL /* USB High Speed */
#define RZ_IP_RTC   13UL /* Real Time Clock */

#define RZ_CLOCK_CPU0       0UL
#define RZ_CLOCK_CPU1       1UL
#define RZ_CLOCK_CA55C0     2UL
#define RZ_CLOCK_CA55C1     3UL
#define RZ_CLOCK_CA55C2     4UL
#define RZ_CLOCK_CA55C3     5UL
#define RZ_CLOCK_CA55SCLK   6UL
#define RZ_CLOCK_ICLK       7UL
#define RZ_CLOCK_PCLKH      8UL
#define RZ_CLOCK_PCLKM      9UL
#define RZ_CLOCK_PCLKL      10UL
#define RZ_CLOCK_PCLKADC    11UL
#define RZ_CLOCK_PCLKGPTL   12UL
#define RZ_CLOCK_PCLKENCO   13UL
#define RZ_CLOCK_PCLKSPI0   14UL
#define RZ_CLOCK_PCLKSPI1   15UL
#define RZ_CLOCK_PCLKSPI2   16UL
#define RZ_CLOCK_PCLKSPI3   17UL
#define RZ_CLOCK_PCLKSCI0   18UL
#define RZ_CLOCK_PCLKSCI1   19UL
#define RZ_CLOCK_PCLKSCI2   20UL
#define RZ_CLOCK_PCLKSCI3   21UL
#define RZ_CLOCK_PCLKSCI4   22UL
#define RZ_CLOCK_PCLKSCI5   23UL
#define RZ_CLOCK_PCLKSCIE0  24UL
#define RZ_CLOCK_PCLKSCIE1  25UL
#define RZ_CLOCK_PCLKSCIE2  26UL
#define RZ_CLOCK_PCLKSCIE3  27UL
#define RZ_CLOCK_PCLKSCIE4  28UL
#define RZ_CLOCK_PCLKSCIE5  29UL
#define RZ_CLOCK_PCLKSCIE6  30UL
#define RZ_CLOCK_PCLKSCIE7  31UL
#define RZ_CLOCK_PCLKSCIE8  32UL
#define RZ_CLOCK_PCLKSCIE9  33UL
#define RZ_CLOCK_PCLKSCIE10 34UL
#define RZ_CLOCK_PCLKSCIE11 35UL
#define RZ_CLOCK_PCLKCAN    36UL
#define RZ_CLOCK_CKIO       37UL
#define RZ_CLOCK_XSPI0_CLK  38UL
#define RZ_CLOCK_XSPI1_CLK  39UL

#define RZ_CLOCK(IP, ch, clk)                                                                      \
	((IP << RZ_IP_SHIFT) | ((ch) << RZ_IP_CH_SHIFT) | ((clk) << RZ_CLOCK_SHIFT))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZTN_CLOCK_H_ */
