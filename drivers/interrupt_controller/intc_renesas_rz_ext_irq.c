/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_ext_irq

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <instances/rzg/r_intc_irq.h>
#include <instances/rzg/r_intc_nmi.h>
#include <zephyr/drivers/interrupt_controller/intc_rz_ext_irq.h>

LOG_MODULE_REGISTER(rz_ext_irq, CONFIG_INTC_LOG_LEVEL);

struct intc_rz_ext_irq_config {
	const struct pinctrl_dev_config *pin_config;
	const external_irq_cfg_t *fsp_cfg;
	const external_irq_api_t *fsp_api;
};

struct intc_rz_ext_irq_data {
	external_irq_ctrl_t *fsp_ctrl;
	intc_rz_ext_irq_callback_t callback;
	void *callback_data;
};

/* FSP interruption handlers. */
void r_intc_irq_isr(void);
void r_intc_nmi_isr(void);

int intc_rz_ext_irq_enable(const struct device *dev)
{
	const struct intc_rz_ext_irq_config *config = dev->config;
	struct intc_rz_ext_irq_data *data = dev->data;
	fsp_err_t err = FSP_SUCCESS;

	err = config->fsp_api->enable(data->fsp_ctrl);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int intc_rz_ext_irq_disable(const struct device *dev)
{
	const struct intc_rz_ext_irq_config *config = dev->config;
	struct intc_rz_ext_irq_data *data = dev->data;
	fsp_err_t err = FSP_SUCCESS;

	err = config->fsp_api->disable(data->fsp_ctrl);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int intc_rz_ext_irq_set_callback(const struct device *dev, intc_rz_ext_irq_callback_t cb, void *arg)
{
	struct intc_rz_ext_irq_data *data = dev->data;

	data->callback = cb;
	data->callback_data = arg;

	return 0;
}

static int intc_rz_ext_irq_init(const struct device *dev)
{
	const struct intc_rz_ext_irq_config *config = dev->config;
	struct intc_rz_ext_irq_data *data = dev->data;
	fsp_err_t err = FSP_SUCCESS;
	int ret = 0;

	if (config->pin_config) {
		ret = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);

		if (ret < 0) {
			LOG_ERR("%s: pinctrl config failed.", __func__);
			return ret;
		}
	}

	err = config->fsp_api->open(data->fsp_ctrl, config->fsp_cfg);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static void intc_rz_ext_irq_callback(external_irq_callback_args_t *args)
{
	const struct device *dev = (const struct device *)args->p_context;
	struct intc_rz_ext_irq_data *data = dev->data;

	if (data->callback) {
		data->callback(data->callback_data);
	}
}

#define EXT_IRQ_RZG_IRQ_CONNECT(index, isr, isr_nmi)                                               \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq), DT_INST_IRQ_BY_IDX(index, 0, priority),     \
		    COND_CODE_0(DT_INST_IRQ_BY_IDX(index, 0, irq),                                 \
		    (isr_nmi), (isr)), NULL, 0);

#define INTC_RZG_EXT_IRQ_INIT(index)                                                               \
	static const external_irq_cfg_t g_external_irq##index##_cfg = {                            \
		.trigger = DT_INST_ENUM_IDX_OR(index, trigger_type, 0),                            \
		.filter_enable = true,                                                             \
		.pclk_div = EXTERNAL_IRQ_PCLK_DIV_BY_1,                                            \
		.p_callback = intc_rz_ext_irq_callback,                                            \
		.p_context = DEVICE_DT_INST_GET(index),                                            \
		.p_extend = NULL,                                                                  \
		.ipl = DT_INST_IRQ_BY_IDX(index, 0, priority),                                     \
		.irq = DT_INST_IRQ_BY_IDX(index, 0, irq),                                          \
		COND_CODE_0(DT_INST_IRQ_BY_IDX(index, 0, irq),                   \
			(.channel = DT_INST_IRQ_BY_IDX(index, 0, irq)),          \
			(.channel = DT_INST_IRQ_BY_IDX(index, 0, irq) - 1)),     \
	};                                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	struct intc_rz_ext_irq_config intc_rz_ext_irq_config##index = {                            \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                               \
		.fsp_cfg = (external_irq_cfg_t *)&g_external_irq##index##_cfg,                     \
		COND_CODE_0(DT_INST_IRQ_BY_IDX(index, 0, irq), (            \
			.fsp_api = &g_external_irq_on_intc_nmi), (          \
			.fsp_api = &g_external_irq_on_intc_irq)),           \
	};                                                                                         \
                                                                                                   \
	COND_CODE_0(DT_INST_IRQ_BY_IDX(index, 0, irq),                                     \
		    (static intc_nmi_instance_ctrl_t g_external_irq##index##_ctrl;),       \
		    (static intc_irq_instance_ctrl_t g_external_irq##index##_ctrl;))       \
                                                                                                   \
	static struct intc_rz_ext_irq_data intc_rz_ext_irq_data##index = {                         \
		.fsp_ctrl = (external_irq_ctrl_t *)&g_external_irq##index##_ctrl,                  \
	};                                                                                         \
                                                                                                   \
	static int intc_rz_ext_irq_init_##index(const struct device *dev)                          \
	{                                                                                          \
		EXT_IRQ_RZG_IRQ_CONNECT(index, r_intc_irq_isr, r_intc_nmi_isr)                     \
		return intc_rz_ext_irq_init(dev);                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, intc_rz_ext_irq_init_##index, NULL,                           \
			      &intc_rz_ext_irq_data##index, &intc_rz_ext_irq_config##index,        \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INTC_RZG_EXT_IRQ_INIT)
