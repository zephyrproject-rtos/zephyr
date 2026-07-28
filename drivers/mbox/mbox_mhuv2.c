/*
 * Copyright (c) 2026, Xsight Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM MHUv2 (Message Handling Unit v2) mailbox driver.
 * Implements the Zephyr mbox API for doorbell-only signalling.
 *
 * Supports all hardware channels from a single instance. Channel IDs
 * are flattened: id = hw_channel * 32 + doorbell_slot.
 */

#define DT_DRV_COMPAT arm_mhuv2

#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_mhuv2, CONFIG_MBOX_LOG_LEVEL);

/** Number of doorbell bits per MHUv2 channel window. */
#define MHU2_SLOTS_PER_CHANNEL	32

/** Maximum channel windows per MHUv2 frame. */
#define MHU2_CHANNEL_MAX	124

/** Maximum total doorbells = configured channels * 32. */
#define MHU2_MAX_DOORBELLS	(CONFIG_MBOX_MHUV2_NUM_CHANS * MHU2_SLOTS_PER_CHANNEL)

BUILD_ASSERT(CONFIG_MBOX_MHUV2_NUM_CHANS <= MHU2_CHANNEL_MAX,
	     "CONFIG_MBOX_MHUV2_NUM_CHANS exceeds hardware maximum of 124");

/** Timeout in microseconds for the ACCESS_READY handshake. */
#define MHU2_ACCESS_READY_TIMEOUT_US 1000

/** Receiver INT_EN.CHCOMB: enables the combined channel interrupt (MHUv2.1+). */
#define MHU2_INT_EN_CHCOMB	BIT(2)

/** AIDR.ARCH_MINOR_REV field: 0 = MHUv2.0, 1 = MHUv2.1. */
#define MHU2_AIDR_ARCH_MINOR_MASK 0xFU

/** @brief PBX (Postbox / sender) per-channel registers, 0x20 bytes each. */
struct mhuv2_send_channel_reg {
	volatile const uint32_t stat;
	uint32_t reserved0[2];
	volatile uint32_t stat_set;
	uint32_t reserved1[4];
};

/** @brief PBX (Postbox / sender) top-level register frame. */
struct mhuv2_send_reg {
	struct mhuv2_send_channel_reg channel[MHU2_CHANNEL_MAX];
	volatile const uint32_t msg_no_cap;
	volatile uint32_t resp_cfg;
	volatile uint32_t access_request;
	volatile const uint32_t access_ready;
	volatile const uint32_t int_access_stat;
	volatile uint32_t int_access_clr;
	volatile uint32_t int_access_en;
};

BUILD_ASSERT(offsetof(struct mhuv2_send_reg, msg_no_cap) == 0xF80);
BUILD_ASSERT(offsetof(struct mhuv2_send_reg, access_request) == 0xF88);
BUILD_ASSERT(offsetof(struct mhuv2_send_reg, access_ready) == 0xF8C);

/** @brief MBX (Mailbox / receiver) per-channel registers, 0x20 bytes each. */
struct mhuv2_recv_channel_reg {
	volatile const uint32_t stat;
	volatile const uint32_t stat_pend;
	volatile uint32_t stat_clear;
	uint32_t reserved0;
	volatile const uint32_t mask;
	volatile uint32_t mask_set;
	volatile uint32_t mask_clear;
	uint32_t reserved1;
};

/** @brief MBX (Mailbox / receiver) top-level register frame. */
struct mhuv2_recv_reg {
	struct mhuv2_recv_channel_reg channel[MHU2_CHANNEL_MAX];
	volatile const uint32_t msg_no_cap;
	uint32_t reserved0[3];
	volatile const uint32_t int_st;
	volatile uint32_t int_clr;
	volatile uint32_t int_en;
	uint32_t reserved1;
	volatile const uint32_t chcomb_int_st[4];
	uint32_t reserved2[6];
	volatile const uint32_t iidr;
	volatile const uint32_t aidr;
};

