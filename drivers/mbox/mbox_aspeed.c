/*
 * Copyright (c) 2026 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast2700_mailbox

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define MBOX_TRIGGER  0x00U
#define MBOX_ENABLE   0x04U
#define MBOX_STATUS   0x08U
#define MBOX_DATA     0x10U
#define MBOX_CHANNELS 4U

#define MBOX_CHANNEL_MASK BIT_MASK(MBOX_CHANNELS)
#define MBOX_MAX_WORDS    8U

#define AST2700_MBOX_MSG_SIZE 32U

struct aspeed_mbox_soc {
	uint8_t msg_size;
};

struct aspeed_mbox_config {
	uintptr_t tx;
	uintptr_t rx;
	const struct aspeed_mbox_soc *soc;
	void (*irq_config)(void);
};

struct aspeed_mbox_data {
	struct k_spinlock lock;
	mbox_callback_t callback[MBOX_CHANNELS];
	void *user_data[MBOX_CHANNELS];
	uint32_t enabled_mask;
	uint32_t rx_buffer[MBOX_MAX_WORDS];
};

static const struct aspeed_mbox_soc aspeed_mbox_ast2700 = {
	.msg_size = AST2700_MBOX_MSG_SIZE,
};

/* Instances without an interrupt are served by one polling thread. */
#define ASPEED_MBOX_POLLED_ONE(n) + COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (0), (1))
#define ASPEED_MBOX_NUM_POLLED    (0 DT_INST_FOREACH_STATUS_OKAY(ASPEED_MBOX_POLLED_ONE))

