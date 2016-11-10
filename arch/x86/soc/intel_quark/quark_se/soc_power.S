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

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
GDATA(_pm_save_gdtr)
GDATA(_pm_save_idtr)
GDATA(_pm_save_esp)

GTEXT(_sys_soc_save_cpu_context)
GTEXT(_sys_soc_restore_cpu_context)
GTEXT(_sys_soc_resume_from_deep_sleep)

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

/*
 * This is an example function to handle the deep sleep resume notification
 * in the absence of bootloader context restore support.
 *
 * Bootloader in Intel Quark SE Microcontroller C1000 boards have
 * context restore support and this would not be required.
 *
 * Disclaimer: This can be used for debug or development purposes. This is not
 * a supported feature in Quark SE boards and to be used at one's own risk.
 */
SECTION_FUNC(TEXT, _sys_soc_resume_from_deep_sleep)
	movl $CONFIG_BSP_SHARED_RAM_ADDR, %eax
	cmpl $_sys_soc_restore_cpu_context, (%eax)
	je _sys_soc_restore_cpu_context
	ret

#endif
