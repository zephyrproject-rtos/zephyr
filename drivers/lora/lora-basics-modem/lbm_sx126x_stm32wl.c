/*
 * Copyright (c) 2021 Fabio Baltieri
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Board glue for the sub-GHz radio inside an STM32WL, where the SX126x core
 * has no pins of its own. Reset comes from the reset controller, busy from a
 * power register flag and the radio interrupt from the NVIC.
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>

#include "lbm_sx126x_common.h"

#define DT_DRV_COMPAT st_stm32wl_subghz_radio

LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

/* The radio interrupt shares EXTI line 44 with the busy signal. */
#define STM32WL_RADIO_EXTI_LINE LL_EXTI_LINE_44

int lbm_sx126x_pins_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Reset, busy and the interrupt are all on-die here. */
	return 0;
}

void lbm_sx126x_reset(const struct device *dev)
{
	struct lbm_sx126x_data *data = dev->data;

	LL_RCC_RF_EnableReset();
	k_sleep(K_MSEC(20));
	LL_RCC_RF_DisableReset();
	k_sleep(K_MSEC(10));

	/* The core comes out of reset asleep, so the next access has to wake it. */
	data->asleep = true;
}

bool lbm_sx126x_is_busy(const struct device *dev)
{
	ARG_UNUSED(dev);

	return LL_PWR_IsActiveFlag_RFBUSYS() != 0;
}

void lbm_driver_dio1_irq_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * The handler leaves the line masked but pending, so clear it here.
	 * Enabling it while still pending re-enters the handler at once, and
	 * the bus traffic that follows wakes a radio that meant to sleep.
	 */
	k_irq_clear_pending(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));
}

void lbm_driver_dio1_irq_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	irq_disable(DT_INST_IRQN(0));
}

static void stm32wl_radio_isr(const struct device *dev)
{
	struct lbm_sx126x_data *data = dev->data;

	/* Level triggered, so keep it masked until the work has read the cause. */
	irq_disable(DT_INST_IRQN(0));

	k_work_schedule(&data->lbm_common.op_done_work, K_NO_WAIT);
}

int lbm_sx126x_variant_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), stm32wl_radio_isr,
		    DEVICE_DT_INST_GET(0), 0);
	LL_EXTI_EnableIT_32_63(STM32WL_RADIO_EXTI_LINE);

	lbm_driver_dio1_irq_enable(dev);

	return 0;
}

int lbm_driver_add_dio1_gpio_callback(const struct device *dev, struct gpio_callback *callback,
				      gpio_callback_handler_t handler)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(handler);

	/* The radio interrupt is an NVIC line here, not a pin. */
	return -ENOTSUP;
}

int lbm_driver_remove_dio1_gpio_callback(const struct device *dev, struct gpio_callback *callback)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);

	return -ENOTSUP;
}
