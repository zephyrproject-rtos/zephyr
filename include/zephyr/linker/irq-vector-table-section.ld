/* SPDX-License-Identifier: Apache-2.0 */

#if !(LINKER_ZEPHYR_FINAL && CONFIG_ISR_TABLES_LOCAL_DECLARATION)
. = ALIGN(CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN);
KEEP(*(_IRQ_VECTOR_TABLE_SECTION_SYMS))
#endif

/*
 * Some ARM platforms require this symbol to be placed after the IRQ vector
 * table (like STM32F0). The symbol defined here is overriding the one in
 * arch/arm/core/vector_table.ld when the IRQ vector table is enabled.
 */
 _vector_end = .;
