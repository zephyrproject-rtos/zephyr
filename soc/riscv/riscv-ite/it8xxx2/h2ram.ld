/*
 * Copyright (c) 2022 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	SECTION_PROLOGUE(.h2ram_pool, (NOLOAD),)
	{
		/*
		 * Ensure this section is 4k-byte aligned.
		 * NOTE: This won't cause gap if this is included first inside
		 * the RAMABLE_REGION. Because RAM base is 4k-byte aligned.
		 */
		. = ALIGN(0x1000);
		_h2ram_pool_start = .;
		KEEP(*(.h2ram_pool))
		_h2ram_pool_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)
	_h2ram_pool_size = ABSOLUTE(_h2ram_pool_end - _h2ram_pool_start);
