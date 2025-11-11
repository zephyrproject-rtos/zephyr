/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include "r_gpt.h"
#include "r_gpt_cfg.h"
#include <zephyr/logging/log.h>
#include <stdio.h>

#ifdef CONFIG_RENESAS_RA_ELC
#include <zephyr/drivers/misc/interconn/renesas_elc/renesas_elc.h>
#endif

LOG_MODULE_REGISTER(pwm_renesas_ra, CONFIG_PWM_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_pwm

#define MAX_PIN                                       2U
#define GPT_PRV_GTIO_HIGH_COMPARE_MATCH_LOW_CYCLE_END 0x6U
#define GPT_PRV_GTIO_LOW_COMPARE_MATCH_HIGH_CYCLE_END 0x9U
#define GPT_PRV_GTIOR_INITIAL_LEVEL_BIT               4
#define GPT_PRV_GTIO_TOGGLE_COMPARE_MATCH             0x3U

struct pwm_renesas_ra_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint64_t period;
	uint64_t pulse;
	bool is_pulse_capture;
	bool is_busy;
	uint32_t overflows;
	bool continuous;
};

struct pwm_renesas_ra_data {
	gpt_instance_ctrl_t fsp_ctrl;
	timer_cfg_t fsp_cfg;
	gpt_extended_cfg_t extend_cfg;

#ifdef CONFIG_RENESAS_RA_ELC
	struct renesas_elc_dt_spec start_renesas_elc;
	struct renesas_elc_dt_spec stop_renesas_elc;
#endif /* CONFIG_RENESAS_RA_ELC */

	uint16_t capture_a_event;
	uint16_t overflow_event;

#ifdef CONFIG_PWM_CAPTURE
	struct pwm_renesas_ra_capture_data capture;
#endif /* CONFIG_PWM_CAPTURE */
};

struct pwm_renesas_ra_config {
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_subsys;
	const struct pinctrl_dev_config *pincfg;
};

static uint32_t pwm_renesas_ra_gtior_calculate(gpt_pin_level_t const stop_level)
{
	/* The stop level is used as both the initial level and the stop level. */
	uint32_t gtior = R_GPT0_GTIOR_OAE_Msk | ((uint32_t)stop_level << R_GPT0_GTIOR_OADFLT_Pos) |
			 ((uint32_t)stop_level << GPT_PRV_GTIOR_INITIAL_LEVEL_BIT);

	uint32_t gtion = GPT_PRV_GTIO_LOW_COMPARE_MATCH_HIGH_CYCLE_END;

	/* Calculate the gtior value for PWM mode only */
	gtior |= gtion;

	return gtior;
}

static int pwm_renesas_ra_apply_gtior_config(gpt_instance_ctrl_t *const p_ctrl,
					     timer_cfg_t const *const p_cfg)
{
	gpt_extended_cfg_t *p_extend = (gpt_extended_cfg_t *)p_cfg->p_extend;
	uint32_t gtior = p_extend->gtior_setting.gtior;

#if GPT_CFG_OUTPUT_SUPPORT_ENABLE

	/* Check if custom GTIOR settings are provided. */
	if (p_extend->gtior_setting.gtior == 0) {
		/* If custom GTIOR settings are not provided, calculate GTIOR. */
		if (p_extend->gtioca.output_enabled) {
			uint32_t gtioca_gtior =
				pwm_renesas_ra_gtior_calculate(p_extend->gtioca.stop_level);

			gtior |= gtioca_gtior << R_GPT0_GTIOR_GTIOA_Pos;
		}

		if (p_extend->gtiocb.output_enabled) {
			uint32_t gtiocb_gtior =
				pwm_renesas_ra_gtior_calculate(p_extend->gtiocb.stop_level);

			gtior |= gtiocb_gtior << R_GPT0_GTIOR_GTIOB_Pos;
		}
	}
#endif

#if GPT_PRV_EXTRA_FEATURES_ENABLED == GPT_CFG_OUTPUT_SUPPORT_ENABLE
	gpt_extended_pwm_cfg_t const *p_pwm_cfg = p_extend->p_pwm_cfg;

	if (NULL != p_pwm_cfg) {
		/* Check if custom GTIOR settings are provided. */
		if (p_extend->gtior_setting.gtior == 0) {
			/* If custom GTIOR settings are not provided, set gtioca_disable_settings
			 * and gtiocb_disable_settings.
			 */
			gtior |= (uint32_t)(p_pwm_cfg->gtioca_disable_setting
					    << R_GPT0_GTIOR_OADF_Pos);
			gtior |= (uint32_t)(p_pwm_cfg->gtiocb_disable_setting
					    << R_GPT0_GTIOR_OBDF_Pos);
		}
	}
#endif

	/* Check if custom GTIOR settings are provided. */
	if (p_extend->gtior_setting.gtior == 0) {
		/*
		 * If custom GTIOR settings are not provided, configure the noise filter for
		 * the GTIOC pins.
		 */
		gtior |= (uint32_t)(p_extend->capture_filter_gtioca << R_GPT0_GTIOR_NFAEN_Pos);
		gtior |= (uint32_t)(p_extend->capture_filter_gtiocb << R_GPT0_GTIOR_NFBEN_Pos);
	}

	/* Set the I/O control register. */
	p_ctrl->p_reg->GTIOR = gtior;

	return 0;
}

