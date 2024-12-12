/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_i2c

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <errno.h>
#include <reg/fsmbm.h>

struct i2c_kb1200_config {
	struct fsmbm_regs *fsmbm;
	const struct pinctrl_dev_config *pcfg;
};

struct i2c_kb1200_data {
	struct k_sem mutex;
	volatile uint8_t *msg_buf;
	volatile uint32_t msg_len;
	volatile uint8_t msg_flags;
	volatile int state;
	volatile uint32_t index;
	volatile int err_code;
};

/* I2C Master local functions */
static void i2c_kb1200_isr(const struct device *dev)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;

	if (data->state == STATE_SENDING) {
		if (config->fsmbm->FSMBMPF & FSMBM_BLOCK_FINISH_EVENT) {
			/* continue block */
			uint32_t remain = data->msg_len - data->index;
			uint32_t send_bytes =
				remain > FSMBM_BUFFER_SIZE ? FSMBM_BUFFER_SIZE : remain;
			memcpy((void *)&config->fsmbm->FSMBMDAT[0],
			       (void *)&data->msg_buf[data->index], send_bytes);
			data->index += send_bytes;
			/* Increase CNT setting let hw can't match counter */
			config->fsmbm->FSMBMPRTC_C += send_bytes;
			/* If it was the last protocol recover the correct length value*/
			if (data->msg_len == data->index) {
				config->fsmbm->FSMBMPRTC_C -= 1;
			}
			config->fsmbm->FSMBMPF = FSMBM_BLOCK_FINISH_EVENT;
		} else if (config->fsmbm->FSMBMPF & FSMBM_COMPLETE_EVENT) {
			/* complete */
			if (((config->fsmbm->FSMBMSTS & FSMBM_STS_MASK) == FSMBM_SMBUS_BUSY) &&
			    ((config->fsmbm->FSMBMFRT & ___STOP) == ___NONE)) {
				/* while packet finish without STOP, the error message is
				 * FSMBM_SMBUS_BUSY
				 */
				data->err_code = 0;
			} else {
				data->err_code = config->fsmbm->FSMBMSTS & FSMBM_STS_MASK;
			}
			data->state = STATE_COMPLETE;
			config->fsmbm->FSMBMPF = FSMBM_COMPLETE_EVENT;
		} else {
			data->err_code = config->fsmbm->FSMBMSTS & FSMBM_STS_MASK;
			data->state = STATE_COMPLETE;
		}
	} else if (data->state == STATE_RECEIVING) {
		uint32_t remain = data->msg_len - data->index;
		uint32_t receive_bytes = (remain > FSMBM_BUFFER_SIZE) ? FSMBM_BUFFER_SIZE : remain;

		memcpy((void *)&data->msg_buf[data->index], (void *)&config->fsmbm->FSMBMDAT[0],
		       receive_bytes);
		data->index += receive_bytes;
		if (config->fsmbm->FSMBMPF & FSMBM_BLOCK_FINISH_EVENT) {
			/* continue block */
			/* Check next protocl information */
			remain = data->msg_len - data->index;
			uint32_t NextLen =
				(remain > FSMBM_BUFFER_SIZE) ? FSMBM_BUFFER_SIZE : remain;
			/* Increase CNT setting let hw can't match counter */
			config->fsmbm->FSMBMPRTC_C += NextLen;
			/* If it was the last protocol recover the correct length value */
			if (data->msg_len == (data->index + NextLen)) {
				config->fsmbm->FSMBMPRTC_C -= 1;
			}
			config->fsmbm->FSMBMPF = FSMBM_BLOCK_FINISH_EVENT;
		} else if (config->fsmbm->FSMBMPF & FSMBM_COMPLETE_EVENT) {
			/* complete */
			if (((config->fsmbm->FSMBMSTS & FSMBM_STS_MASK) == FSMBM_SMBUS_BUSY) &&
			    ((config->fsmbm->FSMBMFRT & ___STOP) == ___NONE)) {
				/* while packet finish without STOP, the error message is
				 * FSMBM_SMBUS_BUSY
				 */
				data->err_code = 0;
			} else {
				data->err_code = config->fsmbm->FSMBMSTS & FSMBM_STS_MASK;
			}
			data->state = STATE_COMPLETE;
			config->fsmbm->FSMBMPF = FSMBM_COMPLETE_EVENT;
		} else {
			data->err_code = config->fsmbm->FSMBMSTS & FSMBM_STS_MASK;
			data->state = STATE_COMPLETE;
		}
	} else if (data->state == STATE_COMPLETE) {
		config->fsmbm->FSMBMPF = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	}
}

