/* compiler_stack_protect.c - Compiler stack protection (kernel part) */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module provides functions to support compiler stack protection
using canaries.  This feature is enabled with configuration
CONFIG_STACK_CANARIES=y.

When this feature is enabled, the compiler generated code refers to
function __stack_chk_fail and global variable __stack_chk_guard.
*/

/* includes */

/* Stack canaries not supported on Diab */
#ifdef __DCC__
#undef CONFIG_STACK_CANARIES
#endif

#if defined(CONFIG_STACK_CANARIES)

#include <nanok.h>
#include <toolchain.h>
#include <sections.h>

/*******************************************************************************
*
* _StackCheckHandler - stack canary error handler
*
* This function is invoked when a stack canary error is detected.
*
* RETURNS: Does not return
*/

void FUNC_NORETURN _StackCheckHandler(void)
{
	/* Stack canary error is a software fatal condition; treat it as such.
	 */

	_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, &__defaultEsf);
}

#endif /* CONFIG_STACK_CANARIES */

#ifdef CONFIG_STACK_CANARIES

/* Global variable */

/*
 * Symbol referenced by GCC compiler generated code for canary value.
 * The canary value gets initialized in _Cstart().
 */

void __noinit *__stack_chk_guard;

/*******************************************************************************
*
* __stack_chk_fail - Referenced by GCC compiler generated code
*
* This routine is invoked when a stack canary error is detected, indicating
* a buffer overflow or stack corruption problem.
*/

FUNC_ALIAS(_StackCheckHandler, __stack_chk_fail, void);
#endif
