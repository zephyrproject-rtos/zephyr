/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ARM LTD Beetle SoC.
 *
 */

#ifndef _ARM_BEETLE_SOC_REGS_H_
#define _ARM_BEETLE_SOC_REGS_H_

/* System Control Register (SYSCON) */
struct syscon {
	/* Offset: 0x000 (r/w) remap control register */
	volatile uint32_t remap;
	/* Offset: 0x004 (r/w) pmu control register */
	volatile uint32_t pmuctrl;
	/* Offset: 0x008 (r/w) reset option register */
	volatile uint32_t resetop;
	/* Offset: 0x00c (r/w) emi control register */
	volatile uint32_t emictrl;
	/* Offset: 0x010 (r/w) reset information register */
	volatile uint32_t rstinfo;
	volatile uint32_t reserved0[3];
	/* Offset: 0x020 (r/w)AHB peripheral access control set */
	volatile uint32_t ahbper0set;
	/* Offset: 0x024 (r/w)AHB peripheral access control clear */
	volatile uint32_t ahbper0clr;
	volatile uint32_t reserved1[2];
	/* Offset: 0x030 (r/w)APB peripheral access control set */
	volatile uint32_t apbper0set;
	/* Offset: 0x034 (r/w)APB peripheral access control clear */
	volatile uint32_t apbper0clr;
	volatile uint32_t reserved2[2];
	/* Offset: 0x040 (r/w) main clock control register */
	volatile uint32_t mainclk;
	/* Offset: 0x044 (r/w) auxiliary / rtc control register */
	volatile uint32_t auxclk;
	/* Offset: 0x048 (r/w) pll control register */
	volatile uint32_t pllctrl;
	/* Offset: 0x04c (r/w) pll status register */
	volatile uint32_t pllstatus;
	/* Offset: 0x050 (r/w) sleep control register */
	volatile uint32_t sleepcfg;
	/* Offset: 0x054 (r/w) flash auxiliary settings control register */
	volatile uint32_t flashauxcfg;
	volatile uint32_t reserved3[10];
	/* Offset: 0x080 (r/w) AHB peripheral clock set in active state */
	volatile uint32_t ahbclkcfg0set;
	/* Offset: 0x084 (r/w) AHB peripheral clock clear in active state */
	volatile uint32_t ahbclkcfg0clr;
	/* Offset: 0x088 (r/w) AHB peripheral clock set in sleep state */
	volatile uint32_t ahbclkcfg1set;
	/* Offset: 0x08c (r/w) AHB peripheral clock clear in sleep state */
	volatile uint32_t ahbclkcfg1clr;
	/* Offset: 0x090 (r/w) AHB peripheral clock set in deep sleep state */
	volatile uint32_t ahbclkcfg2set;
	/* Offset: 0x094 (r/w) AHB peripheral clock clear in deep sleep state */
	volatile uint32_t ahbclkcfg2clr;
	volatile uint32_t reserved4[2];
	/* Offset: 0x0a0 (r/w) APB peripheral clock set in active state */
	volatile uint32_t apbclkcfg0set;
	/* Offset: 0x0a4 (r/w) APB peripheral clock clear in active state */
	volatile uint32_t apbclkcfg0clr;
	/* Offset: 0x0a8 (r/w) APB peripheral clock set in sleep state */
	volatile uint32_t apbclkcfg1set;
	/* Offset: 0x0ac (r/w) APB peripheral clock clear in sleep state */
	volatile uint32_t apbclkcfg1clr;
	/* Offset: 0x0b0 (r/w) APB peripheral clock set in deep sleep state */
	volatile uint32_t apbclkcfg2set;
	/* Offset: 0x0b4 (r/w) APB peripheral clock clear in deep sleep state */
	volatile uint32_t apbclkcfg2clr;
	volatile uint32_t reserved5[2];
	/* Offset: 0x0c0 (r/w) AHB peripheral reset select set */
	volatile uint32_t ahbprst0set;
	/* Offset: 0x0c4 (r/w) AHB peripheral reset select clear */
	volatile uint32_t ahbprst0clr;
	/* Offset: 0x0c8 (r/w) APB peripheral reset select set */
	volatile uint32_t apbprst0set;
	/* Offset: 0x0cc (r/w) APB peripheral reset select clear */
	volatile uint32_t apbprst0clr;
	/* Offset: 0x0d0 (r/w) AHB power down sleep wakeup source set */
	volatile uint32_t pwrdncfg0set;
	/* Offset: 0x0d4 (r/w) AHB power down sleep wakeup source clear */
	volatile uint32_t pwrdncfg0clr;
	/* Offset: 0x0d8 (r/w) APB power down sleep wakeup source set */
	volatile uint32_t pwrdncfg1set;
	/* Offset: 0x0dc (r/w) APB power down sleep wakeup source clear */
	volatile uint32_t pwrdncfg1clr;
	/* Offset: 0x0e0 ( /w) rtc reset */
	volatile uint32_t rtcreset;
	/* Offset: 0x0e4 (r/w) event interface control register */
	volatile uint32_t eventcfg;
	volatile uint32_t reserved6[2];
	/* Offset: 0x0f0 (r/w) sram power control overide */
	volatile uint32_t pwrovride0;
	/* Offset: 0x0f4 (r/w) embedded flash power control overide */
	volatile uint32_t pwrovride1;
	/* Offset: 0x0f8 (r/ ) memory status register */
	volatile uint32_t memorystatus;
	volatile uint32_t reserved7[1];
	/* Offset: 0x100 (r/w) io pad settings */
	volatile uint32_t gpiopadcfg0;
	/* Offset: 0x104 (r/w) io pad settings */
	volatile uint32_t gpiopadcfg1;
	/* Offset: 0x108 (r/w) testmode boot bypass */
	volatile uint32_t testmodecfg;
};

#endif /* _ARM_BEETLE_SOC_REGS_H_ */
