/*
 * Copyright (c) 2024 Ian Morris
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

. = 0x400;
FILL(0xFF)
KEEP(*(.opt_set_mem*))
. = 0x500;
