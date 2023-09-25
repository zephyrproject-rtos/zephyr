/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_comp

#include "comparator_common.h"
#include <zephyr/dt-bindings/comparator/nrf-comp.h>
#include <zephyr/logging/log.h>
#include <nrfx_comp.h>

LOG_MODULE_REGISTER(comp_nrfx_comp, CONFIG_COMPARATOR_LOG_LEVEL);

enum dev_state {
	NOT_CONFIGURED,
	CONFIGURED,
	STARTED,
};

struct driver_data {
	comparator_callback_t callback;
	void *user_data;
	struct k_spinlock lock;
	uint32_t event_mask;
	enum dev_state state;
};

struct driver_config {
	const struct comparator_cfg *dt_cfg;
};

static void event_handler(nrf_comp_event_t event)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct driver_data *dev_data = dev->data;
	uint32_t cb_evt;

	if (event == NRF_COMP_EVENT_DOWN) {
		cb_evt = COMPARATOR_STATE_BELOW;
	} else if (event == NRF_COMP_EVENT_UP) {
		cb_evt = COMPARATOR_STATE_ABOVE;
	} else {
		return;
	}

	if (dev_data->callback) {
		dev_data->callback(dev, cb_evt, dev_data->user_data);
	}
}

static int comp_input_set(const struct comparator_cfg *cfg,
			  nrf_comp_input_t *input)
{
	switch (cfg->input_positive) {
	case NRF_COMP_POS_AIN0:
		*input = NRF_COMP_INPUT_0;
		break;
	case NRF_COMP_POS_AIN1:
		*input = NRF_COMP_INPUT_1;
		break;
	case NRF_COMP_POS_AIN2:
		*input = NRF_COMP_INPUT_2;
		break;
	case NRF_COMP_POS_AIN3:
		*input = NRF_COMP_INPUT_3;
		break;
#if defined(COMP_PSEL_PSEL_AnalogInput4)
	case NRF_COMP_POS_AIN4:
		*input = NRF_COMP_INPUT_4;
		break;
#endif
#if defined(COMP_PSEL_PSEL_AnalogInput5)
	case NRF_COMP_POS_AIN5:
		*input = NRF_COMP_INPUT_5;
		break;
#endif
#if defined(COMP_PSEL_PSEL_AnalogInput6)
	case NRF_COMP_POS_AIN6:
		*input = NRF_COMP_INPUT_6;
		break;
#endif
#if defined(COMP_PSEL_PSEL_AnalogInput7)
	case NRF_COMP_POS_AIN7:
		*input = NRF_COMP_INPUT_7;
		break;
#endif
#if defined(COMP_PSEL_PSEL_VddDiv2)
	case NRF_COMP_POS_VDD_DIV2:
		*input = NRF_COMP_VDD_DIV2;
		break;
#endif
#if defined(COMP_PSEL_PSEL_VddhDiv5)
	case NRF_COMP_POS_VDDH_DIV2:
		*input = NRF_COMP_VDDH_DIV2;
		break;
#endif
	default:
		LOG_ERR("Invalid positive input specified: %u",
			cfg->input_negative);
		return -EINVAL;
	}

	return 0;
}

static int comp_diff_ref_set(const struct comparator_cfg *cfg,
			     nrf_comp_ext_ref_t *ext_ref)
{
	switch (cfg->input_negative) {
	case NRF_COMP_NEG_DIFF_AIN0:
		*ext_ref = NRF_COMP_EXT_REF_0;
		break;
	case NRF_COMP_NEG_DIFF_AIN1:
		*ext_ref = NRF_COMP_EXT_REF_1;
		break;
	case NRF_COMP_NEG_DIFF_AIN2:
		*ext_ref = NRF_COMP_EXT_REF_2;
		break;
	case NRF_COMP_NEG_DIFF_AIN3:
		*ext_ref = NRF_COMP_EXT_REF_3;
		break;
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference4)
	case NRF_COMP_NEG_DIFF_AIN4:
		*ext_ref = NRF_COMP_EXT_REF_4;
		break;
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference5)
	case NRF_COMP_NEG_DIFF_AIN5:
		*ext_ref = NRF_COMP_EXT_REF_5;
		break;
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference6)
	case NRF_COMP_NEG_DIFF_AIN6:
		*ext_ref = NRF_COMP_EXT_REF_6;
		break;
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference7)
	case NRF_COMP_NEG_DIFF_AIN7:
		*ext_ref = NRF_COMP_EXT_REF_7;
		break;
#endif
	default:
		LOG_ERR("Invalid differential negative input specified: %u",
			cfg->input_negative);
		return -EINVAL;
	}

	return 0;
}

