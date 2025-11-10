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

LOG_MODULE_REGISTER(opamp_stm32, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_opamp

/* There is a spell-mistake in original driver - L268, stm32g4xx_ll_opamp.h
 * use custom definitions for the macros to avoid modifying original driver
 */
#define OPAMP_INTERNAL_OUTPUT_DISABLED (0x00000000UL)
#define OPAMP_INTERNAL_OUTPUT_ENABLED OPAMP_CSR_OPAMPINTEN

#define STM32_OPAMP_TRIM_VAL_MAX 0x1f /* maximum allowed trimming value */
#define STM32_OPAMP_TRIM_VAL_UNDEFINED 0xff /* undefined trimming value */

/*
 * Definitions to map Zephyr API input selections to LL driver values
 * This definition section is needed for simpler DTS bindings match to LL values
 * Without macros below the macros usage for DTS bindings would be clumsy and unclear
 */
#define OPAMP_INM_NONE (UINT32_MAX - 1) /* used to represent states not present in ll */
/* LL_OPAMP_INPUT_INVERT_CONNECT_NO: only for OPAMP in follower or PGA
 * with positive gain without bias
 */
#define OPAMP_INM_NC LL_OPAMP_INPUT_INVERT_CONNECT_NO
#define OPAMP_INM_VINM0 LL_OPAMP_INPUT_INVERT_IO0
#define OPAMP_INM_VINM1 LL_OPAMP_INPUT_INVERT_IO1

#define OPAMP_INM_SEC_NONE (UINT32_MAX - 2) /* used to represent states not present in ll */
/* only applicable in standalone mode */
#define OPAMP_INM_SEC_VINM0 LL_OPAMP_INPUT_INVERT_IO0_SEC
/* only applicable in standalone mode */
#define OPAMP_INM_SEC_VINM1 LL_OPAMP_INPUT_INVERT_IO1_SEC

#define OPAMP_INP_VINP0 LL_OPAMP_INPUT_NONINVERT_IO0
#define OPAMP_INP_VINP1 LL_OPAMP_INPUT_NONINVERT_IO1
#define OPAMP_INP_VINP2 LL_OPAMP_INPUT_NONINVERT_IO2
#define OPAMP_INP_VINP3 LL_OPAMP_INPUT_NONINVERT_IO3
#define OPAMP_INP_DAC LL_OPAMP_INPUT_NONINVERT_DAC

#define OPAMP_INP_SEC_NONE (UINT32_MAX - 2) /* used to represent states not present in ll */
#define OPAMP_INP_SEC_VINP0 LL_OPAMP_INPUT_NONINVERT_IO0_SEC
#define OPAMP_INP_SEC_VINP1 LL_OPAMP_INPUT_NONINVERT_IO1_SEC
#define OPAMP_INP_SEC_VINP2 LL_OPAMP_INPUT_NONINVERT_IO2_SEC
#define OPAMP_INP_SEC_VINP3 LL_OPAMP_INPUT_NONINVERT_IO3_SEC
#define OPAMP_INP_SEC_DAC LL_OPAMP_INPUT_NONINVERT_DAC_SEC

#define OPAMP_INM_FILTERING_NONE (0)
#define OPAMP_INM_FILTERING_VINM0 (1)
#define OPAMP_INM_FILTERING_VINM1 (2)

struct stm32_opamp_config {
	OPAMP_TypeDef *opamp;
	struct stm32_pclken *pclken;
	const struct pinctrl_dev_config *pincfg;
	const struct adc_dt_spec *adc_ch; /* ADC channel opamp is connected to */
	const uint32_t inm[2];        /* Primary and secondary entries */
	const uint32_t inp[2];        /* Primary and secondary entries */
	uint32_t power_mode;
	uint32_t inputs_mux_mode;
	uint32_t inm_filtering;
	const size_t pclk_len;
	uint8_t functional_mode;
	uint8_t pmos_trimming_value;
	uint8_t nmos_trimming_value;
	bool lock_enable;
	bool self_calibration;
};

struct stm32_opamp_data {
	struct k_mutex dev_mtx; /* Sync between device accesses */
};

static void stm32_opamp_config_log_dbg(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	const char *self_calib_str = cfg->self_calibration ? "true" : "false";
	const char *lock_enable_str = cfg->lock_enable ? "true" : "false";
	const struct adc_dt_spec *adc_ch = cfg->adc_ch;
	const int adc_ch_id = adc_ch != NULL ? adc_ch->channel_id : -1;
	const struct device *adc_dev = adc_ch != NULL ? adc_ch->dev : NULL;
	const char *adc_dev_name = adc_dev != NULL ? adc_dev->name : "none";

	LOG_DBG("%s config:\n"
		"  functional_mode: 0x%x\n"
		"  power_mode: 0x%x\n"
		"  inm: {0x%08x,0x%08x}\n"
		"  inp: {0x%08x,0x%08x}\n"
		"  adc_ch: %s channel: %d\n"
		"  lock_enable: %s\n"
		"  self_calibration: %s\n"
		"  inputs_mux_mode: 0x%x\n"
		"  inm_filtering: 0x%x\n"
		"  pmos_trimming_value: 0x%02x\n"
		"  nmos_trimming_value: 0x%02x\n"
		"  OPAMPx_CSR: 0x%08x\n"
		"  OPAMPx_TCMR: 0x%08x\n",
		dev->name, cfg->functional_mode, cfg->power_mode,
		cfg->inm[0], cfg->inm[1], cfg->inp[0], cfg->inp[1],
		adc_dev_name, adc_ch_id, lock_enable_str,
		self_calib_str, cfg->inputs_mux_mode, cfg->inm_filtering,
		(uint32_t)cfg->pmos_trimming_value, (uint32_t)cfg->nmos_trimming_value,
		(uint32_t)cfg->opamp->CSR, (uint32_t)cfg->opamp->TCMR);
}

static bool stm32_opamp_is_locked(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;

	if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE) {
		return LL_OPAMP_IsLocked(opamp) && LL_OPAMP_IsTimerMuxLocked(opamp);
	}

	return LL_OPAMP_IsLocked(opamp);
}

