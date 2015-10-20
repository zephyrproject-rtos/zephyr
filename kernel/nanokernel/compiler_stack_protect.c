/* compiler_stack_protect.c - Compiler stack protection (kernel part) */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
DESCRIPTION
This module provides functions to support compiler stack protection
using canaries.  This feature is enabled with configuration
CONFIG_STACK_CANARIES=y.

When this feature is enabled, the compiler generated code refers to
function __stack_chk_fail and global variable __stack_chk_guard.
 */

#include <toolchain.h> /* compiler specific configurations */

#if defined(CONFIG_STACK_CANARIES)

#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>

/**
 *
 * @brief Stack canary error handler
 *
 * This function is invoked when a stack canary error is detected.
 *
 * @return Does not return
 */
void FUNC_NORETURN _StackCheckHandler(void)
{
	/* Stack canary error is a software fatal condition; treat it as such.
	 */

	_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, &_default_esf);
}

#endif /* CONFIG_STACK_CANARIES */

#ifdef CONFIG_STACK_CANARIES

/* Global variable */

/*
 * Symbol referenced by GCC compiler generated code for canary value.
 * The canary value gets initialized in _Cstart().
 */
void __noinit *__stack_chk_guard;

/**
 *
 * @brief Referenced by GCC compiler generated code
 *
 * This routine is invoked when a stack canary error is detected, indicating
 * a buffer overflow or stack corruption problem.
 */
FUNC_ALIAS(_StackCheckHandler, __stack_chk_fail, void);
#endif
