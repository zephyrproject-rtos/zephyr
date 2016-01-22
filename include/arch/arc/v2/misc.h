/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * @brief ARCv2 public nanokernel miscellaneous
 *
 * ARC-specific nanokernel miscellaneous interface. Included by arc/arch.h.
 */

#ifndef _ARCH_ARC_V2_MISC_H_
#define _ARCH_ARC_V2_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern unsigned int nano_cpu_sleep_mode;
extern void nano_cpu_idle(void);
extern void nano_cpu_atomic_idle(unsigned int key);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARC_V2_MISC_H_ */
