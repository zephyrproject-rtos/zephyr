/* vector_table.h - definitions for the boot vector table */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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

No other symbol has the same format, so they are easy to spot.
*/

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

#ifdef _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>

/* location of MSP and PSP upon boot: at the end of SRAM */
.equ __CORTEXM_BOOT_MSP, (0x20000000 + SRAM_SIZE - 8)
.equ __CORTEXM_BOOT_PSP, (__CORTEXM_BOOT_MSP - 0x100)

GTEXT(__start)
GTEXT(_VectorTableROM)

GTEXT(__reset)
GTEXT(__nmi)
GTEXT(__hard_fault)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
GTEXT(__svc)
GTEXT(__debug_monitor)
GTEXT(__pendsv)
GTEXT(__reserved)

GTEXT(_PrepC)
GTEXT(_isr_wrapper)

#endif /* _ASMLANGUAGE */

#endif /* _VECTOR_TABLE__H_ */
