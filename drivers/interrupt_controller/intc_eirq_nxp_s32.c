/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/intc_eirq_nxp_s32.h>

#include <Siul2_Icu_Ip_Irq.h>

#define NXP_S32_NUM_CHANNELS		SIUL2_ICU_IP_NUM_OF_CHANNELS
/*
 * The macros from low level driver contains a bracket so
 * it cannot be used for some Zephyr macros (e.g LISTIFY).
 * This just does remove the bracket to be used for such macro.
 */
#define NXP_S32_NUM_CHANNELS_DEBRACKET	__DEBRACKET SIUL2_ICU_IP_NUM_OF_CHANNELS

struct eirq_nxp_s32_config {
	uint8_t instance;
	mem_addr_t disr0;
	mem_addr_t direr0;

	const Siul2_Icu_Ip_ConfigType *icu_cfg;
	const struct pinctrl_dev_config *pincfg;
};

/* Wrapper callback for each EIRQ line, from low level driver callback to GPIO callback */
struct eirq_nxp_s32_cb {
	eirq_nxp_s32_callback_t cb;
	uint8_t pin;
	void *data;
};

struct eirq_nxp_s32_data {
	struct eirq_nxp_s32_cb *cb;
};

int eirq_nxp_s32_set_callback(const struct device *dev, uint8_t line,
				eirq_nxp_s32_callback_t cb, uint8_t pin, void *arg)
{
	struct eirq_nxp_s32_data *data = dev->data;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	if (data->cb[line].cb) {
		return -EBUSY;
	}

	data->cb[line].cb   = cb;
	data->cb[line].pin  = pin;
	data->cb[line].data = arg;

	return 0;
}

void eirq_nxp_s32_unset_callback(const struct device *dev, uint8_t line)
{
	struct eirq_nxp_s32_data *data = dev->data;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	data->cb[line].cb    = NULL;
	data->cb[line].pin   = 0;
	data->cb[line].data  = NULL;
}

void eirq_nxp_s32_enable_interrupt(const struct device *dev, uint8_t line,
					Siul2_Icu_Ip_EdgeType edge_type)
{
	const struct eirq_nxp_s32_config *config = dev->config;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	Siul2_Icu_Ip_SetActivationCondition(config->instance, line, edge_type);
	Siul2_Icu_Ip_EnableNotification(config->instance, line);
	Siul2_Icu_Ip_EnableInterrupt(config->instance, line);
}

void eirq_nxp_s32_disable_interrupt(const struct device *dev, uint8_t line)
{
	const struct eirq_nxp_s32_config *config = dev->config;

	__ASSERT(line < NXP_S32_NUM_CHANNELS, "Interrupt line is out of range");

	Siul2_Icu_Ip_DisableInterrupt(config->instance, line);
	Siul2_Icu_Ip_DisableNotification(config->instance, line);
	Siul2_Icu_Ip_SetActivationCondition(config->instance, line, SIUL2_ICU_DISABLE);
}

uint32_t eirq_nxp_s32_get_pending(const struct device *dev)
{
	const struct eirq_nxp_s32_config *config = dev->config;

	return sys_read32(config->disr0) & sys_read32(config->direr0);
}

static void eirq_nxp_s32_callback(const struct device *dev, uint8 line)
{
	const struct eirq_nxp_s32_data *data = dev->data;

	if (data->cb[line].cb != NULL) {
		data->cb[line].cb(data->cb[line].pin, data->cb[line].data);
	}
}

static int eirq_nxp_s32_init(const struct device *dev)
{
	const struct eirq_nxp_s32_config *config = dev->config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	if (Siul2_Icu_Ip_Init(config->instance, config->icu_cfg)) {
		return -EINVAL;
	}

	return 0;
}

#define EIRQ_NXP_S32_NODE(n)	DT_NODELABEL(eirq##n)

#define EIRQ_NXP_S32_CALLBACK(line, n)								\
	void nxp_s32_icu_##n##_eirq_line_##line##_callback(void)				\
	{											\
		const struct device *dev = DEVICE_DT_GET(EIRQ_NXP_S32_NODE(n));			\
												\
		eirq_nxp_s32_callback(dev, line);						\
	}

#define EIRQ_NXP_S32_CHANNEL_CONFIG(idx, n)							\
	{											\
		.hwChannel = idx,								\
		.digFilterEn = DT_PROP_OR(DT_CHILD(EIRQ_NXP_S32_NODE(n), line_##idx),		\
						filter_enable, 0),				\
		.maxFilterCnt = DT_PROP_OR(DT_CHILD(EIRQ_NXP_S32_NODE(n), line_##idx),		\
						filter_counter, 0),				\
		.intSel = SIUL2_ICU_IRQ,							\
		.intEdgeSel = SIUL2_ICU_DISABLE,						\
		.callback = NULL,								\
		.Siul2ChannelNotification = nxp_s32_icu_##n##_eirq_line_##idx##_callback,	\
		.callbackParam = 0U								\
	}

#define EIRQ_NXP_S32_CHANNELS_CONFIG(n)								\
	static const Siul2_Icu_Ip_ChannelConfigType eirq_##n##_channel_nxp_s32_cfg[] = {	\
		LISTIFY(NXP_S32_NUM_CHANNELS_DEBRACKET,	EIRQ_NXP_S32_CHANNEL_CONFIG, (,), n)	\
	}

#define EIRQ_NXP_S32_INSTANCE_CONFIG(n)								\
	static const Siul2_Icu_Ip_InstanceConfigType eirq_##n##_instance_nxp_s32_cfg = {	\
		.intFilterClk = DT_PROP_OR(EIRQ_NXP_S32_NODE(n),				\
						filter_prescaler, (0)),				\
		.altIntFilterClk = 0U,								\
	}

