/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZG_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZG_CLOCK_H_

/** RZ clock configuration values */
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
#define RZ_IP_SCIF  2UL /* Serial Communications Interface with FIFO */
#define RZ_IP_RIIC  3UL /* I2C Bus Interface */
#define RZ_IP_RSPI  4UL /* Renesas Serial Peripheral Interface */
#define RZ_IP_MHU   5UL /* Message Handling Unit */
#define RZ_IP_DMAC  6UL /* Direct Memory Access Controller */
#define RZ_IP_CANFD 7UL /* CANFD Interface (RS-CANFD) */
#define RZ_IP_ADC   8UL /* A/D Converter */

#define RZ_CLOCK_ICLK    0UL  /* Cortex-A55 Clock */
#define RZ_CLOCK_I2CLK   1UL  /* Cortex-M33 Clock */
#define RZ_CLOCK_I3CLK   2UL  /* Cortex-M33 FPU Clock */
#define RZ_CLOCK_S0CLK   3UL  /* DDR-PHY Clock */
#define RZ_CLOCK_OC0CLK  4UL  /* OCTA0 Clock */
#define RZ_CLOCK_OC1CLK  5UL  /* OCTA1 Clock */
#define RZ_CLOCK_SPI0CLK 6UL  /* SPI0 Clock */
#define RZ_CLOCK_SPI1CLK 7UL  /* SPI1 Clock */
#define RZ_CLOCK_SD0CLK  8UL  /* SDH0 Clock */
#define RZ_CLOCK_SD1CLK  9UL  /* SDH1 Clock */
#define RZ_CLOCK_SD2CLK  10UL /* SDH2 Clock */
#define RZ_CLOCK_M0CLK   11UL /* VCP LCDC Clock */
#define RZ_CLOCK_HPCLK   12UL /* Ethernet Clock */
#define RZ_CLOCK_TSUCLK  13UL /* TSU Clock */
#define RZ_CLOCK_ZTCLK   14UL /* JAUTH Clock */
#define RZ_CLOCK_P0CLK   15UL /* APB-BUS Clock */
#define RZ_CLOCK_P1CLK   16UL /* AXI-BUS Clock */
#define RZ_CLOCK_P2CLK   17UL /* P2CLK */
#define RZ_CLOCK_P3CLK   18UL /* P3CLK */
#define RZ_CLOCK_P4CLK   19UL /* P4CLK */
#define RZ_CLOCK_P5CLK   20UL /* P5CLK */
#define RZ_CLOCK_ATCLK   21UL /* ATCLK */
#define RZ_CLOCK_OSCCLK  22UL /* OSC Clock */
#define RZ_CLOCK_OSCCLK2 23UL /* OSC2 Clock */

#define RZ_CLOCK(IP, ch, clk, div)                                                                 \
	((RZ_IP_##IP << RZ_IP_SHIFT) | ((ch) << RZ_IP_CH_SHIFT) | ((clk) << RZ_CLOCK_SHIFT) |      \
	 ((div) << RZ_CLOCK_DIV_SHIFT))

/**
 * Pack clock configurations in a 32-bit value
 * as expected for the Device Tree `clocks` property on Renesas RZ/G.
 *
 * @param ch Peripheral channel/unit
 */

/* SCIF */
#define RZ_CLOCK_SCIF(ch) RZ_CLOCK(SCIF, ch, RZ_CLOCK_P0CLK, 1)

/* GPT */
#define RZ_CLOCK_GPT(ch) RZ_CLOCK(GPT, ch, RZ_CLOCK_P0CLK, 1)

/* MHU */
#define RZ_CLOCK_MHU(ch) RZ_CLOCK(MHU, ch, RZ_CLOCK_P1CLK, 2)

/* ADC */
#define RZ_CLOCK_ADC(ch) RZ_CLOCK(ADC, ch, RZ_CLOCK_TSUCLK, 1)

/* RIIC */
#define RZ_CLOCK_RIIC(ch) RZ_CLOCK(RIIC, ch, RZ_CLOCK_P0CLK, 1)

/* GTM */
#define RZ_CLOCK_GTM(ch) RZ_CLOCK(GTM, ch, RZ_CLOCK_P0CLK, 1)

/* CAN */
#define RZ_CLOCK_CANFD(ch) RZ_CLOCK(CANFD, ch, RZ_CLOCK_P4CLK, 2)

/* RSPI */
#define RZ_CLOCK_RSPI(ch) RZ_CLOCK(RSPI, ch, RZ_CLOCK_P0CLK, 1)

/* DMAC */
#define RZ_CLOCK_DMAC(ch) RZ_CLOCK(DMAC, ch, RZ_CLOCK_P3CLK, 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RENESAS_RZG_CLOCK_H_ */
