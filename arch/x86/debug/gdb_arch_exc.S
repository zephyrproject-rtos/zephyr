/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
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
 * Hooking of exception handlers.
 */

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <arch/x86/arch.h>
#include <asmPrv.h>
#include <debug/gdb_arch.h>

#ifdef GDB_ARCH_HAS_RUNCONTROL

	GTEXT(gdb_bp_handler)
	GTEXT(gdb_trace_handler)
	GTEXT(gdb_div_by_zero_handler)
	GTEXT(gdb_pfault_handler)

NANO_CPU_EXC_CONNECT_NO_ERR(gdb_div_by_zero_handler, IV_DIVIDE_ERROR, 0);
NANO_CPU_EXC_CONNECT_NO_ERR(gdb_trace_handler, IV_DEBUG, 0);
NANO_CPU_EXC_CONNECT_NO_ERR(gdb_bp_handler, IV_BREAKPOINT, 0);
NANO_CPU_EXC_CONNECT(gdb_pfault_handler, IV_PAGE_FAULT, 0);

#endif