BUILD_ASSERT(offsetof(struct mhuv2_recv_reg, msg_no_cap) == 0xF80);
BUILD_ASSERT(offsetof(struct mhuv2_recv_reg, int_en) == 0xF98);
BUILD_ASSERT(offsetof(struct mhuv2_recv_reg, aidr) == 0xFCC);
BUILD_ASSERT(sizeof(struct mhuv2_send_channel_reg) == 0x20);
BUILD_ASSERT(sizeof(struct mhuv2_recv_channel_reg) == 0x20);

/** @brief Immutable per-instance configuration, populated from devicetree. */
struct mhuv2_config {
	DEVICE_MMIO_NAMED_ROM(send);
	DEVICE_MMIO_NAMED_ROM(recv);
	void (*irq_config)(const struct device *dev);
};

/** @brief Mutable per-instance runtime data. */
struct mhuv2_data {
	DEVICE_MMIO_NAMED_RAM(send);
	DEVICE_MMIO_NAMED_RAM(recv);
	mbox_callback_t cb[MHU2_MAX_DOORBELLS];
	void *user_data[MHU2_MAX_DOORBELLS];
	uint32_t num_channels;
	uint32_t num_doorbells;
	atomic_t send_busy;
};

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(dev)  ((const struct mhuv2_config *)((dev)->config))
#define DEV_DATA(dev) ((struct mhuv2_data *)((dev)->data))

#define MHUV2_SEND(dev) ((struct mhuv2_send_reg *)DEVICE_MMIO_NAMED_GET((dev), send))
#define MHUV2_RECV(dev) ((struct mhuv2_recv_reg *)DEVICE_MMIO_NAMED_GET((dev), recv))

static inline uint32_t mhuv2_channel_of(mbox_channel_id_t id)
{
	return id / MHU2_SLOTS_PER_CHANNEL;
}

static inline uint32_t mhuv2_slot_of(mbox_channel_id_t id)
{
	return id % MHU2_SLOTS_PER_CHANNEL;
}

/**
 * @brief MHUv2 combined receive interrupt handler.
 *
 * Iterates all hardware channels, checking stat_pend for pending doorbells.
 * Each pending slot's callback is invoked, then the bit is cleared.
 *
 * @param[in] dev MHUv2 device instance.
 */
static void mhuv2_isr(const struct device *dev)
{
	struct mhuv2_recv_reg *recv = MHUV2_RECV(dev);
	struct mhuv2_data *data = dev->data;

	for (uint32_t ch = 0; ch < data->num_channels; ch++) {
		volatile struct mhuv2_recv_channel_reg *recv_ch = &recv->channel[ch];
		uint32_t stat = recv_ch->stat_pend;

		while (stat != 0) {
			uint32_t slot = u32_count_trailing_zeros(stat);
			uint32_t flat_id = ch * MHU2_SLOTS_PER_CHANNEL + slot;

			if (flat_id < data->num_doorbells &&
			    data->cb[flat_id] != NULL) {
				data->cb[flat_id](dev, flat_id,
						  data->user_data[flat_id],
						  NULL);
			}

			recv_ch->stat_clear = BIT(slot);
			stat &= ~BIT(slot);
		}
	}
}

/**
 * @brief Send a doorbell to the remote processor.
 *
 * Performs the ACCESS_REQUEST / ACCESS_READY handshake, verifies the previous
 * doorbell on this slot has been consumed, writes STAT_SET, and releases access.
 *
 * @param[in] dev        MHUv2 device instance.
 * @param[in] channel_id Flattened doorbell ID (hw_channel * 32 + slot).
 * @param[in] msg        Must be NULL (signalling-only, no data payload).
 *
 * @retval 0         Doorbell sent successfully.
 * @retval -EINVAL   Invalid channel ID.
 * @retval -EMSGSIZE msg is non-NULL (data transfer not supported).
 * @retval -ETIMEDOUT ACCESS_READY not asserted within the timeout.
 * @retval -EBUSY    Another sender is in flight, or the previous doorbell
 *                   on this slot has not yet been cleared by the remote.
 */
