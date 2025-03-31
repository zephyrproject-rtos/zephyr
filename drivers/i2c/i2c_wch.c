/*
 * Copyright (c) 2025 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_i2c

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_wch);

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#include "i2c-priv.h"

#include <ch32fun.h>

typedef void (*irq_config_func_t)(const struct device *port);

struct i2c_wch_config {
	const struct pinctrl_dev_config *pcfg;
	irq_config_func_t irq_config_func;
	const struct device *clk_dev;
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
	I2C_TypeDef *regs;
	uint32_t bitrate;
	uint8_t clk_id;
};

struct i2c_wch_data {
	atomic_t xfer_done;
	struct {
		struct i2c_msg *msg;
		uint32_t idx;
		union {
			struct {
				uint16_t addr : 10;
				uint16_t err : 6;
			};
			uint16_t: 16;
		};
	} current;
};

static void wch_i2c_handle_start_bit(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;

	if ((data->current.msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		regs->DATAR = ((data->current.addr << 1) & 0xFF);
	} else {
		regs->DATAR = ((data->current.addr << 1) & 0xFF) | 1;
		if (data->current.msg->len == 2U) {
			regs->CTLR1 |= I2C_CTLR1_POS;
		}
	}
}

static void wch_i2c_handle_addr(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;

	if ((data->current.msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		if (data->current.msg->len == 1U) {
			regs->CTLR1 &= ~I2C_CTLR1_ACK;
			if (data->current.msg->flags & I2C_MSG_STOP) {
				regs->CTLR1 |= I2C_CTLR1_STOP;
			}
		} else if (data->current.msg->len == 2U) {
			regs->CTLR1 &= ~I2C_CTLR1_ACK;
		}
	}

	regs->STAR1;
	regs->STAR2;
}

static void wch_i2c_handle_txe(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;

	if (data->current.idx < data->current.msg->len) {
		regs->DATAR = data->current.msg->buf[data->current.idx++];
		if (data->current.idx == data->current.msg->len) {
			regs->CTLR2 &= ~I2C_CTLR2_ITBUFEN;
		}
	} else {
		if (data->current.msg->flags & I2C_MSG_STOP) {
			regs->CTLR1 |= I2C_CTLR1_STOP;
		}

		if (regs->STAR1 & I2C_STAR1_BTF) {
			regs->DATAR;
		}

		atomic_set(&data->xfer_done, 1);
	}
}

static void wch_i2c_handle_rxne(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;

	if (data->current.idx < data->current.msg->len) {
		if (data->current.idx == data->current.msg->len - 1) {
			regs->CTLR1 &= ~I2C_CTLR1_ACK;
			regs->CTLR1 &= ~I2C_CTLR1_POS;

			if (data->current.msg->flags & I2C_MSG_STOP) {
				regs->CTLR1 |= I2C_CTLR1_STOP;
			}

			regs->CTLR2 &= ~I2C_CTLR2_ITBUFEN;

			data->current.msg->buf[data->current.idx++] = regs->DATAR;

			atomic_set(&data->xfer_done, 1);
		} else {
			data->current.msg->buf[data->current.idx++] = regs->DATAR;
		}
	} else {
		if (data->current.msg->flags & I2C_MSG_STOP) {
			regs->CTLR1 |= I2C_CTLR1_STOP;
		}

		atomic_set(&data->xfer_done, 1);
	}
}

static void wch_i2c_handle_btf(const struct device *dev)
{
	struct i2c_wch_data *data = dev->data;

	if ((data->current.msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		wch_i2c_handle_txe(dev);
	} else {
		wch_i2c_handle_rxne(dev);
	}
}

static void i2c_wch_event_isr(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;
	uint16_t status = regs->STAR1;
	bool write;

	write = ((data->current.msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE);

	if (status & I2C_STAR1_SB) {
		wch_i2c_handle_start_bit(dev);
	} else if (status & I2C_STAR1_ADDR) {
		wch_i2c_handle_addr(dev);
	} else if (status & I2C_STAR1_BTF) {
		wch_i2c_handle_btf(dev);
	} else if ((status & I2C_STAR1_TXE) && write) {
		wch_i2c_handle_txe(dev);
	} else if ((status & I2C_STAR1_RXNE) && (!write)) {
		wch_i2c_handle_rxne(dev);
	}
}

static void i2c_wch_error_isr(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;
	uint16_t status = regs->STAR1;

	if (status & (I2C_STAR1_AF | I2C_STAR1_ARLO | I2C_STAR1_BERR)) {
		if (status & I2C_STAR1_AF) {
			regs->CTLR1 |= I2C_CTLR1_STOP;
		}

		data->current.err |= ((status >> 8) & 0x7);

		regs->STAR1 &= ~(I2C_STAR1_AF | I2C_STAR1_ARLO | I2C_STAR1_BERR);

		atomic_set(&data->xfer_done, 1);
	}
}

static void wch_i2c_msg_init(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;

	atomic_set(&data->xfer_done, 0);

	data->current.msg = msg;
	data->current.idx = 0U;
	data->current.err = 0U;
	data->current.addr = addr;

	regs->CTLR1 |= I2C_CTLR1_PE;
	regs->CTLR1 |= I2C_CTLR1_ACK;

	if (msg->flags & I2C_MSG_RESTART) {
		if (regs->CTLR1 & I2C_CTLR1_STOP) {
			regs->CTLR1 &= ~I2C_CTLR1_STOP;
		}
		regs->CTLR1 |= I2C_CTLR1_START;
	}
}

static int32_t wch_i2c_msg_end(const struct device *dev)
{
	struct i2c_wch_data *data = dev->data;

	if (!data->current.err) {
		return 0;
	}

	if (data->current.err & (I2C_STAR1_ARLO >> 8)) {
		LOG_DBG("ARLO");
	}

	if (data->current.err & (I2C_STAR1_AF >> 8)) {
		LOG_DBG("NACK");
	}

	if (data->current.err & (I2C_STAR1_BERR >> 8)) {
		LOG_DBG("ERR");
	}

	data->current.err = 0;

	return -EIO;
}

static void wch_i2c_config_interrupts(I2C_TypeDef *regs, bool enable)
{
	if (enable) {
		regs->CTLR2 |= (I2C_CTLR2_ITERREN | I2C_CTLR2_ITEVTEN | I2C_CTLR2_ITBUFEN);
	} else {
		regs->CTLR2 &= ~(I2C_CTLR2_ITERREN | I2C_CTLR2_ITEVTEN | I2C_CTLR2_ITBUFEN);
	}
}

static int32_t wch_i2c_begin_transfer(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_wch_config *config = dev->config;
	struct i2c_wch_data *data = dev->data;
	I2C_TypeDef *regs = config->regs;

	wch_i2c_msg_init(dev, msg, addr);

	wch_i2c_config_interrupts(regs, true);

	k_sched_lock();
	while (atomic_get(&data->xfer_done) == 0) {
	}
	k_sched_unlock();

	return wch_i2c_msg_end(dev);
}

static void wch_i2c_finish_transfer(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	I2C_TypeDef *regs = config->regs;

	wch_i2c_config_interrupts(regs, false);

	regs->CTLR1 &= ~I2C_CTLR1_PE;
}

static void wch_i2c_configure_timing(I2C_TypeDef *regs, uint32_t clock_rate, uint32_t bitrate)
{
	uint16_t clock_config;
	uint16_t freq_range;

	if (bitrate <= I2C_BITRATE_STANDARD) {
		clock_config = MAX((uint16_t)(clock_rate / (bitrate << 1)), 4);
	} else {
		clock_config = MAX((uint16_t)(clock_rate / (bitrate * 3)), 1) | I2C_CKCFGR_FS;
	}

	regs->CKCFGR = clock_config;

	freq_range = regs->CTLR2;
	freq_range &= ~I2C_CTLR2_FREQ;
	freq_range |= (uint16_t)(clock_rate / 1000000);

	regs->CTLR2 = freq_range;
}

static int i2c_wch_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_wch_config *config = dev->config;
	I2C_TypeDef *regs = config->regs;
	clock_control_subsys_t clk_sys;
	uint32_t clock_rate;
	int err;

	clk_sys = (clock_control_subsys_t)(uintptr_t)config->clk_id;

	err = clock_control_get_rate(config->clk_dev, clk_sys, &clock_rate);
	if (err != 0) {
		return err;
	}

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -EINVAL;
	}

	regs->CTLR1 &= ~I2C_CTLR1_PE;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		wch_i2c_configure_timing(regs, clock_rate, 100000);
		break;
	case I2C_SPEED_FAST:
		wch_i2c_configure_timing(regs, clock_rate, 400000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2c_wch_transfer(const struct device *dev, struct i2c_msg *msg,
			    uint8_t num_msgs, uint16_t addr)
{
	int ret = 0;

	msg[0].flags |= I2C_MSG_RESTART;

	for (uint8_t i = 1; i < num_msgs; i++) {
		if ((msg[i - 1].flags & I2C_MSG_RW_MASK) != (msg[i].flags & I2C_MSG_RW_MASK)) {
			if (!(msg[i].flags & I2C_MSG_RESTART)) {
				return -EINVAL;
			}
		}

		if (msg[i - 1].flags & I2C_MSG_STOP) {
			return -EINVAL;
		}
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		ret = wch_i2c_begin_transfer(dev, &msg[i], addr);
		if (ret < 0) {
			break;
		}
	}

	wch_i2c_finish_transfer(dev);

	return ret;
}

static int wch_i2c_reset(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	I2C_TypeDef *regs = config->regs;
	int err;

	regs->CTLR1 &= ~I2C_CTLR1_PE;

	gpio_pin_configure_dt(&config->scl, GPIO_OUTPUT | config->scl.dt_flags);
	gpio_pin_configure_dt(&config->sda, GPIO_OUTPUT | config->sda.dt_flags);

	gpio_pin_set_dt(&config->scl, 1);
	gpio_pin_set_dt(&config->sda, 1);

	while (!gpio_pin_get_dt(&config->scl)) {
	}

	while (!gpio_pin_get_dt(&config->sda)) {
	}

	gpio_pin_set_dt(&config->sda, 0);
	while (gpio_pin_get_dt(&config->sda)) {
	}

	gpio_pin_set_dt(&config->scl, 0);
	while (gpio_pin_get_dt(&config->scl)) {
	}

	gpio_pin_set_dt(&config->scl, 1);
	while (!gpio_pin_get_dt(&config->scl)) {
	}

	gpio_pin_set_dt(&config->sda, 1);
	while (!gpio_pin_get_dt(&config->sda)) {
	}

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	regs->CTLR1 |= I2C_CTLR1_SWRST;
	regs->CTLR1 &= ~I2C_CTLR1_SWRST;

	regs->CTLR1 |= I2C_CTLR1_PE;
	regs->CTLR1 &= ~I2C_CTLR1_PE;

	if ((!gpio_pin_get_dt(&config->scl)) || (!gpio_pin_get_dt(&config->sda))) {
		return -EBUSY;
	}

	return 0;
}

static int i2c_wch_init(const struct device *dev)
{
	const struct i2c_wch_config *config = dev->config;
	clock_control_subsys_t clk_sys;
	int err;

	clk_sys = (clock_control_subsys_t)(uintptr_t)config->clk_id;

	err = clock_control_on(config->clk_dev, clk_sys);
	if (err < 0) {
		return err;
	}

	do {
		err = wch_i2c_reset(dev);
	} while (err == -EBUSY);

	if (err < 0) {
		return err;
	}

	err = i2c_wch_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (err < 0) {
		return err;
	}

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(i2c, i2c_wch_api) = {
	.configure = i2c_wch_configure,
	.transfer = i2c_wch_transfer,
};

#define I2C_WCH_INIT(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static void i2c_wch_config_func_##inst(const struct device *dev);		\
											\
	static struct i2c_wch_config i2c_wch_cfg_##inst = {				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.irq_config_func = i2c_wch_config_func_##inst,				\
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),			\
		.scl = GPIO_DT_SPEC_INST_GET(inst, scl_gpios),				\
		.sda = GPIO_DT_SPEC_INST_GET(inst, sda_gpios),				\
		.regs = (I2C_TypeDef *)DT_INST_REG_ADDR(inst),				\
		.bitrate = DT_INST_PROP(inst, clock_frequency),				\
		.clk_id = DT_INST_CLOCKS_CELL(inst, id)					\
	};										\
											\
	static struct i2c_wch_data i2c_wch_data_##inst;					\
											\
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_wch_init, NULL, &i2c_wch_data_##inst,	\
				 &i2c_wch_cfg_##inst, PRE_KERNEL_1,			\
				 CONFIG_I2C_INIT_PRIORITY, &i2c_wch_api);		\
											\
	static void i2c_wch_config_func_##inst(const struct device *dev)		\
	{										\
		ARG_UNUSED(dev);							\
											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),				\
			    DT_INST_IRQ_BY_IDX(inst, 0, priority),			\
			    i2c_wch_event_isr, DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));				\
											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 1, irq),				\
			    DT_INST_IRQ_BY_IDX(inst, 1, priority),			\
			    i2c_wch_error_isr, DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 1, irq));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_WCH_INIT)
