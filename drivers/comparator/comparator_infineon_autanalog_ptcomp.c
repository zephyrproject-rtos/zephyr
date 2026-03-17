/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Comparator driver for Infineon AutAnalog PTComp.
 *
 * Each instance represents one of the two comparators (Comp0, Comp1) within
 * a PTComp block.  The driver implements the Zephyr comparator API providing
 * output reading, trigger configuration, and trigger callbacks via the shared
 * AutAnalog interrupt.
 */

#define DT_DRV_COMPAT infineon_autanalog_ptcomp_comp

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/comparator.h>
#include <infineon_autanalog_ptcomp.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(comparator_ifx_autanalog_ptcomp, CONFIG_COMPARATOR_LOG_LEVEL);

struct comparator_ifx_ptcomp_config {
	const struct device *ptcomp_mfd;
	uint8_t comp_idx;
	uint8_t initial_power_mode;
	uint8_t initial_hysteresis;
	uint8_t initial_int_mode;
	uint8_t initial_positive_mux;
	uint8_t initial_negative_mux;
};

struct comparator_ifx_ptcomp_data {
	comparator_callback_t callback;
	void *user_data;
	enum comparator_trigger trigger;
	bool trigger_pending;
};

static int comparator_ifx_ptcomp_get_output(const struct device *dev)
{
	const struct comparator_ifx_ptcomp_config *cfg = dev->config;

	return ifx_autanalog_ptcomp_read(cfg->ptcomp_mfd, cfg->comp_idx);
}

static int comparator_ifx_ptcomp_set_trigger(const struct device *dev,
					     enum comparator_trigger trigger)
{
	const struct comparator_ifx_ptcomp_config *cfg = dev->config;
	struct comparator_ifx_ptcomp_data *data = dev->data;

	if (trigger > COMPARATOR_TRIGGER_BOTH_EDGES) {
		return -EINVAL;
	}

	data->trigger = trigger;
	data->trigger_pending = false;

	return ifx_autanalog_ptcomp_set_static_cfg(cfg->ptcomp_mfd, cfg->comp_idx,
						   cfg->initial_power_mode, cfg->initial_hysteresis,
						   (uint8_t)trigger);
}

static void comparator_ifx_ptcomp_isr(const struct device *dev)
{
	struct comparator_ifx_ptcomp_data *data = dev->data;

	if (data->trigger != COMPARATOR_TRIGGER_NONE) {
		if (data->callback != NULL) {
			data->callback(dev, data->user_data);
		} else {
			data->trigger_pending = true;
		}
	}
}

static int comparator_ifx_ptcomp_set_trigger_callback(const struct device *dev,
						      comparator_callback_t callback,
						      void *user_data)
{
	struct comparator_ifx_ptcomp_data *data = dev->data;

	data->callback = callback;
	data->user_data = user_data;

	if (callback != NULL && data->trigger_pending) {
		callback(dev, user_data);
	}

	return 0;
}

static int comparator_ifx_ptcomp_trigger_is_pending(const struct device *dev)
{
	struct comparator_ifx_ptcomp_data *data = dev->data;
	int pending = data->trigger_pending ? 1 : 0;

	data->trigger_pending = false;

	return pending;
}

static DEVICE_API(comparator, comparator_ifx_ptcomp_api) = {
	.get_output = comparator_ifx_ptcomp_get_output,
	.set_trigger = comparator_ifx_ptcomp_set_trigger,
	.set_trigger_callback = comparator_ifx_ptcomp_set_trigger_callback,
	.trigger_is_pending = comparator_ifx_ptcomp_trigger_is_pending,
};

static int comparator_ifx_ptcomp_init(const struct device *dev)
{
	const struct comparator_ifx_ptcomp_config *cfg = dev->config;
	struct comparator_ifx_ptcomp_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->ptcomp_mfd)) {
		LOG_ERR("PTComp MFD device not ready");
		return -ENODEV;
	}

	data->trigger = COMPARATOR_TRIGGER_NONE;
	data->trigger_pending = false;
	data->callback = NULL;
	data->user_data = NULL;

	/* Apply initial dynamic config (input mux routing).
	 * In basic mode, comp_idx is used as the dyn_cfg slot index
	 * (comp0 → slot 0, comp1 → slot 1).
	 */
	ret = ifx_autanalog_ptcomp_set_dynamic_cfg(cfg->ptcomp_mfd, cfg->comp_idx,
						   cfg->initial_positive_mux,
						   cfg->initial_negative_mux);
	if (ret != 0) {
		LOG_ERR("Failed to set PTComp dynamic config: %d", ret);
		return ret;
	}

	ret = ifx_autanalog_ptcomp_set_static_cfg(cfg->ptcomp_mfd, cfg->comp_idx,
						  cfg->initial_power_mode, cfg->initial_hysteresis,
						  cfg->initial_int_mode);
	if (ret != 0) {
		LOG_ERR("Failed to set PTComp static config: %d", ret);
		return ret;
	}

	/* Register ISR with the PTComp MFD for per-comparator dispatch */
	ifx_autanalog_ptcomp_set_irq_handler(cfg->ptcomp_mfd, dev, cfg->comp_idx,
					     comparator_ifx_ptcomp_isr);

	LOG_DBG("PTComp Comp%u initialized", cfg->comp_idx);
	return 0;
}

#define COMPARATOR_IFX_PTCOMP_DEFINE(n)                                                            \
	static struct comparator_ifx_ptcomp_data comparator_ifx_ptcomp_data_##n;                   \
                                                                                                   \
	static const struct comparator_ifx_ptcomp_config comparator_ifx_ptcomp_config_##n = {      \
		.ptcomp_mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                    \
		.comp_idx = (uint8_t)DT_INST_REG_ADDR(n),                                          \
		.initial_power_mode = (uint8_t)DT_INST_PROP(n, power_mode),                        \
		.initial_hysteresis = (uint8_t)DT_INST_PROP(n, hysteresis),                        \
		.initial_int_mode = (uint8_t)DT_INST_PROP(n, interrupt_mode),                      \
		.initial_positive_mux = (uint8_t)DT_INST_PROP(n, positive_mux),                    \
		.initial_negative_mux = (uint8_t)DT_INST_PROP(n, negative_mux),                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &comparator_ifx_ptcomp_init, NULL,                                \
			      &comparator_ifx_ptcomp_data_##n, &comparator_ifx_ptcomp_config_##n,  \
			      POST_KERNEL,                                                         \
			      CONFIG_COMPARATOR_INFINEON_AUTANALOG_PTCOMP_INIT_PRIORITY,           \
			      &comparator_ifx_ptcomp_api);

DT_INST_FOREACH_STATUS_OKAY(COMPARATOR_IFX_PTCOMP_DEFINE)
