/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>
#include <Mru_Ip.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_mru);

#define MRU_MAX_INT_GROUPS	2
#define MRU_MAX_CHANNELS	12
#define MRU_MAX_MBOX_PER_CHAN	1
#define MRU_MBOX_SIZE		4
#define MRU_CHANNEL_OFFSET	0x1000

#define MRU_NODE(n)		DT_NODELABEL(mru##n)
#define MRU_BASE(n)		((RTU_MRU_Type *)DT_REG_ADDR(MRU_NODE(n)))
#define MRU_RX_CHANNELS(n)	DT_PROP_OR(MRU_NODE(n), rx_channels, 0)
#define MRU_MBOX_ADDR(n, ch, mb)	\
	(DT_REG_ADDR(MRU_NODE(n)) + ((ch + 1) * MRU_CHANNEL_OFFSET) + (MRU_MBOX_SIZE * mb))

/* Utility macros to convert from GIC index to interrupt group index */
#define _MRU_IRQ_17		MRU_IP_INT_GROUP_0
#define _MRU_IRQ_18		MRU_IP_INT_GROUP_1
#define MRU_INT_GROUP(irq)	_CONCAT(_MRU_IRQ_, irq)

#define _CONCAT7(...)		DT_CAT7(__VA_ARGS__)
#define MRU_ISR_FUNC(n)							\
	_CONCAT7(Mru_Ip_RTU, CONFIG_NXP_S32_RTU_INDEX, _MRU, n, _Int,	\
		 MRU_INT_GROUP(DT_IRQN(MRU_NODE(n))), _IRQHandler)

struct nxp_s32_mru_data {
	mbox_callback_t cb[MRU_MAX_CHANNELS];
	void *user_data[MRU_MAX_CHANNELS];
};

struct nxp_s32_mru_config {
	RTU_MRU_Type *base;
	Mru_Ip_ConfigType hw_cfg;
	void (*config_irq)(void);
};

static inline bool is_rx_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct nxp_s32_mru_config *cfg = dev->config;

	return ((ch < MRU_MAX_CHANNELS) && (ch < cfg->hw_cfg.NumChannel));
}

/* Get a channel's mailbox address, no boundaries validation */
static inline uintptr_t get_mbox_addr(const struct device *dev, uint32_t channel,
				      uint32_t mbox)
{
	const struct nxp_s32_mru_config *cfg = dev->config;

	return ((uintptr_t)cfg->base + (channel + 1) * MRU_CHANNEL_OFFSET
		+ mbox * MRU_MBOX_SIZE);
}

