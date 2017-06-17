/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Global Descriptor Table support
 *
 * This module contains routines for updating the global descriptor table (GDT)
 * for the IA-32 architecture.
 */

#include <linker/linker-defs.h>
#include <toolchain.h>
#include <linker/sections.h>

#include <kernel_structs.h>
#include <arch/cpu.h>
#include <arch/x86/segmentation.h>

/*
 * The RAM based global descriptor table. It is aligned on an 8 byte boundary
 * as the Intel manuals recommend this for best performance, see
 * Section 3.5.1 of IA architecture SW developer manual, Vol 3.
 *
 * TODO: CPU never looks at the 8-byte zero entry at all. Save a few bytes by
 * stuffing the 6-byte pseudo descriptor there.
 */
static struct segment_descriptor _gdt_entries[] __aligned(8) = {
	DT_ZERO_ENTRY,
	DT_CODE_SEG_ENTRY(0, 0xFFFFF, DT_GRAN_PAGE, 0, DT_READABLE,
			  DT_NONCONFORM),
	DT_DATA_SEG_ENTRY(0, 0xFFFFF, DT_GRAN_PAGE, 0, DT_WRITABLE,
			  DT_EXPAND_UP)
};

struct pseudo_descriptor _gdt = DT_INIT(_gdt_entries);

