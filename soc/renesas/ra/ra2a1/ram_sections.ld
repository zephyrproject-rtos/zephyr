/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#ifdef CONFIG_USE_RA_FSP_DTC
SECTION_DATA_PROLOGUE(.fsp_dtc_vector_table,(NOLOAD),)
{
	/* If DTC is used, put the DTC vector table at the start of SRAM.
	   This avoids memory holes due to 1K alignment required by it. */
	*(.fsp_dtc_vector_table)
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
#endif /* CONFIG_USE_RA_FSP_DTC */
