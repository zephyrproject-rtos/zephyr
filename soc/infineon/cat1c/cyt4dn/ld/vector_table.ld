/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#if !(LINKER_ZEPHYR_FINAL && CONFIG_ISR_TABLES_LOCAL_DECLARATION)
. = ALIGN(4);
KEEP(*(_IRQ_VECTOR_TABLE_SECTION_SYMS))
#endif

/*
 * CYT4DN has 16 L1 interrupts but CONFIG_NUM_IRQS counts all
 * multi-level slots (6368 on M7, 3986 on M0+). gen_isr_tables.py sizes
 * _irq_vector_table to NUM_IRQS with no way to clamp it, so
 * CONFIG_GEN_IRQ_VECTOR_TABLE=n and soc_vector_table.c supplies the
 * 16-entry table manually. That also skips the stock
 * irq-vector-table-section.ld; this fragment restores placement of
 * .gnu.linkonce.irq_vector_table so VTOR-based dispatch works.
 */
 _vector_end = .;
