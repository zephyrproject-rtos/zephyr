/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pwm/renesas_rz_pwm.h>
#include "r_gpt.h"
#include "r_gpt_cfg.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_renesas_rz_gpt, CONFIG_PWM_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_rz_gpt_pwm

#define GPT_PRV_GTIO_HIGH_COMPARE_MATCH_LOW_CYCLE_END 0x6U
#define GPT_PRV_GTIO_LOW_COMPARE_MATCH_HIGH_CYCLE_END 0x9U
#define GPT_PRV_GTIOR_INITIAL_LEVEL_BIT               4

#define CAPTURE_BOTH_MODE_FIRST_EVENT_IS_CAPTURE_PULSE   1
#define CAPTURE_BOTH_MODE_SECOND_EVENT_IS_CAPTURE_PERIOD 2

struct pwm_rz_gpt_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint64_t period;
	uint64_t pulse;
	uint16_t capture_type_flag;
	uint32_t capture_both_event_count;
	bool is_busy;
	uint32_t overflows;
	bool continuous;
	uint32_t capture_channel;
};

struct pwm_rz_gpt_data {
	timer_cfg_t *fsp_cfg;
	gpt_instance_ctrl_t *fsp_ctrl;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_rz_gpt_capture_data capture;
#endif /* CONFIG_PWM_CAPTURE */
};

struct pwm_rz_gpt_config {
	const struct pinctrl_dev_config *pincfg;
	const timer_api_t *fsp_api;
};

static uint32_t pwm_rz_gpt_gtior_calculate(gpt_pin_level_t const stop_level)
{
	/* The stop level is used as both the initial level and the stop level. */
	uint32_t gtior = R_GPT0_GTIOR_OAE_Msk | ((uint32_t)stop_level << R_GPT0_GTIOR_OADFLT_Pos) |
			 ((uint32_t)stop_level << GPT_PRV_GTIOR_INITIAL_LEVEL_BIT);

	uint32_t gtion = GPT_PRV_GTIO_LOW_COMPARE_MATCH_HIGH_CYCLE_END;

	/* Calculate the gtior value for PWM mode only */
	gtior |= gtion;

	return gtior;
}

