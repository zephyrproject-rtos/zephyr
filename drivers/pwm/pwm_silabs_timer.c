/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_timer_pwm

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <sl_hal_timer.h>

LOG_MODULE_REGISTER(pwm_silabs_timer, CONFIG_PWM_LOG_LEVEL);

struct silabs_timer_pwm_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	TIMER_TypeDef *base;
	void (*irq_config_func)(const struct device *dev);
	uint16_t clock_div;
	uint8_t num_channels;
	uint8_t counter_size;
	bool run_in_debug;
};

struct silabs_timer_pwm_data {
	pwm_flags_t flags;
	pwm_capture_callback_handler_t cb;
	void *user_data;
	int skip_trigger;
};

static int silabs_timer_pwm_set_cycles(const struct device *dev, uint32_t channel,
				       uint32_t period_cycles, uint32_t pulse_cycles,
				       pwm_flags_t flags)
{
	bool invert_polarity = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	const struct silabs_timer_pwm_config *config = dev->config;

	if (channel > config->num_channels) {
		return -EINVAL;
	}

	if (LOG2(period_cycles) >= config->counter_size ||
	    LOG2(pulse_cycles) >= config->counter_size) {
		return -ENOTSUP;
	}

	if ((config->base->CC[channel].CFG & _TIMER_CC_CFG_MODE_MASK) != TIMER_CC_CFG_MODE_PWM) {
		sl_hal_timer_channel_config_t ch_config = SL_HAL_TIMER_CHANNEL_CONFIG_PWM;
		uint32_t timer_status = config->base->STATUS;

		ch_config.output_invert = invert_polarity;
		sl_hal_timer_channel_init(config->base, channel, &ch_config);
		sl_hal_timer_enable(config->base);
		sl_hal_timer_wait_sync(config->base);

		/* The channel init function disables and reenables the timer, which may cause
		 * pending pulse updates on other channels to be lost. Re-arm the compare buffer
		 * with its existing content to ensure an OCB->OC update will happen if an update
		 * was pending when the timer was disabled. The same issue applies to pending
		 * period updates, but the period will be unconditionally updated below since all
		 * channels share a single period.
		 */
		for (int i = 0; i < config->num_channels; i++) {
			if (channel != i && (timer_status & BIT(_TIMER_STATUS_OCBV0_SHIFT + i))) {
				config->base->CC[i].OCB = config->base->CC[i].OCB;
			}
		}
	} else {
		if (invert_polarity) {
			config->base->CC_SET[channel].CTRL = TIMER_CC_CTRL_OUTINV;
		} else {
			config->base->CC_CLR[channel].CTRL = TIMER_CC_CTRL_OUTINV;
		}
	}

	if (config->base->STATUS & TIMER_STATUS_RUNNING) {
		sl_hal_timer_set_top_buffer(config->base, period_cycles - 1);
		sl_hal_timer_channel_set_compare_buffer(config->base, channel, pulse_cycles);
	} else {
		sl_hal_timer_set_top(config->base, period_cycles - 1);
		sl_hal_timer_channel_set_compare(config->base, channel, pulse_cycles);
		sl_hal_timer_start(config->base);
	}

	return 0;
}

