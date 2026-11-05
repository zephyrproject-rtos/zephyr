/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE) && \
	(defined(LINKER_ZEPHYR_PREBUILT) || defined(LINKER_ZEPHYR_FINAL))
KEEP(*(.vectors.__start))
. = ALIGN(CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN);
INCLUDE isr_tables_vt.ld
#else
KEEP(*(.vectors.*))
#endif