static int comp_se_ref_set(const struct comparator_cfg *cfg,
			   nrf_comp_ref_t *reference,
			   nrf_comp_ext_ref_t *ext_ref)
{
	switch (cfg->input_negative) {
	case NRF_COMP_NEG_SE_INT_1V2:
		*reference = NRF_COMP_REF_INT_1V2;
		break;
#if defined(COMP_REFSEL_REFSEL_Int1V8)
	case NRF_COMP_NEG_SE_INT_1V8:
		*reference = NRF_COMP_REF_INT_1V8;
		break;
#endif
#if defined(COMP_REFSEL_REFSEL_Int2V4)
	case NRF_COMP_NEG_SE_INT_2V4:
		*reference = NRF_COMP_REF_INT_2V4;
		break;
#endif
#if defined(COMP_REFSEL_REFSEL_VDD)
	case NRF_COMP_NEG_SE_VDD:
		*reference = NRF_COMP_REF_VDD;
		break;
#endif
	case NRF_COMP_NEG_SE_AREF_AIN0:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_0;
		break;
	case NRF_COMP_NEG_SE_AREF_AIN1:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_1;
		break;
	case NRF_COMP_NEG_SE_AREF_AIN2:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_2;
		break;
	case NRF_COMP_NEG_SE_AREF_AIN3:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_3;
		break;
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference4)
	case NRF_COMP_NEG_SE_AREF_AIN4:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_4;
		break;
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference5)
	case NRF_COMP_NEG_SE_AREF_AIN5:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_5;
		break;
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference6)
	case NRF_COMP_NEG_SE_AREF_AIN6:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_6;
		break;
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference7)
	case NRF_COMP_NEG_SE_AREF_AIN7:
		*reference = NRF_COMP_REF_AREF;
		*ext_ref = NRF_COMP_EXT_REF_7;
		break;
#endif
	default:
		LOG_ERR("Invalid single-ended negative input specified: %u",
			cfg->input_negative);
		return -EINVAL;
	}

	return 0;
}

static int configure_comp(const struct device *dev,
			  const struct comparator_cfg *cfg)
{
	struct driver_data *dev_data = dev->data;
	uint32_t supported_flags = COMPARATOR_FLAG_SIGNAL_MASK
				 | NRF_COMP_FLAG_MODE_MASK;
	nrfx_comp_config_t cfg_nrfx = { 0 };
	nrfx_err_t err;
	int ret;

	ret = comp_input_set(cfg, &cfg_nrfx.input);
	if (ret < 0) {
		return ret;
	}

	if (cfg->input_negative <= NRF_COMP_NEG_DIFF_AIN7) {
		/* Differential mode. */
		cfg_nrfx.main_mode = NRF_COMP_MAIN_MODE_DIFF;

		ret = comp_diff_ref_set(cfg, &cfg_nrfx.ext_ref);
		if (ret < 0) {
			return ret;
		}

		supported_flags |= NRF_COMP_FLAG_DIFF_HYSTERESIS;

		if (cfg->flags & NRF_COMP_FLAG_DIFF_HYSTERESIS) {
#if defined(COMP_HYST_HYST_Hyst40mV)
			cfg_nrfx.hyst = NRF_COMP_HYST_40MV;
#else
			cfg_nrfx.hyst = NRF_COMP_HYST_50MV;
#endif
		} else {
			cfg_nrfx.hyst = NRF_COMP_HYST_NO_HYST;
		}
	} else {
		/* Single-ended mode. */
		cfg_nrfx.main_mode = NRF_COMP_MAIN_MODE_SE;

		ret = comp_se_ref_set(cfg,
				      &cfg_nrfx.reference,
				      &cfg_nrfx.ext_ref);
		if (ret < 0) {
			return ret;
		}

		supported_flags |= NRF_COMP_FLAG_SE_THDOWN_MASK
				|  NRF_COMP_FLAG_SE_THUP_MASK;

		cfg_nrfx.threshold.th_down =
			(cfg->flags & NRF_COMP_FLAG_SE_THDOWN_MASK)
			>> NRF_COMP_FLAG_SE_THDOWN_POS;
		cfg_nrfx.threshold.th_up =
			(cfg->flags & NRF_COMP_FLAG_SE_THUP_MASK)
			>> NRF_COMP_FLAG_SE_THUP_POS;
	}

	if (cfg->flags & ~supported_flags) {
		LOG_ERR("Unsupported flag specified: 0x%08x", cfg->flags);
		return -EINVAL;
	}

	dev_data->event_mask = 0;
	if (cfg->flags & COMPARATOR_FLAG_SIGNAL_BELOW) {
		dev_data->event_mask |= NRFX_COMP_EVT_EN_DOWN_MASK;
	}
	if (cfg->flags & COMPARATOR_FLAG_SIGNAL_ABOVE) {
		dev_data->event_mask |= NRFX_COMP_EVT_EN_UP_MASK;
	}

	switch (cfg->flags & NRF_COMP_FLAG_MODE_MASK) {
	case NRF_COMP_FLAG_MODE_LOW_POWER:
		cfg_nrfx.speed_mode = NRF_COMP_SP_MODE_LOW;
		break;
#if defined(COMP_MODE_SP_Normal)
	case NRF_COMP_FLAG_MODE_NORMAL:
		cfg_nrfx.speed_mode = NRF_COMP_SP_MODE_NORMAL;
		break;
#endif
	case NRF_COMP_FLAG_MODE_HIGH_SPEED:
		cfg_nrfx.speed_mode = NRF_COMP_SP_MODE_HIGH;
		break;
	default:
		LOG_ERR("Invalid mode specified: 0x%08x", cfg->flags);
		return -EINVAL;
	}

	err = nrfx_comp_init(&cfg_nrfx, event_handler);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_comp_init failed: 0x%08x", err);
		return -EINVAL;
	}

	return 0;
}

