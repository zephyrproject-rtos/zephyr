/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_opamp.h>
#include <stm32_ll_system.h>

#define CONFIG_OPAMP_LOG_LEVEL 4  /// TODO: remove at release

LOG_MODULE_REGISTER(opamp_stm32g4, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32g4_opamp

/* There is a spell-mistake in original driver - L268, stm32g4xx_ll_opamp.h */
#if !defined(LL_OPAMP_INTERNAL_OUTPUT_DISABLED)
#define LL_OPAMP_INTERNAL_OUTPUT_DISABLED LL_OPAMP_INTERNAL_OUPUT_DISABLED
#endif /* !LL_OPAMP_INTERNAL_OUTPUT_DISABLED */

#if !defined(LL_OPAMP_INTERNAL_OUTPUT_ENABLED)
#define LL_OPAMP_INTERNAL_OUTPUT_ENABLED LL_OPAMP_INTERNAL_OUPUT_ENABLED
#endif /* !LL_OPAMP_INTERNAL_OUTPUT_ENABLED */

typedef enum {
	OPAMP_INM_NONE = LL_OPAMP_INPUT_INVERT_CONNECT_NO,
	OPAMP_INM_VINM0 = LL_OPAMP_INPUT_INVERT_IO0,
	OPAMP_INM_VINM1 = LL_OPAMP_INPUT_INVERT_IO1,

	OPAMP_INM_SEC_NONE = -1,
	OPAMP_INM_SEC_VINM0 = LL_OPAMP_INPUT_INVERT_IO0_SEC,
	OPAMP_INM_SEC_VINM1 = LL_OPAMP_INPUT_INVERT_IO1_SEC,
	OPAMP_INM_SEC_PGA = LL_OPAMP_INPUT_INVERT_PGA_SEC,
	OPAMP_INM_SEC_FOLLOWER = LL_OPAMP_INPUT_INVERT_FOLLOWER_SEC,
} opamp_vinm_t;

typedef enum {
	OPAMP_INP_VINP0 = LL_OPAMP_INPUT_NONINVERT_IO0,
	OPAMP_INP_VINP1 = LL_OPAMP_INPUT_NONINVERT_IO1,
	OPAMP_INP_VINP2 = LL_OPAMP_INPUT_NONINVERT_IO2,
	OPAMP_INP_VINP3 = LL_OPAMP_INPUT_NONINVERT_IO3,
	OPAMP_INP_DAC = LL_OPAMP_INPUT_NONINVERT_DAC,

	OPAMP_INP_SEC_NONE  = -1,
	OPAMP_INP_SEC_VINP0 = LL_OPAMP_INPUT_NONINVERT_IO0_SEC,
	OPAMP_INP_SEC_VINP1 = LL_OPAMP_INPUT_NONINVERT_IO1_SEC,
	OPAMP_INP_SEC_VINP2 = LL_OPAMP_INPUT_NONINVERT_IO2_SEC,
	OPAMP_INP_SEC_VINP3 = LL_OPAMP_INPUT_NONINVERT_IO3_SEC,
	OPAMP_INP_SEC_DAC = LL_OPAMP_INPUT_NONINVERT_DAC_SEC,
} opamp_vinp_t;

struct stm32_opamp_config {
	OPAMP_TypeDef *opamp;
	uint8_t functional_mode;
	uint32_t power_mode;
	opamp_vinm_t vinm[2]; /* Primary and secondary entries */
	opamp_vinp_t vinp[2]; /* Primary and secondary entries */
	const struct adc_dt_spec adc_ch; /* ADC channel opamp is connected to */
	bool lock_enable;
	bool self_calibration;
	uint32_t inputs_mux_mode;
	const struct stm32_pclken *pclken;
	const size_t pclk_len;
	const struct pinctrl_dev_config *pincfg;
};

static bool stm32_opamp_is_resumed(const struct device *dev)
{
#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	return state == PM_DEVICE_STATE_ACTIVE;
#else
	return true;
#endif /* CONFIG_PM_DEVICE */
}

static int stm32_opamp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;

	if (LL_OPAMP_IsLocked(opamp)) {
		return -EPERM;
	}

	if (action == PM_DEVICE_ACTION_RESUME) {
		LL_OPAMP_Enable(opamp);
		if(cfg->lock_enable) {
			LL_OPAMP_Lock(opamp);
		}

		if(cfg->lock_enable && cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE) {
			LL_OPAMP_LockTimerMux(opamp);
		}
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		LL_OPAMP_Disable(opamp);
	}

	return 0;
}

