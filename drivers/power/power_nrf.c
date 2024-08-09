/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <nrfx_power.h>
#include <zephyr/sys/irq.h>

#define POWER_NRF_NODE DT_NODELABEL(power)

static int power_nrf_isr_wrapper(const void *data)
{
	ARG_UNUSED(data);

	nrfx_power_irq_handler();
	return SYS_IRQ_HANDLED;
}

SYS_DT_DEFINE_IRQ_HANDLER(POWER_NRF_NODE, power_nrf_isr_wrapper, NULL);

static int power_nrf_init(void)
{
	sys_irq_configure(SYS_DT_IRQN(POWER_NRF_NODE), SYS_DT_IRQ_FLAGS(POWER_NRF_NODE));
	sys_irq_enable(SYS_DT_IRQN(POWER_NRF_NODE));
	return 0;
}

SYS_INIT(power_nrf_init, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY);
