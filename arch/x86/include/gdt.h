/* gdt.h - IA-32 Global Descriptor Table (GDT) definitions */

/*
 * Copyright (c) 2011-2012, 2014 Wind River Systems, Inc.
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
This file provides definitions for the Global Descriptor Table (GDT) for the
IA-32 architecture.
*/

#ifndef _GDT_H
#define _GDT_H

/* includes */

#include <arch/x86/arch.h>

#ifndef _ASMLANGUAGE

#include <stdint.h>
#include <toolchain.h>

/* typedefs */

/* a generic GDT entry structure definition */

typedef struct s_gdtDesc {
	uint16_t limitLowerWord;    /* bits 0:15 of segment limit */
	uint16_t baseAdrsLowerWord; /* bits 0:15 of segment base address */
	uint8_t baseAdrsMidByte;    /* bits 16:23 of segment base address */
	uint8_t descType;	   /* descriptor type fields */
	uint8_t limitUpperByte;     /* bits 16:19 of limit + more type fields */
	uint8_t baseAdrsUpperByte;  /* bits 24:31 of segment base address */
} tGdtDesc;

/*
 * structure definition for the GDT "header"
 * (does not include the GDT entries).
 * The structure is packed to force the structure to appear "as is".
 * Unfortunately, this appears to remove any alignment restrictions
 * so the aligned attribute is used.
 */

typedef struct PACK_STRUCT s_gdtHeader
{
	uint16_t limit;     /* GDT limit */
	tGdtDesc *pEntries; /* pointer to the GDT entries */
} tGdtHeader __aligned(4);

/* externs */

extern tGdtHeader _gdt; /* GDT is implemented in arch/x86/core/gdt.c */


#endif /* _ASMLANGUAGE */
#endif /* _GDT_H */
