/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_lpcomp

#include "comparator_common.h"
#include <zephyr/dt-bindings/comparator/nrf-lpcomp.h>
#include <zephyr/logging/log.h>
#include <nrfx_lpcomp.h>

LOG_MODULE_REGISTER(comp_nrfx_lpcomp, CONFIG_COMPARATOR_LOG_LEVEL);

enum dev_state {
	NOT_CONFIGURED,
	CONFIGURED,
	STARTED,
};

struct driver_data {
	comparator_callback_t callback;
	void *user_data;
	struct k_spinlock lock;
	enum dev_state state;
};

struct driver_config {
	const struct comparator_cfg *dt_cfg;
};

static void event_handler(nrf_lpcomp_event_t event)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct driver_data *dev_data = dev->data;
	uint32_t cb_evt;

	if (event == NRF_LPCOMP_EVENT_DOWN) {
		cb_evt = COMPARATOR_STATE_BELOW;
	} else if (event == NRF_LPCOMP_EVENT_UP) {
		cb_evt = COMPARATOR_STATE_ABOVE;
	} else {
		return;
	}

	if (dev_data->callback) {
		dev_data->callback(dev, cb_evt, dev_data->user_data);
	}
}

static int lpcomp_input_set(const struct comparator_cfg *cfg,
			    nrf_lpcomp_input_t *input)
{
	switch (cfg->input_positive) {
	case NRF_LPCOMP_POS_AIN0:
		*input = NRF_LPCOMP_INPUT_0;
		break;
	case NRF_LPCOMP_POS_AIN1:
		*input = NRF_LPCOMP_INPUT_1;
		break;
	case NRF_LPCOMP_POS_AIN2:
		*input = NRF_LPCOMP_INPUT_2;
		break;
	case NRF_LPCOMP_POS_AIN3:
		*input = NRF_LPCOMP_INPUT_3;
		break;
	case NRF_LPCOMP_POS_AIN4:
		*input = NRF_LPCOMP_INPUT_4;
		break;
	case NRF_LPCOMP_POS_AIN5:
		*input = NRF_LPCOMP_INPUT_5;
		break;
	case NRF_LPCOMP_POS_AIN6:
		*input = NRF_LPCOMP_INPUT_6;
		break;
	case NRF_LPCOMP_POS_AIN7:
		*input = NRF_LPCOMP_INPUT_7;
		break;
	default:
		LOG_ERR("Invalid positive input specified: %u",
			cfg->input_negative);
		return -EINVAL;
	}

	return 0;
}

static int lpcomp_reference_set(const struct comparator_cfg *cfg,
				nrf_lpcomp_ref_t *reference)
{
	switch (cfg->input_negative) {
	case NRF_LPCOMP_NEG_VDD_1_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_1_8;
		break;
	case NRF_LPCOMP_NEG_VDD_2_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_2_8;
		break;
	case NRF_LPCOMP_NEG_VDD_3_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_3_8;
		break;
	case NRF_LPCOMP_NEG_VDD_4_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_4_8;
		break;
	case NRF_LPCOMP_NEG_VDD_5_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_5_8;
		break;
	case NRF_LPCOMP_NEG_VDD_6_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_6_8;
		break;
	case NRF_LPCOMP_NEG_VDD_7_8:
		*reference = NRF_LPCOMP_REF_SUPPLY_7_8;
		break;
#if (LPCOMP_REFSEL_RESOLUTION == 16)
	case NRF_LPCOMP_NEG_VDD_1_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_1_16;
		break;
	case NRF_LPCOMP_NEG_VDD_3_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_3_16;
		break;
	case NRF_LPCOMP_NEG_VDD_5_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_5_16;
		break;
	case NRF_LPCOMP_NEG_VDD_7_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_7_16;
		break;
	case NRF_LPCOMP_NEG_VDD_9_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_9_16;
		break;
	case NRF_LPCOMP_NEG_VDD_11_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_11_16;
		break;
	case NRF_LPCOMP_NEG_VDD_13_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_13_16;
		break;
	case NRF_LPCOMP_NEG_VDD_15_16:
		*reference = NRF_LPCOMP_REF_SUPPLY_15_16;
		break;
#endif
	case NRF_LPCOMP_NEG_AREF_AIN0:
		*reference = NRF_LPCOMP_REF_EXT_REF0;
		break;
	case NRF_LPCOMP_NEG_AREF_AIN1:
		*reference = NRF_LPCOMP_REF_EXT_REF1;
		break;
	default:
		LOG_ERR("Invalid negative input specified: %u",
			cfg->input_negative);
		return -EINVAL;
	}

	return 0;
}

