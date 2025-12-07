/*
 * Copyright 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

KEEP(*(.vectors.__start))
. = ALIGN(CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN);
KEEP(*(_IRQ_VECTOR_TABLE_SECTION_SYMS))
_vector_end = .;
