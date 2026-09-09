/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxa platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxa platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
#include <fsl_spc.h>
#include <fsl_cmc.h>
#define MCXA_CMC	((CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc)))
#define MCXA_SPC	((SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc)))
#endif

#ifdef CONFIG_SOC_MCXA_EARLY_SRAM_ECC_INIT
#include <stddef.h>
/* CMC Sticky System Reset Status (SSRS) register and its WAKEUP flag, set when
 * a reset was caused by a wakeup from Deep Power Down. Deep Power Down is how
 * PM_STATE_SUSPEND_TO_RAM and poweroff are entered, and it is the only
 * low-power state that wakes through the reset routine. The transparent S2RAM
 * resume keeps its marker and CPU context in this same first 8 KiB of SRAM, so
 * on a wakeup reset the block already holds valid, ECC-checked data and must be
 * left intact for arch_pm_s2ram_resume(). It is only cleared to initialize ECC
 * check bits on a cold boot.
 */
#define MCXA_CMC_SSRS_ADDR	(DT_REG_ADDR(DT_INST(0, nxp_cmc)) + offsetof(CMC_Type, SSRS))
#define MCXA_CMC_SSRS_WAKEUP	CMC_SSRS_WAKEUP_MASK

__attribute__((naked)) void soc_early_reset_hook(void)
{
	__asm__ volatile(
		"mov r1, pc\n"
		"tst r1, #0x24000000\n"
		"bne 2f\n"
		"ldr r0, =%c[ssrs]\n"
		"ldr r0, [r0]\n"
		"tst r0, %[wakeup]\n"
		"bne 2f\n"
		"ldr r0, =0x20000000\n"
		"ldr r1, =0x20002000\n"
		"movs r2, #0\n"
		"movs r3, #0\n"
		"movs r4, #0\n"
		"movs r5, #0\n"
		"1:\n"
		"stmia r0!, {r2-r5}\n"
		"cmp r0, r1\n"
		"bcc 1b\n"
		"2:\n"
		"bx lr\n"
		:
		: [ssrs] "i" (MCXA_CMC_SSRS_ADDR),
		  [wakeup] "i" (MCXA_CMC_SSRS_WAKEUP));
}
#endif

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
	if ((CMC_GetSystemResetStatus(MCXA_CMC) & kCMC_WakeUpReset) != 0UL) {
		SPC_ClearPeriphIOIsolationFlag(MCXA_SPC);
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXA_SPC, kSPC_PowerDomain0);
		SPC_ClearLowPowerRequest(MCXA_SPC);
	}
#endif
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	SystemInit();
#endif /* ! CONFIG_TRUSTED_EXECUTION_NONSECURE */
}
#endif

void enable_ecc(uint32_t mask)
{
	SYSCON->RAM_CTRL = mask;
}