static int pwm_rz_gpt_apply_gtior_config(gpt_instance_ctrl_t *const p_ctrl,
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
				pwm_rz_gpt_gtior_calculate(p_extend->gtioca.stop_level);
			gtior |= gtioca_gtior << R_GPT0_GTIOR_GTIOA_Pos;
		}

		if (p_extend->gtiocb.output_enabled) {
			uint32_t gtiocb_gtior =
				pwm_rz_gpt_gtior_calculate(p_extend->gtiocb.stop_level);
			gtior |= gtiocb_gtior << R_GPT0_GTIOR_GTIOB_Pos;
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

static int pwm_rz_gpt_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
				 uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_rz_gpt_config *cfg = dev->config;
	struct pwm_rz_gpt_data *data = dev->data;
	gpt_extended_cfg_t *fsp_cfg_extend = (gpt_extended_cfg_t *)data->fsp_cfg->p_extend;
	uint32_t pulse;
	fsp_err_t err;
	uint32_t pin;

	/* gtioca and gtiocb setting */
	if (channel == RZ_PWM_GPT_IO_A) {
		pin = GPT_IO_PIN_GTIOCA;
		fsp_cfg_extend->gtioca.output_enabled = true;
	} else if (channel == RZ_PWM_GPT_IO_B) {
		pin = GPT_IO_PIN_GTIOCB;
		fsp_cfg_extend->gtiocb.output_enabled = true;
	} else {
		LOG_ERR("Valid only for RZ_PWM_GPT_IO_A and RZ_PWM_GPT_IO_B pins");
		return -EINVAL;
	}

	if ((data->fsp_ctrl->variant == TIMER_VARIANT_16_BIT && period_cycles > UINT16_MAX) ||
	    (data->fsp_ctrl->variant == TIMER_VARIANT_32_BIT && period_cycles > UINT32_MAX)) {
		LOG_ERR("Out of range period cycles are not valid");
		return -EINVAL;
	}

	pulse = (flags & PWM_POLARITY_INVERTED) ? period_cycles - pulse_cycles : pulse_cycles;

	/* Apply gtio output setting */
	pwm_rz_gpt_apply_gtior_config(data->fsp_ctrl, data->fsp_cfg);

	/* Stop timer */
	err = cfg->fsp_api->stop(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Update period cycles, reflected at an overflow */
	err = cfg->fsp_api->periodSet(data->fsp_ctrl, period_cycles);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Update pulse cycles, reflected at an overflow */
	err = cfg->fsp_api->dutyCycleSet(data->fsp_ctrl, pulse, pin);
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

static int pwm_rz_gpt_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	const struct pwm_rz_gpt_config *cfg = dev->config;
	struct pwm_rz_gpt_data *data = dev->data;
	timer_info_t info;
	fsp_err_t err;

	if (!(channel == RZ_PWM_GPT_IO_A || channel == RZ_PWM_GPT_IO_B)) {
		LOG_ERR("Valid only for RZ_PWM_GPT_IO_A and RZ_PWM_GPT_IO_B pins");
		return -EINVAL;
	}

	err = cfg->fsp_api->infoGet(data->fsp_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*cycles = (uint64_t)info.clock_frequency;

	return 0;
};

extern void gpt_capture_a_isr(void);
extern void gpt_capture_b_isr(void);
extern void gpt_counter_overflow_isr(void);

#ifdef CONFIG_PWM_CAPTURE

static int pwm_rz_gpt_configure_capture(const struct device *dev, uint32_t channel,
					pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					void *user_data)
{
	struct pwm_rz_gpt_data *data = dev->data;
	gpt_extended_cfg_t *fsp_cfg_extend = (gpt_extended_cfg_t *)data->fsp_cfg->p_extend;

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No PWWM capture type specified");
		return -EINVAL;
	}
	data->capture.capture_type_flag = flags & PWM_CAPTURE_TYPE_MASK;
	data->capture.capture_channel = channel;
	if (data->capture.is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}
	if (channel == RZ_PWM_GPT_IO_A) {
		if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_BOTH) {
			data->capture.capture_both_event_count = 0;
			if (flags & PWM_POLARITY_INVERTED) {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
			} else {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
			}
			fsp_cfg_extend->capture_a_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
					       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
					       GPT_SOURCE_NONE);
		} else if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_PERIOD) {
			if (flags & PWM_POLARITY_INVERTED) {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_a_source = fsp_cfg_extend->start_source;
			} else {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_a_source = fsp_cfg_extend->start_source;
			}
		} else {
			if (flags & PWM_POLARITY_INVERTED) {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_a_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
			} else {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_a_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW |
						       GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |
						       GPT_SOURCE_NONE);
			}
		}
	} else if (channel == RZ_PWM_GPT_IO_B) {
		if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_BOTH) {
			data->capture.capture_both_event_count = 0;
			if (flags & PWM_POLARITY_INVERTED) {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
			} else {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
			}
			fsp_cfg_extend->capture_b_source =
				(gpt_source_t)(GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |
					       GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_HIGH |
					       GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_LOW |
					       GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_HIGH |
					       GPT_SOURCE_NONE);
		} else if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_PERIOD) {
			if (flags & PWM_POLARITY_INVERTED) {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_b_source = fsp_cfg_extend->start_source;
			} else {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_b_source = fsp_cfg_extend->start_source;
			}
		} else {
			if (flags & PWM_POLARITY_INVERTED) {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_b_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
			} else {
				fsp_cfg_extend->start_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
				fsp_cfg_extend->capture_b_source =
					(gpt_source_t)(GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_LOW |
						       GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_HIGH |
						       GPT_SOURCE_NONE);
			}
		}
	}

	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.continuous = flags & PWM_CAPTURE_MODE_CONTINUOUS;
	if (data->capture.continuous) {
		if (channel == RZ_PWM_GPT_IO_A) {
			fsp_cfg_extend->stop_source = fsp_cfg_extend->capture_a_source;
		} else if (channel == RZ_PWM_GPT_IO_B) {
			fsp_cfg_extend->stop_source = fsp_cfg_extend->capture_b_source;
		}
		fsp_cfg_extend->clear_source = fsp_cfg_extend->start_source;
	}

	else {
		fsp_cfg_extend->stop_source = (gpt_source_t)(GPT_SOURCE_NONE);
		fsp_cfg_extend->clear_source = (gpt_source_t)(GPT_SOURCE_NONE);
	}

	return 0;
}

