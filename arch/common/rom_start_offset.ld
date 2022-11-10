/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

KEEP(*(.rom_start_area))
KEEP(*(".rom_start_area.*"))

. = CONFIG_ROM_START_OFFSET;
. = ALIGN(4);
