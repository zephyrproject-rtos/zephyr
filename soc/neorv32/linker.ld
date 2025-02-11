/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>

MEMORY
{
	IO (rw) : ORIGIN = 0xFFFFFE00, LENGTH = 512
}

SECTIONS
{
	SECTION_PROLOGUE(io, (NOLOAD),)
	{
		PROVIDE(__io_start = ORIGIN(IO));
		PROVIDE(__io_end   = ORIGIN(IO) + LENGTH(IO));
	} GROUP_LINK_IN(IO)
}

#include <zephyr/arch/riscv/common/linker.ld>
