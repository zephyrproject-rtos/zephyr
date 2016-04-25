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

/**
 * @file
 * @brief Cortex-M public exception handling
 *
 * ARM-specific nanokernel exception handling interface. Included by ARM/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_EXC_H_
#define _ARCH_ARM_CORTEXM_EXC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_ExcExit);
#else
#include <stdint.h>

struct __esf {
	sys_define_gpr_with_alias(a1, r0);
	sys_define_gpr_with_alias(a2, r1);
	sys_define_gpr_with_alias(a3, r2);
	sys_define_gpr_with_alias(a4, r3);
	sys_define_gpr_with_alias(ip, r12);
	sys_define_gpr_with_alias(lr, r14);
	sys_define_gpr_with_alias(pc, r15);
	uint32_t xpsr;
#ifdef CONFIG_FLOAT
	float s[16];
	uint32_t fpscr;
	uint32_t undefined;
#endif
};

typedef struct __esf NANO_ESF;

extern const NANO_ESF _default_esf;

extern void _ExcExit(void);

#if !defined(CONFIG_XIP)

/* currently, exception connecting is only available to non-XIP kernels */

/**
 * @brief signature for an exception handler
 */

#define sys_exc_handler_sig(x) void (x)(NANO_ESF *esf)

/**
 * @brief exception handler data type
 */

typedef sys_exc_handler_sig(sys_exc_handler_t);

/**
 * @brief connect a handler to an exception vector
 *
 * Connect the @a handler to the exception vector @a num.
 *
 * The @a unused parameter is only there to match the x86 signature.
 *
 * @param num Exception vector number
 * @param handler Exception handler to connect
 * @param unused Unused
 *
 * @return N/A
 */

extern void sys_exc_connect(unsigned int num, sys_exc_handler_t *handler,
							void *unused);

/**
 * @brief alias of sys_exc_connect
 *
 * See sys_exc_connect().
 */

extern void nanoCpuExcConnect(unsigned int, sys_exc_handler_t *, void *);

/**
 * @brief display the contents of a exception stack frame
 *
 * @return N/A
 */

extern void sys_exc_esf_dump(NANO_ESF *esf);

#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARM_CORTEXM_EXC_H_ */
