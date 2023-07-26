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
 */

#ifndef LINKER_ZEPHYR_FINAL
SECTION_PROLOGUE(.intList, 0 (INFO),)
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
}
#else
/DISCARD/ :
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
}
#endif
