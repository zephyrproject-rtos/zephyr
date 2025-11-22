/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Only define LLEXT sections if the subsystem is enabled.
 * Check here to avoid duplication in every linker script.
 */
#ifdef CONFIG_LLEXT
	/*
	 * Save the current address to avoid changing it in this file.
	 */
	HIDDEN(__llext_current_addr = .);

	/*
	 * Map the 'no_syscall_impl' symbol in 'syscall_export_llext.c' to
	 * absolute address 0 so other weak symbols are exported as NULL.
	 * This section is used for mapping that symbol only and is not
	 * to be included in the final binary.
	 */
	SECTION_PROLOGUE(llext_no_syscall_impl, 0 (INFO), )
	{
	  *(llext_no_syscall_impl)
	}

	/*
	 * Special section used by LLEXT if CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
	 * is enabled. Declare this section to prevent it from being considered orphan.
	 *
	 * This section is used to temporarily save the exported symbols' names in the
	 * Zephyr ELF for post-processing, but it is not included in the final binary.
	 *
	 * NOTE: This section MUST start at address 0, as the post-processing scripts
	 * assume that the address of any data in this section (i.e., symbol names) is
	 * strictly equivalent to the offset inside the section.
	 */
#ifdef CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
	SECTION_PROLOGUE(llext_exports_strtab, 0 (INFO), )
	{
	  KEEP(*(llext_exports_strtab))
	}
#endif

	/*
	 * Special sections for symbols marked for export but ultimately rejected
	 * from the Zephyr export table. This corresponds to symbols which have
	 * been placed in an export group whose corresponding Kconfig symbol has
	 * value "n" or is missing (the latter is an error from application author).
	 * The first section contains (modified) export table entries whereas the
	 * second holds strings such that they don't go in the final binary.
	 */
	SECTION_PROLOGUE(llext_discarded_exports_table, 0 (INFO), )
	{
		KEEP(*(llext_discarded_exports_table))
	}

	SECTION_PROLOGUE(llext_discarded_exports_strtab, 0 (INFO), )
	{
		KEEP(*(llext_discarded_exports_strtab))
	}

	. = __llext_current_addr;
#endif /* CONFIG_LLEXT */
