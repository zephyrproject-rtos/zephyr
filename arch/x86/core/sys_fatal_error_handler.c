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

/**
 * @file
 * @brief Common system fatal error handler
 *
 * This module provides the _SysFatalErrorHandler() routine which is common to
 * supported platforms.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <kernel_structs.h>
#include <misc/printk.h>

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
					 const NANO_ESF *pEsf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

	if (k_is_in_isr() || _is_thread_essential()) {
		printk("Fatal fault in %s! Spinning...\n",
		       k_is_in_isr() ? "ISR" : "essential thread");
		for (;;)
			; /* spin forever */
	}
	printk("Fatal fault in thread! Aborting.\n");
	k_thread_abort(_current);

	CODE_UNREACHABLE;
}