static int stm32_opamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t stm32_ll_gain = UINT32_MAX;

	/* In opamp-controler.yaml there are no negative gains present, but
	 * inverting and non-inverting modes instead. Therefore OPAMP_GAIN_1
	 * corresponds to -1 gain in inverting mode, and +2 gain in non-inverting
	 * mode and gain assignment below shall be correct.
	 */
	if (gain == OPAMP_GAIN_2 || gain == OPAMP_GAIN_1) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_2_OR_MINUS_1;
	} else if (gain == OPAMP_GAIN_4 || gain == OPAMP_GAIN_3) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_4_OR_MINUS_3;
	} else if (gain == OPAMP_GAIN_8 || gain == OPAMP_GAIN_7) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_8_OR_MINUS_7;
	} else if (gain == OPAMP_GAIN_16 || gain == OPAMP_GAIN_15) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_16_OR_MINUS_15;
	} else if (gain == OPAMP_GAIN_32 || gain == OPAMP_GAIN_31) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_32_OR_MINUS_31;
	} else if (gain == OPAMP_GAIN_64 || gain == OPAMP_GAIN_63) {
		stm32_ll_gain = LL_OPAMP_PGA_GAIN_64_OR_MINUS_63;
	} else {
		LOG_ERR("%s: invalid gain value %d", dev->name, gain);
		return -EINVAL;
	}

	LL_OPAMP_SetPGAGain(opamp, stm32_ll_gain);

	return 0;
}

static int stm32_opamp_set_functional_mode(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t stm32_ll_functional_mode = UINT32_MAX;

	/* Ensure OPAMP is in functional mode (not calibration mode) */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_FUNCTIONAL);

	switch (cfg->functional_mode) {
	case OPAMP_FUNCTIONAL_MODE_STANDALONE:
		/* Standalone mode: External feedback network defines gain
		 * Both VINP and VINM must be configured externally
		 */
		/* Standalone mode requires explicit input configuration */
		LL_OPAMP_SetInputInverting(opamp, cfg->vinm[0]);
		LL_OPAMP_SetInputNonInverting(opamp, cfg->vinp[0]);

		/* Configure secondary inputs if timer-controlled mux is enabled */
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE) {
			if (cfg->vinm[1] != OPAMP_INM_SEC_NONE) {
				LL_OPAMP_SetInputInvertingSecondary(opamp, cfg->vinm[1]);
			}
			if (cfg->vinp[1] != OPAMP_INP_SEC_NONE) {
				LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->vinp[1]);
			}
		}
		stm32_ll_functional_mode = LL_OPAMP_MODE_STANDALONE;
		break;
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
		/* Follower mode: VINP connected, VINM internally connected to VOUT
		 * Input configuration is handled by SetFunctionalMode
		 */
		/* Follower mode: Only configure non-inverting input */
		LL_OPAMP_SetInputNonInverting(opamp, cfg->vinp[0]);
		LL_OPAMP_SetInputInverting(opamp, OPAMP_INM_NONE);

		/* Configure secondary inputs if timer-controlled mux is enabled */
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE) {
			if (cfg->vinp[1] != OPAMP_INP_SEC_NONE) {
				LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->vinp[1]);
			}
		}
		stm32_ll_functional_mode = LL_OPAMP_MODE_FOLLOWER;
		break;
	case OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL:
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
		/* PGA mode: Internal resistor network for gain
		 * The actual gain is set via opamp_set_gain() API
		 */
		/* Precise PGA mode depending on filtering settings selection */
		// LL_OPAMP_MODE_PGA /* OPAMP functional mode, OPAMP operation in PGA */
		// LL_OPAMP_MODE_PGA_IO0 /* In PGA mode, the inverting input is connected to VINM0 for filtering */
		// LL_OPAMP_MODE_PGA_IO0_BIAS   /* In PGA mode, the inverting input is connected to VINM0
						/* - Input signal on VINM0, bias on VINPx: negative gain
						 * - Bias on VINM0, input signal on VINPx: positive gain
						 */
		// LL_OPAMP_MODE_PGA_IO0_IO1_BIAS /* In PGA mode, the inverting input is connected to VINM0
						  /* - Input signal on VINM0, bias on VINPx: negative gain
						   * - Bias on VINM0, input signal on VINPx: positive gain
						   * And VINM1 is connected too for filtering
						   */

		/* PGA modes: Configure both inputs
		 * The internal resistor ladder is connected based on SetFunctionalMode
		 */
		LL_OPAMP_SetInputNonInverting(opamp, cfg->vinp[0]);
		LL_OPAMP_SetInputInverting(opamp, cfg->vinm[0]);

		/* Configure secondary inputs if timer-controlled mux is enabled */
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE) {
			if (cfg->vinp[1] != OPAMP_INP_SEC_NONE) {
				LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->vinp[1]);
			}
			if (cfg->vinm[1] != OPAMP_INM_SEC_NONE) {
				LL_OPAMP_SetInputInvertingSecondary(opamp, cfg->vinm[1]);
			}
		}

		/* Set default initial gain
		 * NOTE: OPAMP_GAIN_1 in inverting mode and OPAMP_GAIN_2 in
		 *	non-inverting mode have same PGA register value
		 */
		int ret = stm32_opamp_set_gain(dev, OPAMP_GAIN_1);
		if(ret < 0) {
			LOG_ERR("%s: failed to set default gain", dev->name);
			return ret;
		}
		stm32_ll_functional_mode = LL_OPAMP_MODE_PGA;
		break;
	default:
		LOG_ERR("%s: invalid functional mode: %d",
			dev->name, cfg->functional_mode);
		return -EINVAL;
	}

	/* Set the functional mode - this configures internal connections */
	LL_OPAMP_SetFunctionalMode(opamp, stm32_ll_functional_mode);

	return 0;
}