static void stm32_opamp_lock(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;

	LL_OPAMP_Lock(opamp);
	if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE) {
		LL_OPAMP_LockTimerMux(opamp);
	}
}

static int stm32_opamp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;

	if (stm32_opamp_is_locked(dev)) {
		LOG_DBG("%s: locked opamp do not accept action: %d", dev->name, action);
		return -EPERM;
	}

	if (action == PM_DEVICE_ACTION_RESUME) {
		LL_OPAMP_Enable(opamp);
		if (cfg->lock_enable) {
			stm32_opamp_lock(dev);
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
	struct stm32_opamp_data *data = dev->data;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t ll_gain;

	k_mutex_lock(&data->dev_mtx, K_FOREVER);

	if (stm32_opamp_is_locked(dev)) {
		LOG_ERR("%s: locked", dev->name);
		k_mutex_unlock(&data->dev_mtx);
		return -EPERM;
	}

	if (cfg->functional_mode == OPAMP_FUNCTIONAL_MODE_FOLLOWER ||
	    cfg->functional_mode == OPAMP_FUNCTIONAL_MODE_STANDALONE) {
		/* Avoid writing gain value to registers in standalone or follower mode */
		LOG_DBG("%s: not supported in %s", dev->name,
			cfg->functional_mode == OPAMP_FUNCTIONAL_MODE_FOLLOWER ? "follower"
									       : "standalone");
		k_mutex_unlock(&data->dev_mtx);
		return 0;
	}

	/* In opamp-controller.yaml there are no negative gains present, but
	 * inverting and non-inverting modes instead. Therefore OPAMP_GAIN_1
	 * corresponds to -1 gain in inverting mode, and +2 gain in non-inverting mode.
	 */
	switch (gain) {
	case OPAMP_GAIN_1:
	case OPAMP_GAIN_2:
		ll_gain = LL_OPAMP_PGA_GAIN_2_OR_MINUS_1;
		break;
	case OPAMP_GAIN_3:
	case OPAMP_GAIN_4:
		ll_gain = LL_OPAMP_PGA_GAIN_4_OR_MINUS_3;
		break;
	case OPAMP_GAIN_7:
	case OPAMP_GAIN_8:
		ll_gain = LL_OPAMP_PGA_GAIN_8_OR_MINUS_7;
		break;
	case OPAMP_GAIN_15:
	case OPAMP_GAIN_16:
		ll_gain = LL_OPAMP_PGA_GAIN_16_OR_MINUS_15;
		break;
	case OPAMP_GAIN_31:
	case OPAMP_GAIN_32:
		ll_gain = LL_OPAMP_PGA_GAIN_32_OR_MINUS_31;
		break;
	case OPAMP_GAIN_63:
	case OPAMP_GAIN_64:
		ll_gain = LL_OPAMP_PGA_GAIN_64_OR_MINUS_63;
		break;
	default:
		LOG_ERR("%s: invalid gain %d", dev->name, gain);
		k_mutex_unlock(&data->dev_mtx);
		return -EINVAL;
	}

	LL_OPAMP_SetPGAGain(opamp, ll_gain);

	k_mutex_unlock(&data->dev_mtx);
	return 0;
}

static int stm32_opamp_set_functional_mode(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	uint32_t ll_functional_mode = UINT32_MAX;

	/* NOTE: The register values for each mode are defined in AN5306 document */
	switch (cfg->functional_mode) {
	case OPAMP_FUNCTIONAL_MODE_STANDALONE:
		LOG_DBG("%s: functional_mode: standalone", dev->name);
		/* Standalone mode: External feedback network defines gain
		 *	INP user defined (OPAMPx_CSR.VP_SEL - defined by DTS)
		 *	INM tied to VINM0 (OPAMPx_CSR.VM_SEL = b00)
		 *	PGA_GAIN = b00000 (reset value)
		 */
		/* Standalone mode requires explicit input configuration */
		LL_OPAMP_SetInputNonInverting(opamp, cfg->inp[0]);

		/* INM tied to VINM0 */
		LL_OPAMP_SetInputInverting(opamp, LL_OPAMP_INPUT_INVERT_IO0);

		/* Configure secondary inputs if timer-controlled mux is enabled */
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE &&
		    cfg->inm[1] != OPAMP_INM_SEC_NONE) {
			LL_OPAMP_SetInputInvertingSecondary(opamp, cfg->inm[1]);
		}
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE &&
		    cfg->inp[1] != OPAMP_INP_SEC_NONE) {
			LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->inp[1]);
		}
		ll_functional_mode = LL_OPAMP_MODE_STANDALONE;
		break;
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
		LOG_DBG("%s: functional_mode: follower", dev->name);
		/* Follower mode:
		 *	INP connected to input signal (VP_SEL - defined by DTS)
		 *	INM internally connected to VOUT (VM_SEL = b11)
		 *	PGA_GAIN = b00000 (reset value)
		 */

		/* Follower mode: Only configure non-inverting input INP */
		LL_OPAMP_SetInputNonInverting(opamp, cfg->inp[0]);

		/*
		 * INM shall be forced to be not-connected -
		 * the mode itself will define right connection
		 */
		LL_OPAMP_SetInputInverting(opamp, OPAMP_INM_NC);

		/* Configure secondary inputs if timer-controlled mux is enabled */
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE &&
		    cfg->inp[1] != OPAMP_INP_SEC_NONE) {
			LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->inp[1]);
		}
		ll_functional_mode = LL_OPAMP_MODE_FOLLOWER;
		break;
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
		/* INM shall be connected to VINM0 in inverting-mode */
		if (cfg->inm[0] != OPAMP_INM_VINM0) {
			LOG_ERR("%s: expected inm to be set to VINM0", dev->name);
			LOG_DBG("%s: VINM0 (0x%lx) != 0x%x", dev->name, OPAMP_INM_VINM0,
				cfg->inm[0]);
			return -EINVAL;
		}
		__fallthrough;
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
		if (cfg->functional_mode == OPAMP_FUNCTIONAL_MODE_INVERTING) {
			LOG_DBG("%s: functional_mode: inverting", dev->name);
		} else {
			LOG_DBG("%s: functional_mode: non_inverting", dev->name);
		}
		/* PGA mode: Gain is set by resistor array feedback
		 * There are four following sub-modes supported:
		 * - LL_OPAMP_MODE_PGA:
		 *	INP is connected to VINMx thus serving as an input signal pin.
		 *		It will be selected by inp DTS property.
		 *	VINPx secondary may be selected too to be muxed by timer
		 *		(see: st,inputs-mux-mode DTS property)
		 *	INM is connected to resistor array feedback (VM_SEL = b10)
		 *	INM is NOT connected to any VINMx (e.g. external pins)
		 *	The OPAMP is in NON-INVERTING MODE with
		 *	positive gains: +2, +4, +8, +16, +32, +64
		 *
		 * - LL_OPAMP_MODE_PGA_IO0:
		 *	Same as LL_OPAMP_MODE_PGA
		 *	INM is additionally connected to VINM0 for filtering
		 *		(see: st,inm-filtering DTS property)
		 *
		 * - LL_OPAMP_MODE_PGA_IO0_BIAS:
		 *	Same as LL_OPAMP_MODE_PGA
		 *	The INM is connected to VINMx
		 *		- Input signal on VINMx, bias on VINPx:
		 *			- negative gains: -1, -3, -7, -15, -31, -63
		 *		- Bias on VINMx, input signal on VINPx:
		 *			- positive gains: +2, +4, +8, +16, +32, +64
		 *
		 * - LL_OPAMP_MODE_PGA_IO0_IO1_BIAS:
		 *	Same as LL_OPAMP_MODE_PGA
		 *	The INM is connected to VINMx
		 *		- Input signal on VINMx, bias on VINPx:
		 *			- negative gains: -1, -3, -7, -15, -31, -63
		 *		- Bias on VINMx, input signal on VINPx:
		 *			- positive gains: +2, +4, +8, +16, +32, +64
		 *	VINM1 is connected too for filtering
		 */

		/* INP is always configured in ALL PGA modes */
		LL_OPAMP_SetInputNonInverting(opamp, cfg->inp[0]);

		/* Configure secondary inputs if timer-controlled mux is enabled */
		if (cfg->inputs_mux_mode != LL_OPAMP_INPUT_MUX_DISABLE &&
		    cfg->inp[1] != OPAMP_INP_SEC_NONE) {
			LL_OPAMP_SetInputNonInvertingSecondary(opamp, cfg->inp[1]);
		}

		/* Select PGA general mode - precise later */
		ll_functional_mode = LL_OPAMP_MODE_PGA;
		if (cfg->inm_filtering == OPAMP_INM_FILTERING_VINM0) {
			/* INM filtering requested on VINM0 and
			 * INM defined by DTS is don't care
			 */
			ll_functional_mode = LL_OPAMP_MODE_PGA_IO0;
		}

		/* Configure INM input if needed */
		if (cfg->functional_mode == OPAMP_FUNCTIONAL_MODE_INVERTING) {
			/* INM shall be connected to VINM0, which will be configured
			 * by PGA mode bits, therefore there is no need to setup INM here
			 */
			/* Select PGA mode */
			ll_functional_mode = LL_OPAMP_MODE_PGA_IO0_BIAS;
			if (cfg->inm_filtering == OPAMP_INM_FILTERING_VINM1) {
				/* INM filtering requested on VINM1 */
				ll_functional_mode = LL_OPAMP_MODE_PGA_IO0_IO1_BIAS;
			}
		}
		break;
	default:
		LOG_ERR("%s: invalid functional_mode: %d", dev->name, cfg->functional_mode);
		return -EINVAL;
	}

	/* Ensure OPAMP is in functional mode (not calibration mode) */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_FUNCTIONAL);
	LL_OPAMP_SetPGAGain(opamp, 0); /* gain reset value */

	/* Set the functional mode - this configures internal connections */
	LL_OPAMP_SetFunctionalMode(opamp, ll_functional_mode);

	return 0;
}

