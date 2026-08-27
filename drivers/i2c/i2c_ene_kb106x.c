/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_i2c

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <errno.h>
#include <reg/fsmbm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ene, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

struct i2c_kb106x_config {
	struct fsmbm_regs *fsmbm;
	uint32_t clock_freq;
	const struct pinctrl_dev_config *pcfg;
};

struct i2c_kb106x_data {
	struct k_sem lock;
	struct k_sem wait;
	volatile uint8_t *msg_buf;
	volatile uint32_t msg_len;
	volatile int state;
	volatile int err_code;
};

/* I2C Master local functions */
static void i2c_kb106x_isr(const struct device *dev)
{
	const struct i2c_kb106x_config *config = dev->config;
	struct i2c_kb106x_data *data = dev->data;

	if (data->state == STATE_SENDING) {
		if (config->fsmbm->fsmbm_pf & FSMBM_BLOCK_FINISH_EVENT) {
			/* continue block */
			uint32_t send_bytes = data->msg_len > FSMBM_BUFFER_SIZE ? FSMBM_BUFFER_SIZE
										: data->msg_len;

			for (uint32_t i = 0; i < send_bytes; i++) {
				config->fsmbm->fsmbm_dat[i] = data->msg_buf[i];
			}
			data->msg_buf += send_bytes;
			data->msg_len -= send_bytes;
			/* Increase CNT setting let hw can't match counter */
			config->fsmbm->fsmbm_prtc_c += send_bytes;
			/* If it was the last protocol recover the correct length value*/
			if (data->msg_len == 0) {
				config->fsmbm->fsmbm_prtc_c -= 1;
			}
			config->fsmbm->fsmbm_pf = FSMBM_BLOCK_FINISH_EVENT;
		} else if (config->fsmbm->fsmbm_pf & FSMBM_COMPLETE_EVENT) {
			/* complete */
			if (((config->fsmbm->fsmbm_sts & FSMBM_STS_MASK) == FSMBM_SMBUS_BUSY) &&
			    ((config->fsmbm->fsmbm_frt & FRT_STOP) == FRT_NONE)) {
				/* while packet finish without STOP, the error message is
				 * FSMBM_SMBUS_BUSY
				 */
				data->err_code = 0;
			} else {
				data->err_code = config->fsmbm->fsmbm_sts & FSMBM_STS_MASK;
			}
			data->state = STATE_COMPLETE;
			k_sem_give(&data->wait);
			config->fsmbm->fsmbm_pf = FSMBM_COMPLETE_EVENT;
		} else {
			data->err_code = config->fsmbm->fsmbm_sts & FSMBM_STS_MASK;
			data->state = STATE_COMPLETE;
			k_sem_give(&data->wait);
		}
	} else if (data->state == STATE_RECEIVING) {
		uint32_t receive_bytes =
			(data->msg_len > FSMBM_BUFFER_SIZE) ? FSMBM_BUFFER_SIZE : data->msg_len;

		for (uint32_t i = 0; i < receive_bytes; i++) {
			data->msg_buf[i] = config->fsmbm->fsmbm_dat[i];
		}
		data->msg_buf += receive_bytes;
		data->msg_len -= receive_bytes;
		if (config->fsmbm->fsmbm_pf & FSMBM_BLOCK_FINISH_EVENT) {
			/* continue block */
			/* Check next protocol information */
			uint32_t next_len = (data->msg_len > FSMBM_BUFFER_SIZE) ? FSMBM_BUFFER_SIZE
										: data->msg_len;
			/* Increase CNT setting let hw can't match counter */
			config->fsmbm->fsmbm_prtc_c += next_len;
			/* If it was the last protocol recover the correct length value */
			if (data->msg_len == next_len) {
				config->fsmbm->fsmbm_prtc_c -= 1;
			}
			config->fsmbm->fsmbm_pf = FSMBM_BLOCK_FINISH_EVENT;
		} else if (config->fsmbm->fsmbm_pf & FSMBM_COMPLETE_EVENT) {
			/* complete */
			if (((config->fsmbm->fsmbm_sts & FSMBM_STS_MASK) == FSMBM_SMBUS_BUSY) &&
			    ((config->fsmbm->fsmbm_frt & FRT_STOP) == FRT_NONE)) {
				/* while packet finish without STOP, the error message is
				 * FSMBM_SMBUS_BUSY
				 */
				data->err_code = 0;
			} else {
				data->err_code = config->fsmbm->fsmbm_sts & FSMBM_STS_MASK;
			}
			data->state = STATE_COMPLETE;
			k_sem_give(&data->wait);
			config->fsmbm->fsmbm_pf = FSMBM_COMPLETE_EVENT;
		} else {
			data->err_code = config->fsmbm->fsmbm_sts & FSMBM_STS_MASK;
			data->state = STATE_COMPLETE;
			k_sem_give(&data->wait);
		}
	} else if (data->state == STATE_COMPLETE) {
		config->fsmbm->fsmbm_pf = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
		k_sem_give(&data->wait);
		data->state = STATE_IDLE;
	} else {
		/* State is idle, nothing to do */
	}
}

