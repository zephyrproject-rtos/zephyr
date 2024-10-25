/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 */

#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
LOG_MODULE_REGISTER(mbox_ipc_w91);

#define DT_DRV_COMPAT telink_mbox_ipc_w91

/************************************************************************
 * MBOX controller registers structure definition
 ************************************************************************/

#define MBOX_W91_ENABLE_OFFSET                                 0x00002000
#define MBOX_W91_ENABLE_SHIFT_PER_TARGET                       7
#define MBOX_W91_PENDING_OFFSET                                0x00001000
#define MBOX_W91_CLAIM_OFFSET                                  0x00200004
#define MBOX_W91_CLAIM_SHIFT_PER_TARGET                        12
#define MBOX_W91_NUM_ISR_TARGET_OFFSET                         0x00001100

/************************************************************************
 * MBOX controller utilities
 ************************************************************************/

ALWAYS_INLINE
static inline uint32_t mbox_w91_num_isr_target(uintptr_t base_addr)
{
	volatile uint32_t *num_isr_target = (volatile uint32_t *)(base_addr +
		MBOX_W91_NUM_ISR_TARGET_OFFSET);

	return  *num_isr_target;
}

ALWAYS_INLINE
static bool mbox_w91_is_enabled_interrupt(uintptr_t base_addr, uint32_t source)
{
	volatile uint32_t *current_ptr = (volatile uint32_t *)(base_addr +
		MBOX_W91_ENABLE_OFFSET + (csr_read(mhartid) << MBOX_W91_ENABLE_SHIFT_PER_TARGET) +
		((source >> 5) << 2));
	uint32_t current = *current_ptr;

	return (bool)(current & (1 << (source & 0x1f)));
}

ALWAYS_INLINE
static void mbox_w91_enable_interrupt(uintptr_t base_addr, uint32_t source, bool enable)
{
	volatile uint32_t *current_ptr = (volatile uint32_t *)(base_addr +
		MBOX_W91_ENABLE_OFFSET + (csr_read(mhartid) << MBOX_W91_ENABLE_SHIFT_PER_TARGET) +
		((source >> 5) << 2));
	uint32_t current = *current_ptr;

	if (enable) {
		current = current | (1 << (source & 0x1f));
	} else {
		current = current & ~((1 << (source & 0x1f)));
	}
	*current_ptr = current;
}

ALWAYS_INLINE
static void mbox_w91_set_pending(uintptr_t base_addr, uint32_t source)
{
	volatile uint32_t *current_ptr = (volatile uint32_t *)(base_addr +
		MBOX_W91_PENDING_OFFSET + ((source >> 5) << 2));

	*current_ptr = (1 << (source & 0x1f));
}

ALWAYS_INLINE
static uint32_t mbox_w91_claim_interrupt(uintptr_t base_addr)
{
	volatile uint32_t *claim_addr = (volatile uint32_t *)(base_addr +
		MBOX_W91_CLAIM_OFFSET + (csr_read(mhartid) << MBOX_W91_CLAIM_SHIFT_PER_TARGET));

	return  *claim_addr;
}

ALWAYS_INLINE
static void mbox_w91_complete_interrupt(uintptr_t base_addr, uint32_t source)
{
	volatile uint32_t *claim_addr = (volatile uint32_t *)(base_addr +
		MBOX_W91_CLAIM_OFFSET + (csr_read(mhartid) << MBOX_W91_CLAIM_SHIFT_PER_TARGET));

	*claim_addr = source;
}

/************************************************************************
 * MBOX data structures
 ************************************************************************/
struct mbox_w91_channel {
	mbox_callback_t             callback;
	void                       *callback_data;
};

struct mbox_w91_data {
	size_t                      num_channels;
	struct mbox_w91_channel    *channel;
};

struct mbox_w91_config {
	uintptr_t                   base_addr;
};

/************************************************************************
 * MBOX driver APIs
 ************************************************************************/

static int mbox_w91_send(const struct device *dev, uint32_t channel,
	const struct mbox_msg *msg)
{
	struct mbox_w91_data *mbox_data = dev->data;

	if (channel >= mbox_data->num_channels) {
		return -EINVAL;
	}

	if (msg) {
		LOG_WRN("Sending data not supported");
		return -ENOTSUP;
	}

	const struct mbox_w91_config *config = dev->config;

	mbox_w91_set_pending(config->base_addr, channel + 1);

	return 0;
}

