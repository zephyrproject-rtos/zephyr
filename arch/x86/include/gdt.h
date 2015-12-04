/*
 * Copyright (c) 2011-2012, 2014 Wind River Systems, Inc.
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
 * @brief IA-32 Global Descriptor Table (GDT) definitions
 *
 * This file provides definitions for the Global Descriptor Table (GDT) for the
 * IA-32 architecture.
 */

#ifndef _GDT_H
#define _GDT_H

#include <arch/x86/arch.h>

#ifndef _ASMLANGUAGE

#include <stdint.h>
#include <toolchain.h>

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

typedef struct __packed s_gdtHeader
{
	uint16_t limit;     /* GDT limit */
	tGdtDesc *pEntries; /* pointer to the GDT entries */
} tGdtHeader __aligned(4);

/* externs */

extern tGdtHeader _gdt; /* GDT is implemented in arch/x86/core/gdt.c */


#endif /* _ASMLANGUAGE */
#endif /* _GDT_H */
