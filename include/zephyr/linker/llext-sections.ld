/* SPDX-License-Identifier: Apache-2.0 */

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
	SECTION_PROLOGUE(llext_no_syscall_impl, 0 (COPY), )
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
	SECTION_PROLOGUE(llext_exports_strtab, 0 (COPY), )
	{
	  KEEP(*(llext_exports_strtab))
	}
#endif

	. = __llext_current_addr;
