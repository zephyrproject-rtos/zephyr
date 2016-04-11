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
 * @brief Information necessary for debugging
 *
 * @internal No routine is provided for getting the current exception stack
 * frame as the exception handler already has knowledge of the ESF.
 */

#ifndef __DEBUG_INFO_H
#define __DEBUG_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nanokernel.h>

#ifndef _ASMLANGUAGE

/**
 * @brief Get the current interrupt stack frame
 *
 * @details This routine (only called from an ISR) returns a
 * pointer to the current interrupt stack frame.
 *
 * @return pointer the current interrupt stack frame
 */
extern NANO_ISF *sys_debug_current_isf_get(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_INFO_H */
