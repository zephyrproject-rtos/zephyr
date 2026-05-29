/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/sys/util.h>

static const struct arm_mmu_region mmu_regions[] = {
	/*
	 * BCM2835 peripherals are visible to the ARM at 0x20000000-0x20ffffff.
	 * Map the entire 16 MB peripheral window as a single device-memory
	 * region rather than enumerating individual IP blocks.  All currently
	 * used peripherals (ARMCTRL, system timer, GPIO, PL011, AUX mini-UART)
	 * fall within this range, and the base address is a fixed hardware
	 * constant for BCM2835. This is SoC infrastructure without a struct
	 * device, so DT-derived per-device MMIO mappings do not apply here.
	 */
		{
			.name = "bcm2835-peripherals",
			.base_pa = 0x20000000,
			.base_va = 0x20000000,
			.size = 0x01000000,
			.attrs = MT_DEVICE | MPERM_R | MPERM_W,
		},
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
