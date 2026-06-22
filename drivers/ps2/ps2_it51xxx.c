/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/ps2.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#define DT_DRV_COMPAT ite_it51xxx_ps2

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_PS2_LOG_LEVEL
LOG_MODULE_REGISTER(ps2_it51xxx);

#define ITE_PS200_CTRL_REG   0x0
#define TX_MODE_SELECTION    BIT(3)
#define HARDWARE_MODE_ENABLE BIT(2)
#define CTRL_CLK_LINE        BIT(1)
#define CTRL_DATA_LINE       BIT(0)

#define ITE_PS204_INT_CTRL_REG      0x4
#define TIMEOUT_INT_ENABLE          BIT(3)
#define TRANSACTION_DONE_INT_ENABLE BIT(2)

#define ITE_PS208_STATUS 0x8
#define BIT_TIMEOUT_ERR  6
#define BIT_FRAME_ERR    5
#define BIT_PARITY_ERR   4
#define XFER_ERROR_MASK  GENMASK(BIT_TIMEOUT_ERR, BIT_PARITY_ERR)
#define TRANSACTION_DONE BIT(3)
#define START_STATUS     BIT(2)
#define CLK_LINE_STATUS  BIT(1)
#define DATA_LINE_STATUS BIT(0)
#define BUS_IDLE         (CLK_LINE_STATUS | DATA_LINE_STATUS)

#define ITE_PS20C_DATA_REG 0xC

#define PS2_BUSY_TIMEOUT_UNIT_US 50

/* We expect the hardware to trigger timeout interrupt within 17 ms.
 * The 40 ms semaphore timeout is safeguard in case the ps2 bus
 * is blocked for unknown reason and hardware timeout interrupt
 * is not triggered.
 */
#define PS2_TRANSMIT_TIMEOUT K_MSEC(40)

struct it51xxx_ps2_data {
	ps2_callback_t callback_isr;
	struct k_sem lock;
	struct k_sem tx_sem;

	int xfer_status;
};

struct it51xxx_ps2_config {
	const struct pinctrl_dev_config *pcfg;
	mm_reg_t base;

	int bus_busy_timeout_us;

#ifdef CONFIG_PM_DEVICE
	struct gpio_dt_spec clk_gpios;
	struct gpio_dt_spec data_gpios;
#endif /* CONFIG_PM_DEVICE */

	void (*irq_config_func)(const struct device *dev);
	uint8_t irq_num;
};

static inline void it51xxx_ps2_inhibit_bus(const struct device *dev, const bool inhibit)
{
	const struct it51xxx_ps2_config *cfg = dev->config;

	if (inhibit) {
		sys_write8(CTRL_DATA_LINE, cfg->base + ITE_PS200_CTRL_REG);
		return;
	}

	/* set bus as idle state (receive mode) */
	sys_write8(HARDWARE_MODE_ENABLE | CTRL_CLK_LINE | CTRL_DATA_LINE,
		   cfg->base + ITE_PS200_CTRL_REG);
}

static inline void enable_standby_state(const bool enable)
{
	if (enable) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	} else {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
}

static int it51xxx_ps2_configure(const struct device *dev, ps2_callback_t callback_isr)
{
	struct it51xxx_ps2_data *data = dev->data;

	if (callback_isr == NULL) {
		return -EINVAL;
	}

	if (k_sem_take(&data->lock, K_NO_WAIT)) {
		return -EACCES;
	}

	data->callback_isr = callback_isr;

	it51xxx_ps2_inhibit_bus(dev, false);

	k_sem_give(&data->lock);

	return 0;
}

static int it51xxx_ps2_bus_busy(const struct device *dev)
{
	const struct it51xxx_ps2_config *cfg = dev->config;
	uint8_t status;
	int ret;

	irq_disable(cfg->irq_num);

	status = sys_read8(cfg->base + ITE_PS208_STATUS);
	ret = ((status & BUS_IDLE) != BUS_IDLE || (status & START_STATUS)) ? -EBUSY : 0;

	irq_enable(cfg->irq_num);

	return ret;
}