static int stm32_opamp_get_calout(const struct device *dev, struct adc_sequence *adc_seq,
				  uint32_t *calout)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	const struct adc_dt_spec *adc_ch = cfg->adc_ch;

	if (LL_OPAMP_GetInternalOutput(opamp) == OPAMP_INTERNAL_OUTPUT_DISABLED || adc_ch == NULL) {
		/* Internal output to ADC is disabled - use CALOUT transition check */
		*calout = LL_OPAMP_IsCalibrationOutputSet(opamp);
		return 0;
	}

	if (adc_read_dt(adc_ch, adc_seq) < 0) {
		LOG_ERR("%s: could not read adc channel #%d", adc_ch->dev->name,
			adc_ch->channel_id);
		return -EIO;
	}

	/*
	 * A change of CALOUT from 1 to 0 corresponds to the change of ADC output data
	 * from values close to the maximum ADC output to values close to the minimum
	 * ADC output (the ADC works as a comparator connected to the OPAMP output).
	 * source: RM0440 Rev 9 pp. 785/2140
	 */
	*calout = (uint32_t)sys_read16((mem_addr_t)adc_seq->buffer);
	return 0;
}

static int stm32_opamp_adc_calib_configure(const struct device *dev, struct adc_sequence *adc_seq)
{
	const struct stm32_opamp_config *cfg = dev->config;
	const struct adc_dt_spec *adc_ch = cfg->adc_ch;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	if (LL_OPAMP_GetInternalOutput(opamp) == OPAMP_INTERNAL_OUTPUT_ENABLED && adc_ch != NULL) {
		ret = adc_channel_setup_dt(adc_ch);
		if (ret < 0) {
			LOG_ERR("%s: could not setup channel #%d", adc_ch->dev->name,
				adc_ch->channel_id);
			return ret;
		}

		ret = adc_sequence_init_dt(adc_ch, adc_seq);
		if (ret < 0) {
			LOG_ERR("%s: could not setup adc sequence for channel #%d",
				adc_ch->dev->name, adc_ch->channel_id);
			return ret;
		}
	}
	return ret;
}

