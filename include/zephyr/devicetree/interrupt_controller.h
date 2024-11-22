/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt controller devicetree macro public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_INTERRUPT_CONTROLLER_H_
#define ZEPHYR_INCLUDE_DEVICETREE_INTERRUPT_CONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/**
 * @defgroup devicetree-interrupt_controller Devicetree Interrupt Controller API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the aggregator level of an interrupt controller
 *
 * @note Aggregator level is equivalent to IRQ_LEVEL + 1 (a 2nd level aggregator has Zephyr level 1
 * IRQ encoding)
 *
 * @param node_id node identifier of an interrupt controller
 *
 * @return Level of the interrupt controller
 */
#define DT_INTC_GET_AGGREGATOR_LEVEL(node_id) UTIL_INC(DT_IRQ_LEVEL(node_id))

/**
 * @brief Get the aggregator level of a `DT_DRV_COMPAT` interrupt controller
 *
 * @note Aggregator level is equivalent to IRQ_LEVEL + 1 (a 2nd level aggregator has Zephyr level 1
 * IRQ encoding)
 *
 * @param inst instance of an interrupt controller
 *
 * @return Level of the interrupt controller
 */
#define DT_INST_INTC_GET_AGGREGATOR_LEVEL(inst) DT_INTC_GET_AGGREGATOR_LEVEL(DT_DRV_INST(inst))

/**
 * @brief Get a interrupt controller specifier's flags cell
 *
 * This macro expects interrupt specifiers with cells named "sense" | "priority".
 * If there is no "sense" or "priority" cell in the interrupt specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * Example devicetree fragment:
 *
 *     intc1: interrupt-controller@... {
 *             compatible = "vnd,interrupt";
 *             #interrupt-cells = <2>;
 *     };
 *
 *     intc2: interrupt-controller@... {
 *             compatible = "vnd,interrupt";
 *             #interrupt-cells = <2>;
 *     };
 *
 *     n: node {
 *             interrupts-extended = <&intc1 3 2 15>,
 *                                   <&intc2 4 3 16>;
 *     };
 *
 * Bindings fragment for the vnd,interrupt compatible:
 *
 *     interrupt-cells:
 *       - irq
 *       - sense
 *       - priority
 *
 * Example usage:
 *
 *     DT_INTC_FLAGS_BY_IDX(DT_NODELABEL(n), 0) // ((2 << 8) | (15 << 0))
 *     DT_INTC_FLAGS_BY_IDX(DT_NODELABEL(n), 1) // ((3 << 8) | (16 << 0))
 *
 * @param inst instance of an interrupt controller
 * @return the flags cell value at index "idx", or zero if there is none
 */
#define DT_INTC_FLAGS_BY_IDX(inst, idx) \
	((DT_INST_IRQ_BY_IDX(inst, idx, sense) << 8) \
	| (DT_INST_IRQ_BY_IDX(inst, idx, priority)) << 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_INTERRUPT_CONTROLLER_H_ */
