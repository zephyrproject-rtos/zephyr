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
 * @brief Definitions for the exception vector table
 *
 *
 * Definitions for the boot vector table.
 *
 * System exception handler names all have the same format:
 *
 *   __<exception name with underscores>
 *
 * Refer to the ARCv2 manual for an explanation of the exceptions.
 */

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

#ifdef __cplusplus
extern "C" {
#endif

#define EXC_EV_TRAP	0x9

#ifdef _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>

GTEXT(__start)
GTEXT(_VectorTable)

GTEXT(__reset)
GTEXT(__memory_error)
GTEXT(__instruction_error)
GTEXT(__ev_machine_check)
GTEXT(__ev_tlb_miss_i)
GTEXT(__ev_tlb_miss_d)
GTEXT(__ev_prot_v)
GTEXT(__ev_privilege_v)
GTEXT(__ev_swi)
GTEXT(__ev_trap)
GTEXT(__ev_extension)
GTEXT(__ev_div_zero)
GTEXT(__ev_dc_error)
GTEXT(__ev_maligned)

GTEXT(_PrepC)
GTEXT(_isr_wrapper)

#else

extern void __reset(void);
extern void __memory_error(void);
extern void __instruction_error(void);
extern void __ev_machine_check(void);
extern void __ev_tlb_miss_i(void);
extern void __ev_tlb_miss_d(void);
extern void __ev_prot_v(void);
extern void __ev_privilege_v(void);
extern void __ev_swi(void);
extern void __ev_trap(void);
extern void __ev_extension(void);
extern void __ev_div_zero(void);
extern void __ev_dc_error(void);
extern void __ev_maligned(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _VECTOR_TABLE__H_ */
