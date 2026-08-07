/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. += CONFIG_FLASH_CONFIG_OFFSET - (. - __rom_start_address);
KEEP(*(.flash_conf))
. += CONFIG_IMAGE_VECTOR_TABLE_OFFSET - (. - __rom_start_address);
KEEP(*(.boot_hdr.ivt))
