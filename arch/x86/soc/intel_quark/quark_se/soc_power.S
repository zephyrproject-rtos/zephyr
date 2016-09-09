/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _ASMLANGUAGE

#include <arch/x86/asm.h>

#define _P_LVL2	0xb0800504
#define _PM1C	0xb0800518
#define _SLPEN	13

#ifdef CONFIG_SYS_POWER_LOW_POWER_STATE
GTEXT(_sys_soc_put_low_power_state)

SECTION_FUNC(TEXT, _sys_soc_put_low_power_state)
	sti
	movl _P_LVL2, %eax		/* reading P_LVL2 causes C2 */
	ret
#endif

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
GDATA(_pm_save_gdtr)
GDATA(_pm_save_idtr)
GDATA(_pm_save_esp)

GTEXT(_sys_soc_save_cpu_context)
GTEXT(_sys_soc_restore_cpu_context)
GTEXT(_sys_soc_put_deep_sleep)
GTEXT(_sys_soc_deep_sleep_post_ops)

SECTION_FUNC(TEXT, _sys_soc_save_cpu_context)
	movl %esp, %eax			/* save ptr to return address */
	pushf				/* save flags */
	pusha				/* save GPRs */

	movl %esp, _pm_save_esp		/* save stack ptr */
	sidtl _pm_save_idtr		/* save idtr */
	sgdtl _pm_save_gdtr		/* save gdtr */

	pushl (%eax)			/* push return address */

	xorl %eax, %eax			/* 0 indicates saved context */
	ret

SECTION_FUNC(TEXT, _sys_soc_restore_cpu_context)
	/*
	 * Will transfer control to _sys_power_save_cpu_context,
	 * from where it will return 1 indicating the function
	 * is exiting after a context switch.
	 */
	lgdtl _pm_save_gdtr		/* restore gdtr */
	lidtl _pm_save_idtr		/* restore idtr */
	movl _pm_save_esp, %esp		/* restore saved stack ptr */
	popa				/* restore saved GPRs */
	popf				/* restore saved flags */

	/*
	 * At this point context is restored as it was saved
	 * in _sys_soc_save_cpu_context. The following ret
	 * will emulate a return from that function. Move 1
	 * to eax to emulate a return 1. The caller of
	 * _sys_soc_save_cpu_context will identify it is
	 * returning from a context restore based on the
	 * return value = 1.
	 */
	xorl %eax, %eax
	incl %eax
	ret

SECTION_FUNC(TEXT, _sys_soc_put_deep_sleep)

	wbinvd				/* invalidate cache */

	movl $_SLPEN, %eax		/* setting SLPEN bit in PM1C */
	lock bts %eax, _PM1C		/* triggers deep sleep */

	ret				/* code unreachable */

SECTION_FUNC(TEXT, _sys_soc_deep_sleep_post_ops)
	/*
	 * At this point, it is resuming after wake up
	 * from deep sleep.
	 *
	 * Interrupts are not enabled yet and needs to
	 * be re-enabled here.  This need to be done
	 * only in deep sleep case.
	 *
	 * In future this will be moved to the kernel.
	 */
	sti
	ret
#endif
