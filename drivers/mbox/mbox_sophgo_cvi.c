/*
 * Copyright (c) 2024 honglin leng <a909204013@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <string.h>

#define DT_DRV_COMPAT sophgo_cvi_mailbox

#define GET_CPUx(base, cpu_num) (base + 0x10 + (0x10 * (cpu_num)))

#define MBOX_INT_CLR_OFFSET    0x00
#define MBOX_INT_DONE_OFFSET   0x08
#define MBOX_INT_TRIGER_OFFSET 0x60
#define MBOX_BUFFER_OFFSET     0x400

#define CPUx_INT_DONE(base, cpu)   (GET_CPUx(base, cpu) + MBOX_INT_DONE_OFFSET)
#define CPUx_INT_CLEAR(base, cpu)  (GET_CPUx(base, cpu) + MBOX_INT_CLR_OFFSET)
#define CPUx_INT_TRIGER(base, cpu) (base + MBOX_INT_TRIGER_OFFSET)
#define CPUx_BUFFER(base)          (base + MBOX_BUFFER_OFFSET)

#define CPUx_INT_ENABLE(base, cpu) (base + (cpu * 0x04))

#define MAILBOX_MAX_NUM 8

struct mbox_cvi_data {
	mbox_callback_t cb[MAILBOX_MAX_NUM];
	void *user_data[MAILBOX_MAX_NUM];
	const struct device *dev;
};

static struct mbox_cvi_data cvi_mbox_data;
static struct mbox_cvi_conf {
	mm_reg_t base;
	mm_reg_t mbox_buffer;
	uint32_t rx_cpu;
	uint32_t tx_cpu;
} cvi_mbox_conf = {
	.base = DT_INST_REG_ADDR(0),
	.mbox_buffer = DT_INST_REG_ADDR(0) + MBOX_BUFFER_OFFSET,
	.tx_cpu = DT_INST_PROP(0, tx_cpu),
	.rx_cpu = DT_INST_PROP(0, rx_cpu),
};

static void mbox_isr(const struct device *dev)
{
	uint8_t set_val;
	uint8_t valid_val;
	uint8_t tmp_valid_val;
	struct mbox_msg msg;
	struct mbox_cvi_data *data = dev->data;
	const struct mbox_cvi_conf *conf = dev->config;

	set_val = sys_read8(CPUx_INT_DONE(conf->base, conf->rx_cpu));
	if (set_val) {
		for (int i = 0; i < MAILBOX_MAX_NUM; i++) {
			valid_val = set_val & (1 << i);
			if (valid_val) {
				msg.data = (unsigned long *)conf->mbox_buffer + i;
				sys_write8(valid_val, CPUx_INT_CLEAR(conf->base, conf->rx_cpu));
				tmp_valid_val =
					sys_read8(CPUx_INT_ENABLE(conf->base, conf->rx_cpu));
				tmp_valid_val &= ~valid_val;
				sys_write8(tmp_valid_val,
					   CPUx_INT_ENABLE(conf->base, conf->rx_cpu));
				if (data->cb[i] != NULL) {
					data->cb[i](dev, i, data->user_data[i], &msg);
					*(unsigned long *)msg.data = 0x0;
				}
			}
		}
	}
}

static int mbox_cvi_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	uint8_t tmp_mbox_info;
	const struct mbox_cvi_conf *conf = dev->config;
	void *data = (unsigned long *)conf->mbox_buffer + channel;

	memcpy(data, msg->data, msg->size);
	sys_write8(1 << channel, CPUx_INT_CLEAR(conf->base, conf->tx_cpu));
	tmp_mbox_info = sys_read8(CPUx_INT_ENABLE(conf->base, conf->tx_cpu));
	tmp_mbox_info |= (1 << channel);
	sys_write8(tmp_mbox_info, CPUx_INT_ENABLE(conf->base, conf->tx_cpu));
	sys_write8(1 << channel, CPUx_INT_TRIGER(conf->base, conf->tx_cpu));
	return 0;
}

static int mbox_cvi_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	struct mbox_cvi_data *data = dev->data;

	if (channel >= MAILBOX_MAX_NUM) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int mbox_cvi_mtu_get(const struct device *dev)
{
	/* We only support signalling */
	return 0;
}

static uint32_t mbox_cvi_max_channels_get(const struct device *dev)
{
	return MAILBOX_MAX_NUM;
}

static int mbox_cvi_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	return 0;
}

static int mbox_cvi_init(const struct device *dev)
{
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

DEVICE_DT_INST_DEFINE(0, mbox_cvi_init, NULL, &cvi_mbox_data, &cvi_mbox_conf, POST_KERNEL,
		      CONFIG_MBOX_INIT_PRIORITY, &mbox_cvi_driver_api);
