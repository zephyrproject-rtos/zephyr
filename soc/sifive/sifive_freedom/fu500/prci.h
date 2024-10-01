/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIFIVE_FU540_PRCI_H
#define _SIFIVE_FU540_PRCI_H

/* Clock controller. */
#define PRCI_BASE_ADDR 0x10000000UL

#define Z_REG32(p, i) (*(volatile uint32_t *) ((p) + (i)))
#define PRCI_REG(offset) Z_REG32(PRCI_BASE_ADDR, offset)

/* Register offsets */

#define PRCI_HFXOSCCFG          (0x0000)
#define PRCI_COREPLLCFG0        (0x0004)
#define PRCI_DDRPLLCFG0         (0x000c)
#define PRCI_DDRPLLCFG1         (0x0010)
#define PRCI_GEMGXLPLLCFG0      (0x001c)
#define PRCI_GEMGXLPLLCFG1      (0x0020)
#define PRCI_CORECLKSEL         (0x0024)
#define PRCI_DEVICESRESETREG    (0x0028)

#define PLL_R(x)       (((x) & 0x3f)  << 0)
#define PLL_F(x)       (((x) & 0x1ff) << 6)
#define PLL_Q(x)       (((x) & 0x7)   << 15)
#define PLL_RANGE(x)   (((x) & 0x7)   << 18)
#define PLL_BYPASS(x)  (((x) & 0x1)   << 24)
#define PLL_FSE(x)     (((x) & 0x1)   << 25)
#define PLL_LOCK(x)    (((x) & 0x1)   << 31)

#define PLL_RANGE_33MHZ        4
#define PLL_BYPASS_DISABLE     0
#define PLL_BYPASS_ENABLE      1
#define PLL_FSE_INTERNAL       1

#define CORECLKSEL_CORECLKSEL(x)    (((x) & 0x1)  << 0)

#define CORECLKSEL_CORE_PLL    0
#define CORECLKSEL_HFCLK       1

#endif /* _SIFIVE_FU540_PRCI_H */
