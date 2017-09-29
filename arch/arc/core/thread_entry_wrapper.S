/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Wrapper for _thread_entry
 *
 * Wrapper for _thread_entry routine when called from the initial context.
 */

#include <toolchain.h>
#include <linker/sections.h>

GTEXT(_thread_entry_wrapper)
GTEXT(_thread_entry)

/*
 * @brief Wrapper for _thread_entry
 *
 * The routine pops parameters for the _thread_entry from stack frame, prepared
 * by the _new_thread() routine.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _thread_entry_wrapper)

	pop_s r3
	pop_s r2
	pop_s r1
	pop_s r0
	j _thread_entry
	nop
