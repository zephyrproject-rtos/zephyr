/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_acmphs

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "r_acmphs.h"

LOG_MODULE_REGISTER(acmphs_renesas_ra, CONFIG_COMPARATOR_LOG_LEVEL);

#define ACMPHS_RENESAS_RA_FLAG_HS BIT(0)

void comp_hs_int_isr(void);

struct acmphs_renesas_ra_global_config {
	const struct pinctrl_dev_config *pcfg;
};

struct acmphs_renesas_ra_config {
	const struct pinctrl_dev_config *pcfg;
};

struct acmphs_renesas_ra_data {
	const struct device *dev;
	acmphs_instance_ctrl_t acmphs;
	comparator_cfg_t fsp_config;
	comparator_callback_t user_cb;
	void *user_cb_data;
	atomic_t flags;
};

static int acmphs_renesas_get_output(const struct device *dev)
{
	struct acmphs_renesas_ra_data *data = dev->data;
	comparator_status_t status;
	fsp_err_t fsp_err;

	fsp_err = R_ACMPHS_StatusGet(&data->acmphs, &status);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	switch (status.state) {
	case COMPARATOR_STATE_OUTPUT_LOW:
		return 0;

	case COMPARATOR_STATE_OUTPUT_HIGH:
		return 1;

	case COMPARATOR_STATE_OUTPUT_DISABLED:
		LOG_ERR("Need to set trigger to open comparator first");
		return -EIO;
	}

	return 0;
}

static int acmphs_renesas_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	struct acmphs_renesas_ra_data *data = dev->data;

	/* Disable interrupt */
	data->acmphs.p_reg->CMPCTL &= ~R_ACMPHS0_CMPCTL_CEG_Msk;

	switch (trigger) {
	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->fsp_config.trigger = COMPARATOR_TRIGGER_RISING;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->fsp_config.trigger = COMPARATOR_TRIGGER_FALLING;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		data->fsp_config.trigger = COMPARATOR_TRIGGER_BOTH_EDGE;
		break;

	case COMPARATOR_TRIGGER_NONE:
		data->fsp_config.trigger = COMPARATOR_TRIGGER_NO_EDGE;
		break;
	}

	data->acmphs.p_reg->CMPCTL |= (data->fsp_config.trigger << R_ACMPHS0_CMPCTL_CEG_Pos);

	return 0;
}

static int acmphs_renesas_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	struct acmphs_renesas_ra_data *data = dev->data;

	/* Disable interrupt */
	data->acmphs.p_reg->CMPCTL &= ~R_ACMPHS0_CMPCTL_CEG_Msk;

	data->user_cb = callback;
	data->user_cb_data = user_data;

	/* Enable interrupt */
	data->acmphs.p_reg->CMPCTL |= (data->fsp_config.trigger << R_ACMPHS0_CMPCTL_CEG_Pos);

	return 0;
}

static int acmphs_renesas_trigger_is_pending(const struct device *dev)
{
	struct acmphs_renesas_ra_data *data = dev->data;

	if (data->flags & ACMPHS_RENESAS_RA_FLAG_HS) {
		atomic_and(&data->flags, ~ACMPHS_RENESAS_RA_FLAG_HS);
		return 1;
	}

	return 0;
}

static void acmphs_renesas_ra_hs_isr(comparator_callback_args_t *fsp_args)
{
	const struct device *dev = fsp_args->p_context;
	struct acmphs_renesas_ra_data *data = dev->data;
	comparator_callback_t cb = data->user_cb;

	if (cb != NULL) {
		cb(dev, data->user_cb_data);
		return;
	}

	atomic_or(&data->flags, ACMPHS_RENESAS_RA_FLAG_HS);
}

static int acmphs_renesas_ra_global_init(const struct device *dev)
{
	const struct acmphs_renesas_ra_global_config *cfg = dev->config;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_DBG("pin VCOUT initial failed");
		return ret;
	}

	return 0;
}

static int acmphs_renesas_ra_init(const struct device *dev)
{
	struct acmphs_renesas_ra_data *data = dev->data;
	const struct acmphs_renesas_ra_config *cfg = dev->config;
	fsp_err_t fsp_err;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_DBG("pin default initial failed");
		return ret;
	}

	data->fsp_config.p_context = (void *)dev;

	fsp_err = R_ACMPHS_Open(&data->acmphs, &data->fsp_config);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	/*
	 * Once the analog comparator is configured, the program must wait
	 * for the ACMPHS stabilization time (300ns enabling + 200ns input switching)
	 * before using the comparator.
	 */
	k_usleep(5);

	fsp_err = R_ACMPHS_OutputEnable(&data->acmphs);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(comparator, acmphs_renesas_ra_api) = {
	.get_output = acmphs_renesas_get_output,
	.set_trigger = acmphs_renesas_set_trigger,
	.set_trigger_callback = acmphs_renesas_set_trigger_callback,
	.trigger_is_pending = acmphs_renesas_trigger_is_pending,
};

