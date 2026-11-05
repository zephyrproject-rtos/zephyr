/* Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
#if defined(LINKER_ZEPHYR_PREBUILT) || defined(LINKER_ZEPHYR_FINAL)
INCLUDE isr_tables_swi.ld
#else
KEEP(*(.isr_pre0.sw))
#endif
#else
*(_SW_ISR_TABLE_SECTION_SYMS)
#endif
