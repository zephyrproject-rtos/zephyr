/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The line below this comment is equivalent to '. = CONFIG_ROM_START_OFFSET'
 * as interpreted by GNU LD, but also compatible with LLVM LLD.
 *
 * Simple assignment doesn't work for LLVM LLD, because the dot inside section
 * is absolute, so assigning offset here results in moving location counter
 * backwards.
 *
 * We can't use '. += CONFIG_ROM_START_OFFSET' here because there might be some
 * other files included before this file.
 *
 * Symbol __rom_start_address is defined in rom_start_address.ld
 */
. += CONFIG_ROM_START_OFFSET - (. - __rom_start_address);
. = ALIGN(4);