static int it51xxx_ps2_write(const struct device *dev, uint8_t value)
{
	const struct it51xxx_ps2_config *cfg = dev->config;
	struct it51xxx_ps2_data *data = dev->data;
	int ret;
	uint8_t ctrl_val;

	if (k_sem_take(&data->lock, K_NO_WAIT)) {
		return -EACCES;
	}

	/* allow the ps2 controller to complete a rx transaction */
	if (!WAIT_FOR(!it51xxx_ps2_bus_busy(dev), cfg->bus_busy_timeout_us,
		      k_busy_wait(PS2_BUSY_TIMEOUT_UNIT_US))) {
		LOG_ERR("failed to send ps2 data");
		k_sem_give(&data->lock);
		return -EBUSY;
	}

	/* enable hardware mode, pull down clk line and pull up data line */
	ctrl_val = TX_MODE_SELECTION | HARDWARE_MODE_ENABLE | CTRL_DATA_LINE;
	sys_write8(ctrl_val, cfg->base + ITE_PS200_CTRL_REG);

	/* inhibit communication for at least 100 microseconds */
	k_busy_wait(100);

	/* set data */
	sys_write8(value, cfg->base + ITE_PS20C_DATA_REG);

	/* pull down data line */
	ctrl_val &= ~CTRL_DATA_LINE;
	sys_write8(ctrl_val, cfg->base + ITE_PS200_CTRL_REG);

	/* pull up clk line */
	ctrl_val |= CTRL_CLK_LINE;
	sys_write8(ctrl_val, cfg->base + ITE_PS200_CTRL_REG);

	enable_standby_state(false);

	if (k_sem_take(&data->tx_sem, PS2_TRANSMIT_TIMEOUT) != 0) {
		LOG_ERR("sw: tx timeout");

		it51xxx_ps2_inhibit_bus(dev, true);
		data->xfer_status = -ETIMEDOUT;
	}

	enable_standby_state(true);

	ret = data->xfer_status;
	it51xxx_ps2_inhibit_bus(dev, false);

	k_sem_give(&data->lock);

	return ret;
}

static int it51xxx_ps2_inhibit_interface(const struct device *dev)
{
	const struct it51xxx_ps2_config *cfg = dev->config;
	struct it51xxx_ps2_data *data = dev->data;

	if (k_sem_take(&data->lock, K_NO_WAIT)) {
		return -EACCES;
	}

	if (it51xxx_ps2_bus_busy(dev)) {
		k_sem_give(&data->lock);
		return -EBUSY;
	}

	irq_disable(cfg->irq_num);

	it51xxx_ps2_inhibit_bus(dev, true);

	irq_enable(cfg->irq_num);

	LOG_DBG("inhibit interface");

	k_sem_give(&data->lock);

	return 0;
}

static int it51xxx_ps2_enable_interface(const struct device *dev)
{
	struct it51xxx_ps2_data *data = dev->data;

	if (k_sem_take(&data->lock, K_NO_WAIT)) {
		return -EACCES;
	}

	it51xxx_ps2_inhibit_bus(dev, false);

	LOG_DBG("enable interface");

	k_sem_give(&data->lock);

	return 0;
}

static DEVICE_API(ps2, it51xxx_ps2_api) = {
	.config = it51xxx_ps2_configure,
	.write = it51xxx_ps2_write,
	.disable_callback = it51xxx_ps2_inhibit_interface,
	.enable_callback = it51xxx_ps2_enable_interface,
};

static int it51xxx_ps2_init(const struct device *dev)
{
	const struct it51xxx_ps2_config *cfg = dev->config;
	struct it51xxx_ps2_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("failed to apply pinctrl, ret %d", ret);
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->tx_sem, 0, 1);

	cfg->irq_config_func(dev);

	/* enable interrupt */
	sys_write8(TIMEOUT_INT_ENABLE | TRANSACTION_DONE_INT_ENABLE,
		   cfg->base + ITE_PS204_INT_CTRL_REG);

	it51xxx_ps2_inhibit_bus(dev, false);

	return 0;
}

