/*
 * Copyright (c) 2021 Fabio Baltieri
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "sx126x_common.h"

#include <stm32wlxx_ll_exti.h>
#include <stm32wlxx_ll_pwr.h>
#include <stm32wlxx_ll_rcc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_DECLARE(sx126x, CONFIG_LORA_LOG_LEVEL);

void sx126x_reset(struct sx126x_data *dev_data)
{
	LL_RCC_RF_EnableReset();
	k_sleep(K_MSEC(20));
	LL_RCC_RF_DisableReset();
	k_sleep(K_MSEC(10));
}

bool sx126x_is_busy(struct sx126x_data *dev_data)
{
	return LL_PWR_IsActiveFlag_RFBUSYS();
}

uint32_t sx126x_get_dio1_pin_state(struct sx126x_data *dev_data)
{
	return 0;
}

void sx126x_dio1_irq_enable(struct sx126x_data *dev_data)
{
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));
}

void sx126x_dio1_irq_disable(struct sx126x_data *dev_data)
{
	irq_disable(DT_INST_IRQN(0));
}

static void radio_isr(const struct device *dev)
{
	struct sx126x_data *dev_data = dev->data;

	irq_disable(DT_INST_IRQN(0));
	k_work_submit(&dev_data->dio1_irq_work);
}

int sx126x_variant_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    radio_isr, DEVICE_DT_INST_GET(0), 0);
	LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_44);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}