#define EIRQ_NXP_S32_COMBINE_CONFIG(n)								\
	static const Siul2_Icu_Ip_ConfigType eirq_##n##_nxp_s32_cfg = {				\
		.numChannels	 = NXP_S32_NUM_CHANNELS,					\
		.pInstanceConfig = &eirq_##n##_instance_nxp_s32_cfg,				\
		.pChannelsConfig = &eirq_##n##_channel_nxp_s32_cfg,				\
	}

#define EIRQ_NXP_S32_CONFIG(n)									\
	LISTIFY(NXP_S32_NUM_CHANNELS_DEBRACKET, EIRQ_NXP_S32_CALLBACK, (), n)			\
	EIRQ_NXP_S32_CHANNELS_CONFIG(n);							\
	EIRQ_NXP_S32_INSTANCE_CONFIG(n);							\
	EIRQ_NXP_S32_COMBINE_CONFIG(n);

#define _EIRQ_NXP_S32_IRQ_NAME(name)	DT_CAT3(SIUL2_EXT_IRQ_, name, _ISR)

#define EIRQ_NXP_S32_IRQ_NAME(idx, n)								\
	COND_CODE_1(DT_NODE_HAS_PROP(EIRQ_NXP_S32_NODE(n), interrupt_names),			\
		(_EIRQ_NXP_S32_IRQ_NAME(							\
			DT_STRING_TOKEN_BY_IDX(EIRQ_NXP_S32_NODE(n), interrupt_names, idx))),	\
		(DT_CAT3(SIUL2_, n, _ICU_EIRQ_SINGLE_INT_HANDLER)))

#define _EIRQ_NXP_S32_IRQ_CONFIG(idx, n)							\
	do {											\
		IRQ_CONNECT(DT_IRQ_BY_IDX(EIRQ_NXP_S32_NODE(n), idx, irq),			\
			DT_IRQ_BY_IDX(EIRQ_NXP_S32_NODE(n), idx, priority),			\
			EIRQ_NXP_S32_IRQ_NAME(idx, n),						\
			DEVICE_DT_GET(EIRQ_NXP_S32_NODE(n)),					\
			COND_CODE_1(CONFIG_GIC,							\
				(DT_IRQ_BY_IDX(EIRQ_NXP_S32_NODE(n), idx, flags)),		\
				(0)));								\
		irq_enable(DT_IRQ_BY_IDX(EIRQ_NXP_S32_NODE(n), idx, irq));			\
	} while (false);

#define EIRQ_NXP_S32_IRQ_CONFIG(n)								\
	LISTIFY(DT_NUM_IRQS(EIRQ_NXP_S32_NODE(n)), _EIRQ_NXP_S32_IRQ_CONFIG, (), n)

#define EIRQ_NXP_S32_INIT_DEVICE(n)								\
	EIRQ_NXP_S32_CONFIG(n)									\
	PINCTRL_DT_DEFINE(EIRQ_NXP_S32_NODE(n));						\
	static const struct eirq_nxp_s32_config eirq_nxp_s32_conf_##n = {			\
		.instance = n,									\
		.disr0    = (mem_addr_t)DT_REG_ADDR_BY_NAME(EIRQ_NXP_S32_NODE(n), disr0),	\
		.direr0   = (mem_addr_t)DT_REG_ADDR_BY_NAME(EIRQ_NXP_S32_NODE(n), direr0),	\
		.icu_cfg  = (Siul2_Icu_Ip_ConfigType *)&eirq_##n##_nxp_s32_cfg,			\
		.pincfg   = PINCTRL_DT_DEV_CONFIG_GET(EIRQ_NXP_S32_NODE(n))			\
	};											\
	static struct eirq_nxp_s32_cb eirq_nxp_s32_cb_##n[NXP_S32_NUM_CHANNELS];		\
	static struct eirq_nxp_s32_data eirq_nxp_s32_data_##n = {				\
		.cb = eirq_nxp_s32_cb_##n,							\
	};											\
	static int eirq_nxp_s32_init##n(const struct device *dev);				\
	DEVICE_DT_DEFINE(EIRQ_NXP_S32_NODE(n),							\
		eirq_nxp_s32_init##n,								\
		NULL,										\
		&eirq_nxp_s32_data_##n,								\
		&eirq_nxp_s32_conf_##n,								\
		PRE_KERNEL_2,									\
		CONFIG_INTC_INIT_PRIORITY,							\
		NULL);										\
	static int eirq_nxp_s32_init##n(const struct device *dev)				\
	{											\
		int err;									\
												\
		err = eirq_nxp_s32_init(dev);							\
		if (err) {									\
			return err;								\
		}										\
												\
		EIRQ_NXP_S32_IRQ_CONFIG(n);							\
												\
		return 0;									\
	}

#if DT_NODE_HAS_STATUS(EIRQ_NXP_S32_NODE(0), okay)
EIRQ_NXP_S32_INIT_DEVICE(0)
#endif

#if DT_NODE_HAS_STATUS(EIRQ_NXP_S32_NODE(1), okay)
EIRQ_NXP_S32_INIT_DEVICE(1)
#endif

#if DT_NODE_HAS_STATUS(EIRQ_NXP_S32_NODE(4), okay)
EIRQ_NXP_S32_INIT_DEVICE(4)
#endif

#if DT_NODE_HAS_STATUS(EIRQ_NXP_S32_NODE(5), okay)
EIRQ_NXP_S32_INIT_DEVICE(5)
#endif
