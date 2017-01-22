/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <arch/arc/v2/aux_regs.h>
#include <swap_macros.h>

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
GDATA(_pm_arc_context)

GTEXT(_sys_soc_resume_from_deep_sleep)
GTEXT(_power_restore_cpu_context)
GTEXT(_power_soc_sleep)
GTEXT(_power_soc_deep_sleep)

#define GPS0_REGISTER  0xb0800100
#define RESTORE_SS_BIT 2

SECTION_FUNC(TEXT, _sys_soc_resume_from_deep_sleep)
        /* Check is this wakeup after sleep event. */
        ld r0,[GPS0_REGISTER]
        bbit1 r0,RESTORE_SS_BIT,restore
	j_s [blink] /* Jump to context of BLINK register. */

restore:
        bclr_s r0,r0,RESTORE_SS_BIT
        st r0,[GPS0_REGISTER]

	/* Enable I-Cache */
        sr 1, [_ARC_V2_IC_CTRL]

	j @_sys_soc_restore_cpu_context

SECTION_FUNC(TEXT, save_cpu_context)
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	_save_callee_saved_regs

	j_s [blink] /* Jump to context of BLINK register. */

SECTION_FUNC(TEXT, _power_soc_sleep)
	/*
	 * Save the return address.
	 * The restore function will pop this and jump
	 * back to the caller.
	 */
	push_s blink

	/* Do not link to preserve blink */
	jl @save_cpu_context
	j @power_soc_sleep
	/* Does not return */

SECTION_FUNC(TEXT, _power_soc_deep_sleep)
	/*
	 * Save the return address.
	 * The restore function will pop this and jump
	 * back to the caller.
	 */
	push_s blink

	/* Do not link to preserve blink */
	jl @save_cpu_context
	j @power_soc_deep_sleep
	/* Does not return */

SECTION_FUNC(TEXT, _sys_soc_restore_cpu_context)
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	_load_callee_saved_regs

	/* Restore return address */
	pop_s blink

	j_s [blink] /* Jump to context of BLINK register. */
#endif
