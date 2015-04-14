/* init.c - initialization module */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This module provides the C initialization routine _Cstart() which is called
when the system is ready to run C code.

This module is responsible for initializing any basic hardware (via the
invocation of _InitHardware(), and then initializing the nanokernel.  After
initializing the nanokernel the _Swap() routine is invoked to effect
a context swap to the background context (whose entry point is main()).
*/

#include <offsets.h>
#include <cputype.h>
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <misc/printk.h>
#include <drivers/rand32.h>
#include <toolchain.h>

/* hardware initialization routine provided in BSP's system.c module */

extern void _InitHardware(void);
extern int main(void);

/* variables utilized by the microkernel */

extern char *__argv[], *__env[];

/* C++ constructor initialization implemented in common/ctors.c */

extern void _Ctors(void);

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

#define BOOT_BANNER "****** BOOTING VXMICRO ******"

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP)
#define PRINT_BOOT_BANNER() printk(BOOT_BANNER "\n")
#else
#define PRINT_BOOT_BANNER() printk(BOOT_BANNER " %s\n", build_timestamp)
#endif


#ifdef CONFIG_STACK_CANARIES
/*******************************************************************************
 *
 * STACK_CANARY_INIT - initialize the kernel's stack canary
 *
 * This macro, only called from the BSP's _Cstart() routine, is used to
 * initialize the kernel's stack canary.  Both of the supported Intel and GNU
 * compilers currently support the stack canary global variable
 * <__stack_chk_guard>.  However, as this might not hold true for all future
 * releases, the initialization of the kernel stack canary has been abstracted
 * out for maintenance and backwards compatibility reasons.
 *
 * INTERNAL
 * Modifying __stack_chk_guard directly at runtime generates a build error
 * with ICC 13.0.2 20121114 on Windows 7. In-line assembly is used as a
 * workaround.
 */

extern void *__stack_chk_guard;

#if defined(VXMICRO_ARCH_x86)
#define _MOVE_INSTR "movl "
#elif defined(VXMICRO_ARCH_arm)
#define _MOVE_INSTR "str "
#else
#error "Unknown VXMICRO_ARCH type"
#endif /* VXMICRO_ARCH */

#define STACK_CANARY_INIT()                                \
	do {                                               \
		register void *tmp;                        \
		_Rand32Init();                             \
		tmp = (void *)_Rand32Get();                \
		__asm__ volatile(_MOVE_INSTR "%1, %0;\n\t" \
				 : "=m"(__stack_chk_guard) \
				 : "r"(tmp));              \
	} while (0)

#else /* !CONFIG_STACK_CANARIES */
#define STACK_CANARY_INIT()
#endif /* CONFIG_STACK_CANARIES */

/*******************************************************************************
*
* _Cstart - C portion of board initialization
*
* This routine is called from the crt0.s routine __start().  It's the first
* C code executed by the image and it's assumed that:
* a) the processor has already been switched into 32-bit protect mode, and
* b) the BSS are has already been cleared/zeroed.
*
* RETURNS: Does not return
*/

FUNC_NORETURN void _Cstart(void)
{
	/* floating point operations are NOT performed during nanokernel init */

	char dummyCCS[__tCCS_NOFLOAT_SIZEOF];

	/*
	 * Initialize the nanokernel.  This step includes initializing the
	 * interrupt subsystem, which must be performed before the
	 * hardware initialization phase (by _InitHardware).
	 *
	 * For the time being don't pass any arguments to the nanokernel.
	 * (Once the future direction of the main() prototype is sorted out
	 * both __argv and _env may come into play.)
	 */

	_nano_init((tCCS *)&dummyCCS, 0, (char **)0, (char **)0);

	/* perform basic hardware initialization */

	_InitHardware();

	STACK_CANARY_INIT();

	/* invoke C++ constructors */
	_Ctors();

	/* display boot banner */

	PRINT_BOOT_BANNER();

	/* context switch into the background context (entry function is main())
	 */

	_nano_start();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