static uint32_t stm32_opamp_get_calout(const struct device *dev,
					struct adc_sequence *adc_sequence)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	const struct adc_dt_spec *adc_ch = &cfg->adc_ch;

	if(LL_OPAMP_GetInternalOutput(opamp) == LL_OPAMP_INTERNAL_OUTPUT_DISABLED) {
		/* Internal output to ADC is disabled - use CALOUT transition check */
		return LL_OPAMP_IsCalibrationOutputSet(opamp);
	}

	if (adc_read_dt(adc_ch, adc_sequence) < 0) {
		LOG_ERR("%s: could not read adc channel #%d",
			adc_ch->dev->name, adc_ch->channel_id);
	}

	/*
	A change of CALOUT from 1 to 0 corresponds to the change of ADC output data
	from values close to the maximum ADC output to values close to the minimum
	ADC output (the ADC works as a comparator connected to the OPAMP output).
	source: RM0440 Rev 8 pp. 784/2138
	*/
	return (uint32_t)(*(uint16_t*)adc_sequence->buffer);
}

/**
 * @brief Self-calibrate the OPAMP
 *
 * @param dev - OPAMP device
 * @return int - 0 on success, negative error code on failure
 */
static int stm32_opamp_self_calibration(const struct device *dev) {
	const struct stm32_opamp_config *cfg = dev->config;
	const struct adc_dt_spec *adc_ch = &cfg->adc_ch;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	if (!cfg->self_calibration) {
		LOG_DBG("%s: self-calibration skipped as per configuration",
			dev->name);
		return ret;
	}

	uint16_t adc_buf = 0;
	struct adc_sequence adc_sequence = {
		.buffer      = &adc_buf,
		.buffer_size = sizeof(adc_buf),
		.calibrate = true,
	};

	if(LL_OPAMP_GetInternalOutput(opamp) != LL_OPAMP_INTERNAL_OUTPUT_DISABLED) {
		ret = adc_channel_setup_dt(adc_ch);
		if (ret < 0) {
			LOG_ERR("%s: could not setup channel #%d",
				adc_ch->dev->name, adc_ch->channel_id);
			return ret;
		}

		ret = adc_sequence_init_dt(adc_ch, &adc_sequence);
		if (ret < 0) {
			LOG_ERR("%s: could not setup adc sequence for channel #%d",
				adc_ch->dev->name, adc_ch->channel_id);
			return ret;
		}
	}

	/*  user trimming values are used for offset calibration */
	LL_OPAMP_SetTrimmingMode(opamp, LL_OPAMP_TRIMMING_USER);

	/* Enable calibration mode */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_CALIBRATION);

	/* Enable opamp */
	LL_OPAMP_Enable(opamp);

	const uint32_t trimming_type[] = {
		LL_OPAMP_TRIMMING_NMOS_VREF_90PC_VDDA, /* 1st calibration - N - 90% Vref */
		LL_OPAMP_TRIMMING_PMOS_VREF_10PC_VDDA /* 2nd calibration - P - 10% Vref */
	};
	for(size_t i = 0; i < ARRAY_SIZE(trimming_type); i++) {
		LOG_DBG("%s: calibrating %s", dev->name, i == 0 ? "NMOS" : "PMOS");
		uint32_t trimming_value = 0;
		uint32_t trimming_min = 0;
		uint32_t calout_min = UINT32_MAX;
		uint32_t calout = 0;
		uint32_t calout_prev = 0;
		bool calib_done = false;
		LL_OPAMP_SetCalibrationSelection(opamp, trimming_type[i]);
		while(trimming_value < 0x1f && !calib_done) {
			calout = stm32_opamp_get_calout(dev, &adc_sequence);
			LL_OPAMP_SetTrimmingValue(opamp, trimming_type[i], trimming_value);
			k_sleep(K_MSEC(2)); /* Wait for offset trimming max time (tOFFTRIMmax >= 2 ms) */
			trimming_value++;

			if(LL_OPAMP_GetInternalOutput(opamp) != LL_OPAMP_INTERNAL_OUTPUT_DISABLED) {
				/* ADC based calibration */
				if(calout < calout_min) {
					calout_min = calout;
					trimming_min = trimming_value;
				}
			} else {
				/* CALOUT based calibration: transition from 1 to 0 */
				if(calout_prev == 0x1 && calout == 0x0) {
					trimming_min = trimming_value;
					calib_done = true;
				}
			}

			LOG_DBG("trimming_min: 0x%x; trimming_value: 0x%x; calout_prev: 0x%x; "
				"calout: 0x%x; calout_min: 0x%x; adc_buf: 0x%x",
				trimming_min, trimming_value, calout_prev, calout, calout_min, adc_buf);

			calout_prev = calout;
		}
		LOG_DBG("Calibration done, trimming value: 0x%x", trimming_min);
		LL_OPAMP_SetTrimmingValue(opamp, trimming_type[i], trimming_min);
	}

	/* Switch back to functional mode and disable */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_FUNCTIONAL);
	LL_OPAMP_Disable(opamp);

	return ret;
}