static int i2c_kb106x_poll_write(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_kb106x_config *config = dev->config;
	struct i2c_kb106x_data *data = dev->data;
	uint32_t send_bytes;
	int ret;

	k_sem_reset(&data->wait);
	if (i2c_is_stop_op(msg)) {
		/* No CMD, No CNT, No PEC, with STOP*/
		config->fsmbm->fsmbm_frt = FRT_STOP;
	} else {
		/* No CMD, No CNT, No PEC, no STOP*/
		config->fsmbm->fsmbm_frt = FRT_NONE;
	}
	data->msg_len = msg->len;
	data->msg_buf = msg->buf;
	data->state = STATE_IDLE;
	data->err_code = 0;

	send_bytes = (data->msg_len > FSMBM_BUFFER_SIZE) ? FSMBM_BUFFER_SIZE : data->msg_len;
	for (uint32_t i = 0; i < send_bytes; i++) {
		config->fsmbm->fsmbm_dat[i] = data->msg_buf[i];
	}
	data->msg_buf += send_bytes;
	data->msg_len -= send_bytes;
	data->state = STATE_SENDING;

	config->fsmbm->fsmbm_cmd = 0;
	config->fsmbm->fsmbm_adr = (addr << 1) | FSMBM_WRITE;
	config->fsmbm->fsmbm_pf = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	/* If data over bufferSize increase 1 to force continue transmit */
	if (data->msg_len >= (FSMBM_BUFFER_SIZE + 1)) {
		config->fsmbm->fsmbm_prtc_c = FSMBM_BUFFER_SIZE + 1;
	} else {
		config->fsmbm->fsmbm_prtc_c = send_bytes;
	}
	config->fsmbm->fsmbm_ie = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	config->fsmbm->fsmbm_prtc_p = FLEXIBLE_PROTOCOL;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->wait, K_MSEC(FSMBM_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		data->err_code |= FSMBM_SDA_TIMEOUT;
	}
	data->state = STATE_IDLE;
	if (data->err_code) {
		/* reset HW  */
		config->fsmbm->fsmbm_cfg |= FSMBM_HW_RESET;
		return -EIO;
	}

	return 0;
}

static int i2c_kb106x_poll_read(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	const struct i2c_kb106x_config *config = dev->config;
	struct i2c_kb106x_data *data = dev->data;
	int ret;

	k_sem_reset(&data->wait);
	if (i2c_is_restart_op(msg) && !i2c_is_stop_op(msg)) {
		LOG_ERR("%p ENE i2c format not support.", config->fsmbm);
		return -ENOTSUP;
	}
	if (i2c_is_stop_op(msg)) {
		/* No CMD, No CNT, No PEC, with STOP*/
		config->fsmbm->fsmbm_frt = FRT_STOP;
	} else {
		/* No CMD, No CNT, No PEC, no STOP*/
		config->fsmbm->fsmbm_frt = FRT_NONE;
	}
	data->msg_len = msg->len;
	data->msg_buf = msg->buf;
	data->state = STATE_IDLE;
	data->err_code = 0;
	data->state = STATE_RECEIVING;

	config->fsmbm->fsmbm_cmd = 0;
	config->fsmbm->fsmbm_adr = (addr << 1) | FSMBM_READ;
	config->fsmbm->fsmbm_pf = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	/* If data over bufferSize increase 1 to force continue receive */
	if (data->msg_len >= (FSMBM_BUFFER_SIZE + 1)) {
		config->fsmbm->fsmbm_prtc_c = FSMBM_BUFFER_SIZE + 1;
	} else {
		config->fsmbm->fsmbm_prtc_c = data->msg_len;
	}
	config->fsmbm->fsmbm_ie = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	config->fsmbm->fsmbm_prtc_p = FLEXIBLE_PROTOCOL;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->wait, K_MSEC(FSMBM_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		data->err_code |= FSMBM_SDA_TIMEOUT;
	}
	data->state = STATE_IDLE;

	if (data->err_code) {
		/* reset HW  */
		config->fsmbm->fsmbm_cfg |= FSMBM_HW_RESET;
		return -EIO;
	}

	return 0;
}

