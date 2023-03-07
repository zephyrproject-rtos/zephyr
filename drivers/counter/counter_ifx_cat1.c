/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Counter driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pinctrl.h>
#include <cyhal_timer.h>

#include <cyhal_tcpwm_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_counter, CONFIG_COUNTER_LOG_LEVEL);

/* Device config structure */
struct ifx_cat1_counter_config {
	struct counter_config_info counter_info;
	TCPWM_CNT_Type *reg_addr;
	uint32_t irqn;
	uint8_t irq_priority;
};

/* Data structure */
struct ifx_cat1_counter_data {
	cyhal_timer_t counter_obj;
	cyhal_timer_cfg_t counter_cfg;
	struct counter_alarm_cfg alarm_cfg_counter;
	struct counter_top_cfg top_value_cfg_counter;
	uint32_t guard_period;
	cyhal_resource_inst_t hw_resource;
	bool alarm_irq_flag;
};

/* Default Counter configuration structure */
const cy_stc_tcpwm_counter_config_t _cyhal_timer_default_config = {
	.period = 32768,
	.clockPrescaler = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
	.runMode = CY_TCPWM_COUNTER_CONTINUOUS,
	.countDirection = CY_TCPWM_COUNTER_COUNT_UP,
	.compareOrCapture = CY_TCPWM_COUNTER_MODE_CAPTURE,
	.compare0 = 16384,
	.compare1 = 16384,
	.enableCompareSwap = false,
	.interruptSources = CY_TCPWM_INT_NONE,
	.captureInputMode = 0x3U,
	.captureInput = CY_TCPWM_INPUT_0,
	.reloadInputMode = 0x3U,
	.reloadInput = CY_TCPWM_INPUT_0,
	.startInputMode = 0x3U,
	.startInput = CY_TCPWM_INPUT_0,
	.stopInputMode = 0x3U,
	.stopInput = CY_TCPWM_INPUT_0,
	.countInputMode = 0x3U,
	.countInput = CY_TCPWM_INPUT_1,
};

static int32_t _get_hw_block_info(TCPWM_CNT_Type *reg_addr, cyhal_resource_inst_t *hw_resource)
{
	int32_t ret = -1;
	uint32_t i;

	for (i = 0u; i < _CYHAL_TCPWM_INSTANCES; i++) {
		TCPWM_Type *base = _CYHAL_TCPWM_DATA[i].base;

		if (((uint32_t)reg_addr > (uint32_t)base) &&
		    ((uint32_t)reg_addr < (uint32_t)base + sizeof(TCPWM_Type))) {

			hw_resource->type = CYHAL_RSC_TCPWM;
			hw_resource->block_num = i;
			hw_resource->channel_num = ((((uint32_t)reg_addr) - (uint32_t)base->CNT) /
						    sizeof(TCPWM_CNT_Type));

			if (hw_resource->channel_num < _CYHAL_TCPWM_DATA[i].num_channels) {
				ret = 0;
			}
			break;
		}
	}

	return ret;
}

static void ifx_cat1_counter_event_callback(void *callback_arg, cyhal_timer_event_t event)
{
	const struct device *dev = (const struct device *) callback_arg;
	struct ifx_cat1_counter_data *const data = dev->data;
	const struct ifx_cat1_counter_config *const config = dev->config;

	/* Alarm compare/capture event */
	if ((data->alarm_cfg_counter.callback != NULL) &&
	    (((CYHAL_TIMER_IRQ_CAPTURE_COMPARE & event) == CYHAL_TIMER_IRQ_CAPTURE_COMPARE) ||
	     data->alarm_irq_flag)) {
		/* Alarm works as one-shot, so disable event */
		cyhal_timer_enable_event(&data->counter_obj,
					 CYHAL_TIMER_IRQ_CAPTURE_COMPARE,
					 config->irq_priority, false);

		/* Call User callback for Alarm */
		data->alarm_cfg_counter.callback(dev, 1, cyhal_timer_read(&data->counter_obj),
						 data->alarm_cfg_counter.user_data);
		data->alarm_irq_flag = false;
	}

	/* Top_value terminal count event */
	if ((data->top_value_cfg_counter.callback != NULL) &&
	    ((CYHAL_TIMER_IRQ_TERMINAL_COUNT & event) == CYHAL_TIMER_IRQ_TERMINAL_COUNT)) {

		/* Call User callback for top value */
		data->top_value_cfg_counter.callback(dev, data->top_value_cfg_counter.user_data);
	}

	/* NOTE: cyhal handles cleaning of interrupts */
}

static void ifx_cat1_counter_set_int_pending(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;
	const struct ifx_cat1_counter_config *const config = dev->config;

	cyhal_timer_enable_event(&data->counter_obj, CYHAL_TIMER_IRQ_CAPTURE_COMPARE,
				 config->irq_priority, true);
	NVIC_SetPendingIRQ((IRQn_Type)config->irqn);
}

