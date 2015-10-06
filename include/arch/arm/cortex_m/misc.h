/* cortex_m/misc.h - Cortex-M public nanokernel miscellaneous */

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
ARM-specific nanokernel miscellaneous interface. Included by ARM/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_MISC_H_
#define _ARCH_ARM_CORTEXM_MISC_H_

#ifndef _ASMLANGUAGE
extern void nano_cpu_idle(void);
#endif

#endif /* _ARCH_ARM_CORTEXM_MISC_H_ */
