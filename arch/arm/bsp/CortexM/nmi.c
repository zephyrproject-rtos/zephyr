/* nmi.c - NMI handler infrastructure */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
Provides a boot time handler that simply hangs in a sleep loop, and a run time
handler that resets the CPU. Also provides a mechanism for hooking a custom
run time handler.
*/

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/printk.h>
#include <toolchain.h>
#include <sections.h>

extern void _SysNmiOnReset(void);
#if !defined(CONFIG_RUNTIME_NMI)
#define handler _SysNmiOnReset
#endif

#ifdef CONFIG_RUNTIME_NMI
typedef void (*_NmiHandler_t)(void);
static _NmiHandler_t handler = _SysNmiOnReset;

/*******************************************************************************
*
* _DefaultHandler - default NMI handler installed when kernel is up
*
* The default handler outputs a error message and reboots the target. It is
* installed by calling _NmiInit();
*
* RETURNS: N/A
*/

static void _DefaultHandler(void)
{
	printk("NMI received! Rebooting...\n");
	_ScbSystemReset();
}

/*******************************************************************************
*
* _NmiInit - install default runtime NMI handler
*
* Meant to be called by BSP code if they want to install a simple NMI handler
* that reboots the target. It should be installed after the console is
* initialized.
*
* RETURNS: N/A
*/

void _NmiInit(void)
{
	handler = _DefaultHandler;
}

/*******************************************************************************
*
* _NmiHandlerSet - install a custom runtime NMI handler
*
* Meant to be called by BSP code if they want to install a custom NMI handler
* that reboots. It should be installed after the console is initialized if it is
* meant to output to the console.
*
* RETURNS: N/A
*/

void _NmiHandlerSet(void (*pHandler)(void))
{
	handler = pHandler;
}
#endif /* CONFIG_RUNTIME_NMI */

/*******************************************************************************
*
* __nmi - handler installed in the vector table
*
* Simply call what is installed in 'static void(*handler)(void)'.
*
* RETURNS: N/A
*/

void __nmi(void)
{
	handler();
	_ExcExit();
}
