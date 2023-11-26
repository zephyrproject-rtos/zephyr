/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IRQ_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif

/* IRQ_TYPE flags */
#define IRQ_TYPE_EDGE_RISING  (1U << 0)
#define IRQ_TYPE_EDGE_FALLING (1U << 1)
#define IRQ_TYPE_EDGE_BOTH    (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH   (1U << 2)
#define IRQ_TYPE_LEVEL_LOW    (1U << 3)
#define IRQ_TYPE_MASK         (IRQ_TYPE_EDGE_BOTH | IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)

/* Test if IRQ_TYPE flags are valid */
#define IRQ_TYPE_IS_VALID(flags)                               \
	(((flags & IRQ_TYPE_MASK) == IRQ_TYPE_EDGE_RISING)  || \
	 ((flags & IRQ_TYPE_MASK) == IRQ_TYPE_EDGE_FALLING) || \
	 ((flags & IRQ_TYPE_MASK) == IRQ_TYPE_EDGE_BOTH)    || \
	 ((flags & IRQ_TYPE_MASK) == IRQ_TYPE_LEVEL_HIGH)   || \
	 ((flags & IRQ_TYPE_MASK) == IRQ_TYPE_LEVEL_LOW))

/* Test if IRQ_TYPE flags dictate edge triggered IRQ */
#define IRQ_TYPE_IS_EDGE_TRIGGERED(flags) \
	(flags & IRQ_TYPE_EDGE_BOTH)

/* Test if IRQ_TYPE flags dictate level triggered IRQ */
#define IRQ_TYPE_IS_LEVEL_TRIGGERED(flags) \
	(flags & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW))

/* Test if IRQ_TYPE flags dictate active high IRQ polarity */
#define IRQ_TYPE_IS_ACTIVE_HIGH(flags) \
	(flags & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_LEVEL_HIGH))

/* Test if IRQ_TYPE flags dictate active low IRQ polarity */
#define IRQ_TYPE_IS_ACTIVE_LOW(flags) \
	(flags & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_LEVEL_LOW))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IRQ_H_ */
