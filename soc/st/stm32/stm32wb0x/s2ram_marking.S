/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * STM32WB0-specific support code for suspend-to-RAM
 */

#include <zephyr/toolchain.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/pm_s2ram.h>

/* Read RCC and PWRC base from Device Tree */
#include <zephyr/devicetree.h>

#define STM32WB0_RCC_CSR	(DT_REG_ADDR(DT_NODELABEL(rcc)) + 0x94)
#define STM32WB0_PWR_BASE	DT_REG_ADDR(DT_NODELABEL(pwrc))
#define STM32WB0_PWR_SR1	0x10
#define STM32WB0_PWR_SR3	0x38

_ASM_FILE_PROLOGUE

GTEXT(pm_s2ram_mark_set)
SECTION_FUNC(TEXT, pm_s2ram_mark_set)
	/*
	 * Managed by hardware - nothing to do here.
	 */
	bx lr

/**
 * @brief	Check whether SoC is waking up from Deepstop
 * @returns	1 if SoC is waking up from Deepstop, 0 otherwise.
 * @note	Registers are cleared by hardware upon reset, or
 * 		the SoC PM code layer upon entry in Deepstop, so
 *		this function does not clear the registers.
 */
GTEXT(pm_s2ram_mark_check_and_clear)
SECTION_FUNC(TEXT, pm_s2ram_mark_check_and_clear)
	/*
	 * Check for Deepstop exit on wakeup event:
	 *	- RCC_CSR is zero
	 *	- PWRC.EXTSRR has bit DEEPSTOPF set
	 *		(optional; RCC_CSR check suffices)
	 *	- Either PWRC_SR1 or PWRC_SR3 is non-zero
	 *
	 * Note that we don't have to clear any register since
	 * they are automatically updated on reset/wake-up.
	 */
	ldr	r0, =STM32WB0_RCC_CSR
	ldr	r0, [r0]
	cmp	r0, #0
	bne	not_deepstop_wakeup
	ldr	r0, =STM32WB0_PWR_BASE
	ldr	r1, [r0, #STM32WB0_PWR_SR1]
	ldr	r0, [r0, #STM32WB0_PWR_SR3]
	orrs	r0, r0, r1
	beq	not_deepstop_wakeup
	/* All conditions met: this is a wakeup from Deepstop. */
	movs	r0, #1
	bx	lr
not_deepstop_wakeup:
	movs	r0, #0
	bx	lr
