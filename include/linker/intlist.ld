/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This creates a special section which is not included by the final binary,
 * instead it is consumed by the gen_isr_tables.py script.
 *
 * What we create here is a data structure:
 *
 * struct {
 *	uint32_t num_vectors;       <- typically CONFIG_NUM_IRQS
 *	struct _isr_list isrs[]; <- Usually of smaller size than num_vectors
 * }
 *
 * Which indicates the memory address of the number of isrs that were
 * defined, the total number of IRQ lines in the system, followed by
 * an appropriate number of instances of struct _isr_list. See
 * include/sw_isr_table.h
 *
 * You will need to declare a bogus memory region for IDT_LIST. It doesn't
 * matter where this region goes as it is stripped from the final ELF image.
 * The address doesn't even have to be valid on the target. However, it
 * shouldn't overlap any other regions. On most arches the following should be
 * fine:
 *
 * MEMORY {
 *	.. other regions ..
 *	IDT_LIST        : ORIGIN = 0xfffff7ff, LENGTH = 2K
 * }
 */

#ifndef LINKER_PASS2
SECTION_PROLOGUE(.intList,,)
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
} GROUP_LINK_IN(IDT_LIST)
#else
/DISCARD/ :
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
}
#endif