static int aspeed_mbox_send(const struct device *dev, mbox_channel_id_t channel,
			    const struct mbox_msg *msg)
{
	const struct aspeed_mbox_config *config = dev->config;
	struct aspeed_mbox_data *data = dev->data;
	uint32_t words[MBOX_MAX_WORDS] = {0U};
	k_spinlock_key_t key;

	if (channel >= MBOX_CHANNELS) {
		return -EINVAL;
	}

	if (msg != NULL) {
		if (msg->size > config->soc->msg_size) {
			return -EMSGSIZE;
		}
		if (msg->size > 0U) {
			if (msg->data == NULL) {
				return -EINVAL;
			}
			memcpy(words, msg->data, msg->size);
		}
	}

	key = k_spin_lock(&data->lock);
	if ((sys_read32(config->tx + MBOX_STATUS) & BIT(channel)) != 0U) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if (msg != NULL) {
		uintptr_t addr = config->tx + MBOX_DATA + config->soc->msg_size * channel;

		for (size_t i = 0U; i < DIV_ROUND_UP(msg->size, sizeof(uint32_t)); i++) {
			sys_write32(words[i], addr + i * sizeof(uint32_t));
		}
	}

	sys_write32(BIT(channel), config->tx + MBOX_TRIGGER);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int aspeed_mbox_register_callback(const struct device *dev, mbox_channel_id_t channel,
					 mbox_callback_t callback, void *user_data)
{
	struct aspeed_mbox_data *data = dev->data;
	k_spinlock_key_t key;

	if (channel >= MBOX_CHANNELS) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->callback[channel] = callback;
	data->user_data[channel] = user_data;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int aspeed_mbox_mtu_get(const struct device *dev)
{
	const struct aspeed_mbox_config *config = dev->config;

	return config->soc->msg_size;
}

static uint32_t aspeed_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MBOX_CHANNELS;
}

/*
 * Deliver every pending message on the enabled channels. The RX status of a
 * channel is cleared after its callback returns, so the peer sees the channel
 * busy until the message has been handled.
 */
static void aspeed_mbox_receive(const struct device *dev)
{
	const struct aspeed_mbox_config *config = dev->config;
	struct aspeed_mbox_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t status = 0U;

	if (data->enabled_mask != 0U) {
		status = sys_read32(config->rx + MBOX_STATUS) & data->enabled_mask;
	}
	k_spin_unlock(&data->lock, key);

	for (mbox_channel_id_t channel = 0U; channel < MBOX_CHANNELS; channel++) {
		struct mbox_msg msg = {
			.data = data->rx_buffer,
			.size = config->soc->msg_size,
		};
		mbox_callback_t callback;
		void *user_data;
		uintptr_t addr;

		if ((status & BIT(channel)) == 0U) {
			continue;
		}

		key = k_spin_lock(&data->lock);
		callback = data->callback[channel];
		user_data = data->user_data[channel];
		addr = config->rx + MBOX_DATA + config->soc->msg_size * channel;
		for (size_t i = 0U; i < config->soc->msg_size / sizeof(uint32_t); i++) {
			data->rx_buffer[i] = sys_read32(addr + i * sizeof(uint32_t));
		}
		k_spin_unlock(&data->lock, key);

		if (callback != NULL) {
			callback(dev, channel, user_data, &msg);
		}
		sys_write32(BIT(channel), config->rx + MBOX_STATUS);
	}
}

static void __maybe_unused aspeed_mbox_isr(const void *arg)
{
	aspeed_mbox_receive(arg);
}

#if ASPEED_MBOX_NUM_POLLED > 0
static K_SEM_DEFINE(aspeed_mbox_wake, 0U, 1U);
static atomic_t aspeed_mbox_polled_channels;
#endif

static int aspeed_mbox_set_enabled(const struct device *dev, mbox_channel_id_t channel,
				   bool enable)
{
	const struct aspeed_mbox_config *config = dev->config;
	struct aspeed_mbox_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t reg;

	if (channel >= MBOX_CHANNELS) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (((data->enabled_mask & BIT(channel)) != 0U) == enable) {
		k_spin_unlock(&data->lock, key);
		return -EALREADY;
	}

	reg = sys_read32(config->rx + MBOX_ENABLE);
	if (enable) {
		reg |= BIT(channel);
		data->enabled_mask |= BIT(channel);
	} else {
		reg &= ~BIT(channel);
		data->enabled_mask &= ~BIT(channel);
	}
	sys_write32(reg, config->rx + MBOX_ENABLE);
	k_spin_unlock(&data->lock, key);

#if ASPEED_MBOX_NUM_POLLED > 0
	if (config->irq_config == NULL) {
		if (enable) {
			atomic_inc(&aspeed_mbox_polled_channels);
		} else {
			atomic_dec(&aspeed_mbox_polled_channels);
		}
		k_sem_give(&aspeed_mbox_wake);
	}
#endif

	return 0;
}

static int aspeed_mbox_init(const struct device *dev)
{
	const struct aspeed_mbox_config *config = dev->config;

	sys_write32(0U, config->rx + MBOX_ENABLE);
	sys_write32(MBOX_CHANNEL_MASK, config->rx + MBOX_STATUS);

	if (config->irq_config != NULL) {
		config->irq_config();
	}

	return 0;
}

static DEVICE_API(mbox, aspeed_mbox_driver_api) = {
	.send = aspeed_mbox_send,
	.register_callback = aspeed_mbox_register_callback,
	.mtu_get = aspeed_mbox_mtu_get,
	.max_channels_get = aspeed_mbox_max_channels_get,
	.set_enabled = aspeed_mbox_set_enabled,
};

#define ASPEED_MBOX_IRQ_CONFIG(n)                                                                  \
	static void aspeed_mbox_irq_config_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), aspeed_mbox_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define ASPEED_MBOX_DEFINE(n, soc_data)                                                            \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (ASPEED_MBOX_IRQ_CONFIG(n)))                        \
	static const struct aspeed_mbox_config aspeed_mbox_config_##n = {                          \
		.tx = DT_INST_REG_ADDR_BY_NAME(n, tx),                                             \
		.rx = DT_INST_REG_ADDR_BY_NAME(n, rx),                                             \
		.soc = &(soc_data),                                                                \
		.irq_config = COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0),                               \
					  (aspeed_mbox_irq_config_##n), (NULL)),                   \
	};                                                                                         \
	static struct aspeed_mbox_data aspeed_mbox_data_##n;                                       \
	DEVICE_DT_INST_DEFINE(n, aspeed_mbox_init, NULL, &aspeed_mbox_data_##n,                    \
			      &aspeed_mbox_config_##n, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,     \
			      &aspeed_mbox_driver_api);

DT_INST_FOREACH_STATUS_OKAY_VARGS(ASPEED_MBOX_DEFINE, aspeed_mbox_ast2700)

#if ASPEED_MBOX_NUM_POLLED > 0
#define ASPEED_MBOX_POLLED_DEVICE(n)                                                               \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (), (DEVICE_DT_INST_GET(n),))

static const struct device *const aspeed_mbox_polled_devices[] = {
	DT_INST_FOREACH_STATUS_OKAY(ASPEED_MBOX_POLLED_DEVICE)};

static void aspeed_mbox_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_timeout_t timeout = K_FOREVER;

		if (atomic_get(&aspeed_mbox_polled_channels) != 0) {
			for (size_t i = 0U; i < ARRAY_SIZE(aspeed_mbox_polled_devices); i++) {
				aspeed_mbox_receive(aspeed_mbox_polled_devices[i]);
			}
			timeout = K_MSEC(CONFIG_MBOX_ASPEED_POLL_INTERVAL_MS);
		}

		(void)k_sem_take(&aspeed_mbox_wake, timeout);
	}
}

K_THREAD_DEFINE(aspeed_mbox_poll_thread, CONFIG_MBOX_ASPEED_THREAD_STACK_SIZE,
		aspeed_mbox_thread, NULL, NULL, NULL, CONFIG_MBOX_ASPEED_THREAD_PRIORITY, 0U, 0);
#endif
