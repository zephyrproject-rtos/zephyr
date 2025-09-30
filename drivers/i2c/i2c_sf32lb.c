/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_i2c

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_sf32lb, CONFIG_I2C_LOG_LEVEL);

#include <register.h>

#include "i2c-priv.h"

#define I2C_CR    offsetof(I2C_TypeDef, CR)
#define I2C_TCR   offsetof(I2C_TypeDef, TCR)
#define I2C_IER   offsetof(I2C_TypeDef, IER)
#define I2C_SR    offsetof(I2C_TypeDef, SR)
#define I2C_DBR   offsetof(I2C_TypeDef, DBR)
#define I2C_SAR   offsetof(I2C_TypeDef, SAR)
#define I2C_LCR   offsetof(I2C_TypeDef, LCR)
#define I2C_WCR   offsetof(I2C_TypeDef, WCR)
#define I2C_RCCR  offsetof(I2C_TypeDef, RCCR)
#define I2C_BMR   offsetof(I2C_TypeDef, BMR)
#define I2C_DNR   offsetof(I2C_TypeDef, DNR)
#define I2C_RSVD1 offsetof(I2C_TypeDef, RSVD1)
#define I2C_FIFO  offsetof(I2C_TypeDef, FIFO)

#define I2C_MODE_STD    (0x00U)
#define I2C_MODE_FS     (0x01U)
#define I2C_MODE_HS_STD (0x02U)
#define I2C_MODE_HS_FS  (0x03U)

struct i2c_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
	uint32_t bitrate;
};

struct i2c_sf32lb_data {
	struct k_mutex lock;
};

static int i2c_sf32lb_send_addr(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;
	uint32_t sr = 0;

	addr = addr << 1;
	if (i2c_is_read_op(msg)) {
		addr |= 1;
	}

	tcr |= I2C_TCR_START;
	tcr |= I2C_TCR_TB;

	if ((msg->len == 0) && i2c_is_stop_op(msg)) {
		tcr |= I2C_TCR_STOP;
	}

	sys_write8(addr, cfg->base + I2C_DBR);
	sys_write32(tcr, cfg->base + I2C_TCR);

	sr = sys_read32(cfg->base + I2C_SR);
	while ((sr & I2C_SR_TE) != I2C_SR_TE) {
		sr = sys_read32(cfg->base + I2C_SR);
	}

	sys_clear_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);
	if ((sr & I2C_SR_NACK) == I2C_SR_NACK) {
		ret = -EIO;
	}

	return ret;
}

static int i2c_sf32lb_master_send(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;
	uint32_t sr = 0;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	for (int j = 0; j < msg->len; j++) {
		bool last = (msg->len - j == 1) ? true : false;

		if (last && i2c_is_stop_op(msg)) {
			tcr |= I2C_TCR_STOP;
		}

		sys_write8(msg->buf[j], cfg->base + I2C_DBR);
		tcr |= I2C_TCR_TB;
		sys_write32(tcr, cfg->base + I2C_TCR);

		sr = sys_read32(cfg->base + I2C_SR);
		while ((sr & I2C_SR_TE) != I2C_SR_TE) {
			sr = sys_read32(cfg->base + I2C_SR);
		}

		sys_clear_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);
		if ((sr & I2C_SR_NACK) == I2C_SR_NACK) {
			ret = -EIO;
			break;
		}

		tcr = 0;
	}

	return ret;
}

static int i2c_sf32lb_master_recv(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;
	uint32_t sr = 0;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	for (int j = 0; j < msg->len; j++) {
		bool last = (j == (msg->len - 1)) ? true : false;

		if (last && i2c_is_stop_op(msg)) {
			tcr |= I2C_TCR_NACK;
		}

		tcr |= I2C_TCR_TB;
		sys_write32(tcr, cfg->base + I2C_TCR);

		sr = sys_read32(cfg->base + I2C_SR);
		while ((sr & I2C_SR_RF) != I2C_SR_RF) {
			sr = sys_read32(cfg->base + I2C_SR);
		}
		sys_clear_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos);

		msg->buf[j] = sys_read32(cfg->base + I2C_DBR) & 0xFF;
		tcr = 0;
	}
	tcr |= I2C_TCR_MA;
	sys_write32(tcr, cfg->base + I2C_TCR);

	return ret;
}

static int i2c_sf32lb_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t cr = sys_read32(cfg->base + I2C_CR);

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}
	cr &= ~(I2C_CR_SLVEN | I2C_CR_MODE);

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		cr |= I2C_MODE_STD;
		break;

	case I2C_SPEED_FAST:
		cr |= I2C_MODE_FS;
		break;

	case I2C_SPEED_FAST_PLUS:
		cr |= I2C_MODE_HS_STD;
		break;

	case I2C_SPEED_HIGH:
		cr |= I2C_MODE_HS_FS;
		break;

	case I2C_SPEED_ULTRA:
		return -ENOTSUP;

	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	sys_write32(cr, cfg->base + I2C_CR);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int i2c_sf32lb_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_sf32lb_data *data = dev->data;
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			break;
		}

		if (msgs[i].flags & I2C_MSG_READ) {
			ret = i2c_sf32lb_master_recv(dev, addr, &msgs[i]);
			if (ret < 0) {
				break;
			}
		} else {
			ret = i2c_sf32lb_master_send(dev, addr, &msgs[i]);
			if (ret < 0) {
				break;
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static const struct i2c_driver_api i2c_sf32lb_driver_api = {
	.configure = i2c_sf32lb_configure,
	.transfer = i2c_sf32lb_transfer,
};

static int i2c_sf32lb_init(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret;

	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	if (config->clock.dev != NULL) {
		if (!sf3232lb_clock_is_ready_dt(&config->clock)) {
			return -ENODEV;
		}

		ret = sf32lb_clock_control_on_dt(&config->clock);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_sf32lb_configure(dev, I2C_MODE_CONTROLLER | (config->bitrate));

	return ret;
}

#define I2C_SF32LB_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct i2c_sf32lb_data i2c_sf32lb_data_##n;                                         \
	static const struct i2c_sf32lb_config i2c_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET_OR(index, {}),                              \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &i2c_sf32lb_init, NULL, &i2c_sf32lb_data_##n,                     \
			      &i2c_sf32lb_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,       \
			      &i2c_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SF32LB_INIT)
