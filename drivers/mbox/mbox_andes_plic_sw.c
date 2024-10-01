/*
 * Copyright (c) 2022 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
LOG_MODULE_REGISTER(mbox_andes_plic_sw);

#define DT_DRV_COMPAT	andestech_plic_sw

#define IRQ_REG(n) (n >> 5)
#define PLIC_BASE(dev) \
	((const struct mbox_andes_conf * const)(dev)->config)->base

#define REG_PRIORITY(dev, irq) \
	(PLIC_BASE(dev) + 0x0 + (irq << 2))
#define REG_PENDING(dev, irq) \
	(PLIC_BASE(dev) + 0x1000 + (IRQ_REG(irq) << 2))
#define REG_ENABLE(dev, hart, irq) \
	(PLIC_BASE(dev) + 0x2000 + (hart << 7) + IRQ_REG(irq))
#define REG_CLAIM(dev, hart) \
	(PLIC_BASE(dev) + 0x200004 + (hart << 12))

#define IPI_NUM DT_INST_PROP(0, channel_max)

static struct mbox_andes_data {
	mbox_callback_t cb[IPI_NUM];
	void *user_data[IPI_NUM];
	uint32_t enabled_channel[CONFIG_MP_MAX_NUM_CPUS];
#ifdef CONFIG_SCHED_IPI_SUPPORTED
	uint32_t reg_cb_channel;
	uint32_t ipi_channel;
#endif
} andes_mbox_data;

static struct mbox_andes_conf {
	uint32_t base;
	uint32_t channel_max;
} andes_mbox_conf = {
	.base = DT_INST_REG_ADDR(0),
	.channel_max = IPI_NUM,
};

static struct k_spinlock mbox_syn;

static void plic_sw_irq_set_pending(const struct device *dev, uint32_t irq)
{
	uint32_t pend;
	k_spinlock_key_t key = k_spin_lock(&mbox_syn);

	pend = sys_read32(REG_PENDING(dev, irq));
	pend |= BIT(irq);
	sys_write32(pend, REG_PENDING(dev, irq));

	k_spin_unlock(&mbox_syn, key);
}

static inline bool is_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_andes_conf *conf = dev->config;

	return (ch <= conf->channel_max);
}

static int mbox_andes_send(const struct device *dev, uint32_t ch,
			   const struct mbox_msg *msg)
{
	if (msg) {
		LOG_WRN("Sending data not supported");
	}

	if (!is_channel_valid(dev, ch)) {
		return -EINVAL;
	}

	/* Send IPI by triggering the pending register of PLIC SW. */
	plic_sw_irq_set_pending(dev, ch + 1);

	return 0;
}

static int mbox_andes_register_callback(const struct device *dev, uint32_t ch,
					mbox_callback_t cb, void *user_data)
{
	struct mbox_andes_data *data = dev->data;
	const struct mbox_andes_conf *conf = dev->config;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&mbox_syn);

	if (ch > conf->channel_max) {
		ret = -EINVAL;
		goto out;
	}

#ifdef CONFIG_SCHED_IPI_SUPPORTED
	if (ch & data->ipi_channel & data->reg_cb_channel) {
		ret = -EALREADY;
		goto out;
	}

	data->reg_cb_channel |= BIT(ch);
#endif
	data->cb[ch] = cb;
	data->user_data[ch] = user_data;

out:
	k_spin_unlock(&mbox_syn, key);

	return 0;
}

static int mbox_andes_mtu_get(const struct device *dev)
{
	/* We only support signalling */
	return 0;
}

static uint32_t mbox_andes_max_channels_get(const struct device *dev)
{
	const struct mbox_andes_conf *conf = dev->config;

	return conf->channel_max;
}

static int mbox_andes_set_enabled(const struct device *dev, uint32_t ch,
				  bool enable)
{
	uint32_t en, is_enabled_ch, hartid, cpu_id, irq;
	struct mbox_andes_data *data = dev->data;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&mbox_syn);

	if (!is_channel_valid(dev, ch)) {
		ret = -EINVAL;
		goto out;
	}

	irq = ch + 1;
	hartid = arch_proc_id();
	cpu_id = _current_cpu->id;

	is_enabled_ch = data->enabled_channel[cpu_id] & BIT(ch);

	if ((!enable && !is_enabled_ch) || (enable && is_enabled_ch)) {
		ret = -EALREADY;
		goto out;
	}

	if (enable && !(data->cb[ch])) {
		LOG_WRN("Enabling channel without a registered callback\n");
	}

	en = sys_read32(REG_ENABLE(dev, hartid, irq));

	if (enable) {
		data->enabled_channel[cpu_id] |= BIT(ch);
		sys_write32(1, REG_PRIORITY(dev, irq));
		en |= BIT(irq);
	} else {
		data->enabled_channel[cpu_id] &= ~BIT(ch);
		en &= ~BIT(irq);
	}

	sys_write32(en, REG_ENABLE(dev, hartid, irq));
out:
	k_spin_unlock(&mbox_syn, key);

	return ret;
}

static void andes_plic_sw_irq_handler(const struct device *dev)
{
	struct mbox_andes_data *data = dev->data;
	uint32_t irq, ch, hartid;

	hartid = arch_proc_id();

	/* PLIC claim: Get the SW IRQ number generating the interrupt. */
	irq = sys_read32(REG_CLAIM(dev, hartid));
	ch = irq - 1;

	if (irq) {
		sys_write32(irq, REG_CLAIM(dev, hartid));

		if (data->cb[ch]) {
			/* Only one MAILBOX, id is unused and set to 0 */
			data->cb[ch](dev, ch, data->user_data[ch], NULL);
		}
	}
}

static int mbox_andes_init(const struct device *dev)
{
	/* Setup IRQ handler for PLIC SW driver */
	IRQ_CONNECT(RISCV_IRQ_MSOFT, 1,
		    andes_plic_sw_irq_handler, DEVICE_DT_INST_GET(0), 0);

#ifndef CONFIG_SMP
	irq_enable(RISCV_IRQ_MSOFT);
#endif
	return 0;
}

static const struct mbox_driver_api mbox_andes_driver_api = {
	.send = mbox_andes_send,
	.register_callback = mbox_andes_register_callback,
	.mtu_get = mbox_andes_mtu_get,
	.max_channels_get = mbox_andes_max_channels_get,
	.set_enabled = mbox_andes_set_enabled,
};

DEVICE_DT_INST_DEFINE(0, mbox_andes_init, NULL, &andes_mbox_data,
		      &andes_mbox_conf, PRE_KERNEL_1, CONFIG_MBOX_INIT_PRIORITY,
		      &mbox_andes_driver_api);
