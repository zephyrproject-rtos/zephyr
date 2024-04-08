/*
 * Copyright (c) 2024 Texas Instruments Incorporated.
 *
 * TI OMAP Mailbox driver for Zephyr's MBOX model.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ti_omap_mailbox);

#define DT_DRV_COMPAT ti_omap_mailbox

#define MAILBOX_IRQ_NEWMSG(m)  (1 << (2 * (m)))
#define MAILBOX_IRQ_NOTFULL(m) (1 << (2 * (m) + 1))
#define OMAP_MAILBOX_NUM_MSGS  16
#define MAILBOX_MAX_CHANNELS   16
#define OMAP_MAILBOX_NUM_USERS 4
#define MAILBOX_MBOX_SIZE      sizeof(uint32_t)

#define DEV_CFG(_dev)     ((const struct omap_mailbox_config *)(_dev)->config)
#define DEV_DATA(_dev)    ((struct omap_mailbox_data *)(_dev)->data)
#define DEV_REG_BASE(dev) ((struct omap_mailbox_regs *)DEVICE_MMIO_NAMED_GET(dev, reg_base))

struct omap_mailbox_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	mbox_callback_t cb[MAILBOX_MAX_CHANNELS];
	void *user_data[MAILBOX_MAX_CHANNELS];
	bool channel_enable[MAILBOX_MAX_CHANNELS];
	uint32_t received_data;
	struct k_spinlock lock;
};

struct omap_mailbox_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t irq;
	uint32_t usr_id;
};

struct omap_mailbox_irq_regs {
	uint32_t status_raw;
	uint32_t status_clear;
	uint32_t enable_set;
	uint32_t enable_clear;
};

struct omap_mailbox_regs {
	uint32_t revision;
	uint32_t __pad0[3];
	uint32_t sysconfig;
	uint32_t __pad1[11];
	uint32_t message[OMAP_MAILBOX_NUM_MSGS];
	uint32_t fifo_status[OMAP_MAILBOX_NUM_MSGS];
	uint32_t msg_status[OMAP_MAILBOX_NUM_MSGS];
	struct omap_mailbox_irq_regs irq_regs[OMAP_MAILBOX_NUM_USERS];
};

static void omap_mailbox_isr(const struct device *dev)
{
	volatile struct omap_mailbox_regs *regs = DEV_REG_BASE(dev);
	const struct omap_mailbox_config *cfg = DEV_CFG(dev);
	struct omap_mailbox_data *data = DEV_DATA(dev);

	uint32_t irq_enabled = regs->irq_regs[cfg->usr_id].enable_set;
	uint32_t flags = regs->irq_regs[cfg->usr_id].status_clear;

	regs->irq_regs[cfg->usr_id].enable_set = 0;

	for (int i_channel = 0; i_channel < MAILBOX_MAX_CHANNELS; i_channel++) {
		if (!data->channel_enable[i_channel]) {
			continue;
		}

		if ((flags & MAILBOX_IRQ_NEWMSG(i_channel))) {
			data->received_data = regs->message[i_channel];
			struct mbox_msg msg = {
				.data = (const void *)&data->received_data,
				.size = MAILBOX_MBOX_SIZE,
			};

			if (data->cb[i_channel]) {
				data->cb[i_channel](dev, i_channel, data->user_data[i_channel],
						    &msg);
			}
		}
	}

	regs->irq_regs[cfg->usr_id].status_clear = flags;
	regs->irq_regs[cfg->usr_id].enable_set = irq_enabled;
}

static int omap_mailbox_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	uint32_t __aligned(4) data32;

	volatile struct omap_mailbox_regs *regs = DEV_REG_BASE(dev);
	struct omap_mailbox_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (regs->fifo_status[channel]) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->lock);
	if (!msg) {
		regs->message[channel] = 0;
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	if (msg->size > MAILBOX_MBOX_SIZE) {
		k_spin_unlock(&data->lock, key);
		return -EMSGSIZE;
	}

	memcpy(&data32, msg->data, msg->size);
	regs->message[channel] = data32;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int omap_mailbox_register_callback(const struct device *dev, uint32_t channel,
					  mbox_callback_t cb, void *user_data)
{
	struct omap_mailbox_data *data = DEV_DATA(dev);
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

static int omap_mailbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MBOX_SIZE;
}

static uint32_t omap_mailbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MAX_CHANNELS;
}

static int omap_mailbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	const struct omap_mailbox_config *cfg = DEV_CFG(dev);
	struct omap_mailbox_data *data = DEV_DATA(dev);
	volatile struct omap_mailbox_regs *regs;
	k_spinlock_key_t key;
	uint32_t irqstatus;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (enable && data->channel_enable[channel]) {
		return -EALREADY;
	}

	key = k_spin_lock(&data->lock);
	regs = DEV_REG_BASE(dev);
	irqstatus = regs->irq_regs[cfg->usr_id].enable_set;

	if (enable) {
		irqstatus |= MAILBOX_IRQ_NEWMSG(channel);
	} else {
		irqstatus &= ~MAILBOX_IRQ_NEWMSG(channel);
	}

	regs->irq_regs[cfg->usr_id].enable_set = irqstatus;
	data->channel_enable[channel] = enable;

	if (enable) {
		irq_enable(cfg->irq);
	} else {
		irq_disable(cfg->irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static const struct mbox_driver_api omap_mailbox_driver_api = {
	.send = omap_mailbox_send,
	.register_callback = omap_mailbox_register_callback,
	.mtu_get = omap_mailbox_mtu_get,
	.max_channels_get = omap_mailbox_max_channels_get,
	.set_enabled = omap_mailbox_set_enabled,
};

#define MAILBOX_INSTANCE_DEFINE(idx)								\
	static struct omap_mailbox_data omap_mailbox_##idx##_data;				\
	const static struct omap_mailbox_config omap_mailbox_##idx##_config = {			\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(idx)),				\
		.irq = DT_INST_IRQN(idx),							\
		.usr_id = DT_INST_PROP(idx, usr_id),						\
	};											\
	static int omap_mailbox_##idx##_init(const struct device *dev)				\
	{											\
		DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);				\
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), omap_mailbox_isr,	\
			    DEVICE_DT_INST_GET(idx),						\
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(idx, flags),			\
					(DT_INST_IRQ(idx, flags)), (0)));			\
		return 0;									\
	}											\
	DEVICE_DT_INST_DEFINE(idx, omap_mailbox_##idx##_init, NULL, &omap_mailbox_##idx##_data,	\
			      &omap_mailbox_##idx##_config, POST_KERNEL,	\
			      CONFIG_MBOX_INIT_PRIORITY, &omap_mailbox_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MAILBOX_INSTANCE_DEFINE)
