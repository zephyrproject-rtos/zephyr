/*
 * Copyright (c) 2025 Texas Instruments Incorporated.
 *
 * SPDX-License-Identifier: Apache-2.0
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

#define MAILBOX_MAX_CHANNELS 32
#define MAILBOX_MBOX_SIZE    60

#define SEC_PROXY_DATA_START_OFFS 0x4
#define SEC_PROXY_DATA_END_OFFS   0x3c

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
	uint32_t interrupts[MAILBOX_MAX_CHANNELS];
};

static inline int secproxy_verify_thread(struct secproxy_thread *spt, uint8_t dir)
{
	/* Check for any errors already available */
	uint32_t status = sys_read32(spt->rt + RT_THREAD_STATUS);

	if (status & RT_THREAD_STATUS_ERROR_MASK) {
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

	if ((status & RT_THREAD_STATUS_CUR_CNT_MASK) == 0) {
		if (dir == THREAD_IS_TX) {
			return -EBUSY;
		} else {
			return -ENODATA;
		}
	}

	return 0;
}

static int secproxy_mailbox_receive(const struct device *dev, uint32_t channel)
{
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	struct secproxy_thread spt;
	uint32_t data_word;
	mem_addr_t data_reg;

	spt.target_data = SEC_PROXY_THREAD(DEV_TDATA(dev), channel);
	spt.rt = SEC_PROXY_THREAD(DEV_RT(dev), channel);
	spt.scfg = SEC_PROXY_THREAD(DEV_SCFG(dev), channel);

	if (secproxy_verify_thread(&spt, THREAD_IS_RX)) {
		/* Silent failure */
		return -EIO;
	}

	data_reg = spt.target_data + SEC_PROXY_DATA_START_OFFS;
	size_t msg_len = MAILBOX_MBOX_SIZE;
	size_t num_words = msg_len / sizeof(uint32_t);
	size_t i;
	struct rx_msg *rx_data = data->user_data[channel];

	if (!rx_data || !rx_data->buf) {
		LOG_ERR("No buffer provided for channel %d", channel);
		return -EINVAL;
	}

	if (rx_data->size < MAILBOX_MBOX_SIZE) {
		LOG_ERR("Buffer too small for channel %d", channel);
		return -ENOBUFS;
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
	if (data->cb[channel]) {
		data->cb[channel](dev, channel, data->user_data[channel], NULL);
	}

	return 0;
}

static void secproxy_mailbox_isr(const struct device *dev, uint32_t channel)
{
	struct secproxy_mailbox_data *data = DEV_DATA(dev);

	if (!data->channel_enable[channel]) {
		return;
	}

	(void)secproxy_mailbox_receive(dev, channel);
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

static void secproxy_mailbox_flush_thread(const struct device *dev, uint32_t channel)
{
	struct secproxy_thread spt;

	spt.target_data = SEC_PROXY_THREAD(DEV_TDATA(dev), channel);
	spt.rt = SEC_PROXY_THREAD(DEV_RT(dev), channel);
	spt.scfg = SEC_PROXY_THREAD(DEV_SCFG(dev), channel);

	/* Drain all pending messages from the thread. Blocking call */
	while ((sys_read32(spt.rt + RT_THREAD_STATUS) & RT_THREAD_STATUS_CUR_CNT_MASK) > 0) {
		/* Read from the last data register to consume one message */
		(void)sys_read32(spt.target_data + SEC_PROXY_DATA_END_OFFS);
	}
}

static int secproxy_mailbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	const struct secproxy_mailbox_config *config = DEV_CFG(dev);
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
		secproxy_mailbox_flush_thread(dev, channel);
		irq_enable(config->interrupts[channel]);
	} else {
		irq_disable(config->interrupts[channel]);
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

int secproxy_mailbox_send_then_poll_rx(const struct device *dev, uint32_t rx_channel,
					  uint32_t tx_channel, const struct mbox_msg *tx_msg)
{
	const struct secproxy_mailbox_config *cfg = DEV_CFG(dev);
	mem_addr_t rx_rt = SEC_PROXY_THREAD(DEV_RT(dev), rx_channel);
	int rv;

	irq_disable(cfg->interrupts[rx_channel]);

	rv = secproxy_mailbox_send(dev, tx_channel, tx_msg);
	if (rv != 0) {
		LOG_ERR("failed to send message on channel %u", tx_channel);
		return rv;
	}

	/* blocking call */
	while (!(sys_read32(rx_rt + RT_THREAD_STATUS) & RT_THREAD_STATUS_CUR_CNT_MASK)) {
	}

	rv = secproxy_mailbox_receive(dev, rx_channel);
	if (rv != 0) {
		LOG_ERR("failed to receive message on channel %u", tx_channel);
		return rv;
	}

	irq_enable(cfg->interrupts[rx_channel]);

	return 0;
}

#define SECPROXY_THREAD_ISR(i, idx)                                                                \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(idx, DT_CAT(rx_, i)),                                     \
		(	\
			static void secproxy_mailbox_isr_##idx##_##i(const struct device *dev)     \
			{	\
				secproxy_mailbox_isr(dev, i);	\
			}	\
		), \
		())

/* Generate IRQ number or 0 for array initialization */
#define SECPROXY_IRQ_OR_ZERO(i, inst)                                                              \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, DT_CAT(rx_, i)),	\
		(DT_INST_IRQ_BY_NAME(inst, DT_CAT(rx_, i), irq)),	\
		(0))

