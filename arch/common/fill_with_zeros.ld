/*
 * Copyright (c) 2023, Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LLVM LLD fills empty spaces (created using ALIGN() or moving the location
 * counter) in executable segments with TrapInstr pattern, e.g. for ARM the
 * TrapInstr pattern is 0xd4d4d4d4. GNU LD fills empty spaces with 0x00
 * pattern.
 *
 * We may want to have some section (e.g. rom_start) filled with 0x00,
 * e.g. because MCU can interpret the pattern as a configuration data.
 */
FILL(0x00);
