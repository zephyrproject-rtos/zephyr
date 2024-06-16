/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_wkpu

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>

#include <Wkpu_Ip_Irq.h>

#define NXP_S32_NUM_CHANNELS		WKPU_IP_NUM_OF_CHANNELS
#define NXP_S32_NUM_CHANNELS_DEBRACKET	__DEBRACKET WKPU_IP_NUM_OF_CHANNELS

struct wkpu_nxp_s32_config {
	uint8_t instance;

	const WKPU_Type *base;

	const Wkpu_Ip_IrqConfigType *wkpu_cfg;
};

/* Wrapper callback for each WKPU line, from low level driver callback to GPIO callback */
struct wkpu_nxp_s32_cb {
	wkpu_nxp_s32_callback_t cb;
	uint8_t pin;
	void *data;
};

struct wkpu_nxp_s32_data {
	struct wkpu_nxp_s32_cb *cb;
};

int wkpu_nxp_s32_set_callback(const struct device *dev, uint8_t line,
				wkpu_nxp_s32_callback_t cb, uint8_t pin, void *arg)
{
	struct wkpu_nxp_s32_data *data = dev->data;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	if ((data->cb[line].cb == cb) && (data->cb[line].data == arg)) {
		return 0;
	}

	if (data->cb[line].cb) {
		return -EBUSY;
	}

	data->cb[line].cb   = cb;
	data->cb[line].pin  = pin;
	data->cb[line].data = arg;

	return 0;
}

void wkpu_nxp_s32_unset_callback(const struct device *dev, uint8_t line)
{
	struct wkpu_nxp_s32_data *data = dev->data;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	data->cb[line].cb    = NULL;
	data->cb[line].pin   = 0;
	data->cb[line].data  = NULL;
}

void wkpu_nxp_s32_enable_interrupt(const struct device *dev, uint8_t line,
				   Wkpu_Ip_EdgeType edge_type)
{
	const struct wkpu_nxp_s32_config *config = dev->config;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	Wkpu_Ip_SetActivationCondition(config->instance, line, edge_type);
	Wkpu_Ip_EnableNotification(line);
	Wkpu_Ip_EnableInterrupt(config->instance, line);
}

void wkpu_nxp_s32_disable_interrupt(const struct device *dev, uint8_t line)
{
	const struct wkpu_nxp_s32_config *config = dev->config;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	Wkpu_Ip_DisableInterrupt(config->instance, line);
	Wkpu_Ip_DisableNotification(line);
	Wkpu_Ip_SetActivationCondition(config->instance, line, WKPU_IP_NONE_EDGE);
}

uint64_t wkpu_nxp_s32_get_pending(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint64_t flags;

	flags = config->base->WISR & config->base->IRER;
#if defined(WKPU_IP_64_CH_USED) && (WKPU_IP_64_CH_USED == STD_ON)
	flags |= ((uint64_t)(config->base->WISR_64 & config->base->IRER_64)) << 32U;
#endif

	return flags;
}

static void wkpu_nxp_s32_callback(const struct device *dev, uint8 line)
{
	const struct wkpu_nxp_s32_data *data = dev->data;

	if (data->cb[line].cb != NULL) {
		data->cb[line].cb(data->cb[line].pin, data->cb[line].data);
	}
}

static int wkpu_nxp_s32_init(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;

	if (Wkpu_Ip_Init(config->instance, config->wkpu_cfg)) {
		return -EINVAL;
	}

	return 0;
}

#define WKPU_NXP_S32_CALLBACK(line, n)								\
	void nxp_s32_wkpu_##n##wkpu_line_##line##_callback(void)				\
	{											\
		const struct device *dev = DEVICE_DT_INST_GET(n);				\
												\
		wkpu_nxp_s32_callback(dev, line);						\
	}

#define WKPU_NXP_S32_CHANNEL_CONFIG(idx, n)							\
	{											\
		.hwChannel = idx,								\
		.filterEn = DT_INST_PROP_OR(DT_INST_CHILD(n, line_##idx), filter_enable, 0),	\
		.edgeEvent = WKPU_IP_NONE_EDGE,							\
		.WkpuChannelNotification = nxp_s32_wkpu_##n##wkpu_line_##idx##_callback,	\
		.callback = NULL,								\
		.callbackParam = 0U								\
	}

#define WKPU_NXP_S32_CHANNELS_CONFIG(n)								\
	static const Wkpu_Ip_ChannelConfigType wkpu_##n##_channel_nxp_s32_cfg[] = {		\
		LISTIFY(NXP_S32_NUM_CHANNELS_DEBRACKET,	WKPU_NXP_S32_CHANNEL_CONFIG, (,), n)	\
	}

#define WKPU_NXP_S32_INSTANCE_CONFIG(n)								\
	static const Wkpu_Ip_IrqConfigType wkpu_##n##_nxp_s32_cfg = {				\
		.numChannels	 = NXP_S32_NUM_CHANNELS,					\
		.pChannelsConfig = &wkpu_##n##_channel_nxp_s32_cfg,				\
	}

#define WKPU_NXP_S32_CONFIG(n)									\
	LISTIFY(NXP_S32_NUM_CHANNELS_DEBRACKET, WKPU_NXP_S32_CALLBACK, (), n)			\
	WKPU_NXP_S32_CHANNELS_CONFIG(n);							\
	WKPU_NXP_S32_INSTANCE_CONFIG(n);

#define WKPU_NXP_S32_INIT_DEVICE(n)								\
	WKPU_NXP_S32_CONFIG(n)									\
	static const struct wkpu_nxp_s32_config wkpu_nxp_s32_conf_##n = {			\
		.instance = n,									\
		.base = (WKPU_Type *)DT_INST_REG_ADDR(n),					\
		.wkpu_cfg = (Wkpu_Ip_IrqConfigType *)&wkpu_##n##_nxp_s32_cfg,			\
	};											\
	static struct wkpu_nxp_s32_cb wkpu_nxp_s32_cb_##n[NXP_S32_NUM_CHANNELS];		\
	static struct wkpu_nxp_s32_data wkpu_nxp_s32_data_##n = {				\
		.cb = wkpu_nxp_s32_cb_##n,							\
	};											\
	static int wkpu_nxp_s32_init##n(const struct device *dev)				\
	{											\
		int err;									\
												\
		err = wkpu_nxp_s32_init(dev);							\
		if (err) {									\
			return err;								\
		}										\
												\
		IRQ_CONNECT(DT_INST_IRQ(n, irq), DT_INST_IRQ(n, priority),			\
			WKPU_EXT_IRQ_SINGLE_ISR, NULL,						\
			COND_CODE_1(CONFIG_GIC, (DT_INST_IRQ(n, flags)), (0)));			\
		irq_enable(DT_INST_IRQ(n, irq));						\
												\
		return 0;									\
	}											\
	DEVICE_DT_INST_DEFINE(n,								\
		wkpu_nxp_s32_init##n,								\
		NULL,										\
		&wkpu_nxp_s32_data_##n,								\
		&wkpu_nxp_s32_conf_##n,								\
		PRE_KERNEL_2,									\
		CONFIG_INTC_INIT_PRIORITY,							\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(WKPU_NXP_S32_INIT_DEVICE)
