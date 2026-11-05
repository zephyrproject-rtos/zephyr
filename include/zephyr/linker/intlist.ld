/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Build-only registration data. arch/common/isr_metadata.c supplies the header.
 * Legacy records contain function/parameter addresses. Local records contain a
 * bounded section name, with the format defined by isr_table_metadata.h.
 * Local discovery data is non-allocated and has no runtime memory budget.
 * Legacy data uses the linker scratch region IDT_LIST.
 */

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
#if !defined(LINKER_ZEPHYR_PREBUILT) && !defined(LINKER_ZEPHYR_FINAL)
.intList 0 (INFO) :
{
	KEEP(*(.irq_info*))
	KEEP(*(.intList*))
}

isr_declarations 0 (INFO) :
{
	KEEP(*(.irq.*))
	KEEP(*(.isr_vector_refs))
}
#else
/DISCARD/ :
{
	*(.irq_info*)
	*(.intList*)
}
#endif
#else
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
#endif /* CONFIG_ISR_TABLES_LOCAL_DECLARATION */
