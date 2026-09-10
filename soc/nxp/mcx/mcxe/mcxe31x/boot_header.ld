/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. += CONFIG_BOOT_HEADER_OFFSET - (. - __rom_start_address);
KEEP(*(.boot_header))
