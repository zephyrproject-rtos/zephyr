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
 * @brief NMI handler infrastructure
 *
 * Provides a boot time handler that simply hangs in a sleep loop, and a run
 * time handler that resets the CPU. Also provides a mechanism for hooking a
 * custom run time handler.
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

/**
 *
 * @brief Default NMI handler installed when kernel is up
 *
 * The default handler outputs a error message and reboots the target. It is
 * installed by calling _NmiInit();
 *
 * @return N/A
 */

static void _DefaultHandler(void)
{
	printk("NMI received! Rebooting...\n");
	_ScbSystemReset();
}

/**
 *
 * @brief Install default runtime NMI handler
 *
 * Meant to be called by platform code if they want to install a simple NMI
 * handler that reboots the target. It should be installed after the console is
 * initialized.
 *
 * @return N/A
 */

void _NmiInit(void)
{
	handler = _DefaultHandler;
}

/**
 *
 * @brief Install a custom runtime NMI handler
 *
 * Meant to be called by platform code if they want to install a custom NMI
 * handler that reboots. It should be installed after the console is
 * initialized if it is meant to output to the console.
 *
 * @return N/A
 */

void _NmiHandlerSet(void (*pHandler)(void))
{
	handler = pHandler;
}
#endif /* CONFIG_RUNTIME_NMI */

/**
 *
 * @brief Handler installed in the vector table
 *
 * Simply call what is installed in 'static void(*handler)(void)'.
 *
 * @return N/A
 */

void __nmi(void)
{
	handler();
	_ExcExit();
}
