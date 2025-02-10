/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RA_COMMON_INTC_SOC_H_
#define ZEPHYR_SOC_RENESAS_RA_COMMON_INTC_SOC_H_

#include <zephyr/dt-bindings/interrupt-controller/renesas-ra-icu-event.h>

#define RENESAS_RA_IRQN_GET_BY_NAME(node_id, name)                                                 \
	COND_CODE_1(DT_IRQ_BY_NAME(node_id, name, irq) != RA_NVIC_IRQN_UNSPECIFIED,                \
		    (DT_IRQ_BY_NAME(node_id, name, irq)), (CONFIG_NUM_IRQS))

#define RENESAS_RA_IRQN_GET_BY_IDX(node_id, idx)                                                   \
	COND_CODE_1(DT_IRQ_BY_NAME(node_id, idx, irq) != RA_NVIC_IRQN_UNSPECIFIED,                \
		    (DT_IRQ_BY_NAME(node_id, idx, irq)), (CONFIG_NUM_IRQS))

#define RENESAS_RA_IRQ_IPL_GET_BY_NAME(node_id, name) DT_IRQ_BY_NAME(node_id, name, priority)

#define RENESAS_RA_IRQ_IPL_GET_BY_IDX(node_id, idx) DT_IRQ_BY_NAME(node_id, idx, priority)

#define RENESAS_RA_IRQ_CHECK(i, node_id)                                                           \
	BUILD_ASSERT(RA_NVIC_IRQN_UNSPECIFIED != (DT_IRQN_BY_IDX(node_id, i)),                     \
		     "This interrupt should be mapped to nvic before use.")

#define RENESAS_RA_FOREACH_IRQ_CHECK(node_id)                                                      \
	LISTIFY(DT_NUM_IRQS(node_id), RENESAS_RA_IRQ_CHECK, (;), node_id)

#endif /* ZEPHYR_SOC_RENESAS_RA_COMMON_INTC_SOC_H_ */
