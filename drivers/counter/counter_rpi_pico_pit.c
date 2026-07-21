/*
 * Copyright (c) 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_pit

#include <hardware/pwm.h>
#include <hardware/structs/pwm.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include "counter_rpi_pico_pit.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_rpi_pico_pit, CONFIG_COUNTER_LOG_LEVEL);

struct counter_rpi_pico_pit_data {
	/* List containing slice callbacks*/
	sys_slist_t cb;
};

struct counter_rpi_pico_pit_config {
	void (*irq_config_func)(const struct device *dev);
	const struct device *clk_dev;
	const clock_control_subsys_t clk_id;
};

static void counter_rpi_pico_pit_isr(const struct device *dev)
{
	/* ISR to call the registered callbacks of the pit channels if the corresponding slice
	 * wrapped and an interrupt is registered for that slice, derived from the gpio api /
	 * gpio_utils
	 */
	struct counter_rpi_pico_pit_data *data = dev->data;
	struct rpi_pico_pit_callback *cb, *tmp;

	uint32_t status_mask = pwm_get_irq_status_mask();

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->cb, cb, tmp, node) {
		if (status_mask & BIT(cb->slice)) {
			__ASSERT(cb->callback, "No callback handler!");
			cb->callback(dev, cb->top_user_data);
			pwm_clear_irq(cb->slice);
		}
	}
}

uint32_t counter_rpi_pico_pit_get_pending_int(const struct device *dev, uint32_t channel)
{
	/* Read register to see which pwm slice wrapped and caused an interrupt. Compare this
	 * against the provided slice to determine if it was responsible. Note: pwm_set_irq_enabled
	 * needs to be set to true for the wrap to generate an interrupt which can be detected with
	 * this function
	 */
	uint32_t status_mask = pwm_get_irq_status_mask();

	return status_mask & BIT(channel);
}

int counter_rpi_pico_pit_manage_callback(const struct device *dev,
					 struct rpi_pico_pit_callback *callback, bool set)
{
	/* Allows pit channels to register and unregister interrupt callbacks, derived from the gpio
	 * api / gpio_utils
	 */
	struct counter_rpi_pico_pit_data *data = dev->data;

	__ASSERT(callback, "No callback!");

	if (!sys_slist_is_empty(&data->cb)) {
		if (!sys_slist_find_and_remove(&data->cb, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	}

	if (set) {
		sys_slist_prepend(&data->cb, &callback->node);
		pwm_set_irq_enabled(callback->slice, true);
	} else {
		pwm_set_irq_enabled(callback->slice, false);
		pwm_clear_irq(callback->slice);
	}

	return 0;
}

int counter_rpi_pico_pit_get_base_frequency(const struct device *dev, uint32_t *frequency)
{
	const struct counter_rpi_pico_pit_config *cfg = dev->config;
	uint32_t pclk;

	int ret = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &pclk);

	if ((ret < 0) || (pclk == 0)) {
		return -EINVAL;
	}

	*frequency = pclk;
	return 0;
}

static int counter_rpi_pico_pit_init(const struct device *dev)
{
	const struct counter_rpi_pico_pit_config *config = dev->config;

	config->irq_config_func(dev);

	return 0;
}

#define COUNTER_RPI_PICO_PIT(inst)                                                                 \
	static void counter_rpi_pico_pit_##inst##_irq_config(const struct device *dev)             \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_DRV_INST(inst)), DT_IRQ(DT_DRV_INST(inst), priority),       \
			    counter_rpi_pico_pit_isr, DEVICE_DT_INST_GET(inst), 0);                \
		irq_enable(DT_IRQN(DT_DRV_INST(inst)));                                            \
	}                                                                                          \
	static const struct counter_rpi_pico_pit_config counter_##inst##_config = {                \
		.irq_config_func = counter_rpi_pico_pit_##inst##_irq_config,                       \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(inst, clocks, 0, clk_id),     \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
	};                                                                                         \
	static struct counter_rpi_pico_pit_data counter_##inst##_data;                             \
	DEVICE_DT_INST_DEFINE(inst, counter_rpi_pico_pit_init, NULL, &counter_##inst##_data,       \
			      &counter_##inst##_config, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RPI_PICO_PIT)
