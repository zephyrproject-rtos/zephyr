/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "soc.h"

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc);

#ifdef CONFIG_XLNX_INTC

/**
 * @brief Override arch_irq_enable with a call to Xintc driver
 *
 * @param irq irq number to enable
 */
void arch_irq_enable(uint32_t irq)
{
	xlnx_intc_irq_enable(irq);
}

/**
 * @brief Override arch_irq_disable with a call to Xintc driver
 *
 * @param irq irq number to disable
 */
void arch_irq_disable(uint32_t irq)
{
	xlnx_intc_irq_disable(irq);
}

/**
 * @brief Override arch_irq_is_enabled with a call to Xintc driver
 *
 * @param irq irq number to see if enabled
 */
int arch_irq_is_enabled(unsigned int irq)
{
	return BIT(irq) & xlnx_intc_irq_get_enabled();
}

/**
 * @brief Returns the currently pending interrupts.
 *
 * @return Pending IRQ bitmask. Pending IRQs will have their bitfield set to 1. 0 if no interrupt is
 * pending.
 */
uint32_t arch_irq_pending(void)
{
	return xlnx_intc_irq_pending();
};

/**
 * @brief Returns the vector for highest pending interrupt.
 *
 * @return Returns the vector for (i.e. index) for highest-prio/lowest-num pending interrupt to be
 * used in a jump table. This is used used for sw_isr_table.
 */
uint32_t arch_irq_pending_vector(uint32_t ipending)
{
	ARG_UNUSED(ipending);
	return xlnx_intc_irq_pending_vector();
}

#endif /* #ifdef CONFIG_XLNX_INTC */

/**
 *
 * @brief Perform basic hardware initialization
 *
 * @return 0
 */
static int soc_init(void)
{
	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