static int silabs_timer_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					       uint64_t *cycles)
{
	const struct silabs_timer_pwm_config *config = dev->config;
	uint32_t clock_rate;
	int err;

	if (channel > config->num_channels) {
		return -EINVAL;
	}

	err = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg,
				     &clock_rate);
	if (err < 0) {
		return err;
	}

	*cycles = clock_rate / config->clock_div;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int silabs_timer_pwm_configure_capture(const struct device *dev, uint32_t channel,
					      pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					      void *user_data)
{
	sl_hal_timer_channel_config_t ch_config = SL_HAL_TIMER_CHANNEL_CONFIG_DEFAULT;
	bool invert_polarity = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	const struct silabs_timer_pwm_config *config = dev->config;
	struct silabs_timer_pwm_data *data = dev->data;

	if (channel != 0) {
		LOG_ERR("Only channel 0 is supported for capture");
		return -ENOTSUP;
	}

	if (config->base->IEN & TIMER_IEN_CC0) {
		LOG_ERR("Capture in progress");
		return -EBUSY;
	}

	data->flags = flags;
	data->cb = cb;
	data->user_data = user_data;

	switch (flags & PWM_CAPTURE_TYPE_MASK) {
	case PWM_CAPTURE_TYPE_PERIOD:
		ch_config.input_capture_edge = invert_polarity ? SL_HAL_TIMER_CHANNEL_EDGE_FALLING
							       : SL_HAL_TIMER_CHANNEL_EDGE_RISING;
		break;
	case PWM_CAPTURE_TYPE_PULSE:
		ch_config.input_capture_edge = invert_polarity ? SL_HAL_TIMER_CHANNEL_EDGE_RISING
							       : SL_HAL_TIMER_CHANNEL_EDGE_FALLING;
		break;
	case PWM_CAPTURE_TYPE_BOTH:
		ch_config.input_capture_edge = SL_HAL_TIMER_CHANNEL_EDGE_BOTH;
		/* Select the opposite edge of the one we want the interrupt to trigger on due to an
		 * issue on Series 2 devices. The interrupt will occur with the correct capture data
		 * for the most recent edge, but the previous edge is used to decide if the
		 * interrupt will fire.
		 */
		ch_config.input_capture_event = invert_polarity
							? SL_HAL_TIMER_CHANNEL_EVENT_RISING
							: SL_HAL_TIMER_CHANNEL_EVENT_FALLING;
		break;
	default:
		LOG_ERR("Invalid capture type");
		return -EINVAL;
	}

	ch_config.channel_mode = SL_HAL_TIMER_CHANNEL_MODE_CAPTURE;
	sl_hal_timer_channel_init(config->base, channel, &ch_config);
	sl_hal_timer_enable(config->base);

	config->base->CTRL_CLR = _TIMER_CTRL_RISEA_MASK | _TIMER_CTRL_FALLA_MASK;
	config->base->CTRL_SET =
		invert_polarity ? TIMER_CTRL_FALLA_RELOADSTART : TIMER_CTRL_RISEA_RELOADSTART;

	sl_hal_timer_set_top(config->base, GENMASK(config->counter_size - 1, 0));

	return 0;
}

static int silabs_timer_pwm_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct silabs_timer_pwm_config *config = dev->config;
	struct silabs_timer_pwm_data *data = dev->data;

	if (channel != 0) {
		LOG_ERR("Only channel 0 is supported for capture");
		return -ENOTSUP;
	}

	if (config->base->IEN & TIMER_IEN_CC0) {
		LOG_ERR("Capture in progress");
		return -EBUSY;
	}

	/* Skip the first two interrupts. This should have been 1 if not for an issue on Series 2 */
	data->skip_trigger = 2;

	while (!(config->base->STATUS & TIMER_STATUS_ICFEMPTY0)) {
		sl_hal_timer_channel_get_capture(config->base, 0);
	}
	sl_hal_timer_clear_interrupts(config->base, _TIMER_IF_MASK);

	sl_hal_timer_enable_interrupts(config->base, TIMER_IEN_CC0);
	sl_hal_timer_start(config->base);

	return 0;
}

static int silabs_timer_pwm_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct silabs_timer_pwm_config *config = dev->config;

	if (channel != 0) {
		LOG_ERR("Only channel 0 is supported for capture");
		return -ENOTSUP;
	}

	sl_hal_timer_disable_interrupts(config->base, TIMER_IEN_CC0);
	sl_hal_timer_clear_interrupts(config->base, _TIMER_IF_MASK);

	return 0;
}