static int stm32_opamp_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_opamp_config *cfg = dev->config;
	const struct adc_dt_spec *adc_ch = &cfg->adc_ch;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	ret = device_is_ready(clk);
	if (!ret) {
		LOG_ERR("%s clock control device not ready (%d)", dev->name, ret);
		return ret;
	}

	if (adc_ch->dev != NULL) {
		if (!adc_is_ready_dt(adc_ch)) {
			LOG_ERR("%s ADC device not ready", adc_ch->dev->name);
			return -ENODEV;
		}
	}

	/* Enable OPAMP bus clock */
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("%s clock op failed (%d)", dev->name, ret);
		return ret;
	}

	/* Enable OPAMP clock source if provided */
	if (cfg->pclk_len > 1) {
		ret = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
		if (ret != 0) {
			LOG_ERR("%s clock configure failed (%d)", dev->name, ret);
			return ret;
		}
	}

	/* Configure OPAMP inputs as specified in Device Tree, if any */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		/*
		* If the OPAMP is used only with internal channels, then no pinctrl is
		* provided in Device Tree, and pinctrl_apply_state returns -ENOENT,
		* but this should not be treated as an error.
		*/
		LOG_ERR("%s pinctrl setup failed (%d)", dev->name, ret);
		return ret;
	}

	/* Power mode shall be set before calibration - since calibration is mode dependent */
	LL_OPAMP_SetPowerMode(opamp, cfg->power_mode);

	LL_OPAMP_SetInternalOutput(opamp, cfg->adc_ch.dev != NULL ?
				   LL_OPAMP_INTERNAL_OUTPUT_ENABLED :
				   LL_OPAMP_INTERNAL_OUTPUT_DISABLED);

	ret = stm32_opamp_self_calibration(dev);
	if (ret != 0) {
		return ret;
	}

	ret = stm32_opamp_set_functional_mode(dev);
	if (ret != 0) {
		return ret;
	}

	LL_OPAMP_SetInputsMuxMode(opamp, cfg->inputs_mux_mode);

	return pm_device_driver_init(dev, stm32_opamp_pm_callback);
}

