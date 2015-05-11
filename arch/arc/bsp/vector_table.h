/* vector_table.h - definitions for the exception vector table */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION

Definitions for the boot vector table.

System exception handler names all have the same format:

  __<exception name with underscores>

Refer to the ARCv2 manual for an explanation of the exceptions.
*/

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

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

#endif /* _VECTOR_TABLE__H_ */
