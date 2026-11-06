/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rh850_ext_irq

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "r_icu.h"
#include "r_ioport.h"
#include <zephyr/drivers/interrupt_controller/intc_rh850_ext_irq.h>

LOG_MODULE_REGISTER(rh850_ext_irq, CONFIG_INTC_LOG_LEVEL);

struct intc_rh850_ext_irq_config {
	const struct pinctrl_dev_config *pin_config;
	const external_irq_api_t *fsp_api;
};

struct intc_rh850_ext_irq_data {
	external_irq_ctrl_t *fsp_ctrl;
	external_irq_cfg_t *fsp_cfg;
	intc_rh850_ext_irq_callback_t callback;
	void *callback_data;
};

/* FSP interruption handlers. */
void r_icu_isr(void);

void intc_ext_irq_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	r_icu_isr();
}

int intc_rh850_ext_irq_enable(const struct device *dev)
{
	const struct intc_rh850_ext_irq_config *config = dev->config;
	struct intc_rh850_ext_irq_data *data = dev->data;
	unsigned int key;
	fsp_err_t err;

	/* Keep interrupts locked while unmask IRQ and set interrupt vector method*/
	key = irq_lock();

	err = config->fsp_api->enable(data->fsp_ctrl);
	if (err == FSP_SUCCESS) {
		/*
		 * The ISR is registered in the Zephyr software ISR table
		 * so the table reference method (EITB) must be selected
		 * through the Zephyr IRQ API.
		 */
		irq_enable(data->fsp_cfg->irq);
	}

	irq_unlock(key);

	switch (err) {
	case FSP_SUCCESS:
		return 0;
	case FSP_ERR_ASSERTION:
		return -EINVAL;
	case FSP_ERR_NOT_OPEN:
		return -EACCES;
	case FSP_ERR_IRQ_BSP_DISABLED:
		return -ENOTSUP;
	default:
		return -EIO;
	}
}

int intc_rh850_ext_irq_disable(const struct device *dev)
{
	const struct intc_rh850_ext_irq_config *config = dev->config;
	struct intc_rh850_ext_irq_data *data = dev->data;
	fsp_err_t err;

	err = config->fsp_api->disable(data->fsp_ctrl);

	switch (err) {
	case FSP_SUCCESS:
		return 0;
	case FSP_ERR_ASSERTION:
		return -EINVAL;
	case FSP_ERR_NOT_OPEN:
		return -EACCES;
	case FSP_ERR_IRQ_BSP_DISABLED:
		return -ENOTSUP;
	default:
		return -EIO;
	}
}

int intc_rh850_ext_irq_set_callback(const struct device *dev, intc_rh850_ext_irq_callback_t cb,
				      void *arg)
{
	struct intc_rh850_ext_irq_data *data = dev->data;

	data->callback = cb;
	data->callback_data = arg;

	return 0;
}

int intc_rh850_ext_irq_set_type(const struct device *dev, uint8_t trig)
{
	const struct intc_rh850_ext_irq_config *config = dev->config;
	struct intc_rh850_ext_irq_data *data = dev->data;
	fsp_err_t err;
	external_irq_cfg_t *p_cfg = (external_irq_cfg_t *)data->fsp_cfg;

	p_cfg->trigger = (external_irq_trigger_t)trig;

	/* Close driver before calling open to avoid already open error */
	err = config->fsp_api->close(data->fsp_ctrl);

	if (err != FSP_SUCCESS) {
		if (err == FSP_ERR_ASSERTION) {
			return -EINVAL;
		} else {
			return -EIO;
		}
	}

	err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	switch (err) {
	case FSP_SUCCESS:
		return 0;
	case FSP_ERR_ASSERTION:
		return -EINVAL;
	case FSP_ERR_INVALID_ARGUMENT:
		return -EINVAL;
	case FSP_ERR_ALREADY_OPEN:
		return -EALREADY;
	case FSP_ERR_IP_CHANNEL_NOT_PRESENT:
		return -ENODEV;
	case FSP_ERR_UNSUPPORTED:
		return -ENOTSUP;
	default:
		return -EIO;
	}
}

