/*
 * Copyright (c) 2019 NXP
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET;
KEEP(*(.boot_hdr.conf))
. = CONFIG_IMAGE_VECTOR_TABLE_OFFSET;
KEEP(*(.boot_hdr.ivt))
KEEP(*(.boot_hdr.data))
#ifdef CONFIG_DEVICE_CONFIGURATION_DATA
	KEEP(*(.boot_hdr.dcd_data))
#endif
