/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_IRQ_CONNECT_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_IRQ_CONNECT_H_

#ifdef CONFIG_GEN_ISR_TABLES

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p)

#else /* !CONFIG_GEN_ISR_TABLES */

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	arch_irq_connect_dynamic(irq_p, priority_p, isr_p, isr_param_p, flags_p)

#endif /* CONFIG_GEN_ISR_TABLES */

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_IRQ_CONNECT_H_ */