static int ifx_cat1_counter_init(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	cy_rslt_t rslt;
	struct ifx_cat1_counter_data *data = dev->data;
	const struct ifx_cat1_counter_config *config = dev->config;

	/* Dedicate Counter HW resource */
	if (_get_hw_block_info(config->reg_addr, &data->hw_resource) != 0) {
		return -EIO;
	}

	cyhal_timer_configurator_t timer_configurator = {
		.resource = &data->hw_resource,
		.config = &_cyhal_timer_default_config,
	};

	/* Initialize timer */
	rslt = cyhal_timer_init_cfg(&data->counter_obj, &timer_configurator);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Initialize counter structure */
	data->alarm_irq_flag = false;
	data->counter_cfg.compare_value = 0;
	data->counter_cfg.period = config->counter_info.max_top_value;
	data->counter_cfg.direction = CYHAL_TIMER_DIR_UP;
	data->counter_cfg.is_compare = true;
	data->counter_cfg.is_continuous = true;
	data->counter_cfg.value = 0;

	/* Configure timer */
	rslt = cyhal_timer_configure(&data->counter_obj, &data->counter_cfg);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Configure frequency */
	rslt = cyhal_timer_set_frequency(&data->counter_obj, config->counter_info.freq);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Register timer event callback */
	cyhal_timer_register_callback(&data->counter_obj,
				      ifx_cat1_counter_event_callback,
				      (void *) dev);

	return 0;
}

static int ifx_cat1_counter_start(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;

	if (cyhal_timer_start(&data->counter_obj) != CY_RSLT_SUCCESS) {
		return -EIO;
	}
	return 0;
}

static int ifx_cat1_counter_stop(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;

	if (cyhal_timer_stop(&data->counter_obj) != CY_RSLT_SUCCESS) {
		return -EIO;
	}
	return 0;
}

static int ifx_cat1_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(ticks != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;

	*ticks = cyhal_timer_read(&data->counter_obj);

	return 0;
}

static int ifx_cat1_counter_set_top_value(const struct device *dev,
					  const struct counter_top_cfg *cfg)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	cy_rslt_t rslt;
	struct ifx_cat1_counter_data *const data = dev->data;
	const struct ifx_cat1_counter_config *const config = dev->config;
	bool ticks_gt_period = false;

	data->top_value_cfg_counter = *cfg;
	data->counter_cfg.period = cfg->ticks;

	/* Check new top value limit */
	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	ticks_gt_period = cfg->ticks > data->counter_cfg.period;
	/* Checks if new period value is not less then old period value */
	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		data->counter_cfg.value = 0u;
	} else if (ticks_gt_period && (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE)) {
		data->counter_cfg.value = 0u;
	} else {
		/* cyhal_timer_configure resets timer counter register to value
		 * defined in config structure 'counter_cfg.value', so update
		 * counter value with current value of counter (read by
		 * cyhal_timer_read function).
		 */
		data->counter_cfg.value = cyhal_timer_read(&data->counter_obj);
	}

	if ((ticks_gt_period == false) ||
	    ((ticks_gt_period == true) && (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE))) {
		/* Reconfigure timer */
		rslt = cyhal_timer_configure(&data->counter_obj, &data->counter_cfg);
		if (rslt != CY_RSLT_SUCCESS) {
			return -EIO;
		}

		/* Register an top_value terminal count event callback handler if
		 * callback is not NULL.
		 */
		if (cfg->callback != NULL) {
			cyhal_timer_enable_event(&data->counter_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT,
						 config->irq_priority, true);
		}
	}
	return 0;
}

static uint32_t ifx_cat1_counter_get_top_value(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;

	return data->counter_cfg.period;
}

static inline bool counter_is_bit_mask(uint32_t val)
{
	/* Return true if value equals 2^n - 1 */
	return !(val & (val + 1U));
}

static uint32_t ifx_cat1_counter_ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;

	/* refer to https://tbrindus.ca/how-builtin-expect-works/ for 'likely' usage */
	if (likely(counter_is_bit_mask(top))) {
		return (val1 + val2) & top;
	}

	to_top = top - val1;

	return (val2 <= to_top) ? (val1 + val2) : (val2 - to_top - 1U);
}

static uint32_t ifx_cat1_counter_ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	/* refer to https://tbrindus.ca/how-builtin-expect-works/ for 'likely' usage */
	if (likely(counter_is_bit_mask(top))) {
		return (val - old) & top;
	}

	/* if top is not 2^n-1 */
	return (val >= old) ? (val - old) : (val + top + 1U - old);
}

