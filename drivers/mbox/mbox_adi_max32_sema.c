/*
 * Copyright (c) 2025 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_mbox_max32_sema

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_max32_rv32.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_adi_max32_sema, CONFIG_MBOX_LOG_LEVEL);

typedef struct {
	uint32_t semaphores[8]; /**< <tt>\b 0x00:</tt> SEMA SEMAPHORES Register */
	uint32_t rsv_0x20_0x3f[8];
	uint32_t irq0;  /**< <tt>\b 0x40:</tt> SEMA IRQ0 Register */
	uint32_t mail0; /**< <tt>\b 0x44:</tt> SEMA MAIL0 Register */
	uint32_t irq1;  /**< <tt>\b 0x48:</tt> SEMA IRQ1 Register */
	uint32_t mail1; /**< <tt>\b 0x4C:</tt> SEMA MAIL1 Register */
	uint32_t rsv_0x50_0xff[44];
	uint32_t status; /**< <tt>\b 0x100:</tt> SEMA STATUS Register */
} mxc_sema_reva_regs_t;

#define MAX32_SEMA_IRQ_EN_BIT  0
#define MAX32_SEMA_IRQ_SET_BIT 16

struct mbox_adi_max32_sema_config {
	mxc_sema_reva_regs_t *regs;
	void (*irq_func)(void);
	uint8_t irq_num;
	const struct device *clock;
	struct max32_perclk perclk;
	bool msg_payloads;
};

struct mbox_adi_max32_sema_data {
	mbox_callback_t cb;
	void *cb_user_data;
};

static inline mem_addr_t get_tx_irq_reg(const struct device *dev)
{
	const struct mbox_adi_max32_sema_config *cfg = dev->config;

	return IS_ENABLED(CONFIG_SOC_FAMILY_MAX32_RV32) ? (mem_addr_t)&cfg->regs->irq0
							: (mem_addr_t)&cfg->regs->irq1;
}

static inline mem_addr_t get_rx_irq_reg(const struct device *dev)
{
	const struct mbox_adi_max32_sema_config *cfg = dev->config;

	return IS_ENABLED(CONFIG_SOC_FAMILY_MAX32_RV32) ? (mem_addr_t)&cfg->regs->irq1
							: (mem_addr_t)&cfg->regs->irq0;
}

static void mbox_adi_max32_sema_isr(const struct device *dev)
{
	const struct mbox_adi_max32_sema_config *cfg = dev->config;
	struct mbox_adi_max32_sema_data *data = dev->data;
	uint32_t payload;
	struct mbox_msg mbox_data = {.size = 4, .data = &payload};
	struct mbox_msg *mbox_data_arg = NULL;
	mem_addr_t irq_reg = get_rx_irq_reg(dev);

	if (data->cb) {
		if (cfg->msg_payloads) {
			payload = sys_read32(irq_reg + 4);
			mbox_data_arg = &mbox_data;
		}

		data->cb(dev, 0, data->cb_user_data, mbox_data_arg);
	} else {
		LOG_WRN("IRQ received with no callback set");
	}

	/* Clear the IRQ */
	sys_clear_bit(irq_reg, MAX32_SEMA_IRQ_SET_BIT);

#if defined(CONFIG_SOC_FAMILY_MAX32_RV32)
	/* Avoid RV32 interrupt controller firing the ISR a superfluous time */
	intc_max32_rv32_irq_clear_pending(cfg->irq_num);
#endif
}

static uint32_t mbox_adi_max32_sema_max_channels_get(const struct device *dev)
{
	/* One bi-directional channel is supported */
	return 1;
}

static int mbox_adi_max32_sema_register_callback(const struct device *dev,
						 mbox_channel_id_t channel_id, mbox_callback_t cb,
						 void *user_data)
{
	struct mbox_adi_max32_sema_data *data = dev->data;

	if (channel_id != 0) {
		LOG_ERR("Only one channel (0) is supported.");
		return -EINVAL;
	}

	data->cb = cb;
	data->cb_user_data = user_data;

	return 0;
}