static int mbox_w91_register_callback(const struct device *dev, uint32_t channel,
	mbox_callback_t cb, void *user_data)
{
	struct mbox_w91_data *mbox_data = dev->data;

	if (channel >= mbox_data->num_channels) {
		return -EINVAL;
	}

	mbox_data->channel[channel].callback = cb;
	mbox_data->channel[channel].callback_data = user_data;

	return 0;
}

static int mbox_w91_mtu_get(const struct device *dev)
{
	/* Only support signalling */
	return 0;
}

static uint32_t mbox_w91_max_channels_get(const struct device *dev)
{
	struct mbox_w91_data *mbox_data = dev->data;

	return mbox_data->num_channels;
}

static int mbox_w91_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	const struct mbox_w91_config *config = dev->config;
	struct mbox_w91_data *mbox_data = dev->data;

	if (channel >= mbox_data->num_channels) {
		return -EINVAL;
	}

	bool isr_now = mbox_w91_is_enabled_interrupt(config->base_addr, channel + 1);

	if (enable == isr_now) {
		return -EALREADY;
	}

	if (enable && !mbox_data->channel[channel].callback) {
		LOG_WRN("Enabling channel without a registered callback");
	}

	mbox_w91_enable_interrupt(config->base_addr, channel + 1, enable);

	return 0;
}

static int mbox_w91_init(const struct device *dev)
{
	const struct mbox_w91_config *config = dev->config;
	struct mbox_w91_data *mbox_data = dev->data;

	size_t num_channels = mbox_w91_num_isr_target(config->base_addr) & 0xffff;

	if (!num_channels) {
		return -EIO;
	}

	for (size_t i = 0; i < num_channels; i++) {
		mbox_w91_enable_interrupt(config->base_addr, i + 1, false);
	}

	mbox_data->channel = k_malloc(sizeof(struct mbox_w91_channel) * num_channels);

	if (mbox_data->channel) {
		for (size_t i = 0; i < num_channels; i++) {
			mbox_data->channel[i].callback = NULL;
			mbox_data->channel[i].callback_data = NULL;
		}
		compiler_barrier();
		mbox_data->num_channels = num_channels;
	} else {
		return -ENOMEM;
	}

	return 0;
}

static void mbox_w91_isr(const struct device *dev)
{
	const struct mbox_w91_config *config = dev->config;
	uint32_t irq_num = mbox_w91_claim_interrupt(config->base_addr);

	if (irq_num) {
		uint32_t channel = irq_num - 1;
		struct mbox_w91_data *mbox_data = dev->data;

		if (channel < mbox_data->num_channels && mbox_data->channel[channel].callback) {
			mbox_data->channel[channel].callback(
				dev, channel, mbox_data->channel[channel].callback_data, NULL);
		}
		mbox_w91_complete_interrupt(config->base_addr, irq_num);
	}
}

/************************************************************************
 * Device instance declaration
 ************************************************************************/

static const struct mbox_driver_api mbox_w91_api = {
	.send              = mbox_w91_send,
	.register_callback = mbox_w91_register_callback,
	.mtu_get           = mbox_w91_mtu_get,
	.max_channels_get  = mbox_w91_max_channels_get,
	.set_enabled       = mbox_w91_set_enabled,
};

#define MBOX_W91_INIT(n)                                               \
                                                                       \
	static struct mbox_w91_data mbox_data_##n;                         \
                                                                       \
	static const struct mbox_w91_config mbox_config_##n = {            \
		.base_addr = DT_INST_REG_ADDR(n),                              \
	};                                                                 \
                                                                       \
	DEVICE_DT_INST_DEFINE(n, mbox_w91_init,                            \
		NULL,                                                          \
		&mbox_data_##n,                                                \
		&mbox_config_##n,                                              \
		POST_KERNEL,                                                   \
		CONFIG_MBOX_INIT_PRIORITY,                                     \
		&mbox_w91_api);

DT_INST_FOREACH_STATUS_OKAY(MBOX_W91_INIT)

#define MBOX_W91_IRQ(n)           mbox_w91_isr(DEVICE_DT_INST_GET(n));

/************************************************************************
 * Common section
 ************************************************************************/

static void mbox_w91_common_isr(void)
{
	DT_INST_FOREACH_STATUS_OKAY(MBOX_W91_IRQ)
}

static int mbox_w91_common_init(void)
{
	IRQ_CONNECT(RISCV_IRQ_MSOFT, 0, mbox_w91_common_isr, NULL, 0);
	irq_enable(RISCV_IRQ_MSOFT);

	return 0;
}

SYS_INIT(mbox_w91_common_init, PRE_KERNEL_1, 10);
