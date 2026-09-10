/*
 * Copyright (c) 2026 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The AMD-Xilinx R5 remoteproc driver checks the first memory-region for
 * this packed metadata before attaching to an already running RPU.
 * uintptr_t is 64 bits in the APU's arm64 kernel, so use QUAD for the
 * resource table address even though the RPU firmware is 32-bit.
 */
SECTION_PROLOGUE(.rproc_metadata ORIGIN(RPROC),,)
{
	__rproc_metadata_start = .;
	LONG(1); /* metadata version */
	LONG(0x78616d70); /* "xamp" magic */
	LONG(0x879e928f); /* complement of the magic */
	LONG(__resource_table_end - __resource_table_start);
	QUAD(__resource_table_start);
	__rproc_metadata_end = .;
} GROUP_LINK_IN(RPROC)

ASSERT(__rproc_metadata_start == ORIGIN(RPROC),
       "remoteproc metadata must start at the RPROC origin")
ASSERT((__rproc_metadata_end - __rproc_metadata_start) == 24,
       "unexpected remoteproc metadata size")
