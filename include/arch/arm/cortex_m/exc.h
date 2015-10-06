/* cortex_m/exc.h - Cortex-M public exception handling */

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

/*
DESCRIPTION
ARM-specific nanokernel exception handling interface. Included by ARM/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_EXC_H_
#define _ARCH_ARM_CORTEXM_EXC_H_

#ifdef _ASMLANGUAGE
GTEXT(_ExcExit);
#else
#include <stdint.h>

struct __esf {
	uint32_t a1; /* r0 */
	uint32_t a2; /* r1 */
	uint32_t a3; /* r2 */
	uint32_t a4; /* r3 */
	uint32_t ip; /* r12 */
	uint32_t lr; /* r14 */
	uint32_t pc; /* r15 */
	uint32_t xpsr;
};

typedef struct __esf NANO_ESF;

extern const NANO_ESF _default_esf;

extern void _ExcExit(void);
#endif

#endif /* _ARCH_ARM_CORTEXM_EXC_H_ */
