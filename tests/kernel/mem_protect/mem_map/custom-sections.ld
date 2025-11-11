/* SPDX-License-Identifier: Apache-2.0 */

	. = ALIGN(CONFIG_MMU_PAGE_SIZE);
	__test_mem_map_start = .;

	/* transplanted_function() must be first */
	KEEP(*(".test_mem_map.transplanted_function*"));
	KEEP(*(".test_mem_map.*"));

	. = ALIGN(CONFIG_MMU_PAGE_SIZE);
	__test_mem_map_end = .;
