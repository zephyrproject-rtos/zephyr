/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_lvd

#include <soc.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <r_lvd.h>
#include <rp_lvd.h>

LOG_MODULE_REGISTER(renesas_ra_lvd, CONFIG_COMPARATOR_LOG_LEVEL);

#define LVD_RENESAS_RA_EVT_PENDING BIT(0)

enum lvd_action {
	LVD_ACTION_NMI,
	LVD_ACTION_MI,
	LVD_ACTION_RESET,
	LVD_ACTION_NONE,
};

extern void lvd_lvd_isr(void);

struct lvd_renesas_ra_data {
	lvd_instance_ctrl_t lvd_ctrl;
	lvd_cfg_t lvd_config;
	comparator_callback_t user_cb;
	void *user_cb_data;
	atomic_t flags;
};

struct lvd_renesas_ra_config {
	bool reset_only;
	enum lvd_action action;
	void (*irq_config_func)(void);
};

static int lvd_renesas_ra_get_output(const struct device *dev)
{
	struct lvd_renesas_ra_data *data = dev->data;
	const struct lvd_renesas_ra_config *config = dev->config;
	lvd_status_t status;
	fsp_err_t err;

	if (config->reset_only) {
		LOG_ERR("Get output is not supported on this LVD channel");
		return -ENOTSUP;
	}

	err = R_LVD_StatusGet(&data->lvd_ctrl, &status);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to get LVD status");
		return -EIO;
	}

	return status.current_state;
}

static int lvd_renesas_ra_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct lvd_renesas_ra_config *config = dev->config;
	struct lvd_renesas_ra_data *data = dev->data;
	bool trigger_set = trigger != COMPARATOR_TRIGGER_NONE;
	bool reset = config->action == LVD_ACTION_RESET;
	lvd_voltage_slope_t voltage_slope = LVD_VOLTAGE_SLOPE_BOTH;
	fsp_err_t fsp_err;

	if (config->action == LVD_ACTION_NONE) {
		LOG_WRN("TRIGGER do not take effect when action is no action");
		return 0;
	}

	if (reset && trigger == COMPARATOR_TRIGGER_BOTH_EDGES) {
		LOG_ERR("Could not set both edges trigger when action is reset");
		return -EINVAL;
	}

	if (trigger_set) {
		switch (trigger) {
		case COMPARATOR_TRIGGER_RISING_EDGE:
			voltage_slope = LVD_VOLTAGE_SLOPE_RISING;
			break;
		case COMPARATOR_TRIGGER_FALLING_EDGE:
			voltage_slope = LVD_VOLTAGE_SLOPE_FALLING;
			break;
		case COMPARATOR_TRIGGER_BOTH_EDGES:
			voltage_slope = LVD_VOLTAGE_SLOPE_BOTH;
			break;
		default:
			return -EINVAL;
		}
	}

	fsp_err = RP_LVD_Enable(&data->lvd_ctrl, trigger_set);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	if (trigger_set) {
		fsp_err = RP_LVD_TriggerSet(&data->lvd_ctrl, reset, voltage_slope);
		if (fsp_err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

static int lvd_renesas_ra_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	struct lvd_renesas_ra_data *data = dev->data;
	const struct lvd_renesas_ra_config *config = dev->config;
	fsp_err_t fsp_err;
	bool enabled_status;

	if (config->action == LVD_ACTION_NONE || config->action == LVD_ACTION_RESET) {
		LOG_ERR("Could not set callback for when action is not interrupt");
		return -ENOTSUP;
	}

	fsp_err = RP_LVD_IsEnable(&data->lvd_ctrl, &enabled_status);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	fsp_err = RP_LVD_Enable(&data->lvd_ctrl, false);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	data->user_cb = callback;
	data->user_cb_data = user_data;

	fsp_err = RP_LVD_Enable(&data->lvd_ctrl, enabled_status);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int lvd_renesas_ra_trigger_is_pending(const struct device *dev)
{
	struct lvd_renesas_ra_data *data = dev->data;
	const struct lvd_renesas_ra_config *config = dev->config;
	fsp_err_t fsp_err;

	if (config->reset_only) {
		LOG_ERR("Get output is not supported on this LVD channel");
		return -ENOTSUP;
	}

	if (atomic_cas(&data->flags, LVD_RENESAS_RA_EVT_PENDING, 0)) {
		fsp_err = R_LVD_StatusClear(&data->lvd_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			return -EIO;
		}

		return 1;
	}

	return 0;
}

static DEVICE_API(comparator, lvd_renesas_ra_api) = {
	.get_output = lvd_renesas_ra_get_output,
	.set_trigger = lvd_renesas_ra_set_trigger,
	.set_trigger_callback = lvd_renesas_ra_set_trigger_callback,
	.trigger_is_pending = lvd_renesas_ra_trigger_is_pending,
};

static int lvd_renesas_ra_init(const struct device *dev)
{
	fsp_err_t err;
	struct lvd_renesas_ra_data *data = dev->data;
	const struct lvd_renesas_ra_config *config = dev->config;

	if (config->reset_only == true) {
		if (data->lvd_config.voltage_slope == LVD_VOLTAGE_SLOPE_RISING) {
			data->lvd_config.detection_response = LVD_RESPONSE_RESET_ON_RISING;
		} else {
			data->lvd_config.detection_response = LVD_RESPONSE_RESET;
		}
	} else {
		switch (config->action) {
		case LVD_ACTION_RESET:
			if (data->lvd_config.voltage_slope == LVD_VOLTAGE_SLOPE_RISING) {
				data->lvd_config.detection_response = LVD_RESPONSE_RESET_ON_RISING;
			} else {
				data->lvd_config.detection_response = LVD_RESPONSE_RESET;
			}
			break;
		case LVD_ACTION_NMI:
			data->lvd_config.detection_response = LVD_RESPONSE_NMI;
			break;
		case LVD_ACTION_MI:
			data->lvd_config.detection_response = LVD_RESPONSE_INTERRUPT;
			break;
		case LVD_ACTION_NONE:
			data->lvd_config.detection_response = LVD_RESPONSE_NONE;
			break;
		default:
			return -EINVAL;
		}
	}

	err = R_LVD_Open(&data->lvd_ctrl, &data->lvd_config);
	if (err != 0) {
		LOG_ERR("Failed to initialize LVD channel %d", data->lvd_config.monitor_number);
		return -EIO;
	}

	config->irq_config_func();

	return 0;
}

static void ra_lvd_callback(lvd_callback_args_t *p_args)
{
	const struct device *dev = (const struct device *)(p_args->p_context);
	struct lvd_renesas_ra_data *data = dev->data;
	comparator_callback_t user_cb = data->user_cb;
	void *user_cb_data = data->user_cb_data;

	if (user_cb) {
		user_cb(dev, user_cb_data);
		return;
	}

	atomic_set(&data->flags, LVD_RENESAS_RA_EVT_PENDING);
}

#define EVENT_LVD_INT(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_LVD_LVD, channel))

#define LVD_RENESAS_RA_IRQ_INIT_FUNC_DEFINE(index)                                                 \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(index, interrupts),                                       \
		   (R_ICU->IELSR_b[DT_INST_IRQN(index)].IELS =                                     \
			    EVENT_LVD_INT(DT_INST_PROP(index, channel));                           \
                                                                                                   \
		    BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_LVD_INT(DT_INST_PROP(index, channel))); \
                                                                                                   \
		    IRQ_CONNECT(DT_INST_IRQ(index, irq), DT_INST_IRQ(index, priority),             \
				lvd_lvd_isr, DEVICE_DT_INST_GET(index), 0);                        \
                                                                                                   \
		    irq_enable(DT_INST_IRQ(index, irq));))