static int intc_rh850_ext_irq_init(const struct device *dev)
{
	const struct intc_rh850_ext_irq_config *config = dev->config;
	struct intc_rh850_ext_irq_data *data = dev->data;
	fsp_err_t err;

	if (config->pin_config) {
		err = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);
		if (err) {
			LOG_ERR("%s: pinctrl config failed.", __func__);
			return -EIO;
		}
	}

	err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	switch (err) {
	case FSP_SUCCESS:
		return 0;
	case FSP_ERR_ASSERTION:
		return -EINVAL;
	case FSP_ERR_INVALID_ARGUMENT:
		return -EINVAL;
	case FSP_ERR_ALREADY_OPEN:
		return -EALREADY;
	case FSP_ERR_IP_CHANNEL_NOT_PRESENT:
		return -ENODEV;
	case FSP_ERR_UNSUPPORTED:
		return -ENOTSUP;
	default:
		return -EIO;
	}
}

static void intc_rh850_ext_irq_callback(external_irq_callback_args_t *args)
{
	const struct device *dev = (const struct device *)args->p_context;
	struct intc_rh850_ext_irq_data *data = dev->data;

	if (data->callback) {
		data->callback(data->callback_data);
	}
}

/*
 * ************************* DRIVER REGISTER SECTION ***************************
 */

/* Stub IOPORT instance used to invoke API 'noiseFilterCfg' of IOPORT
 * in API 'intc_rh850_ext_irq_init'
 */
static ioport_instance_ctrl_t g_ioport_stub_ctrl = {
	.open = IOPORT_OPEN,
};

static const ioport_instance_t g_ioport_stub_instance = {
	.p_api = &g_ioport_on_ioport,
	.p_ctrl = &g_ioport_stub_ctrl,
	.p_cfg = NULL,
};

#define EXT_IRQ_RH850_IRQ_CONNECT(index, isr)                                              \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq), DT_INST_IRQ_BY_IDX(index, 0, priority), \
		    isr, DEVICE_DT_INST_GET(index), 0)

#define INTC_RH850_EXT_IRQ_INIT(index)                                                \
	static icu_extended_cfg_t g_icu##index##_extend = {                               \
		.p_ioport_instance = &g_ioport_stub_instance,                                 \
	};                                                                                \
                                                                                      \
	static external_irq_cfg_t g_external_irq##index##_cfg = {                         \
		.trigger = DT_INST_ENUM_IDX_OR(index, trigger_type, 0),                       \
		.p_callback = intc_rh850_ext_irq_callback,                                    \
		.p_context = (void *)DEVICE_DT_INST_GET(index),                               \
		.p_extend = &g_icu##index##_extend,                                           \
		.ipl = DT_INST_IRQ(index, priority),                                          \
		.irq = DT_INST_IRQ(index, irq),                                               \
		.channel = DT_INST_REG_ADDR(index),                                           \
	};                                                                                \
                                                                                      \
	PINCTRL_DT_INST_DEFINE(index);                                                    \
                                                                                      \
	static const struct intc_rh850_ext_irq_config intc_rh850_ext_irq_config##index = {\
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                          \
		.fsp_api = &g_external_irq_on_icu,                                            \
	};                                                                                \
                                                                                      \
	static icu_instance_ctrl_t g_external_irq##index##_ctrl;                          \
                                                                                      \
	static struct intc_rh850_ext_irq_data intc_rh850_ext_irq_data##index = {          \
		.fsp_ctrl = (external_irq_ctrl_t *)&g_external_irq##index##_ctrl,             \
		.fsp_cfg = (external_irq_cfg_t *)&g_external_irq##index##_cfg,                \
	};                                                                                \
                                                                                      \
	static int intc_rh850_ext_irq_init_##index(const struct device *dev)              \
	{                                                                                 \
		EXT_IRQ_RH850_IRQ_CONNECT(index, intc_ext_irq_isr);                           \
		return intc_rh850_ext_irq_init(dev);                                          \
	};                                                                                \
                                                                                      \
	DEVICE_DT_INST_DEFINE(index, intc_rh850_ext_irq_init_##index, NULL,               \
			      &intc_rh850_ext_irq_data##index,                                    \
			      &intc_rh850_ext_irq_config##index, PRE_KERNEL_1,                    \
			      CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INTC_RH850_EXT_IRQ_INIT)
