/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

FILL(0XFF)
KEEP(*(.rom_start)) /* Header for ROM code */
__end_header = . ;
. = ALIGN(512);
