/*
 * Copyright (c) 2025 Yoan Dumas.
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Mailboxes Reference:
 *    https://github.com/raspberrypi/firmware/wiki/Mailboxes
 *
 */

#define DT_DRV_COMPAT brcm_bcm2711_mbox

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mbox.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_bcm2711, CONFIG_MBOX_LOG_LEVEL);

#define MBOX0 0x00
#define MBOX1 0x20

#define MBOX_RW     0x00
#define MBOX_STATUS 0x18
#define MBOX_CONFIG 0x1C
#define MBOX_EMPTY  BIT(30)
#define MBOX_FULL   BIT(31)

#define MBOX0_READ   (MBOX0 + MBOX_RW)
#define MBOX0_STATUS (MBOX0 + MBOX_STATUS)
#define MBOX0_CONFIG (MBOX0 + MBOX_CONFIG)

#define MBOX1_WRITE  (MBOX1 + MBOX_RW)
#define MBOX1_STATUS (MBOX1 + MBOX_STATUS)
#define MBOX1_CONFIG (MBOX1 + MBOX_CONFIG)

#define MBOX_DATA(data)       (data & ~0xF)
#define MBOX_CHANNEL(channel) (channel & 0xF)
#define MBOX_CHANNEL_MAX      10UL

#define MBOX_IRQ_DISABLE 0x00
#define MBOX_IRQ_ENABLE  0x01

#define DEV_CFG(dev)  ((const struct bcm2711_mbox_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct bcm2711_mbox_data *const)(dev)->data)

struct bcm2711_mbox_config {
	mem_addr_t shm;
	size_t shm_size;
	uint32_t irq;
	void (*irq_config_func)(const struct device *dev);

	DEVICE_MMIO_NAMED_ROM(mbox);
};

struct bcm2711_mbox_data {
	mem_addr_t shm;
	mbox_callback_t cb[MBOX_CHANNEL_MAX];
	void *user_data[MBOX_CHANNEL_MAX];
	struct k_spinlock lock;

	DEVICE_MMIO_NAMED_RAM(mbox);
};

static inline uint32_t bcm2711_mbox_rd(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, mbox) + offset);
}

static inline void bcm2711_mbox_wr(const struct device *dev, uint32_t offset, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, mbox) + offset);
}

static void bcm2711_mbox_isr(const struct device *dev)
{
	struct bcm2711_mbox_data *data = DEV_DATA(dev);
	const struct bcm2711_mbox_config *config = DEV_CFG(dev);
	struct mbox_msg rx_msg;
	uint32_t message;
	uint8_t channel;

	while ((bcm2711_mbox_rd(dev, MBOX0_STATUS) & MBOX_EMPTY) == 0) {
		message = bcm2711_mbox_rd(dev, MBOX0_READ);
		channel = MBOX_CHANNEL(message);

		LOG_DBG("virt_addr: 0x%08lx, phy_addr: 0x%08x, ch: %d", data->shm,
			MBOX_DATA(message), channel);

		if (data->cb[channel] != NULL) {
			rx_msg.data = (uint32_t *)data->shm;
			rx_msg.size = config->shm_size;
			data->cb[channel](dev, channel, data->user_data[channel], &rx_msg);
		}
	}
}

