/* test_stubs.S - Exception and interrupt stubs */

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
This module implements assembler exception and interrupt stubs for regression
testing.
 */

#define _ASMLANGUAGE

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