/**
 * @brief Self-calibrate the OPAMP
 *
 * @param dev - OPAMP device
 * @return int - 0 on success, negative error code on failure
 */
static int stm32_opamp_self_calibration(const struct device *dev)
{
	const struct stm32_opamp_config *cfg = dev->config;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	uint16_t adc_buf = 0;
	struct adc_sequence adc_seq = {
		.buffer = &adc_buf,
		.buffer_size = sizeof(adc_buf),
		.calibrate = true,
	};

	/* Configure ADC for calibration if internal output is enabled */
	ret = stm32_opamp_adc_calib_configure(dev, &adc_seq);
	if (ret < 0) {
		return ret;
	}

	/* User trimming values are used for offset calibration */
	LL_OPAMP_SetTrimmingMode(opamp, LL_OPAMP_TRIMMING_USER);

	/* Enable calibration mode */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_CALIBRATION);

	/* Enable opamp */
	LL_OPAMP_Enable(opamp);

	const uint32_t trimming_type[] = {
		LL_OPAMP_TRIMMING_NMOS_VREF_90PC_VDDA, /* 1st calibration - N - 90% Vref */
		LL_OPAMP_TRIMMING_PMOS_VREF_10PC_VDDA  /* 2nd calibration - P - 10% Vref */
	};

	for (size_t i = 0; i < ARRAY_SIZE(trimming_type); i++) {
		LOG_DBG("%s: calibrating %s", dev->name, i == 0 ? "NMOS" : "PMOS");
		uint32_t trimming_value = 0;
		uint32_t trimming_min = 0;
		uint32_t calout_min = UINT32_MAX;
		uint32_t calout = 0;
		uint32_t calout_prev = 0;
		bool calib_done = false;

		LL_OPAMP_SetCalibrationSelection(opamp, trimming_type[i]);
		while (trimming_value < STM32_OPAMP_TRIM_VAL_MAX && !calib_done) {
			ret = stm32_opamp_get_calout(dev, &adc_seq, &calout);
			if (ret < 0) {
				break;
			}
			LL_OPAMP_SetTrimmingValue(opamp, trimming_type[i], trimming_value);
			/* Wait for offset trimming max time (tOFFTRIMmax >= 2 ms)
			 * source: RM0440 Rev 9 pp. 785/2140 for stm32g4
			 */
			k_msleep(2);
			trimming_value++;

			if (LL_OPAMP_GetInternalOutput(opamp) !=
			    OPAMP_INTERNAL_OUTPUT_DISABLED) {
				/* ADC based calibration */
				if (calout < calout_min) {
					calout_min = calout;
					trimming_min = trimming_value;
				}
			} else {
				/* CALOUT based calibration: transition from 1 to 0 */
				if (calout_prev == 0x1 && calout == 0x0) {
					trimming_min = trimming_value;
					calib_done = true;
				}
			}

			LOG_DBG("trimming_min: 0x%x; trimming_value: 0x%x; "
				"calout_prev: 0x%x; calout: 0x%x; "
				"calout_min: 0x%x; adc_buf: 0x%x",
				trimming_min, trimming_value, calout_prev, calout, calout_min,
				adc_buf);

			calout_prev = calout;
		}

		if (ret == 0) {
			LL_OPAMP_SetTrimmingValue(opamp, trimming_type[i], trimming_min);
			LOG_DBG("%s: calibration succeed, trimming value: 0x%x", dev->name,
				trimming_min);
		} else {
			LOG_DBG("%s: calibration failed", dev->name);
			break;
		}
	}

	/* Revert register values */
	LL_OPAMP_SetCalibrationSelection(opamp, 0); /* set to register reset value */
	LL_OPAMP_SetMode(opamp, LL_OPAMP_MODE_FUNCTIONAL);
	LL_OPAMP_Disable(opamp);

	return ret;
}

