/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This header provides constants for most IRQ bindings.
 *
 * Most IRQ bindings include a flags cell as part of the IRQ specifier.
 * In most cases, the format of the flags cell uses the standard values
 * defined in this header.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IRQ_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IRQ_H_

#define IRQ_TYPE_NONE		0
#define IRQ_TYPE_EDGE_RISING	1
#define IRQ_TYPE_EDGE_FALLING	2
#define IRQ_TYPE_EDGE_BOTH	(IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH	4
#define IRQ_TYPE_LEVEL_LOW	8

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IRQ_H_ */
