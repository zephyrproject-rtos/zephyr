/* linker-epilog.h - defines structures that conclude linker script */

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


#ifndef _LINKER_EPILOG_H
#define _LINKER_EPILOG_H
#include <linker-defs.h>

#ifdef _LINKER

/*
 * DIAB linker does not support multiple SECTIONS commands
 * in linker script
 */
#if defined(__GCC_LINKER_CMD__)

/*
 * The following guard prevents usage of orphaned sections --
 * the sections that are not explicitly mentioned in the linker script
 */

SECTIONS
	{
	/* list all debugging sections that needed in resulting ELF */
	.shstrtab	   0 (OPTIONAL): { *(.shstrtab) }
	.symtab		 0 (OPTIONAL): { *(.symtab) }
	.strtab		 0 (OPTIONAL): { *(.strtab) }
	.iplt		   0 (OPTIONAL): { *(.iplt) }
	.igot.plt	   0 (OPTIONAL): { *(.igot.plt) }
	.rel.plt		0 (OPTIONAL): { *(.rel.plt) }
	.rela.plt	   0 (OPTIONAL): { *(.rela.plt) }
	.rel.dyn		0 (OPTIONAL): { *(".rel.*") }
	.rela.dyn	   0 (OPTIONAL): { *(".rela.*") }
	.comment		0 (OPTIONAL): { *(.comment) }
	/* DWARF debugging sections */
	/* DWARF 1 */
	.debug		  0 (OPTIONAL): { *(.debug) }
	.line		   0 (OPTIONAL): { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 (OPTIONAL): { *(.debug_aranges) }
	.debug_pubnames 0 (OPTIONAL): { *(.debug_pubnames) }
	.debug_pubtypes 0 (OPTIONAL): { *(.debug_pubtypes) }
	/* DWARF 2 */
	.debug_line	 0 (OPTIONAL): { *(.debug_line) }
	.debug_info	 0 (OPTIONAL): { *(.debug_info) }
	.debug_macinfo  0 (OPTIONAL): { *(.debug_macinfo) }
	.debug_abbrev   0 (OPTIONAL): { *(.debug_abbrev) }
	.debug_loc	  0 (OPTIONAL): { *(.debug_loc) }
	.debug_ranges   0 (OPTIONAL): { *(.debug_ranges) }
	.debug_str	  0 (OPTIONAL): { *(.debug_str) }
	.debug_frame	0 (OPTIONAL): { *(.debug_frame) }
	.debug_macro	0 (OPTIONAL): { *(.debug_macro) }
	/* generate a linker error if orphan section is used */
	.trashcan :
		{
	*(.*)
		}
	}
ASSERT(SIZEOF(.trashcan) == 0, "Section(s) undefined in the linker script used.")

#endif /* (__GCC_LINKER_CMD__) */
#endif /* _LINKER */

#endif /* _LINKER_EPILOG_H */
