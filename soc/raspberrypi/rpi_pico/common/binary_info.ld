/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = ALIGN(4);
__binary_info_start = .;
KEEP(*(.binary_info.keep.*))
*(.binary_info.*)
__binary_info_end = .;