static int bcm2711_mbox_send(const struct device *dev, mbox_channel_id_t channel,
			     const struct mbox_msg *msg)
{
	const struct bcm2711_mbox_config *config = DEV_CFG(dev);
	struct bcm2711_mbox_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;
	uint32_t message;

	if (msg == NULL || msg->size == 0) {
		return -ENOMSG;
	}

	if (msg->size > config->shm_size) {
		return -ENOMEM;
	}

	if (channel >= MBOX_CHANNEL_MAX) {
		return -EINVAL;
	}

	while (bcm2711_mbox_rd(dev, MBOX1_STATUS) & MBOX_FULL) {
		k_busy_wait(1);
	}

	/* The mailbox interface has 28 bits (MSB) available for the
	 * value (i.e. buffer address which must be 16 bytes aligned)
	 * and 4 bits (LSB) for the channel.
	 *
	 * All shared memory addresses exchanged via the mailbox must be
	 * physical bus addresses. The VideoCore has no knowledge of the
	 * virtual address space.
	 */

	key = k_spin_lock(&data->lock);

	memcpy((void *)data->shm, msg->data, msg->size);
	message = MBOX_DATA(config->shm) | MBOX_CHANNEL(channel);
	LOG_DBG("virt_addr: 0x%08lx, phy_addr: 0x%08x, ch: %d", data->shm, MBOX_DATA(message),
		channel);
	bcm2711_mbox_wr(dev, MBOX1_WRITE, message);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int bcm2711_mbox_register_callback(const struct device *dev, mbox_channel_id_t channel,
					  mbox_callback_t cb, void *user_data)
{
	struct bcm2711_mbox_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	if (channel >= MBOX_CHANNEL_MAX) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->cb[channel] = cb;
	data->user_data[channel] = user_data;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int bcm2711_mbox_mtu_get(const struct device *dev)
{
	const struct bcm2711_mbox_config *config = DEV_CFG(dev);

	return config->shm_size;
}

static uint32_t bcm2711_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MBOX_CHANNEL_MAX;
}

static int bcm2711_mbox_set_enabled(const struct device *dev, mbox_channel_id_t channel,
				    bool enable)
{
	ARG_UNUSED(channel);

	const struct bcm2711_mbox_config *config = DEV_CFG(dev);
	struct bcm2711_mbox_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (enable) {
		bcm2711_mbox_wr(dev, MBOX0_CONFIG, MBOX_IRQ_ENABLE);
		irq_enable(config->irq);
	} else {
		bcm2711_mbox_wr(dev, MBOX0_CONFIG, MBOX_IRQ_DISABLE);
		irq_disable(config->irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int bcm2711_mbox_init(const struct device *dev)
{
	const struct bcm2711_mbox_config *config = DEV_CFG(dev);
	struct bcm2711_mbox_data *data = DEV_DATA(dev);

	DEVICE_MMIO_NAMED_MAP(dev, mbox, K_MEM_CACHE_NONE);

	device_map(&data->shm, config->shm, config->shm_size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);

	bcm2711_mbox_wr(dev, MBOX0_CONFIG, MBOX_IRQ_DISABLE);

	config->irq_config_func(dev);

	LOG_DBG("Mailbox initialized");

	return 0;
}

static DEVICE_API(mbox, bcm2711_mbox_driver_api) = {
	.send = bcm2711_mbox_send,
	.register_callback = bcm2711_mbox_register_callback,
	.mtu_get = bcm2711_mbox_mtu_get,
	.max_channels_get = bcm2711_mbox_max_channels_get,
	.set_enabled = bcm2711_mbox_set_enabled,
};

#define BCM2711_MBOX_IRQ_CONF_FUNC(n)                                                              \
	static void irq_config_func_##n(const struct device *dev)                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), bcm2711_mbox_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}

#define BCM2711_MBOX_DEV_DATA(n) static struct bcm2711_mbox_data bcm2711_mbox_data_##n;

#define BCM2711_MBOX_DEV_CFG(n)                                                                    \
	static const struct bcm2711_mbox_config bcm2711_mbox_config_##n = {                        \
		DEVICE_MMIO_NAMED_ROM_INIT(mbox, DT_DRV_INST(n)),                                  \
		.shm_size = DT_REG_SIZE(DT_PHANDLE(DT_DRV_INST(n), shared_memory)),                \
		.shm = DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(n), shared_memory)),                     \
		.irq = DT_INST_IRQN(n),                                                            \
		.irq_config_func = irq_config_func_##n,                                            \
	};

#define BCM2711_MBOX_INIT(n)                                                                       \
	DEVICE_DT_INST_DEFINE(n, bcm2711_mbox_init, NULL, &bcm2711_mbox_data_##n,                  \
			      &bcm2711_mbox_config_##n, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,    \
			      &bcm2711_mbox_driver_api);

#define BCM2711_MBOX_INSTANTIATE(inst)                                                             \
	BCM2711_MBOX_IRQ_CONF_FUNC(inst);                                                          \
	BCM2711_MBOX_DEV_DATA(inst);                                                               \
	BCM2711_MBOX_DEV_CFG(inst);                                                                \
	BCM2711_MBOX_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(BCM2711_MBOX_INSTANTIATE)
