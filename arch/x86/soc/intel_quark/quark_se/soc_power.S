/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/asm.h>

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
GDATA(_pm_save_gdtr)
GDATA(_pm_save_idtr)
GDATA(_pm_save_esp)

GTEXT(_sys_soc_resume_from_deep_sleep)
GTEXT(_power_restore_cpu_context)
GTEXT(_power_soc_sleep)
GTEXT(_power_soc_deep_sleep)

SECTION_FUNC(TEXT, save_cpu_context)
	movl %esp, %eax                 /* save ptr to return address */

	pushf				/* save flags */
	pusha				/* save GPRs */
	movl %esp, _pm_save_esp		/* save stack ptr */
	sidtl _pm_save_idtr		/* save idtr */
	sgdtl _pm_save_gdtr		/* save gdtr */

	pushl (%eax)                    /* push return address */
	ret

SECTION_FUNC(TEXT, _power_restore_cpu_context)
	lgdtl _pm_save_gdtr		/* restore gdtr */
	lidtl _pm_save_idtr		/* restore idtr */
	movl _pm_save_esp, %esp		/* restore saved stack ptr */
	popa				/* restore saved GPRs */
	popf				/* restore saved flags */

	/*
	 * At this point the stack contents will be as follows:
	 *
	 *          Saved context
	 * ESP ---> Return address of save_cpu_context
	 *          Return address of _power_soc_sleep/deep_sleep
	 *
	 * We just popped the saved context. Next we pop out the address
	 * of the caller of save_cpu_context.Then the ret would return
	 * to caller of _power_soc_sleep or _power_soc_deep_sleep.
	 *
	 */
	addl $4, %esp
	ret

SECTION_FUNC(TEXT, _power_soc_sleep)
	call save_cpu_context
	wbinvd
	call power_soc_sleep
	/* Does not return */

SECTION_FUNC(TEXT, _power_soc_deep_sleep)
	call save_cpu_context
	wbinvd
	call power_soc_deep_sleep
	/* Does not return */

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
	cmpl $_power_restore_cpu_context, (%eax)
	je _power_restore_cpu_context
	ret

#endif