static int nxp_s32_mru_send(const struct device *dev, uint32_t channel,
			    const struct mbox_msg *msg)
{
	const struct nxp_s32_mru_config *cfg = dev->config;
	uint32_t *tx_mbox_addr[MRU_MAX_MBOX_PER_CHAN];
	Mru_Ip_TransmitChannelType tx_cfg;
	Mru_Ip_StatusType status;

	if (channel >= MRU_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (msg == NULL) {
		return -EINVAL;
	} else if (msg->size > (MRU_MBOX_SIZE * MRU_MAX_MBOX_PER_CHAN)) {
		return -EMSGSIZE;
	}

	for (int i = 0; i < MRU_MAX_MBOX_PER_CHAN; i++) {
		tx_mbox_addr[i] = (uint32_t *)get_mbox_addr(dev, channel, i);
	}

	tx_cfg.NumTxMB = MRU_MAX_MBOX_PER_CHAN,
	tx_cfg.LastTxMBIndex = MRU_MAX_MBOX_PER_CHAN - 1,
	tx_cfg.MBAddList = (volatile uint32 * const *)tx_mbox_addr,
	tx_cfg.ChMBSTATAdd = &cfg->base->CHXCONFIG[channel].CH_MBSTAT,

	status = Mru_Ip_Transmit(&tx_cfg, (const uint32_t *)msg->data);

	return (status == MRU_IP_STATUS_SUCCESS ? 0 : -EBUSY);
}

static int nxp_s32_mru_register_callback(const struct device *dev, uint32_t channel,
					 mbox_callback_t cb, void *user_data)
{
	struct nxp_s32_mru_data *data = dev->data;

	if (!is_rx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int nxp_s32_mru_mtu_get(const struct device *dev)
{
	return (MRU_MBOX_SIZE * MRU_MAX_MBOX_PER_CHAN);
}

static uint32_t nxp_s32_mru_max_channels_get(const struct device *dev)
{
	return MRU_MAX_CHANNELS;
}

static int nxp_s32_mru_set_enabled(const struct device *dev, uint32_t channel,
				   bool enable)
{
	struct nxp_s32_mru_data *data = dev->data;
	const struct nxp_s32_mru_config *cfg = dev->config;

	const Mru_Ip_ChannelCfgType *ch_cfg = cfg->hw_cfg.ChannelCfg;

	if (!is_rx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	if (enable && (data->cb[channel] == NULL)) {
		LOG_WRN("Enabling channel without a registered callback\n");
	}

	if (enable) {
		/*
		 * Make the channel's registers writable and then once again after
		 * enabling interrupts and mailboxes so remote can transmit
		 */
		*ch_cfg[channel].ChCFG0Add = RTU_MRU_CH_CFG0_CHE(1);
		*ch_cfg[channel].ChCFG0Add = RTU_MRU_CH_CFG0_IE(1)
						| RTU_MRU_CH_CFG0_MBE0(1)
						| RTU_MRU_CH_CFG0_CHE(1);
	} else {
		/*
		 * Disable interrupts and mailboxes on this channel, making
		 * the channel's registers not writable afterwards
		 */
		*ch_cfg[channel].ChCFG0Add = RTU_MRU_CH_CFG0_IE(0)
						| RTU_MRU_CH_CFG0_MBE0(0);
	}

	return 0;
}

static int nxp_s32_mru_init(const struct device *dev)
{
	const struct nxp_s32_mru_config *cfg = dev->config;

	if (cfg->hw_cfg.NumChannel == 0) {
		/* Nothing to do if no Rx channels are configured */
		return 0;
	}

	/* All configured Rx channels will be disabled after this call */
	Mru_Ip_Init(&cfg->hw_cfg);

	/*
	 * Configure and enable interrupt group, but channel's interrupt are
	 * disabled until calling .set_enabled()
	 */
	cfg->config_irq();

	return 0;
}

static const struct mbox_driver_api nxp_s32_mru_driver_api = {
	.send = nxp_s32_mru_send,
	.register_callback = nxp_s32_mru_register_callback,
	.mtu_get = nxp_s32_mru_mtu_get,
	.max_channels_get = nxp_s32_mru_max_channels_get,
	.set_enabled = nxp_s32_mru_set_enabled,
};

#define MRU_ISR_FUNC_DECLARE(n)		extern void MRU_ISR_FUNC(n)(void)

#define MRU_INIT_IRQ_FUNC(n)					\
	MRU_ISR_FUNC_DECLARE(n);				\
	static void nxp_s32_mru_##n##_init_irq(void)		\
	{							\
		IRQ_CONNECT(DT_IRQN(MRU_NODE(n)),		\
			    DT_IRQ(MRU_NODE(n), priority),	\
			    MRU_ISR_FUNC(n),			\
			    NULL,				\
			    DT_IRQ(MRU_NODE(n), flags));	\
		irq_enable(DT_IRQN(MRU_NODE(n)));		\
	}

#define MRU_CH_RX_CFG(i, n)								\
	static volatile const uint32_t * const						\
	nxp_s32_mru_##n##_ch_##i##_rx_mbox_addr[MRU_MAX_MBOX_PER_CHAN] = {		\
		(uint32_t *const)MRU_MBOX_ADDR(n, i, 0),				\
	};										\
	static uint32_t nxp_s32_mru_##n##_ch_##i##_buf[MRU_MAX_MBOX_PER_CHAN];		\
	static const Mru_Ip_ReceiveChannelType nxp_s32_mru_##n##_ch_##i##_rx_cfg = {	\
		.ChannelId = i,								\
		.InstanceId = n,							\
		.ChannelIndex = i,							\
		.NumRxMB = MRU_MAX_MBOX_PER_CHAN,					\
		.MBAddList = nxp_s32_mru_##n##_ch_##i##_rx_mbox_addr,			\
		.RxBuffer = nxp_s32_mru_##n##_ch_##i##_buf,				\
		.ReceiveNotification = nxp_s32_mru_##n##_cb				\
	}

#define MRU_CH_RX_LINK_CFG_MBOX(i, n, chan, intgroup)					\
	{										\
		[intgroup] = { &nxp_s32_mru_##n##_ch_##chan##_rx_cfg }			\
	}

#define MRU_CH_RX_LINK_CFG(i, n)							\
	static const Mru_Ip_MBLinkReceiveChannelType					\
	nxp_s32_mru_##n##_ch_##i##_rx_link_cfg[MRU_MAX_MBOX_PER_CHAN][MRU_MAX_INT_GROUPS] = {\
		MRU_CH_RX_LINK_CFG_MBOX(0, n, i, MRU_INT_GROUP(DT_IRQN(MRU_NODE(n))))	\
	}

#define MRU_CH_CFG(i, n)								\
	{										\
		.ChCFG0Add = &MRU_BASE(n)->CHXCONFIG[i].CH_CFG0,			\
		.ChCFG0 = RTU_MRU_CH_CFG0_IE(0) | RTU_MRU_CH_CFG0_MBE0(0),		\
		.ChCFG1Add = &MRU_BASE(n)->CHXCONFIG[i].CH_CFG1,			\
		.ChCFG1 = RTU_MRU_CH_CFG1_MBIC0(MRU_INT_GROUP(DT_IRQN(MRU_NODE(n)))),	\
		.ChMBSTATAdd = &MRU_BASE(n)->CHXCONFIG[i].CH_MBSTAT,			\
		.NumMailbox = MRU_MAX_MBOX_PER_CHAN,					\
		.MBLinkReceiveChCfg = nxp_s32_mru_##n##_ch_##i##_rx_link_cfg		\
	}

/* Callback wrapper to adapt MRU's baremetal driver callback to Zephyr's mbox driver callback */
#define MRU_CALLBACK_WRAPPER_FUNC(n)								\
	void nxp_s32_mru_##n##_cb(uint8_t channel, const uint32_t *buf, uint8_t mbox_count)	\
	{											\
		const struct device *dev = DEVICE_DT_GET(MRU_NODE(n));				\
		struct nxp_s32_mru_data *data = dev->data;					\
												\
		if (is_rx_channel_valid(dev, channel)) {					\
			if (data->cb[channel] != NULL) {					\
				struct mbox_msg msg = {						\
					.data = (const void *)buf,				\
					.size = mbox_count * MRU_MBOX_SIZE			\
				};								\
				data->cb[channel](dev, channel, data->user_data[channel], &msg);\
			}									\
		}										\
	}

#define MRU_CH_RX_DEFINITIONS(n)						\
	MRU_CALLBACK_WRAPPER_FUNC(n)						\
	MRU_INIT_IRQ_FUNC(n)							\
	LISTIFY(MRU_RX_CHANNELS(n), MRU_CH_RX_CFG, (;), n);			\
	LISTIFY(MRU_RX_CHANNELS(n), MRU_CH_RX_LINK_CFG, (;), n);		\
	static const Mru_Ip_ChannelCfgType nxp_s32_mru_##n##_ch_cfg[] = {	\
		LISTIFY(MRU_RX_CHANNELS(n), MRU_CH_CFG, (,), n)			\
	}

#define MRU_INSTANCE_DEFINE(n)								\
	COND_CODE_0(MRU_RX_CHANNELS(n), (EMPTY), (MRU_CH_RX_DEFINITIONS(n)));		\
	static struct nxp_s32_mru_data nxp_s32_mru_##n##_data;				\
	static struct nxp_s32_mru_config nxp_s32_mru_##n##_config = {			\
		.base = MRU_BASE(n),							\
		.hw_cfg = {								\
			.InstanceId = n,						\
			.StateIndex = n,						\
			.NumChannel = MRU_RX_CHANNELS(n),				\
			.ChannelCfg = COND_CODE_0(MRU_RX_CHANNELS(n),			\
						  (NULL), (nxp_s32_mru_##n##_ch_cfg)),	\
			.NOTIFYAdd = {							\
				&MRU_BASE(n)->NOTIFY0,					\
				&MRU_BASE(n)->NOTIFY1					\
			},								\
		},									\
		.config_irq = COND_CODE_0(MRU_RX_CHANNELS(n),				\
					  (NULL), (nxp_s32_mru_##n##_init_irq)),	\
	};										\
											\
	DEVICE_DT_DEFINE(MRU_NODE(n), nxp_s32_mru_init, NULL,				\
			&nxp_s32_mru_##n##_data, &nxp_s32_mru_##n##_config,		\
			POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,				\
			&nxp_s32_mru_driver_api)

#if DT_NODE_HAS_STATUS(MRU_NODE(0), okay)
MRU_INSTANCE_DEFINE(0);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(1), okay)
MRU_INSTANCE_DEFINE(1);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(2), okay)
MRU_INSTANCE_DEFINE(2);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(3), okay)
MRU_INSTANCE_DEFINE(3);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(4), okay)
MRU_INSTANCE_DEFINE(4);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(5), okay)
MRU_INSTANCE_DEFINE(5);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(6), okay)
MRU_INSTANCE_DEFINE(6);
#endif

#if DT_NODE_HAS_STATUS(MRU_NODE(7), okay)
MRU_INSTANCE_DEFINE(7);
#endif
