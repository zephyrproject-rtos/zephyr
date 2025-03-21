/* SPDX-License-Identifier: Apache-2.0 */

SECTION_DATA_PROLOGUE(_CUSTOM_SECTION_NAME2,,)
{
	. = ALIGN(16);
	__custom_section_start = .;
	KEEP(*(".got"));
	. = ALIGN(16);
	KEEP(*(".got.plt"));
	. = ALIGN(16);
	KEEP(*("input_data_sec"));
	. = ALIGN(16);
	KEEP(*("network_model_sec"));
	. = ALIGN(16);
	KEEP(*(".tbss"));
	__custom_section_end = ALIGN(4);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
