/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pwm/renesas_rz_pwm.h>
#include "r_mtu3.h"
#include "r_mtu3_cfg.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_renesas_rz_mtu, CONFIG_PWM_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_rz_mtu_pwm

#define INPUT_LOW  (0)
#define INPUT_HIGH (1)

#define CAPTURE_STOP  (0)
#define CAPTURE_START (1)

struct pwm_rz_mtu_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint64_t period;
	uint64_t pulse;
	bool is_busy;
	uint32_t overflows;
	bool continuous;
	uint32_t capture_channel;
	bool is_pulse_capture;
	bsp_io_port_pin_t port_pin;
};

struct pwm_rz_mtu_data {
	timer_cfg_t *fsp_cfg;
	mtu3_instance_ctrl_t *fsp_ctrl;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_rz_mtu_capture_data capture;
	bool start_flag;
	uint8_t start_source;
	uint8_t capture_source;
#endif /* CONFIG_PWM_CAPTURE */
};

struct pwm_rz_mtu_config {
	const struct pinctrl_dev_config *pincfg;
	const timer_api_t *fsp_api;
};

static void pwm_rz_disable_all_irqs(const struct device *dev)
{
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;

	irq_disable(data->fsp_cfg->cycle_end_irq);
	irq_disable(fsp_cfg_extend->capture_a_irq);
	irq_disable(fsp_cfg_extend->capture_b_irq);
}

