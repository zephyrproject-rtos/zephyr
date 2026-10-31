/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/* PL011 exposes its interrupt API for each UART with an interrupts property. */
#define RP1_UART_IRQ_ASSERT(node_id)                                                               \
	BUILD_ASSERT(!DT_NODE_HAS_COMPAT(node_id, arm_pl011) ||                                    \
			     !DT_NODE_HAS_PROP(node_id, interrupts),                               \
		     DT_NODE_PATH(node_id) " requires CONFIG_RP1_IRQ for interrupt-driven UART");

#if DT_NODE_EXISTS(DT_NODELABEL(rp1_intc))
DT_FOREACH_CHILD_STATUS_OKAY(DT_PARENT(DT_NODELABEL(rp1_intc)), RP1_UART_IRQ_ASSERT)
#endif
