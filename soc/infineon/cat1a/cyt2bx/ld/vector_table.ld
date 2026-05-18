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
 * CYT2BX uses a hand-crafted IRQ vector table (soc_vector_table.c) and sets
 * CONFIG_GEN_IRQ_VECTOR_TABLE=n, which skips Zephyr's stock
 * irq-vector-table-section.ld. This local fragment restores placement of
 * the .gnu.linkonce.irq_vector_table section contiguously after the
 * exception vectors so VTOR-based IRQ dispatch works.
 */
 _vector_end = .;
