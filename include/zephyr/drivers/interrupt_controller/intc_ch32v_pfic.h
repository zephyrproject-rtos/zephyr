/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_CH32V_PFIC_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_CH32V_PFIC_

void ch32v_pfic_enable(unsigned int irq);
void ch32v_pfic_disable(unsigned int irq);
int ch32v_pfic_is_enabled(unsigned int irq);
void ch32v_pfic_priority_set(unsigned int irq, unsigned int prio, unsigned int flags);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_CH32V_PFIC_ */
