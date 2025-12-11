/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_ext_irq

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#if CONFIG_DT_HAS_RENESAS_RZ_INTC_ENABLED
#include "r_intc_irq.h"
#elif CONFIG_DT_HAS_RENESAS_RZ_ICU_ENABLED
#include "r_icu.h"
#endif

#if CONFIG_RENESAS_RZ_INTC_HAS_NMI
#include "r_intc_nmi.h"
#endif

#include <zephyr/drivers/interrupt_controller/intc_rz_ext_irq.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>

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
#if CONFIG_DT_HAS_RENESAS_RZ_INTC_ENABLED
void r_intc_irq_isr(void *irq);
void r_intc_nmi_isr(void *irq);
#define INTC_IRQ_ISR r_intc_irq_isr
#elif CONFIG_DT_HAS_RENESAS_RZ_ICU_ENABLED
void r_icu_isr(void *irq);
#define INTC_IRQ_ISR r_icu_isr
#endif

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

int intc_rz_ext_irq_set_type(const struct device *dev, uint8_t trig)
{
	const struct intc_rz_ext_irq_config *config = dev->config;
	struct intc_rz_ext_irq_data *data = dev->data;
	fsp_err_t err = FSP_SUCCESS;
	external_irq_cfg_t *p_cfg = (external_irq_cfg_t *)config->fsp_cfg;

	p_cfg->trigger = (external_irq_trigger_t)trig;
	err = config->fsp_api->close(data->fsp_ctrl);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = config->fsp_api->open(data->fsp_ctrl, config->fsp_cfg);

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

static void intc_rz_ext_irq_callback(external_irq_callback_args_t *args)
{
	const struct device *dev = (const struct device *)args->p_context;
	struct intc_rz_ext_irq_data *data = dev->data;

	if (data->callback) {
		data->callback(data->callback_data);
	}
}

static void intc_rz_ext_irq_isr_handle(const struct device *dev)
{
	const struct intc_rz_ext_irq_config *config = dev->config;

	INTC_IRQ_ISR((void *)config->fsp_cfg->irq);
}

#ifdef CONFIG_CPU_CORTEX_M
#define GET_IRQ_FLAGS(index) 0
#else /* Cortex-A/R */
#define GET_IRQ_FLAGS(index) DT_INST_IRQ_BY_IDX(index, 0, flags)
#endif

#define EXT_IRQ_RZ_IRQ_CONNECT(index, isr)                                                         \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq), DT_INST_IRQ_BY_IDX(index, 0, priority),     \
		    isr, DEVICE_DT_INST_GET(index), GET_IRQ_FLAGS(index))