static int mbox_adi_max32_sema_set_enabled(const struct device *dev, mbox_channel_id_t channel_id,
					   bool enabled)
{
	const struct mbox_adi_max32_sema_config *cfg = dev->config;
	mem_addr_t irq_reg = get_rx_irq_reg(dev);

	if (channel_id != 0) {
		LOG_ERR("Only one channel (0) is supported.");
		return -EINVAL;
	}

	if (enabled) {
		if (sys_test_bit(irq_reg, MAX32_SEMA_IRQ_EN_BIT) != 0) {
			return -EALREADY;
		}

		irq_enable(cfg->irq_num);
		sys_set_bit(irq_reg, MAX32_SEMA_IRQ_EN_BIT);
	} else {
		if (sys_test_bit(irq_reg, MAX32_SEMA_IRQ_EN_BIT) == 0) {
			return -EALREADY;
		}

		irq_disable(cfg->irq_num);
		sys_clear_bit(irq_reg, MAX32_SEMA_IRQ_EN_BIT);
	}

	return 0;
}

static int mbox_adi_max32_sema_mtu_get(const struct device *dev)
{
	const struct mbox_adi_max32_sema_config *cfg = dev->config;

	return cfg->msg_payloads ? 4 : 0;
}

static int mbox_adi_max32_sema_send(const struct device *dev, mbox_channel_id_t channel_id,
				    const struct mbox_msg *msg)
{
	const struct mbox_adi_max32_sema_config *cfg = dev->config;
	mem_addr_t irq_reg = get_tx_irq_reg(dev);
	uint32_t data;

	if (channel_id != 0) {
		LOG_ERR("Only one channel (0) is supported.");
		return -EINVAL;
	}

	if (cfg->msg_payloads && msg) {
		if (msg->size != 4) {
			LOG_ERR("Invalid payload size %d", msg->size);
			return -EINVAL;
		}

		memcpy(&data, msg->data, 4);

		sys_write32(data, irq_reg + 4);
	}

	sys_set_bit(irq_reg, MAX32_SEMA_IRQ_SET_BIT);

	return 0;
}

static int mbox_adi_max32_init(const struct device *dev)
{
	int ret = 0;
	const struct mbox_adi_max32_sema_config *cfg = dev->config;

	if (cfg->clock != NULL) {
		/* enable clock */
		ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
		if (ret != 0) {
			LOG_ERR("cannot enable SEMA clock");
			return ret;
		}
	}

	cfg->irq_func();

	/* IRQ register not necessarily reset, if just the RV32 core is restarted */
	sys_clear_bits(get_rx_irq_reg(dev),
		       (BIT(MAX32_SEMA_IRQ_EN_BIT) | BIT(MAX32_SEMA_IRQ_SET_BIT)));

	return ret;
}

static DEVICE_API(mbox, adi_max32_sema_driver_api) = {
	.max_channels_get = mbox_adi_max32_sema_max_channels_get,
	.register_callback = mbox_adi_max32_sema_register_callback,
	.set_enabled = mbox_adi_max32_sema_set_enabled,
	.mtu_get = mbox_adi_max32_sema_mtu_get,
	.send = mbox_adi_max32_sema_send,
};

#define ADI_MAX32_SEMA(n)                                                                          \
	static void mbox_adi_max32_sema_irq_init_##n(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mbox_adi_max32_sema_isr,    \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}                                                                                          \
	static const struct mbox_adi_max32_sema_config mbox_adi_max32_sema_cfg_##n = {             \
		.regs = (mxc_sema_reva_regs_t *)DT_INST_REG_ADDR(n),                               \
		.irq_func = mbox_adi_max32_sema_irq_init_##n,                                      \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.msg_payloads = DT_INST_PROP(n, msg_payloads),                                     \
		.clock = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(n)),                            \
		.perclk.bus = DT_INST_CLOCKS_CELL(n, offset),                                      \
		.perclk.bit = DT_INST_CLOCKS_CELL(n, bit),                                         \
	};                                                                                         \
	static struct mbox_adi_max32_sema_data mbox_adi_max32_sema_data_##n;                       \
	DEVICE_DT_INST_DEFINE(n, &mbox_adi_max32_init, NULL, &mbox_adi_max32_sema_data_##n,        \
			      &mbox_adi_max32_sema_cfg_##n, PRE_KERNEL_2,                          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &adi_max32_sema_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADI_MAX32_SEMA)
