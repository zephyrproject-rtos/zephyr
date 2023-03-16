/* SPDX-License-Identifier: Apache-2.0 */

	ITERABLE_SECTION_ROM(log_strings, 4)

	SECTION_DATA_PROLOGUE(log_const_sections,,)
	{
		__log_const_start = .;
		KEEP(*(SORT(.log_const_*)));
		__log_const_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_ROM(log_backend, 4)

	ITERABLE_SECTION_ROM(log_link, 4)
