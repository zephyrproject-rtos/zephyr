/*
 * Copyright (c) 2017 SiFive Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIFIVE_PRCI_H
#define _SIFIVE_PRCI_H

/* Clock controller. */
#define PRCI_BASE_ADDR 0x10008000UL

#define Z_REG32(p, i) (*(volatile uint32_t *) ((p) + (i)))
#define PRCI_REG(offset) Z_REG32(PRCI_BASE_ADDR, offset)

/* Register offsets */

#define PRCI_HFROSCCFG   (0x0000)
#define PRCI_HFXOSCCFG   (0x0004)
#define PRCI_PLLCFG      (0x0008)
#define PRCI_PLLDIV      (0x000C)
#define PRCI_PROCMONCFG  (0x00F0)

/* Fields */
#define ROSC_DIV(x)    (((x) & 0x2F) << 0)
#define ROSC_TRIM(x)   (((x) & 0x1F) << 16)
#define ROSC_EN(x)     (((x) & 0x1) << 30)
#define ROSC_RDY(x)    (((x) & 0x1) << 31)

#define XOSC_EN(x)     (((x) & 0x1) << 30)
#define XOSC_RDY(x)    (((x) & 0x1) << 31)

#define PLL_R(x)       (((x) & 0x7)  << 0)
/* single reserved bit for F LSB. */
#define PLL_F(x)       (((x) & 0x3F) << 4)
#define PLL_Q(x)       (((x) & 0x3)  << 10)
#define PLL_SEL(x)     (((x) & 0x1)  << 16)
#define PLL_REFSEL(x)  (((x) & 0x1)  << 17)
#define PLL_BYPASS(x)  (((x) & 0x1)  << 18)
#define PLL_LOCK(x)    (((x) & 0x1)  << 31)

#define PLL_R_default 0x1
#define PLL_F_default 0x1F
#define PLL_Q_default 0x3

#define PLL_REFSEL_HFROSC 0x0
#define PLL_REFSEL_HFXOSC 0x1

#define PLL_SEL_HFROSC 0x0
#define PLL_SEL_PLL    0x1

#define PLL_FINAL_DIV(x)      (((x) & 0x3F) << 0)
#define PLL_FINAL_DIV_BY_1(x) (((x) & 0x1) << 8)

#define PROCMON_DIV(x)   (((x) & 0x1F) << 0)
#define PROCMON_TRIM(x)  (((x) & 0x1F) << 8)
#define PROCMON_EN(x)    (((x) & 0x1)  << 16)
#define PROCMON_SEL(x)   (((x) & 0x3)  << 24)
#define PROCMON_NT_EN(x) (((x) & 0x1)  << 28)

#define PROCMON_SEL_HFCLK     0
#define PROCMON_SEL_HFXOSCIN  1
#define PROCMON_SEL_PLLOUTDIV 2
#define PROCMON_SEL_PROCMON   3

#endif /* _SIFIVE_PRCI_H */