#define DIGITAL_FILTER_GET(index)                                                                  \
	COND_CODE_1(IS_EQ(DT_INST_PROP(index, noise_filter), 1), (LVD_SAMPLE_CLOCK_DISABLED),      \
		    (UTIL_CAT(LVD_SAMPLE_CLOCK_LOCO_DIV_, DT_INST_PROP(index, noise_filter))))

#define IRQ_PARAMETER(index)                                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, interrupts), (DT_INST_IRQ(index, irq)),           \
		    (FSP_INVALID_VECTOR))

#define IPL_PARAMETER(index)                                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, interrupts), (DT_INST_IRQ(index, priority)),      \
		    (BSP_IRQ_DISABLED))

#define LVD_RENESAS_RA_INIT(index)                                                                 \
                                                                                                   \
	void lvd_renesas_ra_irq_config_func_##index(void)                                          \
	{                                                                                          \
		LVD_RENESAS_RA_IRQ_INIT_FUNC_DEFINE(index)                                         \
	}                                                                                          \
                                                                                                   \
	static const struct lvd_renesas_ra_config lvd_renesas_ra_config_##index = {                \
		.reset_only = DT_INST_PROP(index, reset_only),                                     \
		.action = DT_INST_ENUM_IDX(index, lvd_action),                                     \
		.irq_config_func = lvd_renesas_ra_irq_config_func_##index,                         \
	};                                                                                         \
                                                                                                   \
	static struct lvd_renesas_ra_data lvd_renesas_ra_data_##index = {                          \
		.lvd_config =                                                                      \
			{                                                                          \
				.monitor_number = DT_INST_PROP(index, channel),                    \
				.voltage_threshold = DT_INST_PROP(index, voltage_level),           \
				.detection_response = LVD_RESPONSE_NONE,                           \
				.voltage_slope = DT_INST_ENUM_IDX(index, lvd_trigger),             \
				.negation_delay = DT_INST_PROP(index, reset_negation_timing),      \
				.sample_clock_divisor = DIGITAL_FILTER_GET(index),                 \
				.irq = IRQ_PARAMETER(index),                                       \
				.monitor_ipl = IPL_PARAMETER(index),                               \
				.p_callback = ra_lvd_callback,                                     \
				.p_context = (void *)DEVICE_DT_INST_GET(index),                    \
				.p_extend = NULL,                                                  \
			},                                                                         \
		.flags = ATOMIC_INIT(0),                                                           \
		.user_cb = NULL,                                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, lvd_renesas_ra_init, NULL, &lvd_renesas_ra_data_##index,      \
			      &lvd_renesas_ra_config_##index, PRE_KERNEL_1,                        \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &lvd_renesas_ra_api);

DT_INST_FOREACH_STATUS_OKAY(LVD_RENESAS_RA_INIT)
