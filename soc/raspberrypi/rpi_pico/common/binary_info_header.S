/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pico/binary_info/defs.h"

.section .binary_info_header

/* binary_info_header */
binary_info_header:
.word BINARY_INFO_MARKER_START
.word __binary_info_start
.word __binary_info_end
.word data_cpy_table /* we may need to decode pointers that are in RAM at runtime.*/
.word BINARY_INFO_MARKER_END

.align 2

/* data_cpy_table */
data_cpy_table:
.word 0 /* centinel */
