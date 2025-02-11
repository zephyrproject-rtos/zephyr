/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = CONFIG_FLASH_CONFIG_OFFSET;
KEEP(*(.flash_conf))
. = CONFIG_IMAGE_VECTOR_TABLE_OFFSET;
KEEP(*(.boot_hdr.ivt))
