/*
 * Copyright (c) 2025 Texas Instruments Incorporated.
 *
 * TI Secureproxy Mailbox driver for Zephyr's MBOX model.
 */

#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ti_secure_proxy);

#define DT_DRV_COMPAT ti_secure_proxy

#define DEV_CFG(dev)  ((const struct secproxy_mailbox_config *)((dev)->config))
#define DEV_DATA(dev) ((struct secproxy_mailbox_data *)(dev)->data)

#define DEV_TDATA(_dev) DEVICE_MMIO_NAMED_GET(_dev, target_data)
#define DEV_RT(_dev)    DEVICE_MMIO_NAMED_GET(_dev, rt)
#define DEV_SCFG(_dev)  DEVICE_MMIO_NAMED_GET(_dev, scfg)

#define RT_THREAD_STATUS               0x0
#define RT_THREAD_THRESHOLD            0x4
#define RT_THREAD_STATUS_ERROR_SHIFT   31
#define RT_THREAD_STATUS_ERROR_MASK    BIT(31)
#define RT_THREAD_STATUS_CUR_CNT_SHIFT 0
#define RT_THREAD_STATUS_CUR_CNT_MASK  GENMASK(7, 0)

#define SCFG_THREAD_CTRL           0x1000
#define SCFG_THREAD_CTRL_DIR_SHIFT 31
#define SCFG_THREAD_CTRL_DIR_MASK  BIT(31)

#define SEC_PROXY_THREAD(base, x) ((base) + (0x1000 * (x)))
#define THREAD_IS_RX              1
#define THREAD_IS_TX              0

#define SECPROXY_MAILBOX_NUM_MSGS 5
#define MAILBOX_MAX_CHANNELS      32
#define MAILBOX_MBOX_SIZE         60

#define SEC_PROXY_DATA_START_OFFS 0x4
#define SEC_PROXY_DATA_END_OFFS   0x3c

#define SEC_PROXY_TIMEOUT_US 1000000

#define GET_MSG_SEQ(buffer) ((uint32_t *)buffer)[1]
struct secproxy_thread {
	mem_addr_t target_data;
	mem_addr_t rt;
	mem_addr_t scfg;
};

/**
 * @struct rx_msg
 * @brief Received message details
 * @param seq: Message sequence number
 * @param size: Message size in bytes
 * @param buf: Buffer for message data
 */
struct rx_msg {
	uint8_t seq;
	size_t size;
	void *buf;
};

struct secproxy_mailbox_data {
	mbox_callback_t cb[MAILBOX_MAX_CHANNELS];
	void *user_data[MAILBOX_MAX_CHANNELS];
	bool channel_enable[MAILBOX_MAX_CHANNELS];

	DEVICE_MMIO_NAMED_RAM(target_data);
	DEVICE_MMIO_NAMED_RAM(rt);
	DEVICE_MMIO_NAMED_RAM(scfg);
	struct k_spinlock lock;
};

struct secproxy_mailbox_config {
	DEVICE_MMIO_NAMED_ROM(target_data);
	DEVICE_MMIO_NAMED_ROM(rt);
	DEVICE_MMIO_NAMED_ROM(scfg);
	uint32_t irq;
};

