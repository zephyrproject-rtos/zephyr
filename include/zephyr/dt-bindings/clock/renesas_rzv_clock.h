/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZV_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZV_CLOCK_H_

/* RZ/V clock configuration values */
#define RZ_IP_MASK         0xFF000000UL
#define RZ_IP_SHIFT        24UL
#define RZ_IP_CH_MASK      0xFF0000UL
#define RZ_IP_CH_SHIFT     16UL
#define RZ_CLOCK_MASK      0xFF00UL
#define RZ_CLOCK_SHIFT     8UL
#define RZ_CLOCK_DIV_MASK  0xFFUL
#define RZ_CLOCK_DIV_SHIFT 0UL

#define RZ_IP_GTM   0UL /* General Timer */
#define RZ_IP_GPT   1UL /* General PWM Timer */
#define RZ_IP_SCI   2UL /* Serial Communications Interface */
#define RZ_IP_SCIF  3UL /* Serial Communications Interface with FIFO */
#define RZ_IP_RIIC  4UL /* I2C Bus Interface */
#define RZ_IP_RSPI  5UL /* Renesas Serial Peripheral Interface */
#define RZ_IP_MHU   6UL /* Message Handling Unit */
#define RZ_IP_DMAC  7UL /* Direct Memory Access Controller */
#define RZ_IP_CANFD 8UL /* CANFD Interface (RS-CANFD) */
#if !defined(CONFIG_SOC_SERIES_RZV2L)
#define RZ_IP_ADC 10UL /* A/D Converter */
#define RZ_IP_WDT 11UL /* Watchdog Timer */
#endif

#define RZ_CLOCK_ICLK    0UL  /* Cortex-A55 Clock */
#define RZ_CLOCK_I2CLK   1UL  /* Cortex-M33 Clock */
#define RZ_CLOCK_GCLK    2UL  /* GPU Clock */
#define RZ_CLOCK_S0CLK   3UL  /* DDR-PHY Clock */
#define RZ_CLOCK_SPI0CLK 4UL  /* SPI0 Clock */
#define RZ_CLOCK_SPI1CLK 5UL  /* SPI1 Clock */
#define RZ_CLOCK_SD0CLK  6UL  /* SDH0 Clock */
#define RZ_CLOCK_SD1CLK  7UL  /* SDH1 Clock */
#define RZ_CLOCK_M0CLK   8UL  /* VCP, LCDC Clock */
#define RZ_CLOCK_M1CLK   9UL  /* MIPI-DSI, MIPI-CSI Clock */
#define RZ_CLOCK_M2CLK   10UL /* CRU, MIPI-DSI Clock */
#define RZ_CLOCK_M3CLK   11UL /* MIPI-DSI, LCDC Clock */
#define RZ_CLOCK_M4CLK   12UL /* MIPI-DSI Clock */
#define RZ_CLOCK_HPCLK   13UL /* Ethernet Clock */
#define RZ_CLOCK_TSUCLK  14UL /* TSU Clock */
#define RZ_CLOCK_ZTCLK   15UL /* JAUTH Clock */
#define RZ_CLOCK_P0CLK   16UL /* APB-BUS Clock */
#define RZ_CLOCK_P1CLK   17UL /* AXI-BUS Clock */
#define RZ_CLOCK_P2CLK   18UL /* P2CLK */
#define RZ_CLOCK_ATCLK   19UL /* ATCLK */
#define RZ_CLOCK_OSCCLK  20UL /* OSC Clock */

#define RZ_CLOCK(IP, ch, clk, div)                                                                 \
	((RZ_IP_##IP << RZ_IP_SHIFT) | ((ch) << RZ_IP_CH_SHIFT) | ((clk) << RZ_CLOCK_SHIFT) |      \
	 ((div) << RZ_CLOCK_DIV_SHIFT))

/**
 * Pack clock configurations in a 32-bit value
 * as expected for the Device Tree `clocks` property on Renesas RZ/V.
 *
 * @param ch Peripheral channel/unit
 */

/* GTM */
#define RZ_CLOCK_GTM(ch) RZ_CLOCK(GTM, ch, RZ_CLOCK_P0CLK, 1)

/* GPT */
#define RZ_CLOCK_GPT(ch) RZ_CLOCK(GPT, ch, RZ_CLOCK_P0CLK, 1)

/* SCI */
#define RZ_CLOCK_SCI(ch) RZ_CLOCK(SCI, ch, RZ_CLOCK_P0CLK, 1)

/* SCIF */
#define RZ_CLOCK_SCIF(ch) RZ_CLOCK(SCIF, ch, RZ_CLOCK_P0CLK, 1)

/* RIIC */
#define RZ_CLOCK_RIIC(ch) RZ_CLOCK(RIIC, ch, RZ_CLOCK_P0CLK, 1)

/* RSPI */
#define RZ_CLOCK_RSPI(ch) RZ_CLOCK(RSPI, ch, RZ_CLOCK_P0CLK, 1)

/* MHU */
#define RZ_CLOCK_MHU(ch) RZ_CLOCK(MHU, ch, RZ_CLOCK_P1CLK, 2)

/* DMAC */
#define RZ_CLOCK_DMAC(ch) RZ_CLOCK(DMAC, ch, RZ_CLOCK_P1CLK, 1)

/* CAN */
#define RZ_CLOCK_CANFD(ch) RZ_CLOCK(CANFD, ch, RZ_CLOCK_P0CLK, 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZV_CLOCK_H_ */