static int pwm_renesas_ra_set_cycles(const struct device *dev, uint32_t pin, uint32_t period_cycles,
				     uint32_t pulse_cycles, pwm_flags_t flags)
{
	struct pwm_renesas_ra_data *data = dev->data;
	uint32_t pulse;
	fsp_err_t err;

	if (pin >= MAX_PIN) {
		LOG_ERR("Only valid for gtioca and gtiocb pins");
		return -EINVAL;
	}

	if ((data->fsp_ctrl.variant == TIMER_VARIANT_16_BIT && period_cycles > UINT16_MAX) ||
	    (data->fsp_ctrl.variant == TIMER_VARIANT_32_BIT && period_cycles > UINT32_MAX)) {
		LOG_ERR("Out of range period cycles are not valid");
		return -EINVAL;
	}

	/* gtioca and gtiocb setting */
	if (pin == GPT_IO_PIN_GTIOCA) {
		data->extend_cfg.gtioca.output_enabled = true;
	} else {
		data->extend_cfg.gtiocb.output_enabled = true;
	}

	pulse = (flags & PWM_POLARITY_INVERTED) ? period_cycles - pulse_cycles : pulse_cycles;

	/* Apply gtio output setting */
	pwm_renesas_ra_apply_gtior_config(&data->fsp_ctrl, &data->fsp_cfg);

	/* Stop timer */
	err = R_GPT_Stop(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Update period cycles, reflected at an overflow */
	err = R_GPT_PeriodSet(&data->fsp_ctrl, period_cycles);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Update pulse cycles, reflected at an overflow */
	err = R_GPT_DutyCycleSet(&data->fsp_ctrl, pulse, pin);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Start timer */
	err = R_GPT_Start(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

#ifdef CONFIG_RENESAS_RA_ELC
	/* Enables external event triggers */
	if (data->extend_cfg.start_source != GPT_SOURCE_NONE ||
	    data->extend_cfg.stop_source != GPT_SOURCE_NONE) {
		err = R_GPT_Enable(&data->fsp_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}
#endif /* CONFIG_RENESAS_RA_ELC */

	LOG_DBG("channel %u, pin %u, pulse %u, period %u, prescaler: %u.", data->fsp_cfg.channel,
		pin, pulse_cycles, period_cycles, data->fsp_cfg.source_div);

	return 0;
};

static int pwm_renesas_ra_get_cycles_per_sec(const struct device *dev, uint32_t pin,
					     uint64_t *cycles)
{
	struct pwm_renesas_ra_data *data = dev->data;
	timer_info_t info;
	fsp_err_t err;

	if (pin >= MAX_PIN) {
		LOG_ERR("Only valid for gtioca and gtiocb pins");
		return -EINVAL;
	}

	err = R_GPT_InfoGet(&data->fsp_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*cycles = (uint64_t)info.clock_frequency;

	return 0;
};

#ifdef CONFIG_PWM_CAPTURE
extern void gpt_capture_compare_a_isr(void);
extern void gpt_counter_overflow_isr(void);

static void enable_irq(IRQn_Type const irq, uint32_t priority, void *p_context)
{
	if (irq >= 0) {
		R_BSP_IrqCfgEnable(irq, priority, p_context);
	}
}
static void disable_irq(IRQn_Type irq)
{
	/* Disable interrupts. */
	if (irq >= 0) {
		R_BSP_IrqDisable(irq);
		R_FSP_IsrContextSet(irq, NULL);
	}
}

static int pwm_renesas_ra_configure_capture(const struct device *dev, uint32_t pin,
					    pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					    void *user_data)
{
	struct pwm_renesas_ra_data *data = dev->data;

	if (pin != GPT_IO_PIN_GTIOCA) {
		LOG_ERR("Feature only support for gtioca");
		return -EINVAL;
	}
	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No PWWM capture type specified");
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

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture.is_pulse_capture = false;

		if (flags & PWM_POLARITY_INVERTED) {
			data->extend_cfg.start_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);
			data->extend_cfg.capture_a_source = data->extend_cfg.start_source;

		} else {
			data->extend_cfg.start_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);
			data->extend_cfg.capture_a_source = data->extend_cfg.start_source;
		}
	} else {
		data->capture.is_pulse_capture = true;

		if (flags & PWM_POLARITY_INVERTED) {
			data->extend_cfg.start_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);

			data->extend_cfg.capture_a_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);
		} else {
			data->extend_cfg.start_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);

			data->extend_cfg.capture_a_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);
		}
	}

	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.continuous = flags & PWM_CAPTURE_MODE_CONTINUOUS;

	if (data->capture.continuous) {
		data->extend_cfg.stop_source = data->extend_cfg.capture_a_source;
		data->extend_cfg.clear_source = data->extend_cfg.start_source;
	} else {
		data->extend_cfg.stop_source = (gpt_source_t)(GPT_SOURCE_NONE);
		data->extend_cfg.clear_source = (gpt_source_t)(GPT_SOURCE_NONE);
	}

	return 0;
}