static int mhuv2_send(const struct device *dev, mbox_channel_id_t channel_id,
		      const struct mbox_msg *msg)
{
	struct mhuv2_data *data = dev->data;
	uint32_t ch = mhuv2_channel_of(channel_id);
	uint32_t slot = mhuv2_slot_of(channel_id);

	if (channel_id >= data->num_doorbells) {
		return -EINVAL;
	}

	if (msg != NULL) {
		return -EMSGSIZE;
	}

	if (!atomic_cas(&data->send_busy, 0, 1)) {
		return -EBUSY;
	}

	struct mhuv2_send_reg *send = MHUV2_SEND(dev);

	send->access_request = 1;

	if (!WAIT_FOR(send->access_ready == 1, MHU2_ACCESS_READY_TIMEOUT_US,
		      k_busy_wait(1))) {
		send->access_request = 0;
		atomic_set(&data->send_busy, 0);
		LOG_ERR("ACCESS_READY timeout on hw channel %u slot %u", ch, slot);
		return -ETIMEDOUT;
	}

	if (send->channel[ch].stat & BIT(slot)) {
		send->access_request = 0;
		atomic_set(&data->send_busy, 0);
		return -EBUSY;
	}

	send->channel[ch].stat_set = BIT(slot);

	send->access_request = 0;
	atomic_set(&data->send_busy, 0);

	return 0;
}

/**
 * @brief Register a callback for incoming doorbells.
 *
 * @param[in] dev        MHUv2 device instance.
 * @param[in] channel_id Flattened doorbell ID.
 * @param[in] cb         Callback, or NULL to unregister.
 * @param[in] user_data  Opaque pointer passed to the callback.
 *
 * @retval 0       Success.
 * @retval -EINVAL Invalid channel ID.
 */
static int mhuv2_register_callback(const struct device *dev,
				   mbox_channel_id_t channel_id,
				   mbox_callback_t cb, void *user_data)
{
	struct mhuv2_data *data = dev->data;

	if (channel_id >= data->num_doorbells) {
		return -EINVAL;
	}

	data->cb[channel_id] = cb;
	data->user_data[channel_id] = user_data;

	return 0;
}

/**
 * @brief Enable or disable interrupt generation for a doorbell.
 *
 * Controls the per-slot hardware MASK register.
 *
 * @param[in] dev        MHUv2 device instance.
 * @param[in] channel_id Flattened doorbell ID.
 * @param[in] enabled    true to unmask (enable), false to mask (disable).
 *
 * @retval 0         Success.
 * @retval -EINVAL   Invalid channel ID.
 * @retval -EALREADY Slot is already in the requested state.
 */
static int mhuv2_set_enabled(const struct device *dev,
			     mbox_channel_id_t channel_id, bool enabled)
{
	struct mhuv2_data *data = dev->data;
	uint32_t ch = mhuv2_channel_of(channel_id);
	uint32_t slot = mhuv2_slot_of(channel_id);

	if (channel_id >= data->num_doorbells) {
		return -EINVAL;
	}

	volatile struct mhuv2_recv_channel_reg *recv_ch = &MHUV2_RECV(dev)->channel[ch];
	bool currently_enabled = !(recv_ch->mask & BIT(slot));

	if (enabled == currently_enabled) {
		return -EALREADY;
	}

	if (enabled) {
		recv_ch->mask_clear = BIT(slot);
	} else {
		recv_ch->mask_set = BIT(slot);
	}

	return 0;
}

/**
 * @brief Return the MTU for outbound messages.
 *
 * @param[in] dev MHUv2 device instance.
 *
 * @return 0, indicating signalling-only mode (no data payload).
 */
static int mhuv2_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/**
 * @brief Return the total number of available doorbells.
 *
 * @param[in] dev MHUv2 device instance.
 *
 * @return Total doorbells (hw_channels * 32).
 */
