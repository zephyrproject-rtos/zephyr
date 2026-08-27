/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

	ITERABLE_SECTION_ROM(tracing_backend, Z_LINK_ITERABLE_SUBALIGN)

	SECTION_DATA_PROLOGUE(zephyr_dbg_info,,)
	{
		KEEP(*(".dbg_thread_info"));
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_SYMTAB
	SECTION_PROLOGUE(symtab,,)
	{
		KEEP(*(_SYMTAB_SECTION_SYMS))
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_SYMTAB */