static int stm32_opamp_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_opamp_config *cfg = dev->config;
	struct stm32_opamp_data *data = dev->data;
	const struct adc_dt_spec *adc_ch = cfg->adc_ch;
	OPAMP_TypeDef *opamp = cfg->opamp;
	int ret = 0;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (adc_ch != NULL && adc_ch->dev != NULL && !adc_is_ready_dt(adc_ch)) {
		LOG_ERR("%s ADC device not ready", adc_ch->dev->name);
		return -ENODEV;
	}

	/* Enable OPAMP bus clock */
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("%s clock op failed (%d)", dev->name, ret);
		return ret;
	}

	/* Configure OPAMP inputs as specified in Device Tree */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s pinctrl setup failed (%d)", dev->name, ret);
		return ret;
	}

	k_mutex_lock(&data->dev_mtx, K_FOREVER);

	/* Power mode shall be set before calibration - since calibration is mode dependent */
	LL_OPAMP_SetPowerMode(opamp, cfg->power_mode);

	if (adc_ch != NULL && adc_ch->dev != NULL) {
		/* Enable internal output to ADC if ADC channel is defined */
		LL_OPAMP_SetInternalOutput(opamp, OPAMP_INTERNAL_OUTPUT_ENABLED);
	} else {
		/* Disable internal output to ADC */
		LL_OPAMP_SetInternalOutput(opamp, OPAMP_INTERNAL_OUTPUT_DISABLED);
	}

	if (cfg->self_calibration) {
		ret = stm32_opamp_self_calibration(dev);
		if (ret != 0) {
			k_mutex_unlock(&data->dev_mtx);
			return ret;
		}
	}

	/* Apply trimming values defined explicitly in DTS. OPAMPx_CSR.USERTRIM
	 * shall be set, before setting trimming values in OPAMPx_CSR.TRIMOFFSETN
	 * and OPAMPx_CSR.TRIMOFFSETP (RM0440 Rev 9 pp. 788/2140).
	 */
	if (cfg->pmos_trimming_value != STM32_OPAMP_TRIM_VAL_UNDEFINED) {
		LL_OPAMP_SetTrimmingMode(opamp, LL_OPAMP_TRIMMING_USER);
		LL_OPAMP_SetTrimmingValue(opamp, LL_OPAMP_TRIMMING_PMOS,
						cfg->pmos_trimming_value);
	}

	if (cfg->nmos_trimming_value != STM32_OPAMP_TRIM_VAL_UNDEFINED) {
		LL_OPAMP_SetTrimmingMode(opamp, LL_OPAMP_TRIMMING_USER);
		LL_OPAMP_SetTrimmingValue(opamp, LL_OPAMP_TRIMMING_NMOS,
						cfg->nmos_trimming_value);
	}

	ret = stm32_opamp_set_functional_mode(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->dev_mtx);
		return ret;
	}

	LL_OPAMP_SetInputsMuxMode(opamp, cfg->inputs_mux_mode);

	/* It is always very useful to show register configuration in debug mode */
	stm32_opamp_config_log_dbg(dev);

	k_mutex_unlock(&data->dev_mtx);
	return pm_device_driver_init(dev, stm32_opamp_pm_callback);
}