#define SECPROXY_IRQ_CONNECT(i, inst) \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, DT_CAT(rx_, i)), \
		( \
			IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, DT_CAT(rx_, i), irq), \
				DT_INST_IRQ_BY_NAME(inst, DT_CAT(rx_, i), priority), \
				secproxy_mailbox_isr_##inst##_##i, \
				DEVICE_DT_INST_GET(inst), \
				COND_CODE_1(DT_INST_IRQ_HAS_CELL(inst, flags), \
					(DT_INST_IRQ_BY_NAME(inst, DT_CAT(rx_, i), flags)),	\
					(0))); \
		), \
		())

#define MAILBOX_INSTANCE_DEFINE(idx) \
	LISTIFY(MAILBOX_MAX_CHANNELS, SECPROXY_THREAD_ISR, (), idx)	\
	static struct secproxy_mailbox_data secproxy_mailbox_##idx##_data; \
	const static struct secproxy_mailbox_config secproxy_mailbox_##idx##_config = { \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(target_data, DT_DRV_INST(idx)), \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(rt, DT_DRV_INST(idx)), \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(scfg, DT_DRV_INST(idx)), \
		.interrupts = {LISTIFY(MAILBOX_MAX_CHANNELS, SECPROXY_IRQ_OR_ZERO, (,), idx) }}; \
	static int secproxy_mailbox_##idx##_init(const struct device *dev) \
	{ \
		DEVICE_MMIO_NAMED_MAP(dev, target_data, K_MEM_CACHE_NONE); \
		DEVICE_MMIO_NAMED_MAP(dev, rt, K_MEM_CACHE_NONE); \
		DEVICE_MMIO_NAMED_MAP(dev, scfg, K_MEM_CACHE_NONE); \
		LISTIFY(MAILBOX_MAX_CHANNELS, SECPROXY_IRQ_CONNECT, (;), idx)	\
		return 0; \
	} \
	DEVICE_DT_INST_DEFINE(idx, secproxy_mailbox_##idx##_init, NULL,                            \
			      &secproxy_mailbox_##idx##_data, &secproxy_mailbox_##idx##_config,    \
			      PRE_KERNEL_1, CONFIG_MBOX_TI_SECURE_PROXY_PRIORITY,                  \
			      &secproxy_mailbox_driver_api)

#define MAILBOX_INST(idx) MAILBOX_INSTANCE_DEFINE(idx);
DT_INST_FOREACH_STATUS_OKAY(MAILBOX_INST)