static int pwm_rz_gpt_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_rz_gpt_config *cfg = dev->config;
	struct pwm_rz_gpt_data *data = dev->data;
	gpt_extended_cfg_t *fsp_cfg_extend = (gpt_extended_cfg_t *)data->fsp_cfg->p_extend;
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

	/* Enable capture source */
	err = cfg->fsp_api->enable(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Enable interruption */
	irq_enable(data->fsp_cfg->cycle_end_irq);
	if (channel == RZ_PWM_GPT_IO_A) {
		irq_enable(fsp_cfg_extend->capture_a_irq);
	} else if (channel == RZ_PWM_GPT_IO_B) {
		irq_enable(fsp_cfg_extend->capture_b_irq);
	}

	return 0;
}

static int pwm_rz_gpt_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_rz_gpt_config *cfg = dev->config;
	struct pwm_rz_gpt_data *data = dev->data;
	fsp_err_t err;
	gpt_extended_cfg_t *fsp_cfg_extend = (gpt_extended_cfg_t *)data->fsp_cfg->p_extend;

	data->capture.capture_channel = channel;
	data->capture.is_busy = false;

	/* Disable interruption */
	irq_disable(data->fsp_cfg->cycle_end_irq);
	if (channel == RZ_PWM_GPT_IO_A) {
		irq_disable(fsp_cfg_extend->capture_a_irq);
	} else if (channel == RZ_PWM_GPT_IO_B) {
		irq_disable(fsp_cfg_extend->capture_b_irq);
	}

	/* Disable capture source */
	err = cfg->fsp_api->disable(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Stop timer */
	err = cfg->fsp_api->stop(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Clear timer */
	err = cfg->fsp_api->reset(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static void fsp_callback(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	const struct pwm_rz_gpt_config *cfg = dev->config;
	struct pwm_rz_gpt_data *data = dev->data;
	timer_info_t info;

	(void)cfg->fsp_api->infoGet(data->fsp_ctrl, &info);

	uint64_t period = info.period_counts;

	/* The maximum period is one more than the maximum 16,32-bit number, but will be reflected
	 * as 0
	 */
	if (period == 0U) {
		if (data->fsp_ctrl->variant == TIMER_VARIANT_16_BIT) {
			period = UINT16_MAX + 1U;
		} else {
			period = UINT32_MAX + 1U;
		}
	}

	/* Capture event */
	if (p_args->event == TIMER_EVENT_CAPTURE_A) {
		if (p_args->capture != 0U) {
			bool check_disable_capture = false;

			if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_BOTH) {
				data->capture.capture_both_event_count++;
				if (data->capture.capture_both_event_count ==
				    CAPTURE_BOTH_MODE_FIRST_EVENT_IS_CAPTURE_PULSE) {
					data->capture.pulse = (data->capture.overflows * period) +
							      p_args->capture;
				}
				if (data->capture.capture_both_event_count ==
				    CAPTURE_BOTH_MODE_SECOND_EVENT_IS_CAPTURE_PERIOD) {
					data->capture.capture_both_event_count = 0;
					data->capture.period = (data->capture.overflows * period) +
							       p_args->capture;
					data->capture.callback(
						dev, GPT_IO_PIN_GTIOCA, data->capture.period,
						data->capture.pulse, 0, data->capture.user_data);

					check_disable_capture = true;
				}
			} else if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_PULSE) {
				data->capture.pulse =
					(data->capture.overflows * period) + p_args->capture;
				data->capture.callback(dev, GPT_IO_PIN_GTIOCA, 0,
						       data->capture.pulse, 0,
						       data->capture.user_data);

				check_disable_capture = true;
			} else {
				data->capture.period =
					(data->capture.overflows * period) + p_args->capture;
				data->capture.callback(dev, GPT_IO_PIN_GTIOCA, data->capture.period,
						       0, 0, data->capture.user_data);

				check_disable_capture = true;
			}
			if (check_disable_capture) {
				data->capture.overflows = 0U;
				/* Disable capture in single mode */
				if (data->capture.continuous == false) {
					pwm_rz_gpt_disable_capture(dev, GPT_IO_PIN_GTIOCA);
				}
			}
		}
	} else if (p_args->event == TIMER_EVENT_CAPTURE_B) {
		if (p_args->capture != 0U) {
			bool check_disable_capture = false;

			if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_BOTH) {
				data->capture.capture_both_event_count++;
				if (data->capture.capture_both_event_count ==
				    CAPTURE_BOTH_MODE_FIRST_EVENT_IS_CAPTURE_PULSE) {
					data->capture.pulse = (data->capture.overflows * period) +
							      p_args->capture;
				}
				if (data->capture.capture_both_event_count ==
				    CAPTURE_BOTH_MODE_SECOND_EVENT_IS_CAPTURE_PERIOD) {
					data->capture.capture_both_event_count = 0;
					data->capture.period = (data->capture.overflows * period) +
							       p_args->capture;
					data->capture.callback(
						dev, GPT_IO_PIN_GTIOCB, data->capture.period,
						data->capture.pulse, 0, data->capture.user_data);

					check_disable_capture = true;
				}
			} else if (data->capture.capture_type_flag == PWM_CAPTURE_TYPE_PULSE) {
				data->capture.pulse =
					(data->capture.overflows * period) + p_args->capture;
				data->capture.callback(dev, GPT_IO_PIN_GTIOCB, 0,
						       data->capture.pulse, 0,
						       data->capture.user_data);

				check_disable_capture = true;
			} else {
				data->capture.period =
					(data->capture.overflows * period) + p_args->capture;
				data->capture.callback(dev, GPT_IO_PIN_GTIOCB, data->capture.period,
						       0, 0, data->capture.user_data);

				check_disable_capture = true;
			}
			if (check_disable_capture) {
				data->capture.overflows = 0U;
				/* Disable capture in single mode */
				if (data->capture.continuous == false) {
					pwm_rz_gpt_disable_capture(dev, GPT_IO_PIN_GTIOCB);
				}
			}
		}
	} else if (p_args->event == TIMER_EVENT_CYCLE_END) {
		data->capture.overflows++;
	} else {
		if (data->capture.capture_channel == RZ_PWM_GPT_IO_A) {
			data->capture.callback(dev, GPT_IO_PIN_GTIOCA, 0, 0, -ECANCELED,
					       data->capture.user_data);
		} else if (data->capture.capture_channel == RZ_PWM_GPT_IO_B) {
			data->capture.callback(dev, GPT_IO_PIN_GTIOCB, 0, 0, -ECANCELED,
					       data->capture.user_data);
		}
	}
}

