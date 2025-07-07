/*
 * Copyright (c) 2025 Google
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if LINKER_ZEPHYR_FINAL && defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
INCLUDE isr_tables_swi.ld
#endif
