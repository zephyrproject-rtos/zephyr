/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/**
 * @brief Device specific implementations of INTC_DT_IRQ_FLAGS macros
 * @{
 */

/** @} */

/**
 * @brief Get interrupt controller's upper compatible
 *
 * @param node_id Interrupt controller's devicetree node
 */
#define INTC_DT_COMPAT_UPPER(node_id) \
	DT_STRING_UPPER_TOKEN_BY_IDX(node_id, compatible, 0)

/**
 * @brief Get a devicetree node's IRQ's flags from the devicetree by index
 *
 * @details The encoding of the IRQ's flags is specific to each interrupt
 * controller, tied to its devicetree compatible, defined in
 * include/zephyr/drivers/intc/irq_flags.h
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 *
 * Example:
 *
 *     static struct foo_config foo_config = {
 *             .irq_flags = INTC_DT_IRQ_FLAGS_BY_IDX(DT_ALIAS(sensor), 0),
 *     };
 */
#define INTC_DT_IRQ_FLAGS_BY_IDX(node_id, idx)							\
	_CONCAT_3(										\
		INTC_DT_,									\
		INTC_DT_COMPAT_UPPER(DT_IRQ_INTC_BY_IDX(node_id, idx)),				\
		_IRQ_FLAGS_BY_IDX								\
	)(node_id, idx)

/**
 * @brief Get a devicetree node's IRQ's flags from the devicetree by name
 *
 * @details The encoding of the IRQ's flags is specific to each interrupt
 * controller, tied to its devicetree compatible, defined in
 * include/zephyr/drivers/intc/irq_flags.h
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 *
 * Example:
 *
 *     static struct foo_config foo_config = {
 *             .irq_flags = INTC_DT_IRQ_FLAGS_BY_NAME(DT_ALIAS(sensor), int1),
 *     };
 */
#define INTC_DT_IRQ_FLAGS_BY_NAME(node_id, name)						\
	_CONCAT_3(										\
		INTC_DT_,									\
		INTC_DT_COMPAT_UPPER(DT_IRQ_INTC_BY_NAME(node_it, name)),			\
		_IRQ_FLAGS_BY_NAME								\
	)(node_id, name)

/**
 * @brief Get a devicetree node's IRQ's flags from the devicetree
 *
 * @details The encoding of the IRQ's flags is specific to each interrupt
 * controller, tied to its devicetree compatible, defined in
 * include/zephyr/drivers/intc/irq_flags.h
 *
 * @param node_id Interrupt generating device's devicetree node
 *
 * Example:
 *
 *     static struct foo_config foo_config = {
 *             .irq_flags = INTC_DT_IRQ_FLAGS(DT_ALIAS(sensor)),
 *     };
 */
#define INTC_DT_IRQ_FLAGS(node_id) \
	INTC_DT_IRQ_FLAGS_BY_IDX(node_id, 0)

/**
 * @brief Device driver instance variants of INTC_DT_IRQ_FLAGS macros
 * @{
 */

#define INTC_DT_INST_IRQ_FLAGS_BY_IDX(node_id, idx) \
	INTC_DT_IRQ_FLAGS_BY_IDX(DT_DRV_INST(node_id), idx)

#define INTC_DT_INST_IRQ_FLAGS_BY_NAME(node_id, name) \
	INTC_DT_IRQ_FLAGS_BY_NAME(DT_DRV_INST(node_id), name)

#define INTC_DT_INST_IRQ_FLAGS(node_id) \
	INTC_DT_IRQ_FLAGS(DT_DRV_INST(node_id))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_FLAGS_H_ */
