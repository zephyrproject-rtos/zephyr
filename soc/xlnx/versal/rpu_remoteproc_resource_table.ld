/*
 * Copyright (c) 2026 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Keep the resource table in memory visible to host-side remoteproc. Its address
 * and size are also published in the metadata at the beginning of RPROC.
 */
SECTION_PROLOGUE(.resource_table,,)
{
	__resource_table_start = .;
	KEEP(*(.resource_table*))
	__resource_table_end = .;
} GROUP_LINK_IN(RPROC)
