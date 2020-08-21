/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

_vector_start = .;
KEEP(*(.exc_vector_table))
KEEP(*(".exc_vector_table.*"))

KEEP(*(_IRQ_VECTOR_TABLE_SECTION_NAME))

KEEP(*(.vectors))

_vector_end = .;

KEEP(*(.openocd_dbg))
KEEP(*(".openocd_dbg.*"))
