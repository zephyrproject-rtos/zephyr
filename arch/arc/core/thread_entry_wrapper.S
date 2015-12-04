/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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
 * @brief Wrapper for _thread_entry
 *
 * Wrapper for _thread_entry routine when called from the initial context.
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>

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
