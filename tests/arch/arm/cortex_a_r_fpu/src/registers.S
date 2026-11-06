/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE
GTEXT(fp_context_call)
GTEXT(fp_capture_callee)

SECTION_FUNC(TEXT, fp_capture_callee)
	vstmia r0, {d8-d15}
	bx lr

/* Load an entire FP context, call an integer-only callback, then capture it.
 * Preserve the caller's ABI-saved registers and FP control state separately.
 */
SECTION_FUNC(TEXT, fp_context_call)
	push {r4-r6, lr}
	vpush {d8-d15}
	vmrs r3, fpscr
	push {r2, r3}
	mov r4, r0
	mov r5, r1
	mov r6, r2
	vldmia r4!, {d0-d15}
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	vldmia r4!, {d16-d31}
#endif
	ldr r3, [r4]
	vmsr fpscr, r3
	blx r6
	vstmia r5!, {d0-d15}
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	vstmia r5!, {d16-d31}
#endif
	vmrs r3, fpscr
	str r3, [r5]
	pop {r2, r3}
	vmsr fpscr, r3
	vpop {d8-d15}
	pop {r4-r6, pc}