#endif /* CONFIG_PWM_CAPTURE */

static DEVICE_API(pwm, pwm_rz_gpt_driver_api) = {
	.get_cycles_per_sec = pwm_rz_gpt_get_cycles_per_sec,
	.set_cycles = pwm_rz_gpt_set_cycles,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_rz_gpt_configure_capture,
	.enable_capture = pwm_rz_gpt_enable_capture,
	.disable_capture = pwm_rz_gpt_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

static int pwm_rz_gpt_init(const struct device *dev)
{
	const struct pwm_rz_gpt_config *cfg = dev->config;
	struct pwm_rz_gpt_data *data = dev->data;
	gpt_extended_cfg_t *fsp_cfg_extend = (gpt_extended_cfg_t *)data->fsp_cfg->p_extend;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure pins for PWM (%d)", err);
		return err;
	}

#if defined(CONFIG_PWM_CAPTURE)
	data->fsp_cfg->p_callback = fsp_callback;
	data->fsp_cfg->p_context = dev;
#endif /* defined(CONFIG_PWM_CAPTURE) */

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	irq_disable(data->fsp_cfg->cycle_end_irq);
	irq_disable(fsp_cfg_extend->capture_a_irq);
	irq_disable(fsp_cfg_extend->capture_b_irq);

	return 0;
}

