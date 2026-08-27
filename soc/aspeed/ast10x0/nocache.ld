/*
 * Copyright (c) ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
.bss_nc (NOLOAD) :
{
	__bss_nc_start__ = .;
	*(.nocache.bss*)
	. = ALIGN(4);
	__bss_nc_end__ = .;
} > SRAM_NC
