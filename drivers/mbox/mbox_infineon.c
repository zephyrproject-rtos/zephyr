/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Mailbox (MBOX) driver for the Infineon inter-processor communication (IPC)
 * block, implemented with direct register access.
 *
 * Each mbox channel maps to one IPC channel and one IPC interrupt structure
 * within an IPC instance and carries one or two 32-bit data words per message.
 * The IPC instance base address, the channel and the interrupt-structure
 * index are all taken from devicetree, so the driver is not tied to any
 * particular SoC.
 */

#define DT_DRV_COMPAT infineon_mbox

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <string.h>

LOG_MODULE_REGISTER(mbox_infineon, CONFIG_MBOX_LOG_LEVEL);

/* Message size is one or two 32-bit words, held in DATA0 (and DATA1), and is
 * fixed per controller from the devicetree "msg-size-bytes" property. Two-word
 * (8-byte) mode requires an IPC block that implements DATA1.
 */
#define IFX_MBOX_MSG_WORDS_MAX 2U

/* IPC_STRUCT register offsets, relative to a channel's base. */
#define IFX_IPC_ACQUIRE     0x00U
#define IFX_IPC_RELEASE     0x04U
#define IFX_IPC_NOTIFY      0x08U
#define IFX_IPC_DATA0       0x0CU
#define IFX_IPC_DATA1       0x10U
#define IFX_IPC_LOCK_STATUS 0x1CU

/* IPC_INTR_STRUCT register offsets, relative to an interrupt structure's base. */
#define IFX_IPC_INTR        0x00U
#define IFX_IPC_INTR_SET    0x04U
#define IFX_IPC_INTR_MASK   0x08U
#define IFX_IPC_INTR_MASKED 0x0CU

/*
 * Within an IPC instance the channel structures start at offset 0 and the
 * interrupt structures at offset 0x1000; both are laid out on a 0x20 stride.
 */
#define IFX_IPC_STRUCT_STRIDE      0x20U
#define IFX_IPC_INTR_STRUCT_BASE   0x1000U
#define IFX_IPC_INTR_STRUCT_STRIDE 0x20U

/* ACQUIRE.SUCCESS and LOCK_STATUS.ACQUIRED both live in bit 31. */
#define IFX_IPC_ACQUIRE_SUCCESS BIT(31)
#define IFX_IPC_LOCK_ACQUIRED   BIT(31)

/*
 * INTR and INTR_MASK pack a per-channel RELEASE bitfield in [15:0] and a
 * per-channel NOTIFY bitfield in [31:16]; the driver only uses NOTIFY.
 */
#define IFX_IPC_INTR_NOTIFY_MASK GENMASK(31, 16)

struct ifx_mbox_config {
	uint32_t num_channels;
	uint8_t msg_words;
	const mem_addr_t *ipc_base;
	const uint8_t *ipc_channel;
	const uint8_t *ipc_intr;
	const uint32_t *irq;
	void (*irq_config)(void);
};

struct ifx_mbox_data {
	mbox_callback_t *cb;
	void **user_data;
	uint32_t *received;
	bool *enabled;
};

static inline mem_addr_t ifx_mbox_channel_addr(const struct ifx_mbox_config *cfg, uint32_t channel)
{
	return cfg->ipc_base[channel] + cfg->ipc_channel[channel] * IFX_IPC_STRUCT_STRIDE;
} /* ifx_mbox_channel_addr() */

static inline mem_addr_t ifx_mbox_intr_addr(const struct ifx_mbox_config *cfg, uint32_t channel)
{
	return cfg->ipc_base[channel] + IFX_IPC_INTR_STRUCT_BASE +
	       cfg->ipc_intr[channel] * IFX_IPC_INTR_STRUCT_STRIDE;
} /* ifx_mbox_intr_addr() */