static int i2c_kb1200_poll_write(const struct device *dev, struct i2c_msg msg, uint16_t addr)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;
	uint8_t send_bytes;

	if (msg.flags & I2C_MSG_STOP) {
		/* No CMD, No CNT, No PEC, with STOP*/
		config->fsmbm->FSMBMFRT = ___STOP;
	} else {
		/* No CMD, No CNT, No PEC, no STOP*/
		config->fsmbm->FSMBMFRT = ___NONE;
	}
	data->msg_len = msg.len;
	data->msg_buf = msg.buf;
	data->msg_flags = msg.flags;
	data->state = STATE_IDLE;
	data->index = 0;
	data->err_code = 0;

	send_bytes = (msg.len > FSMBM_BUFFER_SIZE) ? FSMBM_BUFFER_SIZE : msg.len;
	memcpy((void *)&config->fsmbm->FSMBMDAT[0], (void *)&data->msg_buf[data->index],
	       send_bytes);
	data->index += send_bytes;
	data->state = STATE_SENDING;

	config->fsmbm->FSMBMCMD = 0;
	config->fsmbm->FSMBMADR = (addr & ~BIT(0)) | FSMBM_WRITE;
	config->fsmbm->FSMBMPF = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	/* If data over bufferSize increase 1 to force continue transmit */
	if (msg.len >= (FSMBM_BUFFER_SIZE + 1)) {
		config->fsmbm->FSMBMPRTC_C = FSMBM_BUFFER_SIZE + 1;
	} else {
		config->fsmbm->FSMBMPRTC_C = send_bytes;
	}
	config->fsmbm->FSMBMIE = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	config->fsmbm->FSMBMPRTC_P = FLEXIBLE_PROTOCOL;
	while (data->state != STATE_COMPLETE) {
		;
	}
	data->state = STATE_IDLE;
	if (data->err_code != 0) {
		/* reset HW  */
		config->fsmbm->FSMBMCFG |= FSMBM_HW_RESET;
		return data->err_code;
	}
	return 0;
}

static int i2c_kb1200_poll_read(const struct device *dev, struct i2c_msg msg, uint16_t addr)
{
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;

	if (msg.flags & I2C_MSG_STOP) {
		/* No CMD, No CNT, No PEC, with STOP*/
		config->fsmbm->FSMBMFRT = ___STOP;
	} else {
		/* No CMD, No CNT, No PEC, no STOP*/
		config->fsmbm->FSMBMFRT = ___NONE;
	}
	data->msg_len = msg.len;
	data->msg_buf = msg.buf;
	data->msg_flags = msg.flags;
	data->state = STATE_IDLE;
	data->index = 0;
	data->err_code = 0;
	data->state = STATE_RECEIVING;

	config->fsmbm->FSMBMCMD = 0;
	config->fsmbm->FSMBMADR = (addr & ~BIT(0)) | FSMBM_READ;
	config->fsmbm->FSMBMPF = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	/* If data over bufferSize increase 1 to force continue receive */
	if (msg.len >= (FSMBM_BUFFER_SIZE + 1)) {
		config->fsmbm->FSMBMPRTC_C = FSMBM_BUFFER_SIZE + 1;
	} else {
		config->fsmbm->FSMBMPRTC_C = msg.len;
	}
	config->fsmbm->FSMBMIE = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	config->fsmbm->FSMBMPRTC_P = FLEXIBLE_PROTOCOL;
	while (data->state != STATE_COMPLETE) {
		;
	}
	data->state = STATE_IDLE;
	if (data->err_code != 0) {
		/* reset HW  */
		config->fsmbm->FSMBMCFG |= FSMBM_HW_RESET;
		return data->err_code;
	}
	return 0;
}

