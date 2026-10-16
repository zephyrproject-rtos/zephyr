/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_MIMXRT2663)

/* Place Neutron DMA-shared data in non-cacheable on-chip SRAM. */
SECTION_PROLOGUE(.neutron_driver, ,SUBALIGN(32))
{
	. = ALIGN(32);
	*(.data.neutron_driver .data.neutron_driver.*)
} GROUP_DATA_LINK_IN(SRAM01_NPU, ROMABLE_REGION)

#endif /* CONFIG_SOC_MIMXRT2663 */
