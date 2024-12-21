/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.align16,,)
{
	. = (ALIGN(16) > 0 ? ALIGN(16) : 16) - 1;
	BYTE(0);
} GROUP_LINK_IN(ROMABLE_REGION)