static uint32_t mhuv2_max_channels_get(const struct device *dev)
{
	struct mhuv2_data *data = dev->data;

	return data->num_doorbells;
}

/**
 * @brief Initialize the MHUv2 driver instance.
 *
 * Auto-detects the number of hardware channels from MIN(send, recv) of
 * MSG_NO_CAP, masks all receive slots on all channels, enables the combined
 * channel interrupt on MHUv2.1+ (via INT_EN.CHCOMB), then connects the IRQ.
 *
 * @param[in] dev MHUv2 device instance.
 *
 * @retval 0       Success.
 * @retval -EINVAL Hardware reports zero channels.
 */
static int mhuv2_init(const struct device *dev)
{
	struct mhuv2_data *data = dev->data;
	struct mhuv2_send_reg *send;
	struct mhuv2_recv_reg *recv;

	DEVICE_MMIO_NAMED_MAP(dev, send, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, recv, K_MEM_CACHE_NONE);

	send = MHUV2_SEND(dev);
	recv = MHUV2_RECV(dev);

	/* PBX and MBX channel counts may differ; use MIN to stay in bounds. */
	uint32_t num_channels = MIN(send->msg_no_cap, recv->msg_no_cap);

	if (num_channels == 0) {
		LOG_ERR("Hardware reports 0 channels");
		return -EINVAL;
	}

	if (num_channels > CONFIG_MBOX_MHUV2_NUM_CHANS) {
		num_channels = CONFIG_MBOX_MHUV2_NUM_CHANS;
	}

	data->num_channels = num_channels;
	data->num_doorbells = num_channels * MHU2_SLOTS_PER_CHANNEL;

	for (uint32_t ch = 0; ch < num_channels; ch++) {
		recv->channel[ch].mask_set = 0xFFFFFFFFU;
	}

	/*
	 * MHUv2.1 added INT_EN.CHCOMB, which gates the receiver-side combined
	 * channel interrupt and must be set by software. MHUv2.0 does not have
	 * this bit. Detect the revision via AIDR and enable CHCOMB on v2.1+.
	 */
	uint32_t arch_minor = recv->aidr & MHU2_AIDR_ARCH_MINOR_MASK;

	if (arch_minor >= 1) {
		recv->int_en |= MHU2_INT_EN_CHCOMB;
	}

	DEV_CFG(dev)->irq_config(dev);

	LOG_DBG("MHUv2 initialized: %u hw channels, %u doorbells",
		num_channels, data->num_doorbells);

	return 0;
}

static DEVICE_API(mbox, mhuv2_driver_api) = {
	.send = mhuv2_send,
	.register_callback = mhuv2_register_callback,
	.mtu_get = mhuv2_mtu_get,
	.max_channels_get = mhuv2_max_channels_get,
	.set_enabled = mhuv2_set_enabled,
};

#define MHUV2_INIT(n)								\
										\
static void mhuv2_irq_config_##n(const struct device *dev)			\
{										\
	IRQ_CONNECT(DT_INST_IRQN(n),						\
		    DT_INST_IRQ(n, priority),					\
		    mhuv2_isr,							\
		    DEVICE_DT_INST_GET(n),					\
		    0);								\
	irq_enable(DT_INST_IRQN(n));						\
}										\
										\
static const struct mhuv2_config mhuv2_cfg_##n = {				\
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(send, DT_DRV_INST(n)),		\
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(recv, DT_DRV_INST(n)),		\
	.irq_config = mhuv2_irq_config_##n,					\
};										\
										\
static struct mhuv2_data mhuv2_data_##n;					\
										\
DEVICE_DT_INST_DEFINE(n,							\
		      &mhuv2_init,						\
		      NULL,							\
		      &mhuv2_data_##n,						\
		      &mhuv2_cfg_##n,						\
		      POST_KERNEL,						\
		      CONFIG_MBOX_INIT_PRIORITY,				\
		      &mhuv2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MHUV2_INIT);
