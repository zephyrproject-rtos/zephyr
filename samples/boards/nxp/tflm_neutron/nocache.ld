/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * Put the Neutron NPU's DMA-shared static data in on-chip, non-cacheable SRAM
 * so it stays coherent with the NPU once the M85 L1 D-cache is enabled. This
 * places .data.neutron_driver in the SRAM01_NPU region (0x22000000, 512 KB,
 * marked ATTR_MPU_RAM_NOCACHE in the board overlay), alongside the tensor
 * arena; the model and the driver's malloc/k_heap allocations live in the
 * non-cacheable SRAM2 region (the board CMakeLists relocates the libc arena
 * and the k_heap pool there).
 *
 * Zephyr's generic CONFIG_NOCACHE_MEMORY region cannot be used for this: it is
 * anchored at the start of the chosen sram, which on this board is the PSRAM
 * LLC-cached alias (0x88000000), i.e. still cached. Pull the driver's data into
 * the truly non-cacheable on-chip SRAM instead:
 *
 *   - .data.neutron_driver: the driver's DMA-shared data (e.g. baseOffsets[],
 *     the inputs/weights/outputs/kernels/scratch base-offset table read by the
 *     NPU from its own point of view). Initialized data, loaded from ROM.
 *
 * This file is shared (kept at the sample root) and pulled in unconditionally
 * by the RT266x branch of CMakeLists.txt. The placement is guarded here to the
 * SoCs that actually declare the non-cacheable SRAM01_NPU region; only RT2663
 * does today, so nothing is emitted elsewhere.
 */

#if defined(CONFIG_SOC_MIMXRT2663)

SECTION_PROLOGUE(.neutron_driver, ,SUBALIGN(32))
{
	. = ALIGN(32);
	*(.data.neutron_driver .data.neutron_driver.*)
} GROUP_DATA_LINK_IN(SRAM01_NPU, ROMABLE_REGION)

#endif /* CONFIG_SOC_MIMXRT2663 */
