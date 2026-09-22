/* SPDX-License-Identifier: Apache-2.0 */

	/*
	 * .rel.* are for relocation.
	 * These are being produced by compiler/linker.
	 * Specify these here so they are not considered orphan sections.
	 */

	SECTION_PROLOGUE(.rel.plt,,)
	{
	*(.rel.plt)

	PROVIDE_HIDDEN (__rel_iplt_start = .);
	*(.rel.iplt)
	PROVIDE_HIDDEN (__rel_iplt_end = .);
	}

	SECTION_PROLOGUE(.rela.plt,,)
	{
	*(.rela.plt)

	PROVIDE_HIDDEN (__rela_iplt_start = .);
	*(.rela.iplt)
	PROVIDE_HIDDEN (__rela_iplt_end = .);
	}

	SECTION_PROLOGUE(.rel.dyn,,)
	{
	*(.rel.*)
	}

	SECTION_PROLOGUE(.rela.dyn,,)
	{
	*(.rela.*)
	}
