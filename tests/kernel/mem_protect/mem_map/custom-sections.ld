/* SPDX-License-Identifier: Apache-2.0 */

SECTION_DATA_PROLOGUE(TEST_MEM_MAP,,SUBALIGN(CONFIG_MMU_PAGE_SIZE))
{
	/* transplanted_function() must be first */
	KEEP(*(".test_mem_map.transplanted_function*"));
	KEEP(*(".test_mem_map.*"));

} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

PROVIDE(__test_mem_map_start = LOADADDR(TEST_MEM_MAP));
PROVIDE(__test_mem_map_size = SIZEOF(TEST_MEM_MAP));
PROVIDE(__test_mem_map_end = __test_mem_map_start + __test_mem_map_size);
