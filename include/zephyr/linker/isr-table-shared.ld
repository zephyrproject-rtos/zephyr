/* Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
#if defined(LINKER_ZEPHYR_PREBUILT) || defined(LINKER_ZEPHYR_FINAL)
INCLUDE isr_tables_shared.ld
#else
KEEP(*(.isr_pre0.shared))
#endif
#else
KEEP(*(_SHARED_SW_ISR_TABLE_SECTION_SYMS))
#endif