#define STM32_OPAMP_DT_POWER_MODE(inst)                                                            \
	CONCAT(LL_OPAMP_POWERMODE_, DT_INST_STRING_TOKEN(inst, st_power_mode))

#define STM32_OPAMP_DT_INPUTS_MUX_MODE(inst)                                                       \
	CONCAT(LL_OPAMP_INPUT_MUX_, DT_INST_STRING_TOKEN(inst, st_inputs_mux_mode))

#define STM32_OPAMP_DT_INM_FILTERING(inst)                                                         \
	CONCAT(OPAMP_INM_FILTERING_, DT_INST_STRING_TOKEN(inst, st_inm_filtering))

#define STM32_OPAMP_DT_INM_PRIMARY(inst)                                                           \
	CONCAT(OPAMP_INM_, DT_INST_STRING_TOKEN_BY_IDX_OR(inst, inm, 0, NONE))

#define STM32_OPAMP_DT_INM_SECONDARY(inst)                                                         \
	CONCAT(OPAMP_INM_SEC_, DT_INST_STRING_TOKEN_BY_IDX_OR(inst, inm, 1, NONE))

#define STM32_OPAMP_DT_INP_PRIMARY(inst)                                                           \
	CONCAT(OPAMP_INP_, DT_INST_STRING_TOKEN_BY_IDX(inst, inp, 0))

