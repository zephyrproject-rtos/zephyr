/* SPDX-License-Identifier: Apache-2.0 */

SECTION_DATA_PROLOGUE(_CUSTOM_SECTION_NAME2,,)
{
	. = ALIGN(4);
	__custom_section_start = .;
	KEEP(*(".custom_section.*"));
	__custom_section_end = ALIGN(4);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
