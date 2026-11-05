/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
/* Registration owners only constrain compiler cloning. */
/DISCARD/ :
{
    *(.isr_vector_owners)
}
#if defined(LINKER_ZEPHYR_PREBUILT) || defined(LINKER_ZEPHYR_FINAL)
/* Actual vector slots retain their handlers after discovery. */
/DISCARD/ :
{
    *(.isr_vector_refs)
}
#endif
#endif

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE) && \
	(defined(LINKER_ZEPHYR_PREBUILT) || defined(LINKER_ZEPHYR_FINAL))
/DISCARD/ :
{
    KEEP(*(.vectors))
    KEEP(*(_IRQ_VECTOR_TABLE_SECTION_SYMS))
}
#endif
