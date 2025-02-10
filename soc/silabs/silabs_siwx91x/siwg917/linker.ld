/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

MEMORY
{
	udma0   (rwx)  : ORIGIN = 0x0002fc00, LENGTH = 0x00000400
	udma1   (rwx)  : ORIGIN = 0x24061c00, LENGTH = 0x00000400
}

SECTIONS
{
	.common_tcm_code :
	{
		*(.common_tcm_code*)
	} > FLASH

	/* These regions of SRAM is where the UDMA descriptors are stored. The corresponding
	   section must be properly declared in the linker script to ensure correct data transfer
	   and proper functioning of the UDMA module */
	.udma_addr0 :
	{
		*(.udma_addr0*)
	} > udma0 AT> FLASH

	.udma_addr1 :
	{
		*(.udma_addr1*)
	} > udma1 AT> FLASH
}
