/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.resource_table,,SUBALIGN(8))
{
	KEEP(*(.resource_table))
} GROUP_LINK_IN(ROMABLE_REGION)
