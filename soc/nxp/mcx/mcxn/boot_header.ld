/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NXP_FLEXSPI_BOOT_HEADER)
	. = CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET;
	KEEP(*(.flexspi_fcb))
#endif
