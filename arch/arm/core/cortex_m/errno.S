/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

/** @file
 *
 * @brief Per-thread errno accessor function
 *
 * Allow accessing the errno for the current thread without involving the
 * context switching.
 */

#define _ASMLANGUAGE

#include <nano_private.h>
#include <offsets.h>	/* nanokernel structure offset definitions */

_ASM_FILE_PROLOGUE

GTEXT(_nanokernel)
GTEXT(_get_errno)

SECTION_FUNC(TEXT, _get_errno)

	ldr r0, =_nanokernel
	ldr r0, [r0, #__tNANO_current_OFFSET]
	adds.n r0, #__tTCS_errno_var_OFFSET
	bx lr
