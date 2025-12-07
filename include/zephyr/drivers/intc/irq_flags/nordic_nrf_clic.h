/*
 * Copyright (c) Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_NORDIC_NRF_CLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_NORDIC_NRF_CLIC_H_

#include <zephyr/devicetree.h>

#define INTC_DT_NORDIC_NRF_CLIC_IRQ_FLAGS_BY_IDX(node_id, idx) \
	DT_IRQ_BY_IDX(node_id, idx, priority)

#define INTC_DT_NORDIC_NRF_CLIC_IRQ_FLAGS_BY_NAME(node_id, name) \
	DT_IRQ_BY_NAME(node_id, name, priority)

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_NORDIC_NRF_CLIC_H_ */
