/* test_stubs.S - Exception and interrupt stubs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module implements assembler exception and interrupt stubs for regression
testing.
 */

#ifdef CONFIG_X86

#include <arch/cpu.h>
#include <kernel_structs.h>
#include <arch/x86/ia32/asm.h>

/* imports (internal APIs) */

	GTEXT(_interrupt_enter)

/* Static interrupt handler stubs */

	GTEXT(int_stub)
SECTION_FUNC(TEXT, int_stub)
        pushl   $0
        pushl   $isr_handler
        jmp     _interrupt_enter
#else

#error Arch not supported

#endif /* CONFIG_X86 */
