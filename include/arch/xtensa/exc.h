/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
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
 * @brief Xtensa public exception handling
 *
 * Xtensa-specific nanokernel exception handling interface. Included by
 * arch/xtensa/arch.h.
 */

#ifndef _ARCH_XTENSA_EXC_H_
#define _ARCH_XTENSA_EXC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
#else
/**
 * @brief Nanokernel Exception Stack Frame
 *
 * A pointer to an "exception stack frame" (ESF) is passed as an argument
 * to exception handlers registered via nanoCpuExcConnect().
 */
struct __esf {
	/* XXX - not finished yet */
	sys_define_gpr_with_alias(a1, sp);
	uint32_t pc;
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;
#endif

#ifdef __cplusplus
}
#endif


#endif /* _ARCH_XTENSA_EXC_H_ */
