/* SPDX-License-Identifier: Apache-2.0 */

	SECTION_DATA_PROLOGUE(log_strings_sections,,)
	{
		__log_strings_start = .;
		KEEP(*(SORT(.log_strings*)));
		__log_strings_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(log_const_sections,,)
	{
		__log_const_start = .;
		KEEP(*(SORT(.log_const_*)));
		__log_const_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(log_backends_sections,,)
	{
		__log_backends_start = .;
		KEEP(*("._log_backend.*"));
		__log_backends_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
