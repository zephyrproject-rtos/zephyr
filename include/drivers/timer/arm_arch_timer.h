/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_ARM_ARCH_TIMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_ARM_ARCH_TIMER_H_

#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/types.h>

#define ARM_TIMER_NODE DT_INST(0, arm_arm_timer)

#define ARM_TIMER_SECURE_IRQ		DT_IRQ_BY_IDX(ARM_TIMER_NODE, 0, irq)
#define ARM_TIMER_NON_SECURE_IRQ	DT_IRQ_BY_IDX(ARM_TIMER_NODE, 1, irq)
#define ARM_TIMER_VIRTUAL_IRQ		DT_IRQ_BY_IDX(ARM_TIMER_NODE, 2, irq)
#define ARM_TIMER_HYP_IRQ		DT_IRQ_BY_IDX(ARM_TIMER_NODE, 3, irq)

#define ARM_TIMER_FLAGS			IRQ_TYPE_EDGE

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_ARM_ARCH_TIMER_H_ */