static int configure_lpcomp(const struct device *dev,
			    const struct comparator_cfg *cfg)
{
	uint32_t supported_flags = COMPARATOR_FLAG_SIGNAL_MASK
#if defined(LPCOMP_FEATURE_HYST_PRESENT)
				 | NRF_LPCOMP_FLAG_ENABLE_HYSTERESIS
#endif
				 | NRF_LPCOMP_FLAG_WAKE_ON_MASK;
	nrfx_lpcomp_config_t cfg_nrfx = { 0 };
	uint32_t int_mask = 0;
	nrfx_err_t err;
	int ret;

	ret = lpcomp_input_set(cfg, &cfg_nrfx.input);
	if (ret < 0) {
		return ret;
	}

	ret = lpcomp_reference_set(cfg, &cfg_nrfx.config.reference);
	if (ret < 0) {
		return ret;
	}

	if (cfg->flags & ~supported_flags) {
		LOG_ERR("Unsupported flag specified: 0x%08x", cfg->flags);
		return -EINVAL;
	}

	if (cfg->flags & COMPARATOR_FLAG_SIGNAL_BELOW) {
		int_mask |= NRF_LPCOMP_INT_DOWN_MASK;
	}
	if (cfg->flags & COMPARATOR_FLAG_SIGNAL_ABOVE) {
		int_mask |= NRF_LPCOMP_INT_UP_MASK;
	}

	if (cfg->flags & NRF_LPCOMP_FLAG_WAKE_ON_BELOW_ONLY) {
		cfg_nrfx.config.detection = NRF_LPCOMP_DETECT_DOWN;
	} else if (cfg->flags & NRF_LPCOMP_FLAG_WAKE_ON_ABOVE_ONLY) {
		cfg_nrfx.config.detection = NRF_LPCOMP_DETECT_UP;
	} else {
		cfg_nrfx.config.detection = NRF_LPCOMP_DETECT_CROSS;
	}

#if defined(LPCOMP_FEATURE_HYST_PRESENT)
	if (cfg->flags & NRF_LPCOMP_FLAG_ENABLE_HYSTERESIS) {
		cfg_nrfx.config.hyst = NRF_LPCOMP_HYST_ENABLED;
	} else {
		cfg_nrfx.config.hyst = NRF_LPCOMP_HYST_NOHYST;
	}
#endif

	err = nrfx_lpcomp_init(&cfg_nrfx, event_handler);
	if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	/* TODO - set this through the driver API when it becomes possible. */
	nrfy_lpcomp_int_init(NRF_LPCOMP, int_mask, 0, true);

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
		nrfx_lpcomp_uninit();
		dev_data->state = NOT_CONFIGURED;
	}

	ret = configure_lpcomp(dev, cfg);
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

	nrfx_lpcomp_enable();
	dev_data->state = STARTED;

	return 0;
}

static int api_stop(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	if (dev_data->state == STARTED) {
		nrfx_lpcomp_disable();
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

	if (nrfy_lpcomp_sample_check(NRF_LPCOMP)) {
		*state = COMPARATOR_STATE_ABOVE;
	} else {
		*state = COMPARATOR_STATE_BELOW;
	}

	return 0;
}

static int init_lpcomp(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_lpcomp_irq_handler, 0);

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
#define LPCOMP_INST(inst)						\
	BUILD_ASSERT(inst == 0, "multiple instances not supported");	\
									\
	static struct driver_data lpcomp##inst##_data = {		\
		.state = NOT_CONFIGURED,				\
	};								\
	COMPARATOR_DT_CFG_DEFINE(DT_DRV_INST(inst));			\
	static const struct driver_config lpcomp##inst##_config = {	\
		.dt_cfg = COMPARATOR_DT_CFG_GET(DT_DRV_INST(inst)),	\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      init_lpcomp, NULL,			\
			      &lpcomp##inst##_data,			\
			      &lpcomp##inst##_config,			\
			      POST_KERNEL,				\
			      CONFIG_COMPARATOR_INIT_PRIORITY,		\
			      &driver_api);

DT_INST_FOREACH_STATUS_OKAY(LPCOMP_INST)
