/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash/flash_ra2.h>
#include <soc.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define SYSC_RSTSR0	(SYSC_BASE + 0x410)
#define SYSC_RSTSR0_PORF	BIT(0)
#define SYSC_RSTSR0_LVD0RF	BIT(1)
#define SYSC_RSTSR0_LVD1RF	BIT(2)
#define SYSC_RSTSR0_LVD2RF	BIT(3)

#define SYSC_RSTSR1	(SYSC_BASE + 0x0C0)
#define SYSC_RSTSR1_IWDTRF	BIT(0)
#define SYSC_RSTSR1_WDTRF	BIT(1)
#define SYSC_RSTSR1_SWRF	BIT(2)
#define SYSC_RSTSR1_RPERF	BIT(8)
#define SYSC_RSTSR1_REERF	BIT(9)
#define SYSC_RSTSR1_BUSSRF	BIT(10)
#define SYSC_RSTSR1_BUSMRF	BIT(11)
#define SYSC_RSTSR1_SPERF	BIT(12)

#define SYSC_RSTSR2	(SYSC_BASE + 0x411)
#define SYSC_RSTSR2_CWSF	BIT(0)

#define SYSC_MEMWAIT	(SYSC_BASE + 0x031)
#define SYSC_MEMWAIT_MEMWAIT	BIT(0)

void set_memwait(bool enable)
{
	uint16_t old_prcr = get_register_protection();

	set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

	if (enable) {
		sys_write8(SYSC_MEMWAIT_MEMWAIT, SYSC_MEMWAIT);
		sys_write8(FLCN_FLDWAITR_FLDWAIT1, FLCN_FLDWAITR);
	} else {
		sys_write8(0, SYSC_MEMWAIT);
		sys_write8(0, FLCN_FLDWAITR);
	}
	set_register_protection(old_prcr);
}

bool get_memwait_state(void)
{
	return !!sys_read8(SYSC_MEMWAIT);
}

uint16_t init_reset_status(void)
{
	/* For the best debug info On RA2, use RCON to read the raw RSTSRx
	 * regs. For compat with the kernel reading 16-bits for reset flags,
	 * pack	the useful bits of RSTSRx into 16-bit
	 */
	uint16_t ret = sys_read8(SYSC_RSTSR0) &
				(SYSC_RSTSR0_PORF | SYSC_RSTSR0_LVD0RF |
				 SYSC_RSTSR0_LVD1RF | SYSC_RSTSR0_LVD2RF);
	uint16_t reg1 = sys_read16(SYSC_RSTSR1);

	ret |= (reg1 & (SYSC_RSTSR1_IWDTRF | SYSC_RSTSR1_WDTRF |
			SYSC_RSTSR1_SWRF)) << 4;

	ret |= reg1 & (SYSC_RSTSR1_RPERF | SYSC_RSTSR1_REERF |
			SYSC_RSTSR1_BUSSRF | SYSC_RSTSR1_BUSMRF |
			SYSC_RSTSR1_SPERF);

	ret |= ((uint16_t)sys_read8(SYSC_RSTSR2) & SYSC_RSTSR2_CWSF) << 13;

	sys_write8(0, SYSC_RSTSR0);
	sys_write16(0, SYSC_RSTSR1);

	/* Write to the Cold/Warm register for next boot */
	sys_write8(SYSC_RSTSR2_CWSF, SYSC_RSTSR2);

	return ret;
}

#if defined(CONFIG_PLATFORM_SPECIFIC_INIT)
void z_arm_platform_init(void)
{
	/*  Enable prefetch buffer */
	sys_write8(FLCN_PFBER_PFBE, FLCN_PFBER);
}
#endif
