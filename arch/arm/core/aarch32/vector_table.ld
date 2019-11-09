/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

_vector_start = .;
KEEP(*(.exc_vector_table))
KEEP(*(".exc_vector_table.*"))

KEEP(*(IRQ_VECTOR_TABLE))

KEEP(*(.vectors))

KEEP(*(.openocd_dbg))
KEEP(*(".openocd_dbg.*"))

_vector_end = .;
