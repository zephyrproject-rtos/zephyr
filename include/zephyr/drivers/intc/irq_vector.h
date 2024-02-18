/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_VECTOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_VECTOR_H_

#include <zephyr/devicetree.h>
#include <zephyr/drivers/intc/intl.h>

/**
 * @brief Override IRQ vector in vector table by index
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>, <2 2>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_DEFINE_IRQ_VECTOR_BY_IDX(DT_NODELABEL(foo), 0)
 *     {
 *             return 1;
 *     }
 *
 *     SYS_DT_DEFINE_IRQ_VECTOR_BY_IDX(DT_NODELABEL(foo), 1)
 *     {
 *             return 1;
 *     }
 *
 * @note See ISR_DIRECT_DECLARE()
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 */
#define INTC_DT_DEFINE_IRQ_VECTOR_BY_IDX(node_id, idx)				\
	ARCH_ISR_DIRECT_DECLARE(						\
		INTC_DT_VECTOR_SYMBOL(						\
			DT_IRQ_INTC_BY_IDX(node_id, idx),			\
			DT_IRQ_BY_IDX(node_id, idx, irq)			\
		)								\
	)

/**
 * @brief Override IRQ vector in vector table by name
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>, <2 2>;
 *             interrupt-names = "bar", "baz";
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_DEFINE_IRQ_VECTOR_BY_NAME(DT_NODELABEL(foo), bar)
 *     {
 *             return 1;
 *     }
 *
 *     SYS_DT_DEFINE_IRQ_VECTOR_BY_NAME(DT_NODELABEL(foo), baz)
 *     {
 *             return 1;
 *     }
 *
 * @note See ISR_DIRECT_DECLARE()
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 */
#define INTC_DT_DEFINE_IRQ_VECTOR_BY_NAME(node_id, name)			\
	ARCH_ISR_DIRECT_DECLARE(						\
		INTC_DT_VECTOR_SYMBOL(						\
			DT_IRQ_INTC_BY_NAME(node_id, name),			\
			DT_IRQ_BY_NAME(node_id, name, irq)			\
		)								\
	)

/**
 * @brief Override IRQ vector in vector table
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_DEFINE_IRQ_VECTOR(DT_NODELABEL(foo))
 *     {
 *             return 1;
 *     }
 *
 * @note See ISR_DIRECT_DECLARE()
 *
 * @param node_id Interrupt generating device's devicetree node
 */
#define INTC_DT_DEFINE_IRQ_VECTOR(node_id) \
	INTC_DT_DEFINE_IRQ_VECTOR_BY_IDX(node_id, 0)

/**
 * @brief Device driver instance variants of INTC_DT_DEFINE_IRQ_VECTOR macros
 * @{
 */

#define INTC_DT_INST_DEFINE_IRQ_VECTOR_BY_IDX(node_id, idx) \
	INTC_DT_DEFINE_IRQ_VECTOR_BY_IDX(DT_DRV_INST(node_id), idx)

#define INTC_DT_INST_DEFINE_IRQ_VECTOR_BY_NAME(node_id, name) \
	INTC_DT_DEFINE_IRQ_VECTOR_BY_NAME(DT_DRV_INST(node_id), name)

#define INTC_DT_INST_DEFINE_IRQ_VECTOR(node_id) \
	INTC_DT_DEFINE_IRQ_VECTOR(DT_DRV_INST(node_id))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_IRQ_VECTOR_H_ */
