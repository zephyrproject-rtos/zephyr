/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* Section needed by gcov when coverage is turned on.*/
SECTION_PROLOGUE (gcov,,)
{
	PROVIDE_HIDDEN (__init_array_start = .);
	KEEP (*(SORT(.init_array.*)))
	KEEP (*(.init_array*))
	PROVIDE_HIDDEN (__init_array_end = .);
} GROUP_LINK_IN(ROMABLE_REGION)
