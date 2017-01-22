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

#ifdef CONFIG_ISA_IA32

/* IA-32 specific */

#include <arch/cpu.h>
#include <kernel_structs.h>
#include <arch/x86/asm.h>
#include <asm_inline.h>

/* imports (internal APIs) */

	GTEXT(_interrupt_enter)

/* Static interrupt handler stubs */

	GTEXT(nanoIntStub)
SECTION_FUNC(TEXT, nanoIntStub)
        pushl   $0
        pushl   $isr_handler
        jmp     _interrupt_enter
#else

#error Arch not supported

#endif /* CONFIG_ISA_IA32 */