static void ifx_mbox_handle_channel(const struct device *dev, uint32_t channel)
{
	const struct ifx_mbox_config *cfg = dev->config;
	struct ifx_mbox_data *data = dev->data;
	mem_addr_t ipc = ifx_mbox_channel_addr(cfg, channel);
	mem_addr_t intr = ifx_mbox_intr_addr(cfg, channel);
	uint32_t ch_mask = BIT(cfg->ipc_channel[channel]);
	struct mbox_msg msg;

	/* Clear the notify interrupt for this channel; the read-back guarantees
	 * the clear has taken effect before the ISR returns.
	 */
	sys_write32(FIELD_PREP(IFX_IPC_INTR_NOTIFY_MASK, ch_mask), intr + IFX_IPC_INTR);
	(void)sys_read32(intr + IFX_IPC_INTR);

	/* A disabled channel still needs the lock released so the sender can
	 * reuse it.
	 */
	if (!data->enabled[channel]) {
		if (sys_read32(ipc + IFX_IPC_LOCK_STATUS) & IFX_IPC_LOCK_ACQUIRED) {
			sys_write32(0U, ipc + IFX_IPC_RELEASE);
		}
		return;
	}

	/* The data words are only valid while the channel is locked. */
	if (sys_read32(ipc + IFX_IPC_LOCK_STATUS) & IFX_IPC_LOCK_ACQUIRED) {
		uint32_t *slot = &data->received[channel * cfg->msg_words];

		slot[0] = sys_read32(ipc + IFX_IPC_DATA0);
		if (cfg->msg_words > 1U) {
			slot[1] = sys_read32(ipc + IFX_IPC_DATA1);
		}

		if (data->cb[channel] != NULL) {
			msg.data = slot;
			msg.size = cfg->msg_words * 4U;
			data->cb[channel](dev, channel, data->user_data[channel], &msg);
		}

		/* Release the channel to signal the sender the words were consumed. */
		sys_write32(0U, ipc + IFX_IPC_RELEASE);
	}
} /* ifx_mbox_handle_channel() */

static int ifx_mbox_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	const struct ifx_mbox_config *cfg = dev->config;
	uint32_t words[IFX_MBOX_MSG_WORDS_MAX];
	mem_addr_t ipc;

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	if (msg == NULL) {
		/* Signalling mode is not supported; a data word is required. */
		return -ENOTSUP;
	}

	if (msg->size != cfg->msg_words * 4U) {
		return -EMSGSIZE;
	}

	ipc = ifx_mbox_channel_addr(cfg, channel);

	/* Reading ACQUIRE attempts to lock the channel; a clear SUCCESS bit
	 * means the receiver has not yet released the previous message.
	 */
	if (!(sys_read32(ipc + IFX_IPC_ACQUIRE) & IFX_IPC_ACQUIRE_SUCCESS)) {
		return -EBUSY;
	}

	memcpy(words, msg->data, cfg->msg_words * 4U);
	sys_write32(words[0], ipc + IFX_IPC_DATA0);
	if (cfg->msg_words > 1U) {
		sys_write32(words[1], ipc + IFX_IPC_DATA1);
	}

	/* Notify the receiver's interrupt structure that the message is ready. */
	sys_write32(BIT(cfg->ipc_intr[channel]), ipc + IFX_IPC_NOTIFY);

	return 0;
} /* ifx_mbox_send() */

static int ifx_mbox_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	const struct ifx_mbox_config *cfg = dev->config;
	struct ifx_mbox_data *data = dev->data;

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
} /* ifx_mbox_register_callback() */

static int ifx_mbox_mtu_get(const struct device *dev)
{
	const struct ifx_mbox_config *cfg = dev->config;

	return cfg->msg_words * 4U;
} /* ifx_mbox_mtu_get() */

static uint32_t ifx_mbox_max_channels_get(const struct device *dev)
{
	const struct ifx_mbox_config *cfg = dev->config;

	return cfg->num_channels;
} /* ifx_mbox_max_channels_get() */

