/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZA_CLOCK_H_

/* RZ/A clock configuration values */
#define RZ_IP_MASK         0xFF000000UL
#define RZ_IP_SHIFT        24UL
#define RZ_IP_CH_MASK      0xFF0000UL
#define RZ_IP_CH_SHIFT     16UL
#define RZ_CLOCK_MASK      0xFF00UL
#define RZ_CLOCK_SHIFT     8UL
#define RZ_CLOCK_DIV_MASK  0xFFUL
#define RZ_CLOCK_DIV_SHIFT 0UL

#define RZ_IP_GTM   0UL /* General Timer */
#define RZ_IP_SCI   1UL /* Serial Communications Interface */
#define RZ_IP_SCIF  2UL /* Serial Communications Interface with FIFO */
#define RZ_IP_RIIC  3UL /* I2C Bus Interface */
#define RZ_IP_RSPI  4UL /* Renesas Serial Peripheral Interface */
#define RZ_IP_DMAC  5UL /* Direct Memory Access Controller */
#define RZ_IP_CANFD 6UL /* CANFD Interface (RS-CANFD) */
#define RZ_IP_ADC   7UL /* A/D Converter */
#define RZ_IP_WDT   8UL /* Watchdog Timer */

#define RZ_CLOCK_ICLK    0UL  /* Cortex-A55 Clock */
#define RZ_CLOCK_I2CLK   1UL  /* Cortex-M33 Clock */
#define RZ_CLOCK_S0CLK   2UL  /* DDR-PHY Clock */
#define RZ_CLOCK_SPI0CLK 3UL  /* SPI0 Clock */
#define RZ_CLOCK_SPI1CLK 4UL  /* SPI1 Clock */
#define RZ_CLOCK_OC0CLK  5UL  /* Octa0 Clock */
#define RZ_CLOCK_OC1CLK  6UL  /* Octa1 Clock */
#define RZ_CLOCK_SD0CLK  7UL  /* SDH0 Clock */
#define RZ_CLOCK_SD1CLK  8UL  /* SDH1 Clock */
#define RZ_CLOCK_M0CLK   9UL  /* VCP, LCDC Clock */
#define RZ_CLOCK_M2CLK   10UL /* CRU, MIPI-DSI Clock */
#define RZ_CLOCK_M3CLK   11UL /* MIPI-DSI, LCDC Clock */
#define RZ_CLOCK_HPCLK   12UL /* Ethernet Clock */
#define RZ_CLOCK_TSUCLK  13UL /* TSU Clock */
#define RZ_CLOCK_ZTCLK   14UL /* JAUTH Clock */
#define RZ_CLOCK_P0CLK   15UL /* APB-BUS Clock */
#define RZ_CLOCK_P1CLK   16UL /* AXI-BUS Clock */
#define RZ_CLOCK_P2CLK   17UL /* P2CLK */
#define RZ_CLOCK_ATCLK   18UL /* ATCLK */
#define RZ_CLOCK_OSCCLK  19UL /* OSC Clock */

#define RZ_CLOCK(IP, ch, clk, div)                                                                 \
	((RZ_IP_##IP << RZ_IP_SHIFT) | ((ch) << RZ_IP_CH_SHIFT) | ((clk) << RZ_CLOCK_SHIFT) |      \
	 ((div) << RZ_CLOCK_DIV_SHIFT))

/**
 * Pack clock configurations in a 32-bit value
 * as expected for the Device Tree `clocks` property on Renesas RZ/A.
 *
 * @param ch Peripheral channel/unit
 */

/* GTM */
#define RZ_CLOCK_GTM(ch) RZ_CLOCK(GTM, ch, RZ_CLOCK_P0CLK, 1)

/* SCI */
#define RZ_CLOCK_SCI(ch) RZ_CLOCK(SCI, ch, RZ_CLOCK_P0CLK, 1)

/* SCIF */
#define RZ_CLOCK_SCIF(ch) RZ_CLOCK(SCIF, ch, RZ_CLOCK_P0CLK, 1)

/* RIIC */
#define RZ_CLOCK_RIIC(ch) RZ_CLOCK(RIIC, ch, RZ_CLOCK_P0CLK, 1)

/* RSPI */
#define RZ_CLOCK_RSPI(ch) RZ_CLOCK(RSPI, ch, RZ_CLOCK_P0CLK, 1)

/* DMAC */
#define RZ_CLOCK_DMAC_NS(ch) RZ_CLOCK(DMAC, ch, RZ_CLOCK_P1CLK, 1)

/* CAN */
#define RZ_CLOCK_CANFD(ch) RZ_CLOCK(CANFD, ch, RZ_CLOCK_P0CLK, 1)

/* ADC */
#define RZ_CLOCK_ADC(ch) RZ_CLOCK(ADC, ch, RZ_CLOCK_P0CLK, 1)

/* WDT */
#define RZ_CLOCK_WDT(ch) RZ_CLOCK(WDT, ch, RZ_CLOCK_P0CLK, 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZA_CLOCK_H_ */
