/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_ARM_ARCH_TIMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_ARM_ARCH_TIMER_H_

#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/types.h>

#define ARM_TIMER_SECURE_IRQ		DT_ARM_ARM_TIMER_TIMER_IRQ_0
#define ARM_TIMER_NON_SECURE_IRQ	DT_ARM_ARM_TIMER_TIMER_IRQ_1
#define ARM_TIMER_VIRTUAL_IRQ		DT_ARM_ARM_TIMER_TIMER_IRQ_2
#define ARM_TIMER_HYP_IRQ		DT_ARM_ARM_TIMER_TIMER_IRQ_3

#define ARM_TIMER_FLAGS			IRQ_TYPE_EDGE

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_ARM_ARCH_TIMER_H_ */
