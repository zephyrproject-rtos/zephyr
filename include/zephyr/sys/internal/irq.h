/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_INTERNAL_IRQ_H_
#define ZEPHYR_INCLUDE_SYS_INTERNAL_IRQ_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/** IRQ interrupt line structure */
struct sys_irq_intl {
	/** Interrupt controller */
	const struct device *intc;
	/** Interrupt line number */
	uint16_t intln;
};

/** Include generated interrupt lines */
#include <sys_irq_internal_generated.h>

#endif /* ZEPHYR_INCLUDE_SYS_INTERNAL_IRQ_H_ */