#define STM32_OPAMP_DT_INP_SECONDARY(inst)                                                         \
	CONCAT(OPAMP_INP_SEC_, DT_INST_STRING_TOKEN_BY_IDX_OR(inst, inp, 1, NONE))

#define STM32_OPAMP_TRIMMING_ASSERT_IMPL(inst, value)                                              \
	BUILD_ASSERT(DT_INST_PROP(inst, value) <= STM32_OPAMP_TRIM_VAL_MAX,                        \
		     "The value exceeds maximum allowed trimming value STM32_OPAMP_TRIM_VAL_MAX")

/* This assert guarantees a value is always in allowed range at compile time */
#define STM32_OPAMP_TRIMMING_ASSERT(inst, value)                                                   \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, value),                                            \
		    (STM32_OPAMP_TRIMMING_ASSERT_IMPL(inst, value)),                               \
		    (/* empty */))

#define STM32_OPAMP_DT_PMOS_TRIMMING(inst)                                                         \
	DT_INST_PROP_OR(inst, st_pmos_trimming_value, STM32_OPAMP_TRIM_VAL_UNDEFINED)

#define STM32_OPAMP_DT_NMOS_TRIMMING(inst)                                                         \
	DT_INST_PROP_OR(inst, st_nmos_trimming_value, STM32_OPAMP_TRIM_VAL_UNDEFINED)

