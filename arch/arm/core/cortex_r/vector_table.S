/*
 * Copyright (c) 2018 Marvell
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Populated vector table in ROM
 */

#include <toolchain.h>
#include <linker/sections.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)
	ldr	pc,	=__reset             /*                   offset 0 */
	ldr	pc,	=__undef_instruction /* undef instruction offset 4 */
	ldr	pc,	=__svc               /* svc               offset 8 */
	ldr	pc,	=__prefetch_abort    /* prefetch abort offset  0xc */
	ldr	pc,	=__data_abort        /* data abort     offset 0x10 */
	nop                          /*                offset 0x14 */
	ldr	pc,	=_isr_wrapper        /* IRQ            offset 0x18 */
	ldr	pc,	=__nmi               /* FIQ            offset 0x1c */
