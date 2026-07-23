/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/pm_cpu_ops.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spin_table, CONFIG_PM_CPU_OPS_LOG_LEVEL);

#define CPUS_NODE DT_PATH(cpus)
#define NUM_CPUS  ARRAY_SIZE(mpid_table)

#define CPU_IS_SPIN_TABLE(node_id) DT_ENUM_HAS_VALUE(node_id, enable_method, spin_table)

#define SPIN_TABLE_RELEASE_ADDR(node_id)                     \
	COND_CODE_1(CPU_IS_SPIN_TABLE(node_id),              \
	(DT_PROP_BY_IDX(node_id, cpu_release_addr, 0)), (0))

/* Per-CPU MPID, indexed identically to release_addr[] below */
static const uint64_t mpid_table[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(CPUS_NODE, DT_REG_ADDR, (,))};

/* Per-CPU spin-table release address; 0 for CPUs not using spin-table */
static const uint64_t release_addr[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(CPUS_NODE, SPIN_TABLE_RELEASE_ADDR, (,))};

BUILD_ASSERT(ARRAY_SIZE(release_addr) == NUM_CPUS, "mpid_table/release_addr size mismatch");

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	volatile uint64_t *release;
	uint8_t *release_vaddr;
	uint64_t phys;

	LOG_DBG("Powering on CPU%lu at entry 0x%lx", cpuid, entry_point);

	for (uint8_t i = 0; i < NUM_CPUS; i++) {
		if (mpid_table[i] != cpuid) {
			continue;
		}

		phys = release_addr[i];
		if (phys == 0) {
			LOG_ERR("CPU%lu has no spin-table release address", cpuid);
			return -ENOTSUP;
		}

		k_mem_map_phys_bare(&release_vaddr, phys, sizeof(uint64_t),
				    K_MEM_PERM_RW | K_MEM_CACHE_NONE);

		release = (volatile uint64_t *)release_vaddr;
		*release = entry_point;

		/* Unmap power controller registers */
		k_mem_unmap_phys_bare(release_vaddr, sizeof(uint64_t));

		/* Ensure power-on request completes */
		barrier_dsync_fence_full();

		/* Send event to wake up the target CPU from WFE loop */
		__asm__ volatile("sev" ::: "memory");

		LOG_DBG("Released CPU%lu via physical address 0x%llx", cpuid, phys);
		return 0;
	}

	LOG_ERR("Unknown CPU ID: 0x%lx", cpuid);
	return -EINVAL;
}

int pm_cpu_off(void)
{
	/*
	 * Spin-table defines no standard mechanism for a core to park
	 * itself back into a wakeable state once released.
	 */
	return -ENOTSUP;
}

int pm_system_reset(unsigned char reset_type)
{
	ARG_UNUSED(reset_type);

	/* No system-level reset call is defined by the spin-table protocol. */
	return -ENOTSUP;
}
