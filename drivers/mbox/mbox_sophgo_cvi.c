/*
 * Copyright (c) 2024 honglin leng <a909204013@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <string.h>

#define DT_DRV_COMPAT sophgo_cvi_mailbox

#define MBOX_BASE   DT_INST_REG_ADDR(0)
#define MBOX_TX_CPU DT_INST_PROP(0, tx_cpu)
#define MBOX_RX_CPU DT_INST_PROP(0, rx_cpu)

#define MBOX_INT_ENABLE(cpu) (MBOX_BASE + 0x00 + (0x04 * cpu))
#define MBOX_INT_CLEAR(cpu)  (MBOX_BASE + 0x10 + (0x10 * cpu))
#define MBOX_INT_DONE(cpu)   (MBOX_BASE + 0x18 + (0x10 * cpu))
#define MBOX_INT_TRIGER      (MBOX_BASE + 0x60)
#define MBOX_BUFFER          (MBOX_BASE + 0x400)

#define MBOX_MAX_NUM 8

struct mbox_cvi_data {
	mbox_callback_t cb[MBOX_MAX_NUM];
	void *user_data[MBOX_MAX_NUM];
};

static struct mbox_cvi_data mbox_data;

static void mbox_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint8_t set_val;
	uint8_t valid_val;
	uint8_t tmp_valid_val;
	struct mbox_msg msg;

	set_val = sys_read8(MBOX_INT_DONE(MBOX_RX_CPU));
	if (set_val) {
		for (int i = 0; i < MBOX_MAX_NUM; i++) {
			valid_val = set_val & BIT(i);
			if (valid_val) {
				msg.data = (const unsigned long *)MBOX_BUFFER + i;
				sys_write8(valid_val, MBOX_INT_CLEAR(MBOX_RX_CPU));
				tmp_valid_val = sys_read8(MBOX_INT_ENABLE(MBOX_RX_CPU));
				tmp_valid_val &= ~valid_val;
				sys_write8(tmp_valid_val, MBOX_INT_ENABLE(MBOX_RX_CPU));
				if (mbox_data.cb[i] != NULL) {
					mbox_data.cb[i](dev, i, mbox_data.user_data[i], &msg);
					*(unsigned long *)msg.data = 0x0;
				}
			}
		}
	}
}

static int mbox_cvi_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	uint8_t tmp_mbox_info;

	ARG_UNUSED(dev);

	memcpy((unsigned long *)MBOX_BUFFER + channel, msg->data, msg->size);
	sys_write8(BIT(channel), MBOX_INT_CLEAR(MBOX_TX_CPU));
	tmp_mbox_info = sys_read8(MBOX_INT_ENABLE(MBOX_TX_CPU));
	tmp_mbox_info |= BIT(channel);
	sys_write8(tmp_mbox_info, MBOX_INT_ENABLE(MBOX_TX_CPU));
	sys_write8(BIT(channel), MBOX_INT_TRIGER);

	return 0;
}

static int mbox_cvi_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(dev);

	if (channel >= MBOX_MAX_NUM) {
		return -EINVAL;
	}

	mbox_data.cb[channel] = cb;
	mbox_data.user_data[channel] = user_data;

	return 0;
}

static int mbox_cvi_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* We only support signalling */
	return 0;
}

static uint32_t mbox_cvi_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MBOX_MAX_NUM;
}

static int mbox_cvi_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(enable);

	return 0;
}

static int mbox_cvi_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mbox_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static const struct mbox_driver_api mbox_cvi_driver_api = {
	.send = mbox_cvi_send,
	.register_callback = mbox_cvi_register_callback,
	.mtu_get = mbox_cvi_mtu_get,
	.max_channels_get = mbox_cvi_max_channels_get,
	.set_enabled = mbox_cvi_set_enabled,
};

DEVICE_DT_INST_DEFINE(0, mbox_cvi_init, NULL, NULL, NULL, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,
		      &mbox_cvi_driver_api);
