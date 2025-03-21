/*
 * Copyright (c) 2025 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_i2c

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sy1xx_i2c, CONFIG_I2C_LOG_LEVEL);

#include <soc.h>
#include <udma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2c.h>

/** cmds for the udma of i2c */
#define SY1XX_I2C_CMD_OFFSET  4
#define SY1XX_I2C_CMD_START   (0x0 << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_STOP    (0x2 << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_RD_ACK  (0x4 << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_RD_NACK (0x6 << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_WR      (0x8 << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_WAIT    (0xA << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_RPT     (0xC << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_CFG     (0xE << SY1XX_I2C_CMD_OFFSET)
#define SY1XX_I2C_CMD_WAIT_EV (0x1 << SY1XX_I2C_CMD_OFFSET)

#define SY1XX_I2C_ADDR_WRITE (0x0)
#define SY1XX_I2C_ADDR_READ  (0x1)

enum sy1xx_i2c_speeds {
	SY1XX_I2C_SPEED_STANDARD = 100000,
	SY1XX_I2C_SPEED_FAST = 400000,
	SY1XX_I2C_SPEED_FAST_PLUS = 1000000,
	SY1XX_I2C_SPEED_HIGH = 3400000,
	SY1XX_I2C_SPEED_ULTRA = 5000000,
};

#define DEVICE_MAX_BUFFER_SIZE (512)

enum sy1xx_i2c_mode {
	UNDEF,
	READ,
	WRITE,
};

struct sy1xx_i2c_dev_config {
	uint32_t base;
	uint32_t inst;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
};

struct sy1xx_i2c_dev_data {
	struct k_sem lock;

	bool error_active;
	uint32_t bitrate;

	uint8_t write[DEVICE_MAX_BUFFER_SIZE];
	uint8_t read[DEVICE_MAX_BUFFER_SIZE];
};

static void sy1xx_i2c_ctrl_init(const struct device *dev)
{
	const struct sy1xx_i2c_dev_config *const cfg = dev->config;
	struct sy1xx_i2c_dev_data *const data = dev->data;
	uint16_t divider;
	uint32_t idx;
	uint8_t *buf;

	k_sem_take(&data->lock, K_FOREVER);

	/* reset the i2c controller */
	SY1XX_UDMA_WRITE_REG(cfg->base, SY1XX_UDMA_SETUP_REG, 0x1);
	k_sleep(K_MSEC(10));
	SY1XX_UDMA_WRITE_REG(cfg->base, SY1XX_UDMA_SETUP_REG, 0x0);
	k_sleep(K_MSEC(10));

	/* prepare udma transfer buffer */
	buf = data->write;
	idx = 0;

	/* fixed pre-scaler 1:5 */
	divider = (sy1xx_soc_get_peripheral_clock() / 5) / data->bitrate;
	buf[idx++] = SY1XX_I2C_CMD_CFG;
	buf[idx++] = (divider & 0xff00) >> 8;
	buf[idx++] = (divider & 0x00ff);

	SY1XX_UDMA_START_RX(cfg->base, (uint32_t)data->read, 3, 0);
	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)buf, idx, 0);

	/* wait for udma run empty */
	k_sleep(K_MSEC(1));

	/* reset udma channels */
	SY1XX_UDMA_CANCEL_RX(cfg->base);
	SY1XX_UDMA_CANCEL_TX(cfg->base);

	data->error_active = false;

	k_sem_give(&data->lock);
}