static int pwm_rz_set_counter_clear(const struct device *dev, int counter_clear_channel)
{
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;

	switch (counter_clear_channel) {
	case RZ_PWM_MTIOCxA:
		fsp_cfg_extend->mtu3_clear = MTU3_TCNT_CLEAR_TGRA;
		break;
	case RZ_PWM_MTIOCxB:
		fsp_cfg_extend->mtu3_clear = MTU3_TCNT_CLEAR_TGRB;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pwm_rz_mtu_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
				 uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_rz_mtu_config *cfg = dev->config;
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;
	uint32_t pulse;
	fsp_err_t err;
	mtu3_io_pin_level_t out_pin_level_a;
	mtu3_io_pin_level_t out_pin_level_b;

	if ((period_cycles > UINT16_MAX) || (pulse_cycles > UINT16_MAX)) {
		LOG_INF("Fail to set period: %d", period_cycles);
		return -EINVAL;
	}

	if (channel != RZ_PWM_MTIOCxA) {
		LOG_ERR("Valid only for RZ_PWM_MTIOCxA pins");
		return -EINVAL;
	}

	out_pin_level_a = MTU3_IO_PIN_LEVEL_INITIAL_HIGH_COMPARE_HIGH;
	out_pin_level_b = MTU3_IO_PIN_LEVEL_INITIAL_HIGH_COMPARE_LOW;

	if (pulse_cycles == period_cycles) {
		/* 100% duty cycle */
		if (flags & PWM_POLARITY_INVERTED) {
			out_pin_level_a = MTU3_IO_PIN_LEVEL_INITIAL_LOW_COMPARE_LOW;
			out_pin_level_b = MTU3_IO_PIN_LEVEL_INITIAL_LOW_COMPARE_LOW;
		} else {
			out_pin_level_a = MTU3_IO_PIN_LEVEL_INITIAL_HIGH_COMPARE_HIGH;
			out_pin_level_b = MTU3_IO_PIN_LEVEL_INITIAL_HIGH_COMPARE_HIGH;
		}

		/* The PWM device apparently does not change state if pulse_cycles == period_cycles,
		 * so we have to reduce pulse_cycles by one. Due to the value of pwm_state, the
		 * signal will remain constant at compare match
		 */
		pulse_cycles--;
	}

	if (pulse_cycles == 0) {
		/* 0% duty cycle */
		if (flags & PWM_POLARITY_INVERTED) {
			out_pin_level_a = MTU3_IO_PIN_LEVEL_INITIAL_HIGH_COMPARE_HIGH;
			out_pin_level_b = MTU3_IO_PIN_LEVEL_INITIAL_HIGH_COMPARE_HIGH;
		} else {
			out_pin_level_a = MTU3_IO_PIN_LEVEL_INITIAL_LOW_COMPARE_LOW;
			out_pin_level_b = MTU3_IO_PIN_LEVEL_INITIAL_LOW_COMPARE_LOW;
		}
	}

	err = cfg->fsp_api->close(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_a = out_pin_level_a;
	fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_b = out_pin_level_b;

	err = pwm_rz_set_counter_clear(dev, channel);
	if (err) {
		return err;
	}

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	pwm_rz_disable_all_irqs(dev);

	/* Stop timer */
	err = cfg->fsp_api->stop(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = cfg->fsp_api->periodSet(data->fsp_ctrl, period_cycles);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	pulse = (flags & PWM_POLARITY_INVERTED) ? period_cycles - pulse_cycles : pulse_cycles;
	/* Update pulse cycles */
	err = cfg->fsp_api->dutyCycleSet(data->fsp_ctrl, pulse, channel);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Start timer */
	err = cfg->fsp_api->start(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
};

static int pwm_rz_mtu_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	const struct pwm_rz_mtu_config *cfg = dev->config;
	struct pwm_rz_mtu_data *data = dev->data;
	timer_info_t info;
	fsp_err_t err;

	if (!(channel == RZ_PWM_MTIOCxA || channel == RZ_PWM_MTIOCxB)) {
		LOG_ERR("Valid only for RZ_PWM_MTIOCxA and RZ_PWM_MTIOCxB pins");
		return -EINVAL;
	}

	err = cfg->fsp_api->infoGet(data->fsp_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*cycles = (uint64_t)info.clock_frequency;

	return 0;
};

#ifdef CONFIG_PWM_CAPTURE
void mtu3_capture_compare_a_isr(void *irq);
void mtu3_capture_compare_b_isr(void *irq);
void mtu3_counter_overflow_isr(void *irq);

void pwm_rz_mtu3_tgia_isr(const struct device *dev)
{
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;

	mtu3_capture_compare_a_isr((void *)fsp_cfg_extend->capture_a_irq);
}

void pwm_rz_mtu3_tgib_isr(const struct device *dev)
{
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;

	mtu3_capture_compare_b_isr((void *)fsp_cfg_extend->capture_b_irq);
}

void pwm_rz_mtu3_tciv_isr(const struct device *dev)
{
	struct pwm_rz_mtu_data *data = dev->data;

	mtu3_counter_overflow_isr((void *)data->fsp_cfg->cycle_end_irq);
}

static int pwm_rz_mtu_configure_capture(const struct device *dev, uint32_t channel,
					pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					void *user_data)
{
	const struct pwm_rz_mtu_config *cfg = dev->config;
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;
	pinctrl_soc_pin_t pin = cfg->pincfg->states->pins[channel];
	fsp_err_t err;

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No PWM capture type specified");
		return -EINVAL;
	}
	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}
	if (data->capture.is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}

	err = cfg->fsp_api->close(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	data->fsp_cfg->mode = TIMER_MODE_PERIODIC;
	data->capture.port_pin = pin.port_pin;

	err = pwm_rz_set_counter_clear(dev, channel);
	if (err) {
		return err;
	}

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture.is_pulse_capture = false;
		if (flags & PWM_POLARITY_INVERTED) {
			if (channel == RZ_PWM_MTIOCxA) {
				fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_a =
					MTU3_IO_PIN_LEVEL_INPUT_FALLING_EDGE;
			} else {
				fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_b =
					MTU3_IO_PIN_LEVEL_INPUT_FALLING_EDGE;
			}

		} else {
			if (channel == RZ_PWM_MTIOCxA) {
				fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_a =
					MTU3_IO_PIN_LEVEL_INPUT_RISING_EDGE;
			} else {
				fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_b =
					MTU3_IO_PIN_LEVEL_INPUT_RISING_EDGE;
			}
		}
	} else {
		data->capture.is_pulse_capture = true;
		if (channel == RZ_PWM_MTIOCxA) {
			fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_a =
				MTU3_IO_PIN_LEVEL_INPUT_BOTH_EDGE;
		} else {
			fsp_cfg_extend->mtioc_ctrl_setting.output_pin_level_b =
				MTU3_IO_PIN_LEVEL_INPUT_BOTH_EDGE;
		}

		if (flags & PWM_POLARITY_INVERTED) {
			data->start_source = INPUT_LOW;
			data->capture_source = INPUT_HIGH;
		} else {
			data->start_source = INPUT_HIGH;
			data->capture_source = INPUT_LOW;
		}
	}

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	pwm_rz_disable_all_irqs(dev);

	data->capture.capture_channel = channel;
	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.continuous = (flags & PWM_CAPTURE_MODE_CONTINUOUS) ? true : false;

	return 0;
}

static int pwm_rz_mtu_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_rz_mtu_config *cfg = dev->config;
	struct pwm_rz_mtu_data *data = dev->data;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;
	fsp_err_t err;

	data->capture.capture_channel = channel;

	if (data->capture.is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}

	if (!data->capture.callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	data->capture.is_busy = true;

	/* Start counter */
	err = cfg->fsp_api->start(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Enable interruption */
	irq_enable(data->fsp_cfg->cycle_end_irq);
	if (channel == RZ_PWM_MTIOCxA) {
		irq_enable(fsp_cfg_extend->capture_a_irq);
	} else if (channel == RZ_PWM_MTIOCxB) {
		irq_enable(fsp_cfg_extend->capture_b_irq);
	}

	return 0;
}

static int pwm_rz_mtu_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_rz_mtu_config *cfg = dev->config;
	struct pwm_rz_mtu_data *data = dev->data;
	fsp_err_t err;
	mtu3_extended_cfg_t *fsp_cfg_extend = (mtu3_extended_cfg_t *)data->fsp_cfg->p_extend;

	data->capture.capture_channel = channel;
	data->capture.is_busy = false;

	/* Disable interruption */
	irq_disable(data->fsp_cfg->cycle_end_irq);
	if (channel == RZ_PWM_MTIOCxA) {
		irq_disable(fsp_cfg_extend->capture_a_irq);
	} else if (channel == RZ_PWM_MTIOCxB) {
		irq_disable(fsp_cfg_extend->capture_b_irq);
	}

	err = cfg->fsp_api->stop(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = cfg->fsp_api->reset(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static void fsp_callback(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct pwm_rz_mtu_data *data = dev->data;
	uint32_t period = UINT16_MAX + 1U;
	uint8_t source = R_BSP_PinRead(data->capture.port_pin);

	if (p_args->event == TIMER_EVENT_CYCLE_END) {
		if (data->start_flag != CAPTURE_STOP) {
			data->capture.overflows++;
		}
	} else {
		if (data->capture.is_pulse_capture) {
			if (source == data->start_source) {
				/* Clear the overflow p_args->capture */
				data->capture.overflows = 0U;
				/* Start pulse width measurement */
				data->start_flag = CAPTURE_START;
			} else if (source == data->capture_source &&
				   data->start_flag == CAPTURE_START) {
				data->capture.pulse =
					(data->capture.overflows * period) + p_args->capture;
				/* Measurement Invalid */
				data->start_flag = CAPTURE_STOP;
				if (data->capture.callback != NULL) {
					data->capture.callback(dev, data->capture.capture_channel,
							       0, data->capture.pulse, 0,
							       data->capture.user_data);
				}

				/* Disable capture in single mode */
				if (data->capture.continuous == false) {
					data->capture.overflows = 0U;
					/* Disable capture in single mode */
					if (data->capture.continuous == false) {
						pwm_rz_mtu_disable_capture(
							dev, data->capture.capture_channel);
					}
				}
			} else {
				/* Do nothing */
			}
		} else {
			if (data->start_flag == CAPTURE_STOP) {
				/* Start measurement */
				data->start_flag = CAPTURE_START;
				/* Clear the overflow p_args->capture */
				data->capture.overflows = 0U;
			} else {
				data->capture.period =
					(data->capture.overflows * period) + p_args->capture;
				data->start_flag = CAPTURE_STOP;
				if (data->capture.callback != NULL) {
					data->capture.callback(dev, data->capture.capture_channel,
							       data->capture.period, 0, 0,
							       data->capture.user_data);
				}

				/* Disable capture in single mode */
				if (data->capture.continuous == false) {
					data->capture.overflows = 0U;
					/* Disable capture in single mode */
					if (data->capture.continuous == false) {
						pwm_rz_mtu_disable_capture(
							dev, data->capture.capture_channel);
					}
				}
			}
		}
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static DEVICE_API(pwm, pwm_rz_mtu_driver_api) = {
	.get_cycles_per_sec = pwm_rz_mtu_get_cycles_per_sec,
	.set_cycles = pwm_rz_mtu_set_cycles,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_rz_mtu_configure_capture,
	.enable_capture = pwm_rz_mtu_enable_capture,
	.disable_capture = pwm_rz_mtu_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

static int pwm_rz_mtu_init(const struct device *dev)
{
	const struct pwm_rz_mtu_config *cfg = dev->config;
	struct pwm_rz_mtu_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure pins for PWM (%d)", err);
		return err;
	}

#ifdef CONFIG_PWM_CAPTURE
	data->fsp_cfg->p_callback = fsp_callback;
	data->fsp_cfg->p_context = dev;
#endif /* CONFIG_PWM_CAPTURE */

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	pwm_rz_disable_all_irqs(dev);

	return 0;
}

/* RZ PWM MTU SOURCE DIVIDER */
#define RZ_PWM_MTU_GET_CLK_DIV(DIV, CH)                                                            \
	((CH) == 0 && (DIV) == 256                                                          ? 0x20 \
	 : (CH) == 0 && (DIV) == 1024                                                       ? 0x28 \
	 : (CH) == 1 && (DIV) == 256                                                        ? 0x6  \
	 : (CH) == 1 && (DIV) == 1024                                                       ? 0x20 \
	 : (CH) == 2 && (DIV) == 256                                                        ? 0x20 \
	 : (CH) == 2 && (DIV) == 1024                                                       ? 0x7  \
	 : ((CH) == 3 || (CH) == 4 || (CH) == 6 || (CH) == 7 || (CH) == 8) && (DIV) == 256  ? 0x4  \
	 : ((CH) == 3 || (CH) == 4 || (CH) == 6 || (CH) == 7 || (CH) == 8) && (DIV) == 1024 ? 0x5  \
	 : (DIV) == 1                                                                       ? 0x0  \
	 : (DIV) == 2                                                                       ? 0x8  \
	 : (DIV) == 4                                                                       ? 0x1  \
	 : (DIV) == 8                                                                       ? 0x10 \
	 : (DIV) == 16                                                                      ? 0x2  \
	 : (DIV) == 32                                                                      ? 0x18 \
	 : (DIV) == 64                                                                      ? 0x3  \
											    : 0x0)

#define RZ_MTU(idx) DT_INST_PARENT(idx)

#ifdef CONFIG_PWM_CAPTURE

#ifdef CONFIG_CPU_CORTEX_M
#define MTU_GET_IRQ_FLAGS(idx, irq_name) 0
#else /* Cortex-A/R */
#define MTU_GET_IRQ_FLAGS(idx, irq_name) DT_IRQ_BY_NAME(RZ_MTU(idx), irq_name, flags)
#endif

#define PWM_RZ_IRQ_CONFIG_INIT(inst)                                                               \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZ_MTU(inst), tgia, irq),                               \
			    DT_IRQ_BY_NAME(RZ_MTU(inst), tgia, priority), pwm_rz_mtu3_tgia_isr,    \
			    DEVICE_DT_INST_GET(inst), MTU_GET_IRQ_FLAGS(inst, tgia));              \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZ_MTU(inst), tgib, irq),                               \
			    DT_IRQ_BY_NAME(RZ_MTU(inst), tgib, priority), pwm_rz_mtu3_tgib_isr,    \
			    DEVICE_DT_INST_GET(inst), MTU_GET_IRQ_FLAGS(inst, tgib));              \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZ_MTU(inst), tciv, irq),                               \
			    DT_IRQ_BY_NAME(RZ_MTU(inst), tciv, priority), pwm_rz_mtu3_tciv_isr,    \
			    DEVICE_DT_INST_GET(inst), MTU_GET_IRQ_FLAGS(inst, tciv));              \
	} while (0)
#endif /* CONFIG_PWM_CAPTURE */

#define PWM_RZ_INIT(inst)                                                                          \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static mtu3_instance_ctrl_t g_timer##inst##_ctrl;                                          \
	static mtu3_extended_cfg_t g_timer##inst##_extend = {                                      \
		.mtu3_clk_div = RZ_PWM_MTU_GET_CLK_DIV(DT_PROP(RZ_MTU(inst), prescaler),           \
						       DT_PROP(RZ_MTU(inst), channel)),            \
		.clk_edge = MTU3_CLOCK_EDGE_RISING,                                                \
		.mtu3_clear = MTU3_TCNT_CLEAR_DISABLE,                                             \
		.mtioc_ctrl_setting = {.output_pin_level_a = MTU3_IO_PIN_LEVEL_NO_OUTPUT,          \
				       .output_pin_level_b = MTU3_IO_PIN_LEVEL_NO_OUTPUT},         \
		.capture_a_irq = DT_IRQ_BY_NAME(RZ_MTU(inst), tgia, irq),                          \
		.capture_b_irq = DT_IRQ_BY_NAME(RZ_MTU(inst), tgib, irq),                          \
		.capture_a_ipl = DT_IRQ_BY_NAME(RZ_MTU(inst), tgia, priority),                     \
		.capture_b_ipl = DT_IRQ_BY_NAME(RZ_MTU(inst), tgib, priority),                     \
		.noise_filter_mtioc_setting = MTU3_NOISE_FILTER_DISABLE,                           \
		.noise_filter_mtioc_clk = MTU3_NOISE_FILTER_CLOCK_DIV_1,                           \
		.noise_filter_mtclk_setting = MTU3_NOISE_FILTER_MTCLK_DISABLE,                     \
		.noise_filter_mtclk_clk = MTU3_NOISE_FILTER_EXTERNAL_CLOCK_DIV_1,                  \
		.adc_activation_setting = MTU3_ADC_TGRA_COMPARE_MATCH_DISABLE,                     \
		.p_pwm_cfg = NULL,                                                                 \
	};                                                                                         \
	static timer_cfg_t g_timer##inst##_cfg = {                                                 \
		.mode = TIMER_MODE_PWM,                                                            \
		.channel = DT_PROP(RZ_MTU(inst), channel),                                         \
		.cycle_end_irq = DT_IRQ_BY_NAME(RZ_MTU(inst), tciv, irq),                          \
		.cycle_end_ipl = DT_IRQ_BY_NAME(RZ_MTU(inst), tciv, priority),                     \
		.p_extend = &g_timer##inst##_extend,                                               \
	};                                                                                         \
	static const struct pwm_rz_mtu_config pwm_rz_mtu_config_##inst = {                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.fsp_api = &g_timer_on_mtu3,                                                       \
	};                                                                                         \
	static struct pwm_rz_mtu_data pwm_rz_mtu_data_##inst = {                                   \
		.fsp_cfg = &g_timer##inst##_cfg, .fsp_ctrl = &g_timer##inst##_ctrl};               \
	static int pwm_rz_mtu_init_##inst(const struct device *dev)                                \
	{                                                                                          \
		IF_ENABLED(CONFIG_PWM_CAPTURE,                                 \
			    (PWM_RZ_IRQ_CONFIG_INIT(inst);))                   \
		int err = pwm_rz_mtu_init(dev);                                                    \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, pwm_rz_mtu_init_##inst, NULL, &pwm_rz_mtu_data_##inst,         \
			      &pwm_rz_mtu_config_##inst, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,    \
			      &pwm_rz_mtu_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_RZ_INIT);