/* I2C Master api functions */
static int i2c_kb106x_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_kb106x_config *config = dev->config;
	uint32_t speed;

	if ((dev_config & I2C_MODE_CONTROLLER) == 0) {
		return -ENOTSUP;
	}

	if ((dev_config & I2C_ADDR_10_BITS) != 0) {
		return -ENOTSUP;
	}

	speed = I2C_SPEED_GET(dev_config);

	switch (speed) {
	case I2C_SPEED_STANDARD:
		config->fsmbm->fsmbm_cfg = (FSMBM_CLK_100K << FSMBM_CLK_POS);
		break;
	case I2C_SPEED_FAST:
		config->fsmbm->fsmbm_cfg = (FSMBM_CLK_400K << FSMBM_CLK_POS);
		break;
	case I2C_SPEED_FAST_PLUS:
		config->fsmbm->fsmbm_cfg = (FSMBM_CLK_1M << FSMBM_CLK_POS);
		break;
	default:
		return -EINVAL;
	}

	config->fsmbm->fsmbm_pf = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	config->fsmbm->fsmbm_ie = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	/* HW reset, Enable FSMBM function, Timeout function*/
	config->fsmbm->fsmbm_cfg |= FSMBM_HW_RESET | FSMBM_TIMEOUT_ENABLE | FSMBM_FUNCTION_ENABLE;

	return 0;
}

static int i2c_kb106x_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_kb106x_config *config = dev->config;

	if ((config->fsmbm->fsmbm_cfg & FSMBM_FUNCTION_ENABLE) == 0x00) {
		LOG_ERR("Cannot find i2c controller on 0x%p!", config->fsmbm);
		return -EIO;
	}

	switch ((config->fsmbm->fsmbm_cfg >> FSMBM_CLK_POS) & FSMBM_CLK_MASK) {
	case FSMBM_CLK_100K:
		*dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case FSMBM_CLK_400K:
	case FSMBM_CLK_500K:
	case FSMBM_CLK_666K:
		*dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case FSMBM_CLK_1M:
		*dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static int i2c_kb106x_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_kb106x_data *data = dev->data;
	int ret = 0;

	/* lock */
	k_sem_take(&data->lock, K_FOREVER);
	for (int i = 0U; i < num_msgs; i++) {
		if (i2c_is_read_op(&msgs[i])) {
			ret = i2c_kb106x_poll_read(dev, &msgs[i], addr);
			if (ret) {
				break;
			}
		} else {
			ret = i2c_kb106x_poll_write(dev, &msgs[i], addr);
			if (ret) {
				break;
			}
		}
	}
	/* release the lock */
	k_sem_give(&data->lock);

	return ret;
}

/* I2C Master driver registration */
static DEVICE_API(i2c, i2c_kb106x_api) = {
	.configure = i2c_kb106x_configure,
	.get_config = i2c_kb106x_get_config,
	.transfer = i2c_kb106x_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define KB106X_FSMBM_DEV(inst) DEVICE_DT_INST_GET(inst),
static void i2c_kb106x_isr_wrap(const void *unused)
{
	static const struct device *const fsmbm_devices[] = {
		DT_INST_FOREACH_STATUS_OKAY(KB106X_FSMBM_DEV)};
	ARG_UNUSED(unused);

	for (size_t i = 0; i < ARRAY_SIZE(fsmbm_devices); i++) {
		const struct device *dev_ = fsmbm_devices[i];
		const struct i2c_kb106x_config *config = dev_->config;

		if (config->fsmbm->fsmbm_ie & config->fsmbm->fsmbm_pf) {
			i2c_kb106x_isr(dev_);
		}
	}
}

static int i2c_kb106x_init(const struct device *dev)
{
	int ret;
	const struct i2c_kb106x_config *config = dev->config;
	struct i2c_kb106x_data *data = dev->data;
	uint32_t bitrate_cfg;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	bitrate_cfg = i2c_map_dt_bitrate(config->clock_freq);
	if (!bitrate_cfg) {
		return -EINVAL;
	}

	i2c_kb106x_configure(dev, bitrate_cfg | I2C_MODE_CONTROLLER);

	k_sem_init(&data->wait, 0, 1);
	/* init lock */
	k_sem_init(&data->lock, 1, 1);

	if (dev == DEVICE_DT_INST_GET(0)) {
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), i2c_kb106x_isr_wrap, NULL,
			    0);
		irq_enable(DT_INST_IRQN(0));
	}

	return 0;
}

#define I2C_KB106X_DEVICE(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct i2c_kb106x_data i2c_kb106x_data_##inst;                                      \
	static const struct i2c_kb106x_config i2c_kb106x_config_##inst = {                         \
		.fsmbm = (struct fsmbm_regs *)DT_INST_REG_ADDR(inst),                              \
		.clock_freq = DT_INST_PROP(inst, clock_frequency),                                 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_kb106x_init, NULL, &i2c_kb106x_data_##inst,            \
				  &i2c_kb106x_config_##inst, POST_KERNEL,                          \
				  CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &i2c_kb106x_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_KB106X_DEVICE)
