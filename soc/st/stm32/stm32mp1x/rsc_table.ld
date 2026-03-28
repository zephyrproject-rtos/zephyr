/*
 * SPDX-FileCopyrightText: Copyright (c) 2019-2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.resource_table,, SUBALIGN(4))
{
	KEEP(*(.resource_table*))
} GROUP_LINK_IN(ROMABLE_REGION)