static int sy1xx_i2c_configure(const struct device *dev, uint32_t flags)
{
	const struct sy1xx_i2c_dev_config *const cfg = dev->config;
	struct sy1xx_i2c_dev_data *const data = dev->data;

	if (!(flags & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Master Mode is required");
		return -EIO;
	}

	if (flags & I2C_ADDR_10_BITS) {
		LOG_ERR("I2C 10-bit addressing is currently not supported");
		LOG_ERR("Please submit a patch");
		return -EIO;
	}

	/* Configure clock */
	switch (I2C_SPEED_GET(flags)) {
	case I2C_SPEED_STANDARD:
		data->bitrate = SY1XX_I2C_SPEED_STANDARD;
		break;
	case I2C_SPEED_FAST:
		data->bitrate = SY1XX_I2C_SPEED_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->bitrate = SY1XX_I2C_SPEED_FAST_PLUS;
		break;
	case I2C_SPEED_DT:
		data->bitrate = cfg->clock_frequency;
		break;

	default:
		LOG_ERR("Unsupported I2C speed value");
		return -EIO;
	}

	sy1xx_i2c_ctrl_init(dev);

	return 0;
}

static int sy1xx_i2c_initialize(const struct device *dev)
{
	const struct sy1xx_i2c_dev_config *const cfg = dev->config;
	struct sy1xx_i2c_dev_data *const data = dev->data;
	int ret;
	uint32_t flags;

	/* UDMA clock enable */
	sy1xx_udma_enable_clock(SY1XX_UDMA_MODULE_I2C, cfg->inst);

	/* PAD config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);

	/* check if DT has bitrate preset */
	flags = I2C_MODE_CONTROLLER;
	if (cfg->clock_frequency > 0) {
		flags |= I2C_SPEED_SET(I2C_SPEED_DT);
	} else {
		flags |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
	}

	return sy1xx_i2c_configure(dev, flags);
}

/**
 *
 * reading expects to receive all line data from i2c lines;
 * so we have to wait until rx fifo fully empty, so
 * we add wait states to the second queue and wait
 * for the switch to second queue - which in that case indicates
 * that reading (of first queue) is complete.
 * then the first queue can immediately take the next transfer
 * and so on.
 *
 */
static int sy1xx_i2c_read(const struct device *dev, struct i2c_msg *msg, uint16_t addr, bool start,
			  bool stop)
{
	const struct sy1xx_i2c_dev_config *const cfg = dev->config;
	struct sy1xx_i2c_dev_data *const data = dev->data;
	int ret;
	uint32_t idx;
	uint8_t *buf;
	uint8_t *wait;
	uint32_t wait_cnt;

	/* prepare udma transfer buffer */
	buf = data->write;
	idx = 0;

	if (start) {
		buf[idx++] = SY1XX_I2C_CMD_START;
		buf[idx++] = SY1XX_I2C_CMD_WR;
		buf[idx++] = ((addr & 0x7F) << 1) | SY1XX_I2C_ADDR_READ;
	}

	if (msg->len > 1) {
		/* repeat byte reads incl. ackn */
		buf[idx++] = SY1XX_I2C_CMD_RPT;
		buf[idx++] = msg->len - 1;
		buf[idx++] = SY1XX_I2C_CMD_RD_ACK;
	}

	/* last read without ack */
	buf[idx++] = SY1XX_I2C_CMD_RD_NACK;

	if (stop) {
		buf[idx++] = SY1XX_I2C_CMD_STOP;
	} else {
		/* add wait cycle for potentially restart condition */
		buf[idx++] = SY1XX_I2C_CMD_WAIT;
		buf[idx++] = 1;
	}

	/* fill 1st fifo queue with reading cmds */
	SY1XX_UDMA_START_RX(cfg->base, (uint32_t)data->read, msg->len, 0);
	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)buf, idx, 0);

	/* fill 2nd fifo queue with one waiting cycle */
	wait = &buf[idx];
	wait_cnt = 0;
	wait[wait_cnt++] = SY1XX_I2C_CMD_WAIT;
	wait[wait_cnt++] = 1;

	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)wait, wait_cnt, 0);

	/* finally wait for the switch from 1st to 2nd queue */
	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);
	SY1XX_UDMA_WAIT_FOR_FINISHED_RX(cfg->base);

	/* make sure all is transferred to fifo */
	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		LOG_ERR("filling fifo failed");
		return -EINVAL;
	}

	if (SY1XX_UDMA_GET_REMAINING_RX(cfg->base)) {
		LOG_ERR("missing read bytes, %d bytes left",
			SY1XX_UDMA_GET_REMAINING_RX(cfg->base));
		return -EINVAL;
	}

	/* copy data back to msg */
	memcpy(msg->buf, data->read, msg->len);

	return 0;
}

/**
 *
 * we just fill the outgoing tx fifo of i2c controller;
 * after leaving this routine, not all bytes may have left the controller to the i2c lines
 *
 * filling the fifo is done by dma transfer to one of the two available queues
 *
 */
