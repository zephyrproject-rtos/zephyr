/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_VECTOR_TABLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_VECTOR_TABLE_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/irq_handler.h>
#include <zephyr/sys/util.h>

/**
 * @brief Synthesize interrupt line handler's symbol by node
 *
 * @param node_id Interrupt controller's devicetree node
 * @param intln Interrupt controller's interrupt line number
 */
#define INTC_DT_INTL_HANDLER_SYMBOL(node_id, intln) \
	_CONCAT_4(__sys_intl_handler_, DT_DEP_ORD(node_id), _, intln)

/**
 * @brief Synthesize vector table's vector's symbol
 *
 * @param node_id Interrupt controller's devicetree node
 * @param intln Interrupt controller's interrupt line number
 */
#define INTC_DT_VECTOR_SYMBOL(node_id, intln) \
	_CONCAT_4(__intc_vector_, DT_DEP_ORD(node_id), _, intln)

/**
 * @brief Device driver instance variants of INTC_DT_ macros
 * @{
 */

#define INTC_DT_INST_INTL_HANDLER_SYMBOL(inst, intln) \
	INTC_DT_INTL_HANDLER_SYMBOL(DT_DRV_INST(inst), intln)

#define INTC_DT_INST_VECTOR_SYMBOL(inst, intln) \
	INTC_DT_VECTOR_SYMBOL(DT_DRV_INST(inst), intln)

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_VECTOR_TABLE_H_ */