static int ifx_mbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	const struct ifx_mbox_config *cfg = dev->config;
	struct ifx_mbox_data *data = dev->data;
	mem_addr_t intr;
	uint32_t mask, ch_mask;

	if (channel >= cfg->num_channels) {
		return -EINVAL;
	}

	if (data->enabled[channel] == enable) {
		return -EALREADY;
	}

	intr = ifx_mbox_intr_addr(cfg, channel);
	ch_mask = BIT(cfg->ipc_channel[channel]);

	/* Read/modify/write only this channel's NOTIFY bit, preserving the mask
	 * bits of any other channels that share this interrupt structure.
	 */
	mask = sys_read32(intr + IFX_IPC_INTR_MASK);

	if (enable) {
		mask |= FIELD_PREP(IFX_IPC_INTR_NOTIFY_MASK, ch_mask);
		sys_write32(mask, intr + IFX_IPC_INTR_MASK);
		data->enabled[channel] = true;
		irq_enable(cfg->irq[channel]);
	} else {
		irq_disable(cfg->irq[channel]);
		data->enabled[channel] = false;
		mask &= ~FIELD_PREP(IFX_IPC_INTR_NOTIFY_MASK, ch_mask);
		sys_write32(mask, intr + IFX_IPC_INTR_MASK);
	}

	return 0;
} /* ifx_mbox_set_enabled() */

static int ifx_mbox_init(const struct device *dev)
{
	const struct ifx_mbox_config *cfg = dev->config;

	/* The ISR maps one interrupt line to one channel and never decodes the
	 * NOTIFY field, so channels sharing an interrupt structure is not
	 * supported and is rejected here.
	 */
	for (uint32_t i = 0U; i < cfg->num_channels; i++) {
		for (uint32_t j = i + 1U; j < cfg->num_channels; j++) {
			if (ifx_mbox_intr_addr(cfg, i) == ifx_mbox_intr_addr(cfg, j)) {
				LOG_ERR("channels %u and %u share an interrupt structure", i, j);
				return -EINVAL;
			}
		}
	}

	cfg->irq_config();

	return 0;
} /* ifx_mbox_init() */

static DEVICE_API(mbox, ifx_mbox_api) = {
	.send = ifx_mbox_send,
	.register_callback = ifx_mbox_register_callback,
	.mtu_get = ifx_mbox_mtu_get,
	.max_channels_get = ifx_mbox_max_channels_get,
	.set_enabled = ifx_mbox_set_enabled,
};

#define IFX_MBOX_N_CH(n) DT_INST_PROP_LEN(n, ipc_configs)

#define IFX_MBOX_MSG_WORDS(n) (DT_INST_PROP(n, msg_size_bytes) / 4U)

#define IFX_MBOX_IPC_NODE(idx, n) DT_INST_PHANDLE_BY_IDX(n, ipc_configs, idx)
#define IFX_MBOX_IPC_BASE(idx, n) DT_REG_ADDR(IFX_MBOX_IPC_NODE(idx, n))
#define IFX_MBOX_IPC_CHAN(idx, n) DT_INST_PHA_BY_IDX(n, ipc_configs, idx, channel)
#define IFX_MBOX_IPC_INTR(idx, n) DT_INST_PHA_BY_IDX(n, ipc_configs, idx, interrupt)
#define IFX_MBOX_IRQN(idx, n)     DT_INST_IRQN_BY_IDX(n, idx)

/* Structure counts and availability bitmasks default to the full 32-entry
 * range when the IPC node omits them.
 */
#define IFX_MBOX_CH_COUNT(idx, n) \
	DT_PROP_OR(IFX_MBOX_IPC_NODE(idx, n), channel_count, 32)
#define IFX_MBOX_INTR_COUNT(idx, n) \
	DT_PROP_OR(IFX_MBOX_IPC_NODE(idx, n), interrupt_count, 32)
#define IFX_MBOX_CH_AVAIL(idx, n) \
	DT_PROP_OR(IFX_MBOX_IPC_NODE(idx, n), channel_mask, 0xFFFFFFFF)
#define IFX_MBOX_INTR_AVAIL(idx, n) \
	DT_PROP_OR(IFX_MBOX_IPC_NODE(idx, n), interrupt_mask, 0xFFFFFFFF)

