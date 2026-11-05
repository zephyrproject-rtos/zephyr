/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* when !XIP, .text is in RAM, and vector table must be at its very start */

KEEP(*(.exc_vector_table))
KEEP(*(".exc_vector_table.*"))

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE) && \
	(defined(LINKER_ZEPHYR_PREBUILT) || defined(LINKER_ZEPHYR_FINAL))
/* IRQs follow the sixteen architecture exception vectors. */
INCLUDE isr_tables_vt.ld
#endif
