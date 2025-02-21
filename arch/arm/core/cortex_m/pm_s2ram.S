/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M suspend-to-RAM code (S2RAM)
 */

#include <zephyr/toolchain.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/pm_s2ram.h>

/**
 * Macro expanding to an integer literal equal to the offset of
 * field `sr_name` in `struct __cpu_context`. This macro has to
 * be implemented in C, because GEN_OFFSET_SYM provides offsets
 * as C preprocessor definitions - there are not visible to the
 * assembler.
 *
 * See also: `arch/arm/core/offsets/offsets_aarch32.c`
 */
#define CPU_CTX_SR_OFFSET(sr_name) \
	___cpu_context_t_ ## sr_name ## _OFFSET

/**
 * Macros used to save / load a special register in __cpu_context.
 * These also have to be implemented in C due to CPU_CTX_SR_OFFSET.
 */
#define SAVE_SPECIAL_REG(sr_name, cpu_ctx_reg, tmp_reg)	\
	mrs	tmp_reg, sr_name;			\
	str	tmp_reg, [cpu_ctx_reg, # CPU_CTX_SR_OFFSET(sr_name)];

#define RESTORE_SPECIAL_REG(sr_name, cpu_ctx_reg, tmp_reg)		\
	ldr	tmp_reg, [cpu_ctx_reg, # CPU_CTX_SR_OFFSET(sr_name)];	\
	msr	sr_name, tmp_reg;

/*
 * The following macros could be written as assembler macros, but C is used
 * for portability (assembler macro syntax may differ between toolchains).
 */

/*
 * Pushes registers r4~r12 and lr on the stack.
 * r0 is unmodified but other GPRs may be overwritten.
 */
#if !defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* `push` on ARMv6-M / ARMv8-M Baseline:
 * only r0~r7 and lr may be pushed
 */
#define PUSH_GPRS							\
	push	{r4-r7};						\
	mov	r1, r8;							\
	mov	r2, r9;							\
	mov	r3, r10;						\
	mov	r4, r11;						\
	mov	r5, r12;						\
	push	{r1-r5, lr}
#else
/* `push` on ARMv7-M and ARMv8-M Mainline: no limitation */
#define PUSH_GPRS							\
	push	{r4-r12, lr}
#endif /* !CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

/*
 * Pops registers r4~r12 and lr from the stack
 * r0 is unmodified but other GPRs may be overwritten.
 */
#if !defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* `pop` on ARMv6-M / ARMv8-M Baseline:
 * can only pop to r0~r7 and pc (not lr!)
 */
#define POP_GPRS							\
	pop	{r1-r6};						\
	mov 	lr, r6;							\
	mov 	r12, r5;						\
	mov 	r11, r4;						\
	mov 	r10, r3;						\
	mov 	r9, r2;							\
	mov 	r8, r1;							\
	pop	{r4-r7}
#else
/* `pop` on ARMv7-M and ARMv8-M Mainline: no limitation */
#define POP_GPRS							\
	pop	{r4-r12, lr}
#endif /* !CONFIG_ARMV7_M_ARMV8_M_MAINLINE */


#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* Registers present only on ARMv7-M and ARMv8-M Mainline */
#define SAVE_FM_BP_REGS(cpu_ctx, tmp_reg)				\
	SAVE_SPECIAL_REG(faultmask,	cpu_ctx, tmp_reg)		\
	SAVE_SPECIAL_REG(basepri,	cpu_ctx, tmp_reg)

#define RESTORE_FM_BP_REGS(cpu_ctx, tmp_reg)				\
	RESTORE_SPECIAL_REG(faultmask,	cpu_ctx, tmp_reg)		\
	RESTORE_SPECIAL_REG(basepri,	cpu_ctx, tmp_reg)
#else
/* Registers not present: do nothing */
#define SAVE_FM_BP_REGS(cpu_ctx, tmp_reg)
#define RESTORE_FM_BP_REGS(cpu_ctx, tmp_reg)
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
/* Registers present only on certain ARMv8-M implementations */
#define SAVE_SPLIM_REGS(cpu_ctx, tmp_reg)				\
	SAVE_SPECIAL_REG(msplim,	cpu_ctx, tmp_reg)		\
	SAVE_SPECIAL_REG(psplim,	cpu_ctx, tmp_reg)

#define RESTORE_SPLIM_REGS(cpu_ctx, tmp_reg)				\
	RESTORE_SPECIAL_REG(msplim,	cpu_ctx, tmp_reg)		\
	RESTORE_SPECIAL_REG(psplim,	cpu_ctx, tmp_reg)
#else
/* Registers not present: do nothing */
#define SAVE_SPLIM_REGS(cpu_ctx, tmp_reg)
#define RESTORE_SPLIM_REGS(cpu_ctx, tmp_reg)
#endif /* CONFIG_CPU_CORTEX_M_HAS_SPLIM */

/*
 * Saves the CPU's special registers in the `struct __cpu_context`
 * pointed to by the `cpu_ctx` register.
 * The `tmp_reg` register is overwritten as part of this process.
 */
#define SAVE_SPECIAL_REGISTERS(cpu_ctx, tmp_reg)			\
	SAVE_SPECIAL_REG(msp,		cpu_ctx, tmp_reg)		\
	SAVE_SPECIAL_REG(psp,		cpu_ctx, tmp_reg)		\
	SAVE_SPECIAL_REG(primask,	cpu_ctx, tmp_reg)		\
	SAVE_SPLIM_REGS(		cpu_ctx, tmp_reg)		\
	SAVE_FM_BP_REGS(		cpu_ctx, tmp_reg)		\
	SAVE_SPECIAL_REG(control,	cpu_ctx, tmp_reg)

/*
 * Restores the CPU's special registers from the `struct __cpu_context`
 * pointed to by the `cpu_ctx` register.
 * The `tmp_reg` register is overwritten as part of this process.
 *
 * N.B.: ISB at the end is required because "Software must use an ISB
 * barrier instruction to ensure a write to the CONTROL register takes
 * effect before the next instruction is executed."
 *
 * If this macro is modified, make sure CONTROL is always the last
 * restored register, and that an ISB follows the MSR instruction.
 */
#define RESTORE_SPECIAL_REGISTERS(cpu_ctx, tmp_reg)			\
	RESTORE_SPECIAL_REG(msp,	cpu_ctx, tmp_reg)		\
	RESTORE_SPECIAL_REG(psp,	cpu_ctx, tmp_reg)		\
	RESTORE_SPECIAL_REG(primask,	cpu_ctx, tmp_reg)		\
	RESTORE_SPLIM_REGS(		cpu_ctx, tmp_reg)		\
	RESTORE_FM_BP_REGS(		cpu_ctx, tmp_reg)		\
	RESTORE_SPECIAL_REG(control,	cpu_ctx, tmp_reg)		\
	isb

_ASM_FILE_PROLOGUE

GTEXT(pm_s2ram_mark_set)
GTEXT(pm_s2ram_mark_check_and_clear)
GDATA(_cpu_context)

SECTION_FUNC(TEXT, arch_pm_s2ram_suspend)
	/*
	 * Save the CPU context
	 *
	 * r0: address of the system_off function
	 */
	PUSH_GPRS

	/* Move system_off to protected register. */
	mov 	r4, r0

	/* Store CPU context */
	ldr	r1, =_cpu_context

	SAVE_SPECIAL_REGISTERS(/* ctx: */ r1, /* tmp: */ r2)

	/*
	 * Mark entering suspend to RAM.
	 */
	mov 	r1, lr
	bl 	pm_s2ram_mark_set
	mov	lr, r1

	/*
	 * Call the system_off function passed as parameter. This should never
	 * return.
	 */
	blx	r4

	/*
	 * The system_off function returns here only when the powering off was
	 * not successful (in r0 the return value).
	 */

	/* Move return value of system_off to callee-saved register. */
	mov 	r4, r0

	/*
	 * Reset the marking of suspend to RAM, return is ignored.
	 */
	mov 	r1, lr
	bl	pm_s2ram_mark_check_and_clear
	mov 	lr, r1

	/* Move the stored return value of system_off back to r0,
	 * setting it as return value for this function.
	 */
	mov	r0, r4

	POP_GPRS
	bx	lr


GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/*
	 * Check if reset occurred after suspending to RAM.
	 */
	mov 	r1, lr
	bl 	pm_s2ram_mark_check_and_clear
	mov 	lr, r1
	cmp     r0, #0x1
	beq     .L_resume
	bx      lr

.L_resume:
	/*
	 * Restore the CPU context
	 */
	ldr	r0, =_cpu_context

	RESTORE_SPECIAL_REGISTERS(/* ctx: */ r0, /* tmp: */ r1)

	POP_GPRS

	/*
	 * Set the return value and return
	 */
	movs	r0, #0
	bx	lr
