/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIFIVE_FU740_PRCI_H
#define _SIFIVE_FU740_PRCI_H

#define Z_REG32(p, i) (*(volatile uint32_t *) ((p) + (i)))
#define PRCI_REG(offset) Z_REG32(PRCI_BASE_ADDR, offset)

/* Register offsets */

#define PRCI_HFXOSCCFG          (0x0000)
#define PRCI_COREPLLCFG         (0x0004)
#define PRCI_COREPLLOUTDIV      (0x0008)
#define PRCI_DDRPLLCFG          (0x000c)
#define PRCI_DDRPLLOUTDIV       (0x0010)
#define PRCI_GEMGXLPLLCFG       (0x001c)
#define PRCI_GEMGXLPLLOUTDIV    (0x0020)
#define PRCI_CORECLKSEL         (0x0024)
#define PRCI_DEVICESRESETN      (0x0028)
#define PRCI_CLKMUXSTATUS       (0x002c)
#define PRCI_COREPLLSEL         (0x0040)
#define PRCI_HFPCLKPLLCFG       (0x0050)
#define PRCI_HFPCLKPLLOUTDIV    (0x0054)
#define PRCI_HFPCLKPLLSEL       (0x0058)

#define PLL_R(x)       (((x) & 0x3f)  << 0)
#define PLL_F(x)       (((x) & 0x1ff) << 6)
#define PLL_Q(x)       (((x) & 0x7)   << 15)
#define PLL_RANGE(x)   (((x) & 0x7)   << 18)
#define PLL_BYPASS(x)  (((x) & 0x1)   << 24)
#define PLL_FSE(x)     (((x) & 0x1)   << 25)
#define PLL_LOCK(x)    (((x) & 0x1)   << 31)

#define PLL_RANGE_RESET        0
#define PLL_RANGE_0MHZ         1
#define PLL_RANGE_11MHZ        2
#define PLL_RANGE_18MHZ        3
#define PLL_RANGE_30MHZ        4
#define PLL_RANGE_50MHZ        5
#define PLL_RANGE_80MHZ        6
#define PLL_RANGE_130MHZ       7
#define PLL_BYPASS_DISABLE     0
#define PLL_BYPASS_ENABLE      1
#define PLL_FSE_INTERNAL       1

#define OUTDIV_PLLCKE(x)    (((x) & 0x1) << 31)

#define OUTDIV_PLLCKE_DIS    0
#define OUTDIV_PLLCKE_ENA    1

#define CLKSEL_SEL(x)    (((x) & 0x1)  << 0)

#define CLKSEL_PLL      0
#define CLKSEL_HFCLK    1

#define CLKMUXSTATUS_CORECLKPLLSEL_OFF    0
#define CLKMUXSTATUS_TLCLKSEL_OFF         1
#define CLKMUXSTATUS_RTCXSEL_OFF          2
#define CLKMUXSTATUS_DDRCTRLCLKSEL_OFF    3
#define CLKMUXSTATUS_DDRPHYCLKSEL_OFF     4
#define CLKMUXSTATUS_GEMGXLCLKSEL_OFF     6
#define CLKMUXSTATUS_MAINMEMCLKSEL_OFF    7

#define COREPLLSEL_SEL(x)    (((x) & 0x1)  << 0)

#define COREPLLSEL_COREPLL        0
#define COREPLLSEL_DVFSCOREPLL    1

#endif /* _SIFIVE_FU740_PRCI_H */