PINCTRL_DT_DEFINE(DT_INST(0, renesas_ra_acmphs_global));

static const struct acmphs_renesas_ra_global_config acmphs_renesas_ra_global_config = {
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST(0, renesas_ra_acmphs_global)),
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_acmphs_global),
		 acmphs_renesas_ra_global_init, NULL, NULL, &acmphs_renesas_ra_global_config,
		 PRE_KERNEL_2, CONFIG_COMPARATOR_INIT_PRIORITY, NULL)

#define ACMPHS_RENESAS_RA_IRQ_INIT(idx)                                                            \
	{                                                                                          \
		R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(idx, hs, irq)].IELS =                           \
			BSP_PRV_IELS_ENUM(CONCAT(EVENT_ACMPHS, DT_INST_PROP(idx, channel), _INT)); \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, hs, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, hs, priority), comp_hs_int_isr,               \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, hs, irq));                                     \
	}

#define FILTER_PARAMETER(idx)                                                                      \
	COND_CODE_1(IS_EQ(DT_INST_PROP(idx, noise_filter), 1),                                     \
	(COMPARATOR_FILTER_OFF), (UTIL_CAT(COMPARATOR_FILTER_, DT_INST_PROP(idx, noise_filter))))

#define INVERT_PARAMETER(idx)                                                                      \
	COND_CODE_1(DT_INST_PROP(idx, output_invert_polarity),                                     \
	(COMPARATOR_POLARITY_INVERT_ON), (COMPARATOR_POLARITY_INVERT_OFF))

#define PIN_OUTPUT_PARAMETER(idx)                                                                  \
	COND_CODE_1(DT_INST_PROP(idx, pin_output_enable),                                          \
	(COMPARATOR_PIN_OUTPUT_ON), (COMPARATOR_PIN_OUTPUT_OFF))

#define IRQ_PARAMETER(idx)                                                                         \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(idx, hs),                                                 \
	(DT_INST_IRQ_BY_NAME(idx, hs, irq)), (FSP_INVALID_VECTOR))

#define IPL_PARAMETER(idx)                                                                         \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(idx, hs),                                                 \
	(DT_INST_IRQ_BY_NAME(idx, hs, priority)), (BSP_IRQ_DISABLED))

#define IRQ_INIT_MACRO_FUNCTION(idx)                                                               \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(idx, hs), (ACMPHS_RENESAS_RA_IRQ_INIT(idx);), ())

#define ACMPHS_RENESAS_RA_INIT(idx)                                                                \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static r_acmphs_extended_cfg_t g_acmphs_cfg_extend_##idx = {                               \
		.input_voltage = UTIL_CAT(ACMPHS_INPUT_,                                           \
					  DT_INST_STRING_UPPER_TOKEN(idx, compare_input_source)),  \
		.reference_voltage =                                                               \
			UTIL_CAT(ACMPHS_REFERENCE_,                                                \
				 DT_INST_STRING_UPPER_TOKEN(idx, reference_input_source)),         \
		.maximum_status_retries = 1024,                                                    \
	};                                                                                         \
	static const struct acmphs_renesas_ra_config acmphs_renesas_ra_config_##idx = {            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
	};                                                                                         \
	static struct acmphs_renesas_ra_data acmphs_renesas_ra_data_##idx = {                      \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(idx, channel),                             \
				.mode = COMPARATOR_MODE_NORMAL,                                    \
				.trigger = COMPARATOR_TRIGGER_NO_EDGE,                             \
				.filter = FILTER_PARAMETER(idx),                                   \
				.invert = INVERT_PARAMETER(idx),                                   \
				.pin_output = PIN_OUTPUT_PARAMETER(idx),                           \
				.p_extend = &g_acmphs_cfg_extend_##idx,                            \
				.irq = IRQ_PARAMETER(idx),                                         \
				.ipl = IPL_PARAMETER(idx),                                         \
				.p_callback = acmphs_renesas_ra_hs_isr,                            \
			},                                                                         \
		.flags = 0,                                                                        \
	};                                                                                         \
	static int acmphs_renesas_ra_init##idx(const struct device *dev)                           \
	{                                                                                          \
		IRQ_INIT_MACRO_FUNCTION(idx)                                                       \
		return acmphs_renesas_ra_init(dev);                                                \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, acmphs_renesas_ra_init##idx, NULL,                              \
			      &acmphs_renesas_ra_data_##idx, &acmphs_renesas_ra_config_##idx,      \
			      POST_KERNEL, CONFIG_COMPARATOR_INIT_PRIORITY,                        \
			      &acmphs_renesas_ra_api)

DT_INST_FOREACH_STATUS_OKAY(ACMPHS_RENESAS_RA_INIT);