static int pwm_renesas_ra_enable_capture(const struct device *dev, uint32_t pin)
{
	struct pwm_renesas_ra_data *data = dev->data;
	fsp_err_t err;

	if (pin != GPT_IO_PIN_GTIOCA) {
		LOG_ERR("Feature only support for gtioca");
		return -EINVAL;
	}

	if (data->capture.is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}

	if (!data->capture.callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	data->capture.is_busy = true;

	/* Enable capture source */
	err = R_GPT_Enable(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Enable interruption */
	enable_irq(data->fsp_cfg.cycle_end_irq, data->fsp_cfg.cycle_end_ipl, &data->fsp_ctrl);
	enable_irq(data->extend_cfg.capture_a_irq, data->extend_cfg.capture_a_ipl, &data->fsp_ctrl);

	R_ICU->IELSR[data->fsp_cfg.cycle_end_irq] = (elc_event_t)data->overflow_event;
	R_ICU->IELSR[data->extend_cfg.capture_a_irq] = (elc_event_t)data->capture_a_event;

	return 0;
}

static int pwm_renesas_ra_disable_capture(const struct device *dev, uint32_t pin)
{
	struct pwm_renesas_ra_data *data = dev->data;
	fsp_err_t err;

	if (pin != GPT_IO_PIN_GTIOCA) {
		LOG_ERR("Feature only support for gtioca");
		return -EINVAL;
	}
	data->capture.is_busy = false;

	/* Disable interruption */
	disable_irq(data->fsp_cfg.cycle_end_irq);
	disable_irq(data->extend_cfg.capture_a_irq);

	R_ICU->IELSR[data->fsp_cfg.cycle_end_irq] = (elc_event_t)ELC_EVENT_NONE;
	R_ICU->IELSR[data->extend_cfg.capture_a_irq] = (elc_event_t)ELC_EVENT_NONE;

	/* Disable capture source */
	err = R_GPT_Disable(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Stop timer */
	err = R_GPT_Stop(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Clear timer */
	err = R_GPT_Reset(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static void fsp_callback(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct pwm_renesas_ra_data *data = dev->data;
	timer_info_t info;

	(void)R_GPT_InfoGet(&data->fsp_ctrl, &info);

	uint64_t period = info.period_counts;

	/* The maximum period is one more than the maximum 16,32-bit number, but will be reflected
	 * as 0
	 */
	if (period == 0U) {
		if (data->fsp_ctrl.variant == TIMER_VARIANT_16_BIT) {
			period = UINT16_MAX + 1U;
		} else {
			period = UINT32_MAX + 1U;
		}
	}

	/* Capture event */
	if (p_args->event == TIMER_EVENT_CAPTURE_A) {
		if (p_args->capture != 0U) {
			if (data->capture.is_pulse_capture == true) {
				data->capture.pulse =
					(data->capture.overflows * period) + p_args->capture;
				data->capture.callback(dev, GPT_IO_PIN_GTIOCA, 0,
						       data->capture.pulse, 0,
						       data->capture.user_data);
			} else {
				data->capture.period =
					(data->capture.overflows * period) + p_args->capture;
				data->capture.callback(dev, GPT_IO_PIN_GTIOCA, data->capture.period,
						       0, 0, data->capture.user_data);
			}
			data->capture.overflows = 0U;
			/* Disable capture in single mode */
			if (data->capture.continuous == false) {
				pwm_renesas_ra_disable_capture(dev, GPT_IO_PIN_GTIOCA);
			}
		}
	} else if (p_args->event == TIMER_EVENT_CYCLE_END) {
		data->capture.overflows++;
	} else {
		data->capture.callback(dev, GPT_IO_PIN_GTIOCA, 0, 0, -ECANCELED,
				       data->capture.user_data);
	}
}

#endif /* CONFIG_PWM_CAPTURE */

static DEVICE_API(pwm, pwm_renesas_ra_driver_api) = {
	.get_cycles_per_sec = pwm_renesas_ra_get_cycles_per_sec,
	.set_cycles = pwm_renesas_ra_set_cycles,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_renesas_ra_configure_capture,
	.enable_capture = pwm_renesas_ra_enable_capture,
	.disable_capture = pwm_renesas_ra_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

static int pwm_renesas_ra_init(const struct device *dev)
{
	struct pwm_renesas_ra_data *data = dev->data;
	const struct pwm_renesas_ra_config *cfg = dev->config;
	int err;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (err < 0) {
		LOG_ERR("Could not initialize clock (%d)", err);
		return err;
	}

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure pins for PWM (%d)", err);
		return err;
	}

#if defined(CONFIG_PWM_CAPTURE)
	data->fsp_cfg.p_callback = fsp_callback;
	data->fsp_cfg.p_context = (void *)dev;
#endif /* defined(CONFIG_PWM_CAPTURE) */

#ifdef CONFIG_RENESAS_RA_ELC
	if (device_is_ready(data->start_renesas_elc.dev) && data->start_renesas_elc.event != 0) {
		err = renesas_elc_link_set(data->start_renesas_elc.dev,
					   data->start_renesas_elc.peripheral,
					   data->start_renesas_elc.event);
		if (err) {
			LOG_ERR("Failed to set Renesas ELC link for PWM start source(%d)", err);
			return err;
		}
	}

	if (device_is_ready(data->stop_renesas_elc.dev) && data->stop_renesas_elc.event != 0) {
		err = renesas_elc_link_set(data->stop_renesas_elc.dev,
					   data->stop_renesas_elc.peripheral,
					   data->stop_renesas_elc.event);
		if (err) {
			LOG_ERR("Failed to set Renesas ELC link for PWM stop source(%d)", err);
			return err;
		}
	}
#endif /* CONFIG_RENESAS_RA_ELC */

	data->fsp_cfg.p_extend = &data->extend_cfg;

	err = R_GPT_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

#define EVENT_GPT_CAPTURE_COMPARE_A(channel)                                                       \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_GPT, channel, _CAPTURE_COMPARE_A))
#define EVENT_GPT_COUNTER_OVERFLOW(channel)                                                        \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_GPT, channel, _COUNTER_OVERFLOW))

#ifdef CONFIG_RENESAS_RA_ELC
#define PWM_RENESAS_ELC_DATA(index)                                                                \
	.start_renesas_elc =                                                                       \
		{                                                                                  \
			.dev = RENESAS_ELC_DT_SPEC_DEVICE_INST_GET_BY_NAME_OR_NULL(index, start),  \
			.peripheral = RENESAS_ELC_DT_SPEC_PERIPHERAL_INST_GET_BY_NAME_OR(          \
				index, start, 0),                                                  \
			.event = RENESAS_ELC_DT_SPEC_EVENT_INST_GET_BY_NAME_OR(index, start, 0),   \
	},                                                                                         \
	.stop_renesas_elc = {                                                                      \
		.dev = RENESAS_ELC_DT_SPEC_DEVICE_INST_GET_BY_NAME_OR_NULL(index, stop),           \
		.peripheral = RENESAS_ELC_DT_SPEC_PERIPHERAL_INST_GET_BY_NAME_OR(index, stop, 0),  \
		.event = RENESAS_ELC_DT_SPEC_EVENT_INST_GET_BY_NAME_OR(index, stop, 0),            \
	},
#else
#define PWM_RENESAS_ELC_DATA(index)
#endif

#ifdef CONFIG_PWM_CAPTURE
#define PWM_RA_IRQ_CONFIG_INIT(index)                                                              \
	do {                                                                                       \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(                                                  \
			EVENT_GPT_CAPTURE_COMPARE_A(DT_INST_PROP(index, channel)));                \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(                                                  \
			EVENT_GPT_COUNTER_OVERFLOW(DT_INST_PROP(index, channel)));                 \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, gtioca, irq),                               \
			    DT_INST_IRQ_BY_NAME(index, gtioca, priority),                          \
			    gpt_capture_compare_a_isr, NULL, 0);                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, overflow, irq),                             \
			    DT_INST_IRQ_BY_NAME(index, overflow, priority),                        \
			    gpt_counter_overflow_isr, NULL, 0);                                    \
	} while (0)

#else
#define PWM_RA_IRQ_CONFIG_INIT(index)
#endif /* CONFIG_PWM_CAPTURE */

#define PWM_INST_SOURCE_OR(index, prop, default_value)                                             \
	DT_INST_STRING_TOKEN_OR(index, prop, default_value)

#define PWM_RA8_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static const gpt_extended_cfg_t g_timer1_extend_##index = {                                \
		.gtioca =                                                                          \
			{                                                                          \
				.output_enabled = false,                                           \
				.stop_level = GPT_PIN_LEVEL_LOW,                                   \
			},                                                                         \
		.gtiocb =                                                                          \
			{                                                                          \
				.output_enabled = false,                                           \
				.stop_level = GPT_PIN_LEVEL_LOW,                                   \
			},                                                                         \
		.start_source =                                                                    \
			(gpt_source_t)(PWM_INST_SOURCE_OR(index, start_source, GPT_SOURCE_NONE)),  \
		.stop_source =                                                                     \
			(gpt_source_t)(PWM_INST_SOURCE_OR(index, stop_source, GPT_SOURCE_NONE)),   \
		.clear_source = (gpt_source_t)(GPT_SOURCE_NONE),                                   \
		.count_up_source = (gpt_source_t)(GPT_SOURCE_NONE),                                \
		.count_down_source = (gpt_source_t)(GPT_SOURCE_NONE),                              \
		.capture_a_source = (gpt_source_t)(GPT_SOURCE_NONE),                               \
		.capture_b_source = (gpt_source_t)(GPT_SOURCE_NONE),                               \
		.capture_a_ipl = DT_INST_IRQ_BY_NAME(index, gtioca, priority),                     \
		.capture_b_ipl = BSP_IRQ_DISABLED,                                                 \
		.capture_a_irq = DT_INST_IRQ_BY_NAME(index, gtioca, irq),                          \
		.capture_b_irq = FSP_INVALID_VECTOR,                                               \
		.capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,                                  \
		.capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,                                  \
		.p_pwm_cfg = NULL,                                                                 \
		.gtior_setting.gtior = (0x0U),                                                     \
		.gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,                                      \
		.gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,                                      \
	};                                                                                         \
	static struct pwm_renesas_ra_data pwm_renesas_ra_data_##index = {                          \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.mode = TIMER_MODE_PWM,                                            \
				.source_div = DT_INST_PROP(index, divider),                        \
				.channel = DT_INST_PROP(index, channel),                           \
				.cycle_end_ipl = DT_INST_IRQ_BY_NAME(index, overflow, priority),   \
				.cycle_end_irq = DT_INST_IRQ_BY_NAME(index, overflow, irq),        \
			},                                                                         \
		.extend_cfg = g_timer1_extend_##index,                                             \
		PWM_RENESAS_ELC_DATA(index).capture_a_event =                                      \
			EVENT_GPT_CAPTURE_COMPARE_A(DT_INST_PROP(index, channel)),                 \
		.overflow_event = EVENT_GPT_COUNTER_OVERFLOW(DT_INST_PROP(index, channel)),        \
	};                                                                                         \
	static const struct pwm_renesas_ra_config pwm_renesas_ra_config_##index = {                \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys = {                                                                  \
			.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(index, 0, mstp),              \
			.stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(index, 0, stop_bit),                \
		}};                                                                                \
	static int pwm_renesas_ra_init_##index(const struct device *dev)                           \
	{                                                                                          \
		PWM_RA_IRQ_CONFIG_INIT(index);                                                     \
		int err = pwm_renesas_ra_init(dev);                                                \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, pwm_renesas_ra_init_##index, NULL,                            \
			      &pwm_renesas_ra_data_##index, &pwm_renesas_ra_config_##index,        \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_RA8_INIT);
