/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Go to correct reset vector. */
PROVIDE( _start_SWAP = (((CONFIG_KERNEL_ENTRY) >> 24) | (((CONFIG_KERNEL_ENTRY) & 0x00FF0000) >> 8) | (((CONFIG_KERNEL_ENTRY) & 0x0000FF00) << 8) | (((CONFIG_KERNEL_ENTRY) & 0x000000FF) << 24)));
PROVIDE_HIDDEN( _SLA_Size = _flash_used + __rom_region_start - __end_header );
PROVIDE( _SLA_Size_SWAP = (((_SLA_Size) >> 24) | (((_SLA_Size) & 0x00FF0000) >> 8) | (((_SLA_Size) & 0x0000FF00) << 8) | (((_SLA_Size) & 0x000000FF) << 24)));