/* I2C Master api functions */
static int i2c_kb1200_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_kb1200_config *config = dev->config;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	uint32_t speed = I2C_SPEED_GET(dev_config);

	switch (speed) {
	case I2C_SPEED_STANDARD:
		config->fsmbm->FSMBMCFG = (FSMBM_CLK_100K << FSMBM_CLK_POS);
		break;
	case I2C_SPEED_FAST:
		config->fsmbm->FSMBMCFG = (FSMBM_CLK_400K << FSMBM_CLK_POS);
		break;
	case I2C_SPEED_FAST_PLUS:
		config->fsmbm->FSMBMCFG = (FSMBM_CLK_1M << FSMBM_CLK_POS);
		break;
	default:
		return -EINVAL;
	}

	config->fsmbm->FSMBMPF = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	config->fsmbm->FSMBMIE = (FSMBM_COMPLETE_EVENT | FSMBM_BLOCK_FINISH_EVENT);
	/* HW reset, Enable FSMBM function, Timeout function*/
	config->fsmbm->FSMBMCFG |= FSMBM_HW_RESET | FSMBM_TIMEOUT_ENABLE | FSMBM_FUNCTION_ENABLE;

	return 0;
}

static int i2c_kb1200_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_kb1200_config *config = dev->config;

	if ((config->fsmbm->FSMBMCFG & FSMBM_FUNCTION_ENABLE) == 0x00) {
		printk("Cannot find i2c controller on 0x%p!\n", config->fsmbm);
		return -EIO;
	}

	switch ((config->fsmbm->FSMBMCFG >> FSMBM_CLK_POS) & FSMBM_CLK_MASK) {
	case FSMBM_CLK_100K:
		*dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case FSMBM_CLK_400K:
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

static int i2c_kb1200_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_kb1200_data *data = dev->data;
	int ret;

	/* get the mutex */
	k_sem_take(&data->mutex, K_FOREVER);
	for (int i = 0U; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_kb1200_poll_write(dev, msgs[i], addr);
			if (ret) {
				printk("%s Write error: 0x%X\n", dev->name, ret);
				break;
			}
		} else {
			ret = i2c_kb1200_poll_read(dev, msgs[i], addr);
			if (ret) {
				printk("%s Read error: 0x%X\n", dev->name, ret);
				break;
			}
		}
	}
	/* release the mutex */
	k_sem_give(&data->mutex);

	return ret;
}

/* I2C Master driver registration */
static DEVICE_API(i2c, i2c_kb1200_api) = {
	.configure = i2c_kb1200_configure,
	.get_config = i2c_kb1200_get_config,
	.transfer = i2c_kb1200_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define KB1200_FSMBM_DEV(inst) DEVICE_DT_INST_GET(inst),
static const struct device *const fsmbm_devices[] = {DT_INST_FOREACH_STATUS_OKAY(KB1200_FSMBM_DEV)};
static void i2c_kb1200_isr_wrap(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(fsmbm_devices); i++) {
		const struct device *dev_ = fsmbm_devices[i];
		const struct i2c_kb1200_config *config = dev_->config;

		if (config->fsmbm->FSMBMIE & config->fsmbm->FSMBMPF) {
			i2c_kb1200_isr(dev_);
		}
	}
}

static bool init_irq = true;
static void kb1200_fsmbm_irq_init(void)
{
	if (init_irq) {
		init_irq = false;
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), i2c_kb1200_isr_wrap, NULL,
			    0);
		irq_enable(DT_INST_IRQN(0));
	}
}

static int i2c_kb1200_init(const struct device *dev)
{
	int ret;
	const struct i2c_kb1200_config *config = dev->config;
	struct i2c_kb1200_data *data = dev->data;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	/* init mutex */
	k_sem_init(&data->mutex, 1, 1);
	kb1200_fsmbm_irq_init();

	return 0;
}

#define I2C_KB1200_DEVICE(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct i2c_kb1200_data i2c_kb1200_data_##inst;                                      \
	static const struct i2c_kb1200_config i2c_kb1200_config_##inst = {                         \
		.fsmbm = (struct fsmbm_regs *)DT_INST_REG_ADDR(inst),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(inst, &i2c_kb1200_init, NULL, &i2c_kb1200_data_##inst,           \
			      &i2c_kb1200_config_##inst, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &i2c_kb1200_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_KB1200_DEVICE)
