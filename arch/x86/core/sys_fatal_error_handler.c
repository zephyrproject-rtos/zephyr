/* sysFatalErrorHandler - common system fatal error handler */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * This module provides the _SysFatalErrorHandler() routine which is common to
 * supported platforms.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PRINTK(...) printk(__VA_ARGS__)
#else
#define PRINTK(...)
#endif /* CONFIG_PRINTK */

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
 * @param reason the fatal error reason
 * @param pEsf the pointer to the exception stack frame
 *
 * @return This function does not return.
 */
FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF * pEsf)
{
	nano_context_type_t curCtx = sys_execution_context_type_get();

	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

	if ((curCtx != NANO_CTX_ISR) && !_is_thread_essential(NULL)) {
#ifdef CONFIG_MICROKERNEL
		if (curCtx == NANO_CTX_TASK) {
			extern FUNC_NORETURN void _TaskAbort(void);
			PRINTK("Fatal task error! Aborting task.\n");
			_TaskAbort();
		} else
#endif /* CONFIG_MICROKERNEL */
		{
			PRINTK("Fatal fiber error! Aborting fiber.\n");
			fiber_abort();
		}
	} else {
#ifdef CONFIG_PRINTK
		/*
		 * Conditionalize the ctxText[] definition to prevent an "unused
		 * variable" warning when the PRINTK kconfig option is disabled.
		 */

		static const char * const ctxText[] = {"ISR", "essential fiber",
					  "essential task"};

		PRINTK("Fatal %s error! Spinning...\n", ctxText[curCtx]);
#endif /* CONFIG_PRINTK */
	}

	do {
	} while (1);
}
