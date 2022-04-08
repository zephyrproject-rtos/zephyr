/* SPDX-License-Identifier: Apache-2.0 */

	ITERABLE_SECTION_ROM(tracing_backend, 4)

	SECTION_DATA_PROLOGUE(zephyr_dbg_info,,)
	{
		KEEP(*(".dbg_thread_info"));
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
