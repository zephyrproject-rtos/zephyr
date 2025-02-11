/* SPDX-License-Identifier: Apache-2.0 */

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