#define GPT(idx) DT_INST_PARENT(idx)

#define PWM_RZ_IRQ_CONFIG_INIT(inst)                                                               \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(GPT(inst), ccmpa, irq),                                 \
			    DT_IRQ_BY_NAME(GPT(inst), ccmpa, priority), gpt_capture_a_isr, NULL,   \
			    0);                                                                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(GPT(inst), ccmpb, irq),                                 \
			    DT_IRQ_BY_NAME(GPT(inst), ccmpb, priority), gpt_capture_b_isr, NULL,   \
			    0);                                                                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(GPT(inst), ovf, irq),                                   \
			    DT_IRQ_BY_NAME(GPT(inst), ovf, priority), gpt_counter_overflow_isr,    \
			    NULL, 0);                                                              \
	} while (0)

#define PWM_RZG_INIT(inst)                                                                         \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static gpt_instance_ctrl_t g_timer##inst##_ctrl;                                           \
	static gpt_extended_cfg_t g_timer##inst##_extend = {                                       \
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
		.start_source = (gpt_source_t)(GPT_SOURCE_NONE),                                   \
		.stop_source = (gpt_source_t)(GPT_SOURCE_NONE),                                    \
		.clear_source = (gpt_source_t)(GPT_SOURCE_NONE),                                   \
		.count_up_source = (gpt_source_t)(GPT_SOURCE_NONE),                                \
		.count_down_source = (gpt_source_t)(GPT_SOURCE_NONE),                              \
		.capture_a_source = (gpt_source_t)(GPT_SOURCE_NONE),                               \
		.capture_b_source = (gpt_source_t)(GPT_SOURCE_NONE),                               \
		.capture_a_ipl = DT_IRQ_BY_NAME(GPT(inst), ccmpa, priority),                       \
		.capture_b_ipl = DT_IRQ_BY_NAME(GPT(inst), ccmpb, priority),                       \
		.capture_a_irq = DT_IRQ_BY_NAME(GPT(inst), ccmpa, irq),                            \
		.capture_b_irq = DT_IRQ_BY_NAME(GPT(inst), ccmpb, irq),                            \
		.capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,                                  \
		.capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,                                  \
		.p_pwm_cfg = NULL,                                                                 \
		.gtior_setting.gtior = (0x0U),                                                     \
	};                                                                                         \
	static timer_cfg_t g_timer##inst##_cfg = {                                                 \
		.mode = TIMER_MODE_PWM,                                                            \
		.channel = DT_PROP(GPT(inst), channel),                                            \
		.source_div = DT_ENUM_IDX(GPT(inst), prescaler),                                   \
		.cycle_end_ipl = DT_IRQ_BY_NAME(GPT(inst), ovf, priority),                         \
		.cycle_end_irq = DT_IRQ_BY_NAME(GPT(inst), ovf, irq),                              \
		.p_extend = &g_timer##inst##_extend,                                               \
	};                                                                                         \
	static const struct pwm_rz_gpt_config pwm_rz_gpt_config_##inst = {                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.fsp_api = &g_timer_on_gpt,                                                        \
	};                                                                                         \
	static struct pwm_rz_gpt_data pwm_rz_gpt_data_##inst = {                                   \
		.fsp_cfg = &g_timer##inst##_cfg, .fsp_ctrl = &g_timer##inst##_ctrl};               \
	static int pwm_rz_gpt_init_##inst(const struct device *dev)                                \
	{                                                                                          \
		PWM_RZ_IRQ_CONFIG_INIT(inst);                                                      \
		int err = pwm_rz_gpt_init(dev);                                                    \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, pwm_rz_gpt_init_##inst, NULL, &pwm_rz_gpt_data_##inst,         \
			      &pwm_rz_gpt_config_##inst, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,    \
			      &pwm_rz_gpt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_RZG_INIT);
