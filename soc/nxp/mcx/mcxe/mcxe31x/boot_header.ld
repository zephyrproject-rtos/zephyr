/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. += CONFIG_FLASH_CONFIG_OFFSET - (. - __rom_start_address);
KEEP(*(.boot_header))
