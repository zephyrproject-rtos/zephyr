/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/arch.h>
#include <zephyr/sys/util.h>

/* MCME Register addresses */
#define MCME_CTL_KEY                0x40498000
#define MCME_PRTN2_PUPD             0x40498504
#define MCME_PRTN2_COFB1_CLKEN      0x40498534
#define MCME_PRTN2_COFB1_STAT       0x40498514

/* MCME bit definitions */
#define MCME_SWT_STARTUP_REQ        BIT(10)
#define MCME_PUPD_PCUD              BIT(0)
#define MCME_KEY                    0x5AF0
#define MCME_INV_KEY                0xA50F

/* SWT Startup Control Register */
#define SWT_STARTUP_CR              0x404A8000

/* SWT Control Register bits */
#define SWT_CR_WEN                  BIT(0)

/*
 * The SWT startup is enabled by default, with a timeout of 320 counter clocks.
 * Since the watchdog driver disables the watchdog too late, use the watchdog hook
 * to prevent resets.
 */
void z_arm_watchdog_init(void)
{
	uint32_t reg_val;

	sys_write32(sys_read32(MCME_PRTN2_COFB1_CLKEN) | MCME_SWT_STARTUP_REQ,
		    MCME_PRTN2_COFB1_CLKEN);

	sys_write32(sys_read32(MCME_PRTN2_PUPD) | MCME_PUPD_PCUD, MCME_PRTN2_PUPD);

	sys_write32(MCME_KEY, MCME_CTL_KEY);
	sys_write32(MCME_INV_KEY, MCME_CTL_KEY);

	while ((sys_read32(MCME_PRTN2_PUPD) & MCME_PUPD_PCUD)) {

	}

	while (!(sys_read32(MCME_PRTN2_COFB1_STAT) & MCME_SWT_STARTUP_REQ)) {

	}

	reg_val = sys_read32(SWT_STARTUP_CR);
	sys_write32(reg_val & ~SWT_CR_WEN, SWT_STARTUP_CR);
}
