/*
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Workaround for RAM_BASE is zero in XIP system, kernel object
 * address maybe zero(NULL) and break some test case assertions.
 * ex: tests/kernel/queue, k_queue_get() return k_queue address
 * 0x0, but treat as NULL fail.
 */
SECTION_DATA_PROLOGUE(ram_start_nonzero,(NOLOAD),)
{
	. = ABSOLUTE(.) ? . : . + 0x8;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