#define STM32_OPAMP_DT_POWER_MODE(inst)	\
	CONCAT(LL_OPAMP_POWERMODE_, DT_INST_STRING_TOKEN(inst, st_power_mode))

#define STM32_OPAMP_DT_INPUTS_MUX_MODE(inst)	\
	CONCAT(LL_OPAMP_INPUT_MUX_,		\
		DT_INST_STRING_TOKEN_OR(inst, st_inputs_mux_mode, DISABLE))

#define STM32_OPAMP_DT_LOCK_ENABLE(inst)	\
	DT_INST_PROP_OR(inst, st_lock_enable, false)

#define STM32_OPAMP_DT_SELF_CALIB(inst)	\
	DT_INST_PROP_OR(inst, st_self_calibration, false)

#define STM32_OPAMP_DT_VINM_PRIMARY(inst)	\
	CONCAT(OPAMP_INM_, DT_INST_STRING_TOKEN_BY_IDX(inst, vinm, 0))

#define STM32_OPAMP_DT_VINM_SECONDARY(inst)	\
	CONCAT(OPAMP_INM_SEC_, DT_INST_STRING_TOKEN_BY_IDX_OR(inst, vinm, 1, NONE))

#define STM32_OPAMP_DT_VINP_PRIMARY(inst)	\
	CONCAT(OPAMP_INP_, DT_INST_STRING_TOKEN_BY_IDX(inst, vinp, 0))

#define STM32_OPAMP_DT_VINP_SECONDARY(inst)	\
	CONCAT(OPAMP_INP_SEC_, DT_INST_STRING_TOKEN_BY_IDX_OR(inst, vinp, 1, NONE))

#if DT_INST_NODE_HAS_PROP(0, io_channels)
#define STM32_OPAMP_DT_ADC_CHANNEL(inst) ADC_DT_SPEC_INST_GET(inst)
#else
#define STM32_OPAMP_DT_ADC_CHANNEL(inst) ((const struct adc_dt_spec){NULL})
#endif

static DEVICE_API(opamp, opamp_api) = {
	.set_gain = stm32_opamp_set_gain,
};

#define STM32_OPAMP_DEVICE(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);						\
										\
	static const struct stm32_pclken stm32_pclken_##inst[] =		\
						STM32_DT_INST_CLOCKS(inst);	\
										\
	static const struct stm32_opamp_config stm32_opamp_config_##inst = {	\
		.opamp = (OPAMP_TypeDef *)DT_INST_REG_ADDR(inst),		\
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),	\
		.power_mode = STM32_OPAMP_DT_POWER_MODE(inst),			\
		.vinp = { STM32_OPAMP_DT_VINP_PRIMARY(inst),			\
			  STM32_OPAMP_DT_VINP_SECONDARY(inst) },		\
		.vinm = { STM32_OPAMP_DT_VINM_PRIMARY(inst),			\
			  STM32_OPAMP_DT_VINM_SECONDARY(inst) },		\
		.adc_ch = STM32_OPAMP_DT_ADC_CHANNEL(inst),			\
		.lock_enable = STM32_OPAMP_DT_LOCK_ENABLE(inst),		\
		.self_calibration = STM32_OPAMP_DT_SELF_CALIB(inst),		\
		.inputs_mux_mode = STM32_OPAMP_DT_INPUTS_MUX_MODE(inst),	\
		.pclken = stm32_pclken_##inst,					\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_opamp_pm_callback);		\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			stm32_opamp_init,					\
			PM_DEVICE_DT_INST_GET(inst),				\
			NULL,							\
			&stm32_opamp_config_##inst,				\
			POST_KERNEL,						\
			CONFIG_OPAMP_INIT_PRIORITY,				\
			&opamp_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_OPAMP_DEVICE)