static int sy1xx_i2c_write(const struct device *dev, struct i2c_msg *msg, uint16_t addr, bool start,
			   bool stop)
{
	const struct sy1xx_i2c_dev_config *const cfg = dev->config;
	struct sy1xx_i2c_dev_data *const data = dev->data;
	int ret;
	uint32_t idx;
	uint8_t *buf;
	uint8_t *wait;

	/* prepare udma transfer buffer */
	buf = data->write;
	idx = 0;

	if (start) {
		buf[idx++] = SY1XX_I2C_CMD_START;
		buf[idx++] = SY1XX_I2C_CMD_WR;
		buf[idx++] = ((addr & 0x7F) << 1) | SY1XX_I2C_ADDR_WRITE;
	}

	if (msg->len) {
		/* repeat byte write for all given data */
		buf[idx++] = SY1XX_I2C_CMD_RPT;
		buf[idx++] = msg->len;
		buf[idx++] = SY1XX_I2C_CMD_WR;

		/* add data */
		for (uint32_t i = 0; i < msg->len; i++) {
			buf[idx++] = msg->buf[i];
		}
	}

	if (stop) {
		buf[idx++] = SY1XX_I2C_CMD_STOP;
	} else {
		/* add wait cycle for potentially restart condition */
		buf[idx++] = SY1XX_I2C_CMD_WAIT;
		buf[idx++] = 1;
	}

	/* fill next tx fifo queue */
	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)buf, idx, 0);

	/* wait for udma has filled i2c controller tx fifo */
	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);

	/* make sure all is transferred to fifo */
	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		LOG_ERR("filling fifo failed");
		return -EINVAL;
	}

	return 0;
}

static int sy1xx_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	struct sy1xx_i2c_dev_data *const data = dev->data;
	int ret;
	bool start_cond, stop_cond;
	enum sy1xx_i2c_mode prv_xfer, cur_xfer;

	if (num_msgs < 0) {
		return 0;
	}

	if (data->error_active) {
		sy1xx_i2c_ctrl_init(dev);
	}

	k_sem_take(&data->lock, K_FOREVER);

	prv_xfer = UNDEF;
	for (uint32_t n = 0; n < num_msgs; n++) {

		/* detect transfer type */
		if ((msgs[n].flags & I2C_MSG_READ) == I2C_MSG_READ) {
			/* read msg */
			cur_xfer = READ;
		} else {
			/* write msg */
			cur_xfer = WRITE;
		}

		/* transfer switched from read to write; or vice versa */
		if (prv_xfer != cur_xfer) {
			prv_xfer = cur_xfer;
			start_cond = true;
		} else {
			start_cond = false;
		}

		/* stop on last msg */
		if (n == (num_msgs - 1)) {
			stop_cond = true;
		} else {
			stop_cond = false;
		}

		if (cur_xfer == READ) {
			ret = sy1xx_i2c_read(dev, &msgs[n], addr, start_cond, stop_cond);
		} else {
			ret = sy1xx_i2c_write(dev, &msgs[n], addr, start_cond, stop_cond);
		}

		if (ret) {
			data->error_active = true;
			break;
		}
	}

	k_sem_give(&data->lock);

	return ret;
}

static DEVICE_API(i2c, sy1xx_i2c_driver_api) = {
	.configure = sy1xx_i2c_configure,
	.transfer = sy1xx_i2c_transfer,
};

#define SY1XX_I2C_INIT(n)                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct sy1xx_i2c_dev_config sy1xx_i2c_dev_config_##n = {                      \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.inst = DT_INST_PROP(n, instance),                                                 \
		.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 0),                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
	static struct sy1xx_i2c_dev_data __attribute__((section(".udma_access")))                  \
	__aligned(4) sy1xx_i2c_dev_data_##n = {};                                                  \
	I2C_DEVICE_DT_INST_DEFINE(n, sy1xx_i2c_initialize, NULL, &sy1xx_i2c_dev_data_##n,          \
				  &sy1xx_i2c_dev_config_##n, POST_KERNEL,                          \
				  CONFIG_I2C_INIT_PRIORITY, &sy1xx_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SY1XX_I2C_INIT)