static void silabs_timer_pwm_isr(const struct device *dev)
{
	const struct silabs_timer_pwm_config *config = dev->config;
	struct silabs_timer_pwm_data *data = dev->data;
	uint32_t period_cycles = 0;
	uint32_t pulse_cycles = 0;

	if (!(sl_hal_timer_get_enabled_pending_interrupts(config->base) & TIMER_IF_CC0)) {
		return;
	}

	sl_hal_timer_clear_interrupts(config->base, TIMER_IF_CC0);

	if (data->skip_trigger) {
		data->skip_trigger--;
		while (!(config->base->STATUS & TIMER_STATUS_ICFEMPTY0)) {
			sl_hal_timer_channel_get_capture(config->base, 0);
		}
		return;
	}

	switch (data->flags & PWM_CAPTURE_TYPE_MASK) {
	case PWM_CAPTURE_TYPE_PERIOD:
		period_cycles = sl_hal_timer_channel_get_capture(config->base, 0);
		break;
	case PWM_CAPTURE_TYPE_PULSE:
		pulse_cycles = sl_hal_timer_channel_get_capture(config->base, 0);
		break;
	case PWM_CAPTURE_TYPE_BOTH:
		pulse_cycles = sl_hal_timer_channel_get_capture(config->base, 0);
		period_cycles = sl_hal_timer_channel_get_capture(config->base, 0);
		break;
	default:
		return;
	}

	if ((data->flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_SINGLE) {
		silabs_timer_pwm_disable_capture(dev, 0);
	}

	if (data->cb) {
		data->cb(dev, 0, period_cycles, pulse_cycles, 0, data->user_data);
	}
}
#endif

static int silabs_timer_pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct silabs_timer_pwm_config *config = dev->config;
	int err;

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = clock_control_on(config->clock_dev,
				       (clock_control_subsys_t)&config->clock_cfg);
		if (err < 0 && err != -EALREADY) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

		for (int i = 0; i < config->num_channels; i++) {
			if ((config->base->CC[i].CFG & _TIMER_CC_CFG_MODE_MASK) !=
			    TIMER_CC_CFG_MODE_OFF) {
				sl_hal_timer_enable(config->base);
				sl_hal_timer_start(config->base);
				break;
			}
		}
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		sl_hal_timer_stop(config->base);
		sl_hal_timer_disable(config->base);

		err = clock_control_off(config->clock_dev,
					(clock_control_subsys_t)&config->clock_cfg);
		if (err < 0) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int silabs_timer_pwm_init(const struct device *dev)
{
	sl_hal_timer_config_t timer_config = SL_HAL_TIMER_CONFIG_DEFAULT;
	const struct silabs_timer_pwm_config *config = dev->config;
	int err;

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	timer_config.debug_run = config->run_in_debug;
	timer_config.prescaler = config->clock_div - 1;
	sl_hal_timer_init(config->base, &timer_config);

	if (config->irq_config_func) {
		config->irq_config_func(dev);
	}

	return pm_device_driver_init(dev, silabs_timer_pwm_pm_action);
}

static DEVICE_API(pwm, silabs_timer_pwm_api) = {
	.set_cycles = silabs_timer_pwm_set_cycles,
	.get_cycles_per_sec = silabs_timer_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = silabs_timer_pwm_configure_capture,
	.enable_capture = silabs_timer_pwm_enable_capture,
	.disable_capture = silabs_timer_pwm_disable_capture,
#endif
};

#ifdef CONFIG_PWM_CAPTURE
#define TIMER_IRQ_CONFIG_FUNC(inst) silabs_timer_pwm_irq_config_##inst
#define TIMER_IRQ_CONFIG_HANDLER(inst)                                                             \
	static void silabs_timer_pwm_irq_config_##inst(const struct device *dev)                   \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ(DT_INST_PARENT(inst), irq),                                     \
			    DT_IRQ(DT_INST_PARENT(inst), priority), silabs_timer_pwm_isr,          \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_IRQ(DT_INST_PARENT(inst), irq));                                     \
	}
#else
#define TIMER_IRQ_CONFIG_FUNC(inst) NULL
#define TIMER_IRQ_CONFIG_HANDLER(inst)
#endif

#define TIMER_PWM_INIT(inst)                                                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	PM_DEVICE_DT_INST_DEFINE(inst, silabs_timer_pwm_pm_action);                                \
	TIMER_IRQ_CONFIG_HANDLER(inst)                                                             \
                                                                                                   \
	static const struct silabs_timer_pwm_config timer_pwm_config_##inst = {                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(inst))),                  \
		.clock_cfg = SILABS_DT_CLOCK_CFG(DT_INST_PARENT(inst)),                            \
		.base = (TIMER_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(inst)),                        \
		.irq_config_func = TIMER_IRQ_CONFIG_FUNC(inst),                                    \
		.clock_div = DT_PROP(DT_INST_PARENT(inst), clock_div),                             \
		.num_channels = DT_PROP(DT_INST_PARENT(inst), channels),                           \
		.counter_size = DT_PROP(DT_INST_PARENT(inst), counter_size),                       \
		.run_in_debug = DT_PROP(DT_INST_PARENT(inst), run_in_debug),                       \
	};                                                                                         \
	static struct silabs_timer_pwm_data timer_pwm_data_##inst;                                 \
	DEVICE_DT_INST_DEFINE(inst, &silabs_timer_pwm_init, PM_DEVICE_DT_INST_GET(inst),           \
			      &timer_pwm_data_##inst, &timer_pwm_config_##inst, POST_KERNEL,       \
			      CONFIG_PWM_INIT_PRIORITY, &silabs_timer_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(TIMER_PWM_INIT)
