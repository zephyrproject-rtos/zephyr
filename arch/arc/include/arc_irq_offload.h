/*
 * Copyright (c) 2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_ARC_IRQ_OFFLOAD_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_ARC_IRQ_OFFLOAD_H_

#ifdef CONFIG_IRQ_OFFLOAD

int arc_irq_offload_init(const struct device *unused);

static inline void arc_irq_offload_init_smp(void)
{
	arc_irq_offload_init(NULL);
}

#else

static inline void arc_irq_offload_init_smp(void) {}

#endif /* CONFIG_IRQ_OFFLOAD */

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_ARC_IRQ_OFFLOAD_H_ */
