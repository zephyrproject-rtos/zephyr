/*
 * Copyright (c) 2025 Texas Instruments Incorporated.
 *
 * TI Secureproxy Mailbox driver for Zephyr's MBOX model.
 */

#include <zephyr/spinlock.h>
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

struct secproxy_thread {
	mem_addr_t target_data;
	mem_addr_t rt;
	mem_addr_t scfg;
	char rx_data[MAILBOX_MBOX_SIZE * 4];
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

	return 0;
}

static void secproxy_mailbox_isr(const struct device *dev)
{
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	struct secproxy_thread spt;

	for (int i_channel = 0; i_channel < MAILBOX_MAX_CHANNELS; i_channel++) {
		if (!data->channel_enable[i_channel]) {
			continue;
		}

		spt.target_data = SEC_PROXY_THREAD(DEV_TDATA(dev), i_channel);
		spt.rt = SEC_PROXY_THREAD(DEV_RT(dev), i_channel);
		spt.scfg = SEC_PROXY_THREAD(DEV_SCFG(dev), i_channel);

		if (secproxy_verify_thread(&spt, THREAD_IS_RX)) {
			LOG_ERR("Thread %d is in error state\n", i_channel);
			continue;
		}

		/* check if there are messages */
		if (!(sys_read32(spt.rt) & 0x7F)) {
			continue;
		}

		for (int i = 4; i <= 0x3C; i += 4) {
			*((uint32_t *)(spt.rx_data + i - 4)) = sys_read32(spt.target_data + i);
		}

		struct mbox_msg msg = {(const void *)spt.rx_data, MAILBOX_MBOX_SIZE};

		if (data->cb[i_channel]) {
			data->cb[i_channel](dev, i_channel, data->user_data[i_channel], &msg);
		}
	}
}

static int secproxy_mailbox_send(const struct device *dev, uint32_t channel,
				 const struct mbox_msg *msg)
{
	struct secproxy_thread spt;
	struct secproxy_mailbox_data *data = DEV_DATA(dev);
	int num_words, trail_bytes;
	const uint32_t *word_data;
	uint32_t data_trail;
	mem_addr_t data_reg;
	k_spinlock_key_t key;

	if (channel >= MAILBOX_MAX_CHANNELS) {
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

	if (msg->size > MAILBOX_MBOX_SIZE) {
		k_spin_unlock(&data->lock, key);
		return -EMSGSIZE;
	}

	data_reg = spt.target_data + 4;
	for (num_words = msg->size / sizeof(uint32_t), word_data = (const uint32_t *)msg->data;
	     num_words; num_words--, data_reg += sizeof(uint32_t), word_data++) {
		sys_write32(*word_data, data_reg);
	}

	trail_bytes = msg->size % sizeof(uint32_t);
	if (trail_bytes) {
		data_trail = *word_data;

		data_trail &= 0xFFFFFFFFU >> (8 * (sizeof(uint32_t) - trail_bytes));
		sys_write32(data_trail, data_reg);
		data_reg += sizeof(uint32_t);
	}

	size_t remaining_size = (spt.target_data + 0x3C) - data_reg + sizeof(uint32_t);

	memset((void *)data_reg, 0, remaining_size);
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
