/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_GIC_CPU_TARGETS_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_GIC_CPU_TARGETS_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/atomic.h>

struct gic_cpu_target {
	uint64_t mpidr;
	atomic_t mask;
};

/* Each slot has one CPU writer; a nonzero mask publishes immutable affinity data. */
static inline void gic_cpu_target_register(struct gic_cpu_target *target, uint64_t mpidr,
					   uint8_t mask)
{
	if (atomic_get(&target->mask) != 0) {
		return;
	}

	target->mpidr = mpidr;
	atomic_set(&target->mask, mask);
}

static inline uint8_t gic_cpu_target_mask(const struct gic_cpu_target *targets, size_t count,
					  uint64_t mpidr)
{
	for (size_t i = 0U; i < count; i++) {
		uint8_t mask = (uint8_t)atomic_get(&targets[i].mask);

		if ((mask != 0U) && (targets[i].mpidr == mpidr)) {
			return mask;
		}
	}

	return 0U;
}

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_GIC_CPU_TARGETS_H_ */
