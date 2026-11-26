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

#ifdef CONFIG_POSITION_INDEPENDENT
/*
 * One pointer per _isr_list entry, in the same order: the param of each
 * IRQ_CONNECT(). Unlike .intList, it is kept in the final image, so the
 * self-relocation turns each pointer into the run-time address of the object
 * it points to, in any memory region. z_adjust_isr_tables() copies them into
 * the SW ISR table. It is also in zephyr_pre0.elf, where gen_isr_tables.py
 * checks that its order matches the one of .intList.
 */
SECTION_PROLOGUE(.intListParam,,)
{
	__isr_param_start = .;
	KEEP(*(.intListParam*))
	__isr_param_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)
#endif

#ifndef LINKER_ZEPHYR_FINAL
SECTION_PROLOGUE(.intList,,)
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
} GROUP_ROM_LINK_IN(IDT_LIST, IDT_LIST)
#else
/DISCARD/ :
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
}
#endif
