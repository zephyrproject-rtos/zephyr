/* SPDX-License-Identifier: Apache-2.0 */

#if LINKER_ZEPHYR_FINAL && CONFIG_ISR_TABLES_LOCAL_DECLARATION
/DISCARD/ :
{
    KEEP(*(.vectors))
    KEEP(*(_IRQ_VECTOR_TABLE_SECTION_SYMS))
}
#endif
