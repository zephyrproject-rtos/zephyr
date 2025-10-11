/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_opamp.h>
#include <stm32_ll_system.h>


LOG_MODULE_REGISTER(stm32g4_opamp, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32g4_opamp

/* There is a spell-mistake in original driver */
#if !defined(LL_OPAMP_INTERNAL_OUTPUT_DISABLED)
#define LL_OPAMP_INTERNAL_OUTPUT_DISABLED LL_OPAMP_INTERNAL_OUPUT_DISABLED
#endif /* !LL_OPAMP_INTERNAL_OUTPUT_DISABLED */

#if !defined(LL_OPAMP_INTERNAL_OUTPUT_ENABLED)
#define LL_OPAMP_INTERNAL_OUTPUT_ENABLED LL_OPAMP_INTERNAL_OUPUT_ENABLED
#endif /* !LL_OPAMP_INTERNAL_OUTPUT_ENABLED */

typedef enum {
	OPAMP_INM_SECONDARY_DISABLED = -1,
	OPAMP_INM_NONE = LL_OPAMP_INPUT_INVERT_CONNECT_NO,
	OPAMP_INM_VINM0 = LL_OPAMP_INPUT_INVERT_IO0,
	OPAMP_INM_VINM1 = LL_OPAMP_INPUT_INVERT_IO1,
} opamp_vinm_t;

typedef enum {
	OPAMP_INP_SECONDARY_DISABLED = -1,
	OPAMP_INP_VINP0 = LL_OPAMP_INPUT_NONINVERT_IO0,
	OPAMP_INP_VINP1 = LL_OPAMP_INPUT_NONINVERT_IO1,
	OPAMP_INP_VINP2 = LL_OPAMP_INPUT_NONINVERT_IO2,
	OPAMP_INP_VINP3 = LL_OPAMP_INPUT_NONINVERT_IO3,
	OPAMP_INP_DAC = LL_OPAMP_INPUT_NONINVERT_DAC,
} opamp_vinp_t;

typedef enum {
	OPAMP_OUT_NONE = 0,
	OPAMP_OUT_VOUT0,
	OPAMP_OUT_VOUT1
} opamp_vout_t;

struct stm32_opamp_config {
	OPAMP_TypeDef *opamp;
	uint8_t functional_mode;
	uint32_t power_mode;
	opamp_vinm_t vinm[2]; /* Primary and secondary entries */
	opamp_vinp_t vinp[2]; /* Primary and secondary entries */
	opamp_vout_t vout;
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

static int stm32_opamp_set_functional_mode(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t stm32_ll_functional_mode = UINT32_MAX;

	/// TODO: Review functional mode selector to fulfill RefMan requirements
	switch (cfg->functional_mode) {
	case OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL:
		stm32_ll_functional_mode = LL_OPAMP_MODE_PGA;
		break;
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
		stm32_ll_functional_mode = LL_OPAMP_MODE_PGA;
		break;
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
		stm32_ll_functional_mode = LL_OPAMP_MODE_PGA;
		break;
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
		stm32_ll_functional_mode = LL_OPAMP_MODE_FOLLOWER;
		break;
	case OPAMP_FUNCTIONAL_MODE_STANDALONE:
		/* In standalone mode the gain will be defined by external circuitry.
		 * The set gain function call has no effect in this mode.
		 */
		stm32_ll_functional_mode = LL_OPAMP_MODE_STANDALONE;
		break;
	default:
		LOG_ERR("%s: invalid functional mode: %d",
			dev->name, cfg->functional_mode);
		return -EINVAL;
	}

	LL_OPAMP_SetFunctionalMode(opamp, stm32_ll_functional_mode);

	/* Ensure OPAMP is in functional mode now */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_FUNCTIONAL);

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

static int stm32_opamp_self_calibration(const struct device *dev) {
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;

	if (!cfg->self_calibration) {
		return 0;
	}

	/* Disable internal output - it makes calibration procedure easier */
	const uint32_t int_out_state = LL_OPAMP_GetInternalOutput(opamp);
	LL_OPAMP_SetInternalOutput(opamp, LL_OPAMP_INTERNAL_OUTPUT_DISABLED);

	/* Enable calibration mode */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_CALIBRATION);

	/// TODO: Add implementation for self-calibration

	/* Switch back to functional mode */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_FUNCTIONAL);

	/* Restore internal output state */
	LL_OPAMP_SetInternalOutput(opamp, int_out_state);
	return 0;
}

static int stm32_opamp_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	ret = device_is_ready(clk);
	if (!ret) {
		LOG_ERR("%s clock control device not ready (%d)", dev->name, ret);
		return ret;
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

	ret = stm32_opamp_self_calibration(dev);
	if (ret != 0) {
		return ret;
	}

	ret = stm32_opamp_set_functional_mode(dev);
	if (ret != 0) {
		return ret;
	}

	LL_OPAMP_SetInputsMuxMode(opamp, cfg->inputs_mux_mode);
	LL_OPAMP_SetPowerMode(opamp, cfg->power_mode);

	/// TODO: Configure vinm / vinp in accordance with dts
	LL_OPAMP_SetInputInverting(opamp, cfg->vinm[0]);
	if (cfg->vinm[1] != OPAMP_INM_SECONDARY_DISABLED) {
		LL_OPAMP_SetInputInvertingSecondary(opamp, cfg->vinm[1]);
	}
	LOG_INF("cfg->vinm[1] == OPAMP_INM_SECONDARY_DISABLED: %d", cfg->vinm[1] == OPAMP_INM_SECONDARY_DISABLED);
	LOG_INF("cfg->vinm[1] == OPAMP_INM_VINM1: %d", cfg->vinm[1] == OPAMP_INM_VINM1);

	LL_OPAMP_SetInputNonInverting(opamp, cfg->vinp[0]);
	if (cfg->vinp[1] != OPAMP_INP_SECONDARY_DISABLED) {
		LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->vinp[1]);
	}

	/// TODO: Internal output shall be enabled only if OPAMP is connected to ADC
	// LL_OPAMP_SetInternalOutput(opamp, LL_OPAMP_INTERNAL_OUTPUT_DISABLED);

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
  CONCAT(OPAMP_INM_, DT_INST_STRING_TOKEN_BY_IDX_OR(inst, vinm, 1, SECONDARY_DISABLED))

#define STM32_OPAMP_DT_VINP_PRIMARY(inst)	\
  CONCAT(OPAMP_INP_, DT_INST_STRING_TOKEN_BY_IDX(inst, vinp, 0))

#define STM32_OPAMP_DT_VINP_SECONDARY(inst)	\
  CONCAT(OPAMP_INP_, DT_INST_STRING_TOKEN_BY_IDX(inst, vinp, 1))

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
		.vinp[0] = STM32_OPAMP_DT_VINP_PRIMARY(inst), 			\
		.vinp[1] = STM32_OPAMP_DT_VINP_SECONDARY(inst),			\
		.vinm[0] = STM32_OPAMP_DT_VINM_PRIMARY(inst), 			\
		.vinm[1] = STM32_OPAMP_DT_VINM_SECONDARY(inst),			\
		.vout = 0,							\
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