#define INTC_RZ_EXT_IRQ_INIT(index)                                                                \
	static external_irq_cfg_t g_external_irq##index##_cfg = {                                  \
		.trigger = DT_INST_ENUM_IDX_OR(index, trigger_type, 0),                            \
		.filter_enable = true,                                                             \
		.clock_source_div = EXTERNAL_IRQ_CLOCK_SOURCE_DIV_1,                               \
		.p_callback = intc_rz_ext_irq_callback,                                            \
		.p_context = DEVICE_DT_INST_GET(index),                                            \
		.p_extend = NULL,                                                                  \
		.ipl = DT_INST_IRQ_BY_IDX(index, 0, priority),                                     \
		.irq = DT_INST_IRQ_BY_IDX(index, 0, irq),                                          \
		COND_CODE_0(DT_NUM_REGS(DT_DRV_INST(index)), (         \
			.channel = 0), (                               \
			.channel = (DT_INST_REG_ADDR(index)))),        \
	};                                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	struct intc_rz_ext_irq_config intc_rz_ext_irq_config##index = {                            \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                               \
		.fsp_cfg = (external_irq_cfg_t *)&g_external_irq##index##_cfg,                     \
		COND_CODE_0(DT_NUM_REGS(DT_DRV_INST(index)), (                     \
			.fsp_api = &g_external_irq_on_intc_nmi), (                 \
			.fsp_api = &g_external_irq_on_intc_irq)),                  \
	};                                                                                         \
                                                                                                   \
	COND_CODE_0(DT_NUM_REGS(DT_DRV_INST(index)),                                       \
		    (static intc_nmi_instance_ctrl_t g_external_irq##index##_ctrl;),       \
		    (static intc_irq_instance_ctrl_t g_external_irq##index##_ctrl;))       \
                                                                                                   \
	static struct intc_rz_ext_irq_data intc_rz_ext_irq_data##index = {                         \
		.fsp_ctrl = (external_irq_ctrl_t *)&g_external_irq##index##_ctrl,                  \
	};                                                                                         \
                                                                                                   \
	static int intc_rz_ext_irq_init_##index(const struct device *dev)                          \
	{                                                                                          \
		COND_CODE_0(DT_NUM_REGS(DT_DRV_INST(index)), (                             \
			EXT_IRQ_RZ_IRQ_CONNECT(index, r_intc_nmi_isr);), (                 \
			EXT_IRQ_RZ_IRQ_CONNECT(index, intc_rz_ext_irq_isr_handle);))       \
		return intc_rz_ext_irq_init(dev);                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, intc_rz_ext_irq_init_##index, NULL,                           \
			      &intc_rz_ext_irq_data##index, &intc_rz_ext_irq_config##index,        \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

#define INTC_RZ_ICU_EXT_IRQ_INIT(index)                                                            \
	static external_irq_cfg_t g_external_irq##index##_cfg = {                                  \
		.trigger = DT_INST_ENUM_IDX_OR(index, trigger_type, 0),                            \
		.filter_enable = true,                                                             \
		.clock_source_div = EXTERNAL_IRQ_CLOCK_SOURCE_DIV_1,                               \
		.p_callback = intc_rz_ext_irq_callback,                                            \
		.p_context = DEVICE_DT_INST_GET(index),                                            \
		.p_extend = NULL,                                                                  \
		.ipl = DT_INST_IRQ_BY_IDX(index, 0, priority),                                     \
		.irq = DT_INST_IRQ_BY_IDX(index, 0, irq),                                          \
		.channel = DT_INST_REG_ADDR(index),                                                \
	};                                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	struct intc_rz_ext_irq_config intc_rz_ext_irq_config##index = {                            \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                               \
		.fsp_cfg = (external_irq_cfg_t *)&g_external_irq##index##_cfg,                     \
		.fsp_api = &g_external_irq_on_icu,                                                 \
	};                                                                                         \
                                                                                                   \
	icu_instance_ctrl_t g_external_irq##index##_ctrl;                                          \
                                                                                                   \
	static struct intc_rz_ext_irq_data intc_rz_ext_irq_data##index = {                         \
		.fsp_ctrl = (icu_instance_ctrl_t *)&g_external_irq##index##_ctrl,                  \
	};                                                                                         \
                                                                                                   \
	static int intc_rz_ext_irq_init_##index(const struct device *dev)                          \
	{                                                                                          \
		EXT_IRQ_RZ_IRQ_CONNECT(index, intc_rz_ext_irq_isr_handle);                         \
		return intc_rz_ext_irq_init(dev);                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, intc_rz_ext_irq_init_##index, NULL,                           \
			      &intc_rz_ext_irq_data##index, &intc_rz_ext_irq_config##index,        \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

#if CONFIG_DT_HAS_RENESAS_RZ_INTC_ENABLED
DT_INST_FOREACH_STATUS_OKAY(INTC_RZ_EXT_IRQ_INIT)
#elif CONFIG_DT_HAS_RENESAS_RZ_ICU_ENABLED
DT_INST_FOREACH_STATUS_OKAY(INTC_RZ_ICU_EXT_IRQ_INIT)
#endif
