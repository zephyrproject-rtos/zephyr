/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * Copyright (c) 2025, Haoran Jiang <halfsweet@halfsweet.cn>
 * Copyright (c) 2025, SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_i2c

#include "i2c-priv.h"

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_sf32lb, CONFIG_I2C_LOG_LEVEL);

#include <register.h>

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

#define SF32LB_I2C_TIMEOUT_MAX_US (30000)

struct i2c_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
	uint32_t bitrate;
};

struct i2c_sf32lb_data {
	struct k_mutex lock;
	uint8_t rw_flags;
};

static int i2c_sf32lb_send_addr(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;

	addr = addr << 1;
	if (i2c_is_read_op(msg)) {
		addr |= 1;
	}

	tcr |= I2C_TCR_START;
	tcr |= I2C_TCR_TB;

	if ((msg->len == 0) && i2c_is_stop_op(msg)) {
		tcr |= I2C_TCR_MA | I2C_TCR_STOP;
	}

	sys_write8(addr, cfg->base + I2C_DBR);
	sys_write32(tcr, cfg->base + I2C_TCR);

	if (!WAIT_FOR(sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos), SF32LB_I2C_TIMEOUT_MAX_US,
		      NULL)) {
		LOG_ERR("Abort timed out(I2C_SR: 0x%08x)", sys_read32(cfg->base + I2C_SR));
		return -EIO;
	}

	sys_write32(sys_read32(cfg->base + I2C_SR), cfg->base + I2C_SR);

	if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
		/* Wait for MSD(Master Stop Detected) to set, it appears slower than NACK */
		WAIT_FOR(sys_test_bit(cfg->base + I2C_SR, I2C_SR_MSD_Pos),
			 SF32LB_I2C_TIMEOUT_MAX_US, NULL);
		ret = -EIO;
	}

	if ((msg->len == 0) && i2c_is_stop_op(msg)) {
		if (!WAIT_FOR(!sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos),
			      SF32LB_I2C_TIMEOUT_MAX_US, NULL)) {
			LOG_ERR("Stop timed out (I2C_SR:0x%08x)", sys_read32(cfg->base + I2C_SR));
		}
	}

	return ret;
}

static int i2c_sf32lb_master_send(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr = I2C_TCR_TB;
	bool stop_needed = i2c_is_stop_op(msg);
	bool addr_sent = (data->rw_flags != (msg->flags & I2C_MSG_RW_MASK));

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	if (addr_sent) {
		ret = i2c_sf32lb_send_addr(dev, addr, msg);
		if (ret < 0) {
			return ret;
		}
	}

	for (uint32_t j = 0U; j < msg->len; j++) {
		bool last = ((msg->len - j) == 1U) ? true : false;

		if (last && stop_needed) {
			tcr |= I2C_TCR_STOP;
		}

		sys_write8(msg->buf[j], cfg->base + I2C_DBR);

		sys_write32(tcr, cfg->base + I2C_TCR);

		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos)) {
		}

		sys_set_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);

		if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
			ret = -EIO;
			break;
		}
	}

	if (stop_needed) {
		while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		}
	}

	return ret;
}

static int i2c_sf32lb_master_recv(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t tcr = I2C_TCR_TB;
	bool stop_needed = i2c_is_stop_op(msg);

	data->rw_flags = msg->flags & I2C_MSG_RW_MASK;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t j = 0U; j < msg->len; j++) {
		bool last = (j == (msg->len - 1U)) ? true : false;

		if (last && stop_needed) {
			tcr |= (I2C_TCR_STOP | I2C_TCR_NACK);
		}

		sys_write32(tcr, cfg->base + I2C_TCR);

		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos)) {
		}

		sys_set_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos);

		msg->buf[j] = sys_read8(cfg->base + I2C_DBR);
	}

	if (stop_needed) {
		while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		}
	}

	return ret;
}

static int i2c_sf32lb_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t cr;
	uint32_t sar;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}

	cr = sys_read32(cfg->base + I2C_CR);
	cr &= ~I2C_CR_MODE_Msk;

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

	default:
		LOG_ERR("Unsupported I2C speed requested:%d", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	sys_write32(cr, cfg->base + I2C_CR);
	sar = sys_read32(cfg->base + I2C_SAR);
	sar &= ~I2C_SAR_ADDR_Msk;
	/* Avoid sharing the same address with targets, 0x7c is a reserved address */
	sar |= FIELD_PREP(I2C_SAR_ADDR_Msk, 0x7CU);
	sys_write32(sar, cfg->base + I2C_SAR);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int i2c_sf32lb_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	sys_set_bits(cfg->base + I2C_CR, I2C_CR_IUE | I2C_CR_MSDE);

	if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		k_mutex_unlock(&data->lock);
		LOG_ERR("Bus busy");
		return -EBUSY;
	};

	for (uint8_t i = 0U; i < num_msgs; i++) {
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

	sys_clear_bit(cfg->base + I2C_CR, I2C_CR_IUE_Pos);

	data->rw_flags = I2C_MSG_READ;

	k_mutex_unlock(&data->lock);

	return ret;
}

static int i2c_sf32lb_recover_bus(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + I2C_CR, I2C_CR_RSTREQ_Pos);
	while (sys_test_bit(config->base + I2C_CR, I2C_CR_RSTREQ_Pos)) {
	}

	return 0;
}

static DEVICE_API(i2c, i2c_sf32lb_driver_api) = {
	.configure = i2c_sf32lb_configure,
	.transfer = i2c_sf32lb_transfer,
	.recover_bus = i2c_sf32lb_recover_bus,
};

static int i2c_sf32lb_init(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_sf32lb_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret < 0) {
		return ret;
	}

	sys_set_bits(config->base + I2C_CR, I2C_CR_IUE | I2C_CR_SCLE);

	data->rw_flags = I2C_MSG_READ;

	return ret;
}

#define I2C_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct i2c_sf32lb_data i2c_sf32lb_data_##n = {                                      \
		.lock = Z_MUTEX_INITIALIZER(i2c_sf32lb_data_##n.lock),                             \
	};                                                                                         \
	static const struct i2c_sf32lb_config i2c_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.bitrate = DT_INST_PROP_OR(n, clock_frequency, 100000),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &i2c_sf32lb_init, NULL, &i2c_sf32lb_data_##n,                     \
			      &i2c_sf32lb_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,       \
			      &i2c_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SF32LB_DEFINE)
