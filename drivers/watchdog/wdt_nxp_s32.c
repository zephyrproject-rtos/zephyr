/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_swt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <Swt_Ip.h>
#include <Swt_Ip_Irq.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(swt_nxp_s32);

#define PARAM_UNUSED	0

struct swt_nxp_s32_config {
	uint8_t instance;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct swt_nxp_s32_data {
	Swt_Ip_ConfigType swt_config;
	wdt_callback_t callback;
	bool timeout_valid;
};

static int swt_nxp_s32_setup(const struct device *dev, uint8_t options)
{
	const struct swt_nxp_s32_config *config = dev->config;
	struct swt_nxp_s32_data *data = dev->data;
	Swt_Ip_StatusType status;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	data->swt_config.bEnRunInStopMode =
		(options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;

	data->swt_config.bEnRunInDebugMode =
		(options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;

	status = Swt_Ip_Init(config->instance, &data->swt_config);

	if (status != SWT_IP_STATUS_SUCCESS) {
		LOG_ERR("Setting up watchdog failed");
		return -EIO;
	}

	return 0;
}

static int swt_nxp_s32_disable(const struct device *dev)
{
	const struct swt_nxp_s32_config *config = dev->config;
	struct swt_nxp_s32_data *data = dev->data;
	Swt_Ip_StatusType status;

	status = Swt_Ip_Deinit(config->instance);

	if (status != SWT_IP_STATUS_SUCCESS) {
		LOG_ERR("Disabling watchdog failed");
		return -EIO;
	}

	data->timeout_valid = false;

	return 0;
}

static int swt_nxp_s32_install_timeout(const struct device *dev,
				       const struct wdt_timeout_cfg *cfg)
{
	const struct swt_nxp_s32_config *config = dev->config;
	struct swt_nxp_s32_data *data = dev->data;
	uint32_t clock_rate;
	int err;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate);
	if (err) {
		LOG_ERR("Failed to get module clock frequency");
		return err;
	}

	data->swt_config.u32TimeoutValue = clock_rate / 1000U * cfg->window.max;

	if (cfg->window.min) {
		data->swt_config.bEnWindow = true;
		data->swt_config.u32WindowValue =
			clock_rate / 1000U * (cfg->window.max - cfg->window.min);
	} else {
		data->swt_config.bEnWindow = false;
		data->swt_config.u32WindowValue = 0;
	}

	if ((data->swt_config.u32TimeoutValue < SWT_MIN_VALUE_TIMEOUT_U32) ||
	    (data->swt_config.u32TimeoutValue < data->swt_config.u32WindowValue)) {
		LOG_ERR("Invalid timeout");
		return -EINVAL;
	}

	data->swt_config.bEnInterrupt = cfg->callback != NULL;
	data->callback = cfg->callback;
	data->timeout_valid = true;
	LOG_DBG("Installed timeout (timeoutValue = %d)",
		data->swt_config.u32TimeoutValue);

	return 0;
}

static int swt_nxp_s32_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	const struct swt_nxp_s32_config *config = dev->config;

	Swt_Ip_Service(config->instance);
	LOG_DBG("Fed the watchdog");

	return 0;
}

void swt_nxp_s32_isr(const struct device *dev)
{
	const struct swt_nxp_s32_config *config = dev->config;

	Swt_Ip_IrqHandler(config->instance);
}

static const struct wdt_driver_api swt_nxp_s32_driver_api = {
	.setup = swt_nxp_s32_setup,
	.disable = swt_nxp_s32_disable,
	.install_timeout = swt_nxp_s32_install_timeout,
	.feed = swt_nxp_s32_feed,
};

#define SWT_NXP_S32_CALLBACK(n)							\
	void swt_nxp_s32_##n##_callback(void)					\
	{									\
		const struct device *dev = DEVICE_DT_INST_GET(n);		\
		struct swt_nxp_s32_data *data = dev->data;			\
										\
		if (data->callback) {						\
			data->callback(dev, PARAM_UNUSED);			\
		}								\
	}

#define SWT_NXP_S32_HW_INSTANCE_CHECK(i, n) \
	((DT_INST_REG_ADDR(n) == IP_SWT_##i##_BASE) ? i : 0)

#define SWT_NXP_S32_HW_INSTANCE(n) \
	LISTIFY(__DEBRACKET SWT_INSTANCE_COUNT, SWT_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define SWT_NXP_S32_DEVICE_INIT(n)						\
	SWT_NXP_S32_CALLBACK(n)							\
	static struct swt_nxp_s32_data swt_nxp_s32_data_##n = {			\
		.swt_config = {							\
			.u8MapEnBitmask = SWT_IP_ALL_MAP_DISABLE |		\
				SWT_IP_MAP0_ENABLE | SWT_IP_MAP1_ENABLE |	\
				SWT_IP_MAP2_ENABLE | SWT_IP_MAP3_ENABLE |	\
				SWT_IP_MAP4_ENABLE | SWT_IP_MAP5_ENABLE |	\
				SWT_IP_MAP6_ENABLE | SWT_IP_MAP7_ENABLE,	\
			.bEnResetOnInvalidAccess = TRUE,			\
			.eServiceMode = FALSE,					\
			.u16InitialKey = 0,					\
			.lockConfig = SWT_IP_UNLOCK,				\
			.pfSwtCallback = &swt_nxp_s32_##n##_callback,		\
		},								\
	};									\
	static const struct swt_nxp_s32_config swt_nxp_s32_config_##n = {	\
		.instance = SWT_NXP_S32_HW_INSTANCE(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
				DT_INST_CLOCKS_CELL(n, name),			\
	};									\
										\
	static int swt_nxp_s32_##n##_init(const struct device *dev)		\
	{									\
		const struct swt_nxp_s32_config *config = dev->config;		\
		int err;							\
										\
		if (!device_is_ready(config->clock_dev)) {			\
			return -ENODEV;						\
		}								\
										\
		err = clock_control_on(config->clock_dev, config->clock_subsys);\
		if (err) {							\
			return err;						\
		}								\
										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    swt_nxp_s32_isr, DEVICE_DT_INST_GET(n),		\
			    DT_INST_IRQ(n, flags));				\
		irq_enable(DT_INST_IRQN(n));					\
										\
		return 0;							\
	}									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			 swt_nxp_s32_##n##_init,				\
			 NULL,							\
			 &swt_nxp_s32_data_##n,					\
			 &swt_nxp_s32_config_##n,				\
			 POST_KERNEL,						\
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
			 &swt_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SWT_NXP_S32_DEVICE_INIT)