static int api_configure(const struct device *dev,
			 const struct comparator_cfg *cfg)
{
	struct driver_data *dev_data = dev->data;
	int ret;

	if (dev_data->state == STARTED) {
		LOG_ERR("Cannot configure started comparator");
		return -EPERM;
	}

	if (dev_data->state == CONFIGURED) {
		nrfx_comp_uninit();
		dev_data->state = NOT_CONFIGURED;
	}

	ret = configure_comp(dev, cfg);
	if (ret == 0) {
		dev_data->state = CONFIGURED;
	}

	return ret;
}

static int api_set_callback(const struct device *dev,
			    comparator_callback_t callback,
			    void *user_data)
{
	struct driver_data *dev_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&dev_data->lock);
	dev_data->callback = callback;
	dev_data->user_data = user_data;
	k_spin_unlock(&dev_data->lock, key);

	return 0;
}

static int api_start(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	if (dev_data->state == NOT_CONFIGURED) {
		LOG_ERR("Not configured");
		return -EPERM;
	}
	if (dev_data->state == STARTED) {
		LOG_ERR("Already started");
		return -EALREADY;
	}

	nrfx_comp_start(dev_data->event_mask, 0);
	dev_data->state = STARTED;

	return 0;
}

static int api_stop(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	if (dev_data->state == STARTED) {
		nrfx_comp_stop();
		dev_data->state = CONFIGURED;
	}

	return 0;
}

static int api_get_state(const struct device *dev, uint32_t *state)
{
	struct driver_data *dev_data = dev->data;

	if (dev_data->state != STARTED) {
		LOG_ERR("Not started");
		return -EPERM;
	}

	if (nrfx_comp_sample()) {
		*state = COMPARATOR_STATE_ABOVE;
	} else {
		*state = COMPARATOR_STATE_BELOW;
	}

	return 0;
}

static int init_comp(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_comp_irq_handler, 0);

	return comparator_common_init(dev, dev_config->dt_cfg);
}

static const struct comparator_driver_api driver_api = {
	.configure    = api_configure,
	.set_callback = api_set_callback,
	.start        = api_start,
	.stop         = api_stop,
	.get_state    = api_get_state,
};

/*
 * There is only one instance on supported SoCs, so inst is guaranteed
 * to be 0 if any instance is okay.
 *
 * Just in case that assumption becomes invalid in the future, we use
 * a BUILD_ASSERT().
 */
#define COMP_INST(inst)							\
	BUILD_ASSERT(inst == 0, "multiple instances not supported");	\
									\
	static struct driver_data comp##inst##_data = {			\
		.state = NOT_CONFIGURED,				\
	};								\
	COMPARATOR_DT_CFG_DEFINE(DT_DRV_INST(inst));			\
	static const struct driver_config comp##inst##_config = {	\
		.dt_cfg = COMPARATOR_DT_CFG_GET(DT_DRV_INST(inst)),	\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      init_comp, NULL,				\
			      &comp##inst##_data,			\
			      &comp##inst##_config,			\
			      POST_KERNEL,				\
			      CONFIG_COMPARATOR_INIT_PRIORITY,		\
			      &driver_api);

DT_INST_FOREACH_STATUS_OKAY(COMP_INST)
