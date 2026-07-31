/*
 * Copyright (c) 2026 Hsiu-Chi Tsai <hctsai@linux.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM spin-table secondary-CPU release backend for pm_cpu_ops.
 *
 * Platforms without PSCI (e.g. Raspberry Pi Zero 2 W / BCM2710, whose boot
 * stub parks the secondary cores in a spin-table) release a core by writing
 * the entry point to that core's release address and issuing SEV. This is the
 * non-PSCI counterpart to drivers/pm_cpu_ops/pm_cpu_ops_psci.c; it provides the
 * strong override of the __weak pm_cpu_on() in pm_cpu_ops_weak_impl.c.
 *
 * The release address is the standard Raspberry Pi armstub8 spin-table mailbox
 * for core N: 0xd8 + N*8 (core1=0xe0, core2=0xe8, core3=0xf0). HW-validated on a
 * retail Zero 2 W: the parked core reads 0 until released, wakes on the write +
 * SEV, and enters at EL2. The low page is not in the SoC's flat MMU map, so it
 * is mapped Device-nGnRnE at runtime (writes reach DRAM with no cache flush,
 * which the cache-off parked core observes immediately).
 *
 * TODO: read each core's release address from its DT cpu node's
 * `cpu-release-addr` (Linux smp_spin_table style) instead of the 0xd8 formula.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/pm_cpu_ops.h>

#define SPIN_TABLE_BASE   0xd8UL          /* spin_cpu0; core N at +N*8 */
#define LOW_PAGE_PHYS     0x0UL
#define LOW_PAGE_SIZE     0x10000UL       /* CONFIG_MMU_PAGE_SIZE granule */

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	unsigned long aff0 = cpuid & 0xffUL;   /* flat single-cluster A53 */
	static mm_reg_t low_va;
	static bool mapped;

	if (aff0 == 0UL || aff0 > 3UL) {
		return -EINVAL;
	}

	if (!mapped) {
		device_map(&low_va, LOW_PAGE_PHYS, LOW_PAGE_SIZE, K_MEM_CACHE_NONE);
		mapped = true;
	}

	*(volatile uint64_t *)(low_va + SPIN_TABLE_BASE + aff0 * 8UL) =
		(uint64_t)entry_point;
	barrier_dsync_fence_full();
	__asm__ volatile("sev" ::: "memory");

	return 0;
}
