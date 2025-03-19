/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_DATA_PROLOGUE(.fsp_dtc_vector_table,(NOLOAD),)
{
	/* If DTC is used, put the DTC vector table at the start of SRAM.
		This avoids memory holes due to 1K alignment required by it. */
	*(.fsp_dtc_vector_table)
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

SECTION_PROLOGUE(.id_code,,)
{
    KEEP(*(.id_code*))
} GROUP_LINK_IN(ID_CODE)
