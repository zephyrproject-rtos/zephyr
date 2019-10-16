/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include <interrupt_controller/gic.h>

void z_soc_irq_init(void)
{
    /* Initialise GIC-400 */
    arm_gic_init();
}

void z_soc_irq_enable(unsigned int irq)
{
    /* Enable interrupt */
    arm_gic_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
    /* Disable interrupt */
    arm_gic_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
    return arm_gic_irq_is_enabled(irq);
}

unsigned int z_soc_irq_get_active(void)
{
    return arm_gic_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
    arm_gic_irq_eoi(irq);
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved.
 *
 * @return N/A
 */
void z_soc_irq_priority_set(
    unsigned int irq, unsigned int prio, unsigned int flags)
{
    arm_gic_irq_set_priority(irq, prio, flags);
}
