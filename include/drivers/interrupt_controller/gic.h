/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_GIC_H_

int arm_gic_init(void);

void arm_gic_irq_enable(unsigned int irq);
void arm_gic_irq_disable(unsigned int irq);
int arm_gic_irq_is_enabled(unsigned int irq);
unsigned int arm_gic_irq_get_active(void);
void arm_gic_irq_eoi(unsigned int irq);

void arm_gic_irq_set_priority(
	unsigned int irq, unsigned int prio, unsigned int flags);

#endif /* ZEPHYR_INCLUDE_DRIVERS_GIC_400_H_ */
