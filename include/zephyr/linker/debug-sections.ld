/* SPDX-License-Identifier: Apache-2.0 */

	/* following sections are obtained via 'ld --verbose' */

	/* Stabs debugging sections. */
	SECTION_PROLOGUE(.stab, 0,)            { *(.stab) }
	SECTION_PROLOGUE(.stabstr, 0,)         { *(.stabstr) }
	SECTION_PROLOGUE(.stab.excl, 0,)       { *(.stab.excl) }
	SECTION_PROLOGUE(.stab.exclstr, 0,)    { *(.stab.exclstr) }
	SECTION_PROLOGUE(.stab.index, 0,)      { *(.stab.index) }
	SECTION_PROLOGUE(.stab.indexstr, 0,)   { *(.stab.indexstr) }
	SECTION_PROLOGUE(.gnu.build.attributes, 0,) { *(.gnu.build.attributes .gnu.build.attributes.*) }
	SECTION_PROLOGUE(.comment, 0,)         { *(.comment) }
	/* DWARF debug sections.
	   Symbols in the DWARF debugging sections are relative to the beginning
	   of the section so we begin them at 0. */
	/* DWARF 1 */
	SECTION_PROLOGUE(.debug, 0,)           { *(.debug) }
	SECTION_PROLOGUE(.line, 0,)            { *(.line) }
	/* GNU DWARF 1 extensions */
	SECTION_PROLOGUE(.debug_srcinfo, 0,)   { *(.debug_srcinfo) }
	SECTION_PROLOGUE(.debug_sfnames, 0,)   { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	SECTION_PROLOGUE(.debug_aranges, 0,)   { *(.debug_aranges) }
	SECTION_PROLOGUE(.debug_pubnames, 0,)  { *(.debug_pubnames) }
	/* DWARF 2 */
	SECTION_PROLOGUE(.debug_info, 0,)      { *(.debug_info .gnu.linkonce.wi.*) }
	SECTION_PROLOGUE(.debug_abbrev, 0,)    { *(.debug_abbrev) }
	SECTION_PROLOGUE(.debug_line, 0,)      { *(.debug_line .debug_line.* .debug_line_end ) }
	SECTION_PROLOGUE(.debug_frame, 0,)     { *(.debug_frame) }
	SECTION_PROLOGUE(.debug_str, 0,)       { *(.debug_str) }
	SECTION_PROLOGUE(.debug_loc, 0,)       { *(.debug_loc) }
	SECTION_PROLOGUE(.debug_macinfo, 0,)   { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions */
	SECTION_PROLOGUE(.debug_weaknames, 0,) { *(.debug_weaknames) }
	SECTION_PROLOGUE(.debug_funcnames, 0,) { *(.debug_funcnames) }
	SECTION_PROLOGUE(.debug_typenames, 0,) { *(.debug_typenames) }
	SECTION_PROLOGUE(.debug_varnames, 0,)  { *(.debug_varnames) }
	/* DWARF 3 */
	SECTION_PROLOGUE(.debug_pubtypes, 0,)  { *(.debug_pubtypes) }
	SECTION_PROLOGUE(.debug_ranges, 0,)    { *(.debug_ranges) }
	/* DWARF Extension. */
	SECTION_PROLOGUE(.debug_macro, 0,)     { *(.debug_macro) }
	/* DWARF 5 */
	SECTION_PROLOGUE(.debug_line_str, 0,)  { *(.debug_line_str) }
	SECTION_PROLOGUE(.debug_loclists, 0,)  { *(.debug_loclists) }
	SECTION_PROLOGUE(.debug_rnglists, 0,)  { *(.debug_rnglists) }