static inline int secproxy_verify_thread(struct secproxy_thread *spt, uint8_t dir)
{
	/* Check for any errors already available */
	if (sys_read32(spt->rt + RT_THREAD_STATUS) & RT_THREAD_STATUS_ERROR_MASK) {
		LOG_ERR("Thread is corrupted, cannot send data.\n");
		return -EINVAL;
	}

	/* Make sure thread is configured for right direction */
	if ((sys_read32(spt->scfg + SCFG_THREAD_CTRL) & SCFG_THREAD_CTRL_DIR_MASK) !=
	    (dir << SCFG_THREAD_CTRL_DIR_SHIFT)) {
		if (dir == THREAD_IS_TX) {
			LOG_ERR("Trying to send data on RX Thread\n");
		} else {
			LOG_ERR("Trying to receive data on TX Thread\n");
		}
		return -EINVAL;
	}

	/* Check the message queue before sending/receiving data */
	int timeout_ms = SEC_PROXY_TIMEOUT_US;
	int waited_ms = 0;
	const int poll_interval_ms = 1000;

	while (!(sys_read32(spt->rt + RT_THREAD_STATUS) & RT_THREAD_STATUS_CUR_CNT_MASK)) {
		k_busy_wait(poll_interval_ms);
		waited_ms += poll_interval_ms;
		if (waited_ms >= timeout_ms) {
			LOG_ERR("Timeout waiting for thread to %s\n",
				(dir == THREAD_IS_TX) ? "empty" : "fill");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void secproxy_mailbox_isr(const struct device *dev)
{
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	struct secproxy_thread spt;
	uint32_t data_word;
	mem_addr_t data_reg;

	for (int i_channel = 0; i_channel < MAILBOX_MAX_CHANNELS; i_channel++) {
		if (!data->channel_enable[i_channel]) {
			continue;
		}

		spt.target_data = SEC_PROXY_THREAD(DEV_TDATA(dev), i_channel);
		spt.rt = SEC_PROXY_THREAD(DEV_RT(dev), i_channel);
		spt.scfg = SEC_PROXY_THREAD(DEV_SCFG(dev), i_channel);

		uint32_t status = sys_read32(spt.rt + RT_THREAD_STATUS);

		if (status & RT_THREAD_STATUS_ERROR_MASK) {
			LOG_ERR("Thread %d error state detected in ISR", i_channel);
			continue;
		}

		if (secproxy_verify_thread(&spt, THREAD_IS_RX)) {
			LOG_ERR("Thread %d is in error state\n", i_channel);
			continue;
		}

		if (!(sys_read32(spt.rt) & 0x7F)) {
			continue;
		}

		data_reg = spt.target_data + SEC_PROXY_DATA_START_OFFS;
		size_t msg_len = MAILBOX_MBOX_SIZE;
		size_t num_words = msg_len / sizeof(uint32_t);
		size_t i;
		struct rx_msg *rx_data = data->user_data[i_channel];

		if (!rx_data || !rx_data->buf) {
			LOG_ERR("No buffer provided for channel %d", i_channel);
			continue;
		}

		if (rx_data->size < MAILBOX_MBOX_SIZE) {
			LOG_ERR("Buffer too small for channel %d", i_channel);
			continue;
		}

		uint8_t *buf = (uint8_t *)rx_data->buf;

		/* Copy full words */
		for (i = 0; i < num_words; i++) {
			data_word = sys_read32(data_reg);
			memcpy(&buf[i * 4], &data_word, sizeof(uint32_t));
			data_reg += sizeof(uint32_t);
		}

		/* Handle trail bytes */
		size_t trail_bytes = msg_len % sizeof(uint32_t);

		if (trail_bytes) {
			uint32_t data_trail = sys_read32(data_reg);

			i = msg_len - trail_bytes;

			while (trail_bytes--) {
				buf[i++] = data_trail & 0xff;
				data_trail >>= 8;
			}
		}

		/* Ensure we read the last register if we haven't already */
		if (data_reg <= (spt.target_data + SEC_PROXY_DATA_END_OFFS)) {
			sys_read32(spt.target_data + SEC_PROXY_DATA_END_OFFS);
		}

		rx_data->size = msg_len;
		rx_data->seq = GET_MSG_SEQ(buf);
		if (data->cb[i_channel]) {
			data->cb[i_channel](dev, i_channel, data->user_data[i_channel], NULL);
		}
	}
}

static int secproxy_mailbox_send(const struct device *dev, uint32_t channel,
				 const struct mbox_msg *msg)
{
	struct secproxy_thread spt;
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	mem_addr_t data_reg;
	k_spinlock_key_t key;

	if (!dev || !msg || !msg->data) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (msg->size == 0) {
		LOG_ERR("Empty message not allowed");
		return -EINVAL;
	}

	if (channel >= MAILBOX_MAX_CHANNELS) {
		LOG_ERR("Channel %d exceeds max channels", channel);
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	spt.target_data = SEC_PROXY_THREAD(DEV_TDATA(dev), channel);
	spt.rt = SEC_PROXY_THREAD(DEV_RT(dev), channel);
	spt.scfg = SEC_PROXY_THREAD(DEV_SCFG(dev), channel);

	if (secproxy_verify_thread(&spt, THREAD_IS_TX)) {
		LOG_ERR("Thread is in error state\n");
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	uint32_t status = sys_read32(spt.rt + RT_THREAD_STATUS);

	if ((status & RT_THREAD_STATUS_CUR_CNT_MASK) == (SECPROXY_MAILBOX_NUM_MSGS)) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if (msg->size > MAILBOX_MBOX_SIZE) {
		LOG_ERR("Message size %zu exceeds max size %d", msg->size, MAILBOX_MBOX_SIZE);
		k_spin_unlock(&data->lock, key);
		return -EMSGSIZE;
	}

	/* Start writing from the data start offset */
	data_reg = spt.target_data + SEC_PROXY_DATA_START_OFFS;

	/* Handle full words */
	const uint8_t *byte_data = msg->data;

	for (int num_words = msg->size / sizeof(uint32_t); num_words > 0; num_words--) {
		uint32_t word_val;

		memcpy(&word_val, byte_data, sizeof(uint32_t));
		sys_write32(word_val, data_reg);
		data_reg += sizeof(uint32_t);
		byte_data += sizeof(uint32_t);
	}

	/* Handle trailing bytes */
	int trail_bytes = msg->size % sizeof(uint32_t);

	if (trail_bytes) {
		uint32_t data_trail = 0;

		for (int i = 0; i < trail_bytes; i++) {
			data_trail <<= 8;
			data_trail |= byte_data[i];
		}
		sys_write32(data_trail, data_reg);
		data_reg += sizeof(uint32_t);
	}

	/* Only write end marker if needed */
	if (data_reg <= (spt.target_data + SEC_PROXY_DATA_END_OFFS)) {
		sys_write32(0, spt.target_data + SEC_PROXY_DATA_END_OFFS);
	}

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int secproxy_mailbox_register_callback(const struct device *dev, uint32_t channel,
					      mbox_callback_t cb, void *user_data)
{
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->cb[channel] = cb;
	data->user_data[channel] = user_data;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int secproxy_mailbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MBOX_SIZE;
}

static uint32_t secproxy_mailbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MAX_CHANNELS;
}

static int secproxy_mailbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	const struct secproxy_mailbox_config *cfg = DEV_CFG(dev);
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (enable && data->channel_enable[channel]) {
		return -EALREADY;
	}

	key = k_spin_lock(&data->lock);
	data->channel_enable[channel] = enable;

	if (enable) {
		irq_enable(cfg->irq);
	} else {
		irq_disable(cfg->irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static DEVICE_API(mbox, secproxy_mailbox_driver_api) = {
	.send = secproxy_mailbox_send,
	.register_callback = secproxy_mailbox_register_callback,
	.mtu_get = secproxy_mailbox_mtu_get,
	.max_channels_get = secproxy_mailbox_max_channels_get,
	.set_enabled = secproxy_mailbox_set_enabled,
};

#define MAILBOX_INSTANCE_DEFINE(idx)                                                               \
	static struct secproxy_mailbox_data secproxy_mailbox_##idx##_data;                         \
	const static struct secproxy_mailbox_config secproxy_mailbox_##idx##_config = {            \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(target_data, DT_DRV_INST(idx)),                 \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(rt, DT_DRV_INST(idx)),                          \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(scfg, DT_DRV_INST(idx)),                        \
		.irq = DT_INST_IRQN(idx),                                                          \
	};                                                                                         \
	static int secproxy_mailbox_##idx##_init(const struct device *dev)                         \
	{                                                                                          \
		DEVICE_MMIO_NAMED_MAP(dev, target_data, K_MEM_CACHE_NONE);                         \
		DEVICE_MMIO_NAMED_MAP(dev, rt, K_MEM_CACHE_NONE);                                  \
		DEVICE_MMIO_NAMED_MAP(dev, scfg, K_MEM_CACHE_NONE);                                \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), secproxy_mailbox_isr,   \
			    DEVICE_DT_INST_GET(idx),                                               \
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(idx, flags),			\
				(DT_INST_IRQ(idx, flags)), (0)));       \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, secproxy_mailbox_##idx##_init, NULL,                            \
			      &secproxy_mailbox_##idx##_data, &secproxy_mailbox_##idx##_config,    \
			      PRE_KERNEL_1, CONFIG_MBOX_TI_SECURE_PROXY_PRIORITY,                  \
			      &secproxy_mailbox_driver_api)

#define MAILBOX_INST(idx) MAILBOX_INSTANCE_DEFINE(idx);
DT_INST_FOREACH_STATUS_OKAY(MAILBOX_INST)
