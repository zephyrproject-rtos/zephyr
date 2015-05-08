/* gdt.c - Global Descriptor Table support */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This module contains routines for updating the global descriptor table (GDT)
for the IA-32 architecture.
*/

/* includes */

#include <linker-defs.h>
#include <toolchain.h>
#include <sections.h>

#include <nanok.h>
#include <nanokernel/cpu.h>
#include <gdt.h>

/* defines */

#if (CONFIG_NUM_GDT_SPARE_ENTRIES < 0)
#error "**** CONFIG_NUM_GDT_SPARE_ENTRIES must be at least 0\n\n"
#endif

#define NUM_BASE_GDT_ENTRIES 3

#define MAX_GDT_ENTRIES \
	(NUM_BASE_GDT_ENTRIES + CONFIG_NUM_GDT_SPARE_ENTRIES)

/* locals */

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
		_gdt_entries[MAX_GDT_ENTRIES]
	__attribute__((aligned(8))) = {
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

/* globals */

tGdtHeader _gdt = {
	sizeof(tGdtDesc[MAX_GDT_ENTRIES - CONFIG_NUM_GDT_SPARE_ENTRIES]) -
		1,
	&_gdt_entries[0]};



