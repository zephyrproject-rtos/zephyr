/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. += CONFIG_ROM_START_OFFSET;
KEEP(*(.init.*))
KEEP(*(.flash_data))
