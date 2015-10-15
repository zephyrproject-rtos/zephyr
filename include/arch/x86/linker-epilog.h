/* linker-epilog.h - defines structures that conclude linker script */

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


#ifndef _LINKER_EPILOG_H
#define _LINKER_EPILOG_H
#include <linker-defs.h>

#ifdef _LINKER

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

#endif /* _LINKER */

#endif /* _LINKER_EPILOG_H */
