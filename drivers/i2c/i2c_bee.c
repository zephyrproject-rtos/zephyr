/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_i2c

#include <errno.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_bee, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_i2c.h>
#include <rtl_rcc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_i2c.h>
#include <rtl876x_rcc.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#if defined(CONFIG_SOC_SERIES_RTL8752H)
extern I2C_Status I2C_CheckAbortStatus(I2C_TypeDef *I2Cx);
#endif

struct i2c_bee_context {
	struct i2c_msg *msgs;
	uint8_t num_msgs;
	uint8_t msg_idx;
	uint16_t tx_idx;
	uint16_t rx_idx;
};

struct i2c_bee_config {
	I2C_TypeDef *reg;
	uint32_t bitrate;
	uint16_t clkid;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_cfg_func)(void);
};

struct i2c_bee_data {
	struct k_mutex bus_mutex;
	struct k_sem sync_sem;
	struct i2c_bee_context ctx;
	uint8_t errs;
};

static inline bool i2c_bee_next_msg_available(struct i2c_bee_context *ctx)
{
	return ctx->msg_idx < ctx->num_msgs;
}

static inline struct i2c_msg *i2c_bee_current_msg(struct i2c_bee_context *ctx)
{
	if (ctx->msg_idx >= ctx->num_msgs) {
		return NULL;
	}
	return &ctx->msgs[ctx->msg_idx];
}

static inline void i2c_bee_advance_msg(struct i2c_bee_context *ctx)
{
	ctx->msg_idx++;
	ctx->tx_idx = 0U;
	ctx->rx_idx = 0U;
}

static int i2c_bee_do_tx(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->reg;
	struct i2c_bee_context *ctx = &data->ctx;
	struct i2c_msg *msg = i2c_bee_current_msg(ctx);
	bool stop_f;
	bool last_byte;
	bool send_stop;

	if (msg == NULL) {
		return 0;
	}

	stop_f = i2c_is_stop_op(msg);

	if (!i2c_is_read_op(msg)) {
		if (ctx->tx_idx >= msg->len) {
			i2c_bee_advance_msg(ctx);
			return 0;
		}

		if (!(i2c->IC_STATUS & I2C_FLAG_TFNF)) {
			return 0;
		}

		last_byte = (ctx->tx_idx >= msg->len - 1U);
		send_stop = stop_f;

		if (last_byte) {
			i2c->IC_DATA_CMD = msg->buf[ctx->tx_idx] | (stop_f ? BIT9 : 0);
			ctx->tx_idx++;
			i2c_bee_advance_msg(ctx);
		} else {
			i2c->IC_DATA_CMD = msg->buf[ctx->tx_idx];
			ctx->tx_idx++;
		}
	} else {
		if (ctx->tx_idx >= msg->len) {
			return 0;
		}

		if (!(i2c->IC_STATUS & I2C_FLAG_TFNF)) {
			return 0;
		}

		last_byte = (ctx->tx_idx >= msg->len - 1U);
		send_stop = last_byte && stop_f;

		i2c->IC_DATA_CMD = BIT8 | (send_stop ? BIT9 : 0);

		ctx->tx_idx++;
	}

	return 0;
}

static int i2c_bee_do_rx(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->reg;
	struct i2c_bee_context *ctx = &data->ctx;
	struct i2c_msg *msg = i2c_bee_current_msg(ctx);

	if (msg == NULL || !(msg->flags & I2C_MSG_READ)) {
		return 0;
	}

	if (!(i2c->IC_STATUS & I2C_FLAG_RFNE)) {
		return 0;
	}

	if (ctx->rx_idx >= msg->len) {
		i2c_bee_advance_msg(ctx);
		return 0;
	}

	msg->buf[ctx->rx_idx] = (uint8_t)i2c->IC_DATA_CMD;
	ctx->rx_idx++;

	if (ctx->rx_idx >= msg->len) {
		i2c_bee_advance_msg(ctx);
	}

	return 0;
}

static void i2c_bee_log_err(struct i2c_bee_data *data)
{
	switch (data->errs) {
	case I2C_ABRT_7B_ADDR_NOACK:
		LOG_ERR("7 bit address no ack");
		break;
	case I2C_ABRT_10ADDR1_NOACK:
	case I2C_ABRT_10ADDR2_NOACK:
		LOG_ERR("10 bit address no ack");
		break;
	case I2C_ABRT_TXDATA_NOACK:
		LOG_ERR("data no ack");
		break;
	case I2C_ARB_LOST:
		LOG_ERR("arbitration lost");
		break;
	case I2C_ERR_TIMEOUT:
		LOG_ERR("timeout");
		break;
	default:
		LOG_ERR("unknown status: 0x%02x", data->errs);
		break;
	}
}

