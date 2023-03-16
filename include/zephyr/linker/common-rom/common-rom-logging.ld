/* SPDX-License-Identifier: Apache-2.0 */

	SECTION_DATA_PROLOGUE(log_strings_sections,,)
	{
		__log_strings_start = .;
		KEEP(*(SORT(.log_strings*)));
		__log_strings_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_ROM(log_const, 4)

	ITERABLE_SECTION_ROM(log_backend, 4)

	ITERABLE_SECTION_ROM(log_link, 4)
