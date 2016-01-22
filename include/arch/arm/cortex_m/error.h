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
 * @brief Cortex-M public error handling
 *
 * ARM-specific nanokernel error handling interface. Included by ARM/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_ERROR_H_
#define _ARCH_ARM_CORTEXM_ERROR_H_

#include <arch/arm/cortex_m/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int,
						 const NANO_ESF*);
extern void _SysFatalErrorHandler(unsigned int, const NANO_ESF*);
#endif

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARM_CORTEXM_ERROR_H_ */
