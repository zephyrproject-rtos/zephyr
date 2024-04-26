/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = 0x400;
FILL(0xFF)
KEEP(*(.rom_registers*))
. = 0x500;
