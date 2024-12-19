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

LOG_MODULE_REGISTER(rz_ext_irq, CONFIG_INTC_LOG_LEVEL);

struct intc_rz_ext_irq_config {
	const struct pinctrl_dev_config *pin_config;
	const external_irq_cfg_t *fsp_cfg;
	const external_irq_api_t *fsp_api;
};

struct intc_rz_ext_irq_data {
	external_irq_ctrl_t *fsp_ctrl;
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

int intc_rz_ext_irq_set_callback(const struct device *dev, void *p_callback)
{
	const struct intc_rz_ext_irq_config *config = dev->config;
	struct intc_rz_ext_irq_data *data = dev->data;
	fsp_err_t err = FSP_SUCCESS;

	err = config->fsp_api->callbackSet(data->fsp_ctrl, p_callback, NULL, NULL);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

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

#define EXT_IRQ_RZG_IRQ_CONNECT(index, isr, isr_nmi)                                               \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq), DT_INST_IRQ_BY_IDX(index, 0, priority),     \
		    COND_CODE_0(DT_INST_IRQ_BY_IDX(index, 0, irq),                                 \
		    (isr_nmi), (isr)), DEVICE_DT_INST_GET(index), 0);

#define INTC_RZG_EXT_IRQ_INIT(index)                                                               \
	static const external_irq_cfg_t g_external_irq##index##_cfg = {                            \
		.trigger = DT_INST_PROP(index, trigger_type),                                      \
		.filter_enable = false,                                                            \
		.pclk_div = EXTERNAL_IRQ_PCLK_DIV_BY_1,                                            \
		.p_callback = NULL,                                                                \
		.p_context = DEVICE_DT_GET(DT_DRV_INST(index)),                                    \
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
		intc_rz_ext_irq_init(dev);                                                         \
		return 0;                                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, intc_rz_ext_irq_init_##index, NULL,                           \
			      &intc_rz_ext_irq_data##index, &intc_rz_ext_irq_config##index,        \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INTC_RZG_EXT_IRQ_INIT)
