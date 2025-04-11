/*
 * Copyright (c) 2025 Yoan Dumas.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2711_mailbox

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_bcm2711, CONFIG_MBOX_LOG_LEVEL);

/**
 * From the RPi documentation:
 * Mailbox registers:
 * +---------+------------+------+--------+--------+--------+
 * | Mailbox | Read/Write | Peek | Sender | Status | Config |
 * |---------+------------+------+--------+--------+--------|
 * | 0       | 0x00       | 0x10 | 0x14   | 0x18   | 0x1c   |
 * | 1       | 0x20       | 0x30 | 0x34   | 0x38   | 0x3c   |
 * +---------+------------+------+--------+--------+--------+
 */
#define MBOX_READ_OFFSET         0x00
#define MBOX_WRITE_OFFSET        0x20
#define MBOX_READ_STATUS_OFFSET  0x18
#define MBOX_WRITE_STATUS_OFFSET 0x38

#define MBOX_BASE_ADDR_REG(dev) \
	(((const struct bcm2711_mbox_data *)dev->data)->base_addr)
#define MBOX_READ_STATUS_REGISTER(dev) \
	((uintptr_t)(MBOX_BASE_ADDR_REG(dev) + MBOX_READ_STATUS_OFFSET))
#define MBOX_WRITE_STATUS_REGISTER(dev) \
	((uintptr_t)(MBOX_BASE_ADDR_REG(dev) + MBOX_WRITE_STATUS_OFFSET))
#define MBOX_READ_REGISTER(dev) \
	((uintptr_t)(MBOX_BASE_ADDR_REG(dev) + MBOX_READ_OFFSET))
#define MBOX_WRITE_REGISTER(dev) \
	((uintptr_t)(MBOX_BASE_ADDR_REG(dev) + MBOX_WRITE_OFFSET))

#define MAIL_FULL  0x80000000
#define MAIL_EMPTY 0x40000000

struct bcm2711_mbox_cfg {
	DEVICE_MMIO_ROM;
};

struct bcm2711_mbox_data {
	DEVICE_MMIO_RAM;
	mem_addr_t base_addr;

	volatile uint32_t *pt;
};

volatile uint32_t *get_mbox_buffer(const struct device *dev)
{
	struct bcm2711_mbox_data *data = dev->data;

	return data->pt;
}

uint32_t bcm2711_mbox_read(const struct device *dev, mbox_channel_id_t channel)
{
	uint32_t value = 0;

	while ((value & 0xF) != channel) {
		while (sys_read32(MBOX_READ_STATUS_REGISTER(dev)) & MAIL_EMPTY) {
			k_busy_wait(1);
		}
		value = sys_read32(MBOX_READ_REGISTER(dev));
		LOG_DBG("Read: 0x%08X", value);
	}

	return value & 0xFFFFFFF0;
}

static int bcm2711_mbox_send(const struct device *dev, mbox_channel_id_t channel,
			     const struct mbox_msg *msg)
{
	uint32_t message;

	__ASSERT(msg, "Given msg is NULL");
	__ASSERT(msg->data, "Given msg->data is NULL");
	if (msg->size > 4) {
		LOG_ERR("Message size is too large: %zu", msg->size);
		return -EMSGSIZE;
	}

	while (sys_read32(MBOX_WRITE_STATUS_REGISTER(dev)) & MAIL_FULL) {
		k_busy_wait(1);
	}

	message = (uint32_t)((uintptr_t)msg->data);
	LOG_DBG("Message to send 0x%08X", message);
	message |= (channel & 0xF);

	LOG_DBG("Write 0x%08X to the mailbox (@ 0x%08lX)", message, MBOX_WRITE_REGISTER(dev));
	sys_write32(message, MBOX_WRITE_REGISTER(dev));

	return 0;
}

static int bcm2711_mbox_register_callback(const struct device *dev, mbox_channel_id_t channel,
					  mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(channel);
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return 0;
}

static int bcm2711_mbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 4;
}

/** From the RPi documentation:
 * Channels:
 * 0: Power Management
 * 1: Framebuffer
 * 2: Virtual UART
 * 3: VCHIQ
 * 4: LEDs
 * 5: Buttons
 * 6: Touchscreen
 * 7:
 * 8: Property Tags (ARM to VC)
 * 9: Property Tags (VC to ARM)
 */
static uint32_t bcm2711_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 10;
}

static int bcm2711_mbox_set_enabled(const struct device *dev, mbox_channel_id_t channel,
				    bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(enable);

	return 0;
}

static DEVICE_API(mbox, bcm2711_mbox_driver_api) = {
	.send = bcm2711_mbox_send,
	.register_callback = bcm2711_mbox_register_callback,
	.mtu_get = bcm2711_mbox_mtu_get,
	.max_channels_get = bcm2711_mbox_max_channels_get,
	.set_enabled = bcm2711_mbox_set_enabled,
};

#define BCM2711_MBOX_INIT(idx)                                                                     \
	static struct bcm2711_mbox_data bcm2711_mbox_data_##idx = {                                \
		.base_addr = 0,                                                                    \
		.pt = NULL,                                                                        \
	};                                                                                         \
	static const struct bcm2711_mbox_cfg bcm2711_mbox_cfg_##idx = {                            \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(idx)),                                            \
	};                                                                                         \
	static int bcm2711_mbox_##idx##_init(const struct device *dev)                             \
	{                                                                                          \
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);                                            \
		struct bcm2711_mbox_data *data = dev->data;                                        \
		data->base_addr = DEVICE_MMIO_GET(dev);                                            \
		uintptr_t shared_mem = DT_REG_ADDR(DT_ALIAS(armvcsdram));                          \
		size_t size = DT_REG_SIZE(DT_ALIAS(armvcsdram));                                   \
		extern void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags); \
		arch_mem_map((void *)shared_mem, shared_mem, size,                                 \
			     K_MEM_CACHE_NONE | K_MEM_PERM_RW);                                    \
		data->pt = (volatile uint32_t *)shared_mem;                                        \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, &bcm2711_mbox_##idx##_init, NULL, &bcm2711_mbox_data_##idx,     \
			      &bcm2711_mbox_cfg_##idx, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bcm2711_mbox_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BCM2711_MBOX_INIT);
