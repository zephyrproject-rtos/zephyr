/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/arch.h>
#include <zephyr/sys/util.h>
#include <mc_me.h>

/* SWT Startup Control Register */
#define SWT_STARTUP_CR                  0x404A8000
/* SWT Control Register bits */
#define SWT_CR_WEN                      BIT(0)

#define MCME_SWT_STARTUP_REQ            BIT(10)
#define MCME_SWT_STARTUP_COFB_IDX       1
#define MCME_SWT_STARTUP_PARTITION_IDX  2

/*
 * The SWT startup is enabled by default, with a timeout of 320 counter clocks.
 * Since the watchdog driver disables the watchdog too late, use the watchdog hook
 * to prevent resets.
 */
void swt_disable(void)
{
	uint32_t reg_val;

	mc_me_configure_cofb(MCME_SWT_STARTUP_PARTITION_IDX,
			     MCME_SWT_STARTUP_COFB_IDX, MCME_SWT_STARTUP_REQ);

	reg_val = sys_read32(SWT_STARTUP_CR);
	sys_write32(reg_val & ~SWT_CR_WEN, SWT_STARTUP_CR);
}