#define IFX_MBOX_CH_ASSERT(idx, n)                                                    \
	BUILD_ASSERT(IFX_MBOX_IPC_CHAN(idx, n) < IFX_MBOX_CH_COUNT(idx, n),               \
		     "ipc-configs channel index out of range");                        \
	BUILD_ASSERT((IFX_MBOX_CH_AVAIL(idx, n) & BIT(IFX_MBOX_IPC_CHAN(idx, n))) != 0U,     \
		     "ipc-configs channel is reserved by the system");                \
	BUILD_ASSERT(IFX_MBOX_IPC_INTR(idx, n) < IFX_MBOX_INTR_COUNT(idx, n),        \
		     "ipc-configs interrupt index out of range");                      \
	BUILD_ASSERT((IFX_MBOX_INTR_AVAIL(idx, n) & BIT(IFX_MBOX_IPC_INTR(idx, n))) != 0U,    \
		     "ipc-configs interrupt is reserved by the system");

#define IFX_MBOX_ISR_DEFINE(idx, n)                                           \
	static void ifx_mbox_isr_##n##_##idx(const struct device *dev)              \
	{                                                                           \
		ifx_mbox_handle_channel(dev, idx);                                     \
	}

#define IFX_MBOX_IRQ_CONNECT(idx, n)                                          \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, idx), DT_INST_IRQ_BY_IDX(n, idx, priority),       \
		    ifx_mbox_isr_##n##_##idx, DEVICE_DT_INST_GET(n), 0);

#define IFX_MBOX_INIT(n)                                                             \
	BUILD_ASSERT(DT_INST_NUM_IRQS(n) == IFX_MBOX_N_CH(n),                            \
		     "interrupts count must match ipc-configs count");                  \
                                                                                   \
	LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_CH_ASSERT, (), n)                           \
                                                                                \
	LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_ISR_DEFINE, (), n)                     \
                                                                                 \
	static void ifx_mbox_irq_config_##n(void)                                   \
	{                                                                     \
		LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_IRQ_CONNECT, (), n)                   \
	}                                                                          \
                                                                                 \
	static const mem_addr_t ifx_mbox_ipc_base_##n[] = {                      \
		LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_IPC_BASE, (,), n)};               \
	static const uint8_t ifx_mbox_ipc_channel_##n[] = {                       \
		LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_IPC_CHAN, (,), n)};               \
	static const uint8_t ifx_mbox_ipc_intr_##n[] = {                             \
		LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_IPC_INTR, (,), n)};                   \
	static const uint32_t ifx_mbox_irq_##n[] = {                                \
		LISTIFY(IFX_MBOX_N_CH(n), IFX_MBOX_IRQN, (,), n)};                 \
                                                                               \
	static mbox_callback_t ifx_mbox_cb_##n[IFX_MBOX_N_CH(n)];                    \
	static void *ifx_mbox_user_data_##n[IFX_MBOX_N_CH(n)];                      \
	static uint32_t ifx_mbox_received_##n[IFX_MBOX_N_CH(n) * IFX_MBOX_MSG_WORDS(n)]; \
	static bool ifx_mbox_enabled_##n[IFX_MBOX_N_CH(n)];                          \
                                                                              \
	static const struct ifx_mbox_config ifx_mbox_config_##n = {                    \
		.num_channels = IFX_MBOX_N_CH(n),                                         \
		.msg_words = IFX_MBOX_MSG_WORDS(n),                                       \
		.ipc_base = ifx_mbox_ipc_base_##n,                                   \
		.ipc_channel = ifx_mbox_ipc_channel_##n,                                    \
		.ipc_intr = ifx_mbox_ipc_intr_##n,                                     \
		.irq = ifx_mbox_irq_##n,                                               \
		.irq_config = ifx_mbox_irq_config_##n,                                   \
	};                                                                              \
                                                                             \
	static struct ifx_mbox_data ifx_mbox_data_##n = {                             \
		.cb = ifx_mbox_cb_##n,                                                    \
		.user_data = ifx_mbox_user_data_##n,                                 \
		.received = ifx_mbox_received_##n,                                    \
		.enabled = ifx_mbox_enabled_##n,                                      \
	};                                                                      \
                                                                                     \
	DEVICE_DT_INST_DEFINE(n, ifx_mbox_init, NULL, &ifx_mbox_data_##n, &ifx_mbox_config_##n, \
			      POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY, &ifx_mbox_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_MBOX_INIT)