#define _STM32_OPAMP_DT_ADC_CHANNEL_DEFINE_1(inst)                                                 \
	static const struct adc_dt_spec adc_ch_##inst = ADC_DT_SPEC_INST_GET(inst);

#define _STM32_OPAMP_DT_ADC_CHANNEL_DEFINE_0(inst) /* empty */

#define STM32_OPAMP_DT_ADC_CHANNEL_DEFINE(inst)                                                    \
	UTIL_CAT(_STM32_OPAMP_DT_ADC_CHANNEL_DEFINE_,                                              \
		 UTIL_BOOL(DT_INST_NODE_HAS_PROP(inst, io_channels)))(inst)

#define STM32_OPAMP_ADC_CHANNEL_PTR(inst)                                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, io_channels), (&adc_ch_##inst), (NULL))

static DEVICE_API(opamp, opamp_api) = {
	.set_gain = stm32_opamp_set_gain,
};

#define STM32_OPAMP_DEVICE(inst)                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct stm32_pclken stm32_pclken_##inst[] = STM32_DT_INST_CLOCKS(inst);             \
                                                                                                   \
	STM32_OPAMP_TRIMMING_ASSERT(inst, st_pmos_trimming_value);                                 \
	STM32_OPAMP_TRIMMING_ASSERT(inst, st_nmos_trimming_value);                                 \
                                                                                                   \
	STM32_OPAMP_DT_ADC_CHANNEL_DEFINE(inst)                                                    \
                                                                                                   \
	static const struct stm32_opamp_config stm32_opamp_config_##inst = {                       \
		.opamp = (OPAMP_TypeDef *)DT_INST_REG_ADDR(inst),                                  \
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),                        \
		.power_mode = STM32_OPAMP_DT_POWER_MODE(inst),                                     \
		.inp = {STM32_OPAMP_DT_INP_PRIMARY(inst), STM32_OPAMP_DT_INP_SECONDARY(inst)},     \
		.inm = {STM32_OPAMP_DT_INM_PRIMARY(inst), STM32_OPAMP_DT_INM_SECONDARY(inst)},     \
		.adc_ch = STM32_OPAMP_ADC_CHANNEL_PTR(inst),                                       \
		.lock_enable = DT_INST_PROP(inst, st_lock_enable),                                 \
		.self_calibration = DT_INST_PROP(inst, st_enable_self_calibration),                \
		.inputs_mux_mode = STM32_OPAMP_DT_INPUTS_MUX_MODE(inst),                           \
		.inm_filtering = STM32_OPAMP_DT_INM_FILTERING(inst),                               \
		.pmos_trimming_value = STM32_OPAMP_DT_PMOS_TRIMMING(inst),                         \
		.nmos_trimming_value = STM32_OPAMP_DT_NMOS_TRIMMING(inst),                         \
		.pclken = stm32_pclken_##inst,                                                     \
		.pclk_len = DT_INST_NUM_CLOCKS(inst),                                              \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
                                                                                                   \
	static struct stm32_opamp_data stm32_opamp_data_##inst = {                                 \
		.dev_mtx = Z_MUTEX_INITIALIZER(stm32_opamp_data_##inst.dev_mtx),                   \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_opamp_pm_callback);                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, stm32_opamp_init, PM_DEVICE_DT_INST_GET(inst),                 \
			      &stm32_opamp_data_##inst, &stm32_opamp_config_##inst, POST_KERNEL,   \
			      CONFIG_OPAMP_INIT_PRIORITY, &opamp_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_OPAMP_DEVICE)