static int ifx_cat1_counter_set_alarm(const struct device *dev, uint8_t chan_id,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(alarm_cfg != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;
	const struct ifx_cat1_counter_config *const config = dev->config;

	uint32_t val = alarm_cfg->ticks;
	uint32_t top_val = ifx_cat1_counter_get_top_value(dev);
	uint32_t flags = alarm_cfg->flags;
	uint32_t max_rel_val;
	bool absolute = ((flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) ? false : true;
	bool irq_on_late;
	int err = 0;

	/* Checks if compare value is not less then period value */
	if (alarm_cfg->ticks > top_val) {
		return -EINVAL;
	}

	if (absolute) {
		max_rel_val = top_val - data->guard_period;
		irq_on_late = ((flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) == 0) ? false : true;
	} else {
		/* If relative value is smaller than half of the counter range it is assumed
		 * that there is a risk of setting value too late and late detection algorithm
		 * must be applied. When late setting is detected, interrupt shall be
		 * triggered for immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CC and counter.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = val < (top_val / 2U);

		/* limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? (top_val / 2U) : top_val;
		val = ifx_cat1_counter_ticks_add(cyhal_timer_read(&data->counter_obj),
						 val, top_val);
	}

	/* Decrement value to detect also case when val == counter_read(dev). Otherwise,
	 * condition would need to include comparing diff against 0.
	 */
	uint32_t curr = cyhal_timer_read(&data->counter_obj);
	uint32_t diff = ifx_cat1_counter_ticks_sub((val - 1), curr, top_val);

	if ((absolute && (val < curr)) || (diff > max_rel_val)) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and for absolute depending
		 * on the flag.
		 */
		if (irq_on_late) {
			data->alarm_irq_flag = true;
			ifx_cat1_counter_set_int_pending(dev);
		}
	} else {
		/* Setting new compare value */
		cy_rslt_t rslt;

		data->alarm_cfg_counter = *alarm_cfg;
		data->counter_cfg.compare_value = val;

		/* cyhal_timer_configure resets timer counter register to value
		 * defined in config structure 'counter_cfg.value', so update
		 * counter value with current value of counter (read by
		 * cyhal_timer_read function).
		 */
		data->counter_cfg.value = cyhal_timer_read(&data->counter_obj);

		/* Reconfigure timer */
		rslt = cyhal_timer_configure(&data->counter_obj, &data->counter_cfg);
		if (rslt != CY_RSLT_SUCCESS) {
			return -EINVAL;
		}

		cyhal_timer_enable_event(&data->counter_obj,
					 CYHAL_TIMER_IRQ_CAPTURE_COMPARE,
					 config->irq_priority, true);
	}

	return err;
}

static int ifx_cat1_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;
	const struct ifx_cat1_counter_config *const config = dev->config;

	cyhal_timer_enable_event(&data->counter_obj, CYHAL_TIMER_IRQ_CAPTURE_COMPARE,
				 config->irq_priority, false);
	return 0;
}

static uint32_t ifx_cat1_counter_get_pending_int(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	const struct ifx_cat1_counter_config *const config = dev->config;

	return NVIC_GetPendingIRQ((IRQn_Type)config->irqn);
}

static uint32_t ifx_cat1_counter_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(dev != NULL);

	struct ifx_cat1_counter_data *const data = dev->data;

	return data->guard_period;
}

static int ifx_cat1_counter_set_guard_period(const struct device *dev, uint32_t guard,
					     uint32_t flags)
{
	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(guard < ifx_cat1_counter_get_top_value(dev));

	struct ifx_cat1_counter_data *const data = dev->data;

	data->guard_period = guard;
	return 0;
}

static const struct counter_driver_api counter_api = {
	.start = ifx_cat1_counter_start,
	.stop = ifx_cat1_counter_stop,
	.get_value = ifx_cat1_counter_get_value,
	.set_alarm = ifx_cat1_counter_set_alarm,
	.cancel_alarm = ifx_cat1_counter_cancel_alarm,
	.set_top_value = ifx_cat1_counter_set_top_value,
	.get_pending_int = ifx_cat1_counter_get_pending_int,
	.get_top_value = ifx_cat1_counter_get_top_value,
	.get_guard_period = ifx_cat1_counter_get_guard_period,
	.set_guard_period = ifx_cat1_counter_set_guard_period,
};

/* Counter driver init macros */
#define INFINEON_CAT1_COUNTER_INIT(n)						     \
										     \
	static struct ifx_cat1_counter_data ifx_cat1_counter##n##_data;		     \
										     \
	static const struct ifx_cat1_counter_config ifx_cat1_counter##n##_config = { \
		.counter_info = {						     \
				.max_top_value = (DT_INST_PROP(n, resolution) == 32) \
				? UINT32_MAX : UINT16_MAX,			     \
				.freq = DT_INST_PROP_OR(n, clock_frequency, 10000),  \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,		     \
				.channels = 1					     \
			},							     \
		.reg_addr = (TCPWM_CNT_Type *)DT_INST_REG_ADDR(n),		     \
		.irq_priority = DT_INST_IRQ(n, priority),			     \
		.irqn = DT_INST_IRQN(n)						     \
	};									     \
										     \
	DEVICE_DT_INST_DEFINE(n,						     \
			      ifx_cat1_counter_init,				     \
			      NULL, &ifx_cat1_counter##n##_data,		     \
			      &ifx_cat1_counter##n##_config, PRE_KERNEL_1,	     \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_COUNTER_INIT);
