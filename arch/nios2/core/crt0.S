/*
 * Copyright (c) 2016 Intel Corporation
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

#define _ASMLANGUAGE
#include <arch/nios2/asm.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(_PrepC)


SECTION_FUNC(TEXT_START, __start)

	/* TODO initialize instruction cache, if present */

	/* TODO if shadow register sets enabled, ensure we are in set 0 */

	/* TODO initialize data cache, if present */

	/* TODO setup stack pointer */

	/* TODO setup global pointer, if used */

	/* TODO if shadow register sets enabled, interate through them to set
	 * up. Need to clear r0, write gp, set the execption stack pointer */

	/* Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters nanokernel _Cstart */
	call _PrepC

