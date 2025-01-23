/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.shared,, SUBALIGN(4))
{
	_shared = .;
	*(.mailbox*)
	*(.shared*)
	_eshared = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
