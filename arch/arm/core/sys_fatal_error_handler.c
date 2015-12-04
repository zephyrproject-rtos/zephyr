/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief ARM Cortex-M system fatal error handler
 *
 * This module provides the _SysFatalErrorHandler() routine for Cortex-M
 * platforms.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PRINTK(...) printk(__VA_ARGS__)
#else
#define PRINTK(...)
#endif

#ifdef CONFIG_MICROKERNEL
extern void _TaskAbort(void);
static inline void nonEssentialTaskAbort(void)
{
	PRINTK("Fatal fault in task ! Aborting task.\n");
	_TaskAbort();
}
#define NON_ESSENTIAL_TASK_ABORT() nonEssentialTaskAbort()
#else
#define NON_ESSENTIAL_TASK_ABORT() \
	do {/* nothing */          \
	} while ((0))
#endif

/**
 *
 * @brief Fatal error handler
 *
 * This routine implements the corrective action to be taken when the system
 * detects a fatal error.
 *
 * This sample implementation attempts to abort the current thread and allow
 * the system to continue executing, which may permit the system to continue
 * functioning with degraded capabilities.
 *
 * System designers may wish to enhance or substitute this sample
 * implementation to take other actions, such as logging error (or debug)
 * information to a persistent repository and/or rebooting the system.
 *
 * @param reason fatal error reason
 * @param pEsf pointer to exception stack frame
 *
 * @return N/A
 */
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF * pEsf)
{
	nano_context_type_t curCtx = sys_execution_context_type_get();

	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

	if ((curCtx == NANO_CTX_ISR) || _is_thread_essential(NULL)) {
		PRINTK("Fatal fault in %s ! Spinning...\n",
		       NANO_CTX_ISR == curCtx
			       ? "ISR"
			       : NANO_CTX_FIBER == curCtx ? "essential fiber"
							  : "essential task");
		for (;;)
			; /* spin forever */
	}

	if (NANO_CTX_FIBER == curCtx) {
		PRINTK("Fatal fault in fiber ! Aborting fiber.\n");
		fiber_abort();
		return;
	}

	NON_ESSENTIAL_TASK_ABORT();
}
