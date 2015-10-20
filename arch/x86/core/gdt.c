/* gdt.c - Global Descriptor Table support */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * This module contains routines for updating the global descriptor table (GDT)
 * for the IA-32 architecture.
 */

#include <linker-defs.h>
#include <toolchain.h>
#include <sections.h>

#include <nano_private.h>
#include <arch/cpu.h>
#include <gdt.h>

#if (CONFIG_NUM_GDT_SPARE_ENTRIES < 0)
#error "**** CONFIG_NUM_GDT_SPARE_ENTRIES must be at least 0\n\n"
#endif

#define NUM_BASE_GDT_ENTRIES 3

#define MAX_GDT_ENTRIES \
	(NUM_BASE_GDT_ENTRIES + CONFIG_NUM_GDT_SPARE_ENTRIES)

/*
 * The RAM based global descriptor table. It is aligned on an 8 byte boundary
 * as the Intel manuals recommend this for best performance.
 */

/*
 * For MM_POMS, _gdt_entries must be global so the linker script can
 * generate a _GdtEntriesP for crt0.s
 */
static
	tGdtDesc
		_gdt_entries[MAX_GDT_ENTRIES] __aligned(8) = {
		{/* Entry 0 (selector=0x0000): The "NULL descriptor" */
		 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},
		{	/* Entry 1 (selector=0x0008): Code descriptor: DPL0 */
		 0xffff, /* limit: xffff */
		 0x0000, /* base : xxxx0000 */
		 0x00,   /* base : xx00xxxx */
		 0x9b,   /* Code e/r, Present, DPL0, Accessed=1 */
		 0xcf,   /* limit: fxxxx, Page Gra, 32 bit */
		 0x00    /* base : 00xxxxxx */
		},
		{	/* Entry 2 (selector=0x0010): Data descriptor: DPL0 */
		 0xffff, /* limit: xffff */
		 0x0000, /* base : xxxx0000 */
		 0x00,   /* base : xx00xxxx */
		 0x93,   /* Data r/w, Present, DPL0, Accessed=1 */
		 0xcf,   /* limit: fxxxx, Page Gra, 32 bit */
		 0x00    /* base : 00xxxxxx */
		},
};

tGdtHeader _gdt = {
	sizeof(tGdtDesc[MAX_GDT_ENTRIES - CONFIG_NUM_GDT_SPARE_ENTRIES]) -
		1,
	&_gdt_entries[0]};



