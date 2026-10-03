/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(_bflb_system_heap, (NOLOAD),)
{
	*(.noinit.*kheap_buf__system_heap*)
} GROUP_LINK_IN(MM_DRAM)
