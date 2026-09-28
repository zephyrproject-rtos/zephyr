/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <zephyr/arch/arm64/tpidrro_el0.h>

#ifdef _ASMLANGUAGE

/*
 * Get CPU id
 */

.macro get_cpu_id xreg0
	mrs	\xreg0, mpidr_el1
	/* FIMXME: aff3 not taken into consideration */
	ubfx	\xreg0, \xreg0, #0, #24
.endm

/*
 * Get CPU logic id by looking up cpu_node_list
 * returns
 *   xreg0: MPID
 *   xreg1: logic id (0 ~ DT_CHILD_NUM_STATUS_OKAY(DT_PATH(cpus)) - 1)
 * clobbers: xreg0, xreg1, xreg2, xreg3
 */
.macro get_cpu_logic_id xreg0, xreg1, xreg2, xreg3
	get_cpu_id \xreg0
	ldr	\xreg3, =cpu_node_list
	mov	\xreg1, 0
1:	ldr	\xreg2, [\xreg3, \xreg1, lsl 3]
	cmp	\xreg2, \xreg0
	beq	2f
	add	\xreg1, \xreg1, 1
	cmp	\xreg1, #DT_CHILD_NUM_STATUS_OKAY(DT_PATH(cpus))
	bne	1b
	b	.
2:
.endm

/*
 * Get CPU pointer
 * Note: keep in sync with `arch_curr_cpu` in include/zephyr/arch/arm64/arch_inlines.h
 */

.macro get_cpu xreg0
	mrs	\xreg0, tpidrro_el0
	and	\xreg0, \xreg0, #TPIDRROEL0_CURR_CPU
.endm

/*
 * Copy the callee-saved registers (x19-x28) into the exception stack frame
 * pointed by xreg.
 *
 * Being callee-saved registers, their values are untouched by the exception
 * entry and are still live in the registers at any point of the exception
 * handling path, so this is used only on the paths that may dump or unwind
 * the frame (synchronous faults, SError, kernel oops, syscall entry) instead
 * of on every exception entry. x18 and lr are caller-saved and are always
 * stored by the exception entry macro.
 *
 * This is a macro and not a function on purpose: a function call would
 * clobber lr, which tail-called paths such as the branch to
 * z_arm64_do_kernel_oops() return through when z_fatal_error() recovers.
 */
#ifdef CONFIG_EXTRA_EXCEPTION_INFO
.macro z_arm64_save_exception_info xreg
	stp	x19, x20, [\xreg, ___esf_t_x19_x20_OFFSET]
	stp	x21, x22, [\xreg, ___esf_t_x21_x22_OFFSET]
	stp	x23, x24, [\xreg, ___esf_t_x23_x24_OFFSET]
	stp	x25, x26, [\xreg, ___esf_t_x25_x26_OFFSET]
	stp	x27, x28, [\xreg, ___esf_t_x27_x28_OFFSET]
.endm
#endif

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
