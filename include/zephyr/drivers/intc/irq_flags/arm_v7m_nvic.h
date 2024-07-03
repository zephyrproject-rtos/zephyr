/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_ARM_V7M_NVIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_ARM_V7M_NVIC_H_

#include <zephyr/devicetree.h>

#define INTC_DT_ARM_V7M_NVIC_IRQ_FLAGS_BY_IDX(node_id, idx) \
	DT_IRQ_BY_IDX(node_id, idx, priority)

#define INTC_DT_ARM_V7M_NVIC_IRQ_FLAGS_BY_NAME(node_id, name) \
	DT_IRQ_BY_NAME(node_id, name, priority)

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_ARM_V7M_NVIC_H_ */