static void it51xxx_ps2_isr(const struct device *dev)
{
	const struct it51xxx_ps2_config *cfg = dev->config;
	struct it51xxx_ps2_data *data = dev->data;
	uint8_t int_status;
	bool xfer_is_tx = (sys_read8(cfg->base + ITE_PS200_CTRL_REG) & TX_MODE_SELECTION) ==
			  TX_MODE_SELECTION;

	int_status = sys_read8(cfg->base + ITE_PS208_STATUS);
	LOG_DBG("isr: interrupt status 0x%x", int_status);

	it51xxx_ps2_inhibit_bus(dev, true);

	if (int_status & XFER_ERROR_MASK) {
		if (IS_BIT_SET(int_status, BIT_TIMEOUT_ERR)) {
			LOG_ERR("isr: %s: timeout event occurs", xfer_is_tx ? "tx" : "rx");

			data->xfer_status = -ETIMEDOUT;
			sys_write8(BIT(BIT_TIMEOUT_ERR), cfg->base + ITE_PS208_STATUS);
		}

		if (IS_BIT_SET(int_status, BIT_FRAME_ERR)) {
			LOG_ERR("isr: %s: frame error occurs", xfer_is_tx ? "tx" : "rx");

			data->xfer_status = -EPROTO;
		}

		if (IS_BIT_SET(int_status, BIT_PARITY_ERR)) {
			LOG_ERR("isr: %s: parity error occurs", xfer_is_tx ? "tx" : "rx");

			data->xfer_status = -EIO;
		}
		goto out;
	}

	if (int_status & TRANSACTION_DONE) {
		LOG_DBG("isr: %s: xfer done", xfer_is_tx ? "tx" : "rx");

		data->xfer_status = 0;
		if (!xfer_is_tx) {
			if (data->callback_isr) {
				data->callback_isr(dev, sys_read8(cfg->base + ITE_PS20C_DATA_REG));
			} else {
				LOG_INF("isr: %s: rx 0x%x", dev->name,
					sys_read8(cfg->base + ITE_PS20C_DATA_REG));
			}
		}
	}

out:
	if (xfer_is_tx) {
		k_sem_give(&data->tx_sem);
	} else {
		it51xxx_ps2_inhibit_bus(dev, false);
	}
}

#ifdef CONFIG_PM_DEVICE
static inline int it51xxx_ps2_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct it51xxx_ps2_config *const cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_interrupt_configure_dt(&cfg->clk_gpios, GPIO_INT_MODE_DISABLED);
		if (ret) {
			LOG_ERR("failed to disable clock-gpio wui, %d", ret);
			return ret;
		}
		ret = gpio_pin_interrupt_configure_dt(&cfg->data_gpios, GPIO_INT_MODE_DISABLED);
		if (ret) {
			LOG_ERR("failed to disable data-gpio wui, %d", ret);
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_pin_interrupt_configure_dt(&cfg->clk_gpios, GPIO_INT_EDGE_FALLING);
		if (ret) {
			LOG_ERR("failed to configure clock-gpio wui, %d", ret);
			return ret;
		}
		ret = gpio_pin_interrupt_configure_dt(&cfg->data_gpios, GPIO_INT_EDGE_FALLING);
		if (ret) {
			LOG_ERR("failed to configure data-gpio wui, %d", ret);
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_PM_DEVICE
#define IT51XXX_PS2_PM_GPIO_PINS(n)                                                                \
	.clk_gpios = GPIO_DT_SPEC_INST_GET(n, clk_gpios),                                          \
	.data_gpios = GPIO_DT_SPEC_INST_GET(n, data_gpios),
#else
#define IT51XXX_PS2_PM_GPIO_PINS(n) /* not used */
#endif                              /* CONFIG_PM_DEVICE */

/* clang-format off */
#define IT51XXX_PS2_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, bus_busy_timeout_ms), (                                \
		BUILD_ASSERT(DT_INST_PROP(n, bus_busy_timeout_ms) <= 500,                          \
			     "bus-busy-timeout-ms must be 500 ms or less for instance " #n);       \
	))                                                                                         \
                                                                                                   \
	static void it51xxx_ps2_config_func_##n(const struct device *dev)                          \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		ite_intc_irq_polarity_set(DT_INST_IRQN(n), DT_INST_IRQ(n, flags));                 \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, it51xxx_ps2_isr, DEVICE_DT_INST_GET(n), 0);        \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static struct it51xxx_ps2_config ps2_config_##n = {                                        \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_num = DT_INST_IRQ(n, irq),                                                    \
		.irq_config_func = it51xxx_ps2_config_func_##n,                                    \
		.bus_busy_timeout_us =                                                             \
			DT_INST_PROP_OR(n, bus_busy_timeout_ms, 10) * USEC_PER_MSEC,               \
		IT51XXX_PS2_PM_GPIO_PINS(n)};                                                      \
	static struct it51xxx_ps2_data ps2_data_##n;                                               \
	PM_DEVICE_DT_INST_DEFINE(n, it51xxx_ps2_pm_action);                                        \
	DEVICE_DT_INST_DEFINE(n, it51xxx_ps2_init, PM_DEVICE_DT_INST_GET(n), &ps2_data_##n,        \
			      &ps2_config_##n, POST_KERNEL, CONFIG_PS2_INIT_PRIORITY,              \
			      &it51xxx_ps2_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_PS2_INIT)