static int i2c_bee_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = (I2C_TypeDef *)cfg->reg;
	int ret = 0;

	if (num_msgs == 0U) {
		return 0;
	}

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	data->ctx.msgs = msgs;
	data->ctx.num_msgs = num_msgs;
	data->ctx.msg_idx = 0U;
	data->ctx.tx_idx = 0U;
	data->ctx.rx_idx = 0U;
	data->errs = I2C_Success;
	k_sem_reset(&data->sync_sem);

	I2C_Cmd(i2c, ENABLE);
	I2C_SetSlaveAddress(i2c, addr);

	I2C_INTConfig(i2c, I2C_INT_TX_ABRT | I2C_INT_RX_FULL | I2C_INT_TX_EMPTY, ENABLE);

	i2c_bee_do_tx(dev);

	k_sem_take(&data->sync_sem, K_FOREVER);

	ret = (data->errs == I2C_Success) ? 0 : -EIO;

	I2C_Cmd(i2c, DISABLE);

	k_mutex_unlock(&data->bus_mutex);

	if (ret < 0) {
		i2c_bee_log_err(data);
	}

	return ret;
}

static int i2c_bee_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = (I2C_TypeDef *)cfg->reg;
	I2C_InitTypeDef i2c_init_struct;
	int err = 0;

	/* Only support Controller mode for now, since Target API is not implemented */
	if ((dev_config & I2C_MODE_CONTROLLER) == 0) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	I2C_Cmd(i2c, DISABLE);

	I2C_StructInit(&i2c_init_struct);

	i2c_init_struct.I2C_DeviveMode = I2C_DeviveMode_Master;

	if (dev_config & I2C_ADDR_10_BITS) {
		i2c_init_struct.I2C_AddressMode = I2C_AddressMode_10BIT;
	} else {
		i2c_init_struct.I2C_AddressMode = I2C_AddressMode_7BIT;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_init_struct.I2C_ClockSpeed = I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		i2c_init_struct.I2C_ClockSpeed = I2C_BITRATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		i2c_init_struct.I2C_ClockSpeed = I2C_BITRATE_FAST_PLUS;
		break;
	default:
		err = -EINVAL;
		goto error;
	}

	I2C_Init(i2c, &i2c_init_struct);

	I2C_Cmd(i2c, ENABLE);

error:
	k_mutex_unlock(&data->bus_mutex);

	return err;
}

static void i2c_bee_isr(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->reg;

	if (I2C_GetINTStatus(i2c, I2C_INT_TX_ABRT)) {
		data->errs = I2C_CheckAbortStatus(i2c);
		I2C_INTConfig(i2c, I2C_INT_TX_ABRT | I2C_INT_RX_FULL | I2C_INT_TX_EMPTY, DISABLE);
		I2C_ClearINTPendingBit(i2c, I2C_INT_TX_ABRT);
		I2C_ClearINTPendingBit(i2c, I2C_INT_RX_FULL);
		I2C_ClearINTPendingBit(i2c, I2C_INT_TX_EMPTY);
		k_sem_give(&data->sync_sem);
	}

	if (I2C_GetINTStatus(i2c, I2C_INT_RX_FULL)) {
		i2c_bee_do_rx(dev);
		I2C_ClearINTPendingBit(i2c, I2C_INT_RX_FULL);
	}

	if (I2C_GetINTStatus(i2c, I2C_INT_TX_EMPTY)) {
		i2c_bee_do_tx(dev);
		I2C_ClearINTPendingBit(i2c, I2C_INT_TX_EMPTY);
	}

	if (!i2c_bee_next_msg_available(&data->ctx)) {
		I2C_INTConfig(i2c, I2C_INT_TX_ABRT | I2C_INT_RX_FULL | I2C_INT_TX_EMPTY, DISABLE);
		I2C_Cmd(i2c, DISABLE);

		k_sem_give(&data->sync_sem);
	}
}

static int i2c_bee_init(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int err;

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

	k_mutex_init(&data->bus_mutex);

	k_sem_init(&data->sync_sem, 0, K_SEM_MAX_LIMIT);

	cfg->irq_cfg_func();

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);
	i2c_bee_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);

	return 0;
}

static DEVICE_API(i2c, i2c_bee_driver_api) = {
	.configure = i2c_bee_configure,
	.transfer = i2c_bee_transfer,
};

#define I2C_IRQ_FUNC_DEFINE(index)                                                                 \
	static void i2c_bee_irq_cfg_func_##index(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2c_bee_isr,        \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define I2C_BEE_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	I2C_IRQ_FUNC_DEFINE(index);                                                                \
	static struct i2c_bee_data i2c_bee_data_##index;                                           \
	const static struct i2c_bee_config i2c_bee_cfg_##index = {                                 \
		.reg = (I2C_TypeDef *)DT_INST_REG_ADDR(index),                                     \
		.bitrate = DT_INST_PROP(index, clock_frequency),                                   \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_cfg_func = i2c_bee_irq_cfg_func_##index,                                      \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_bee_init, NULL, &i2c_bee_data_##index,                \
				  &i2c_bee_cfg_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
				  &i2c_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_BEE_INIT)
