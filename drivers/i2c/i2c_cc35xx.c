/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_i2c

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#include <inc/hw_memmap.h>
#include <driverlib/i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_cc35xx);

struct i2c_cc35xx_config {
	uint32_t base;
	uint32_t sys_clk_freq;
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

struct i2c_cc35xx_data {
	struct k_mutex mutex;
	struct k_sem i2c_msg_done;
	uint8_t *buf;
	int buflen;
	int bufpos;
	int bus_error;
	int flags;
};

static int i2c_cc35xx_configure(const struct device *dev, uint32_t dev_config_raw)
{
	const struct i2c_cc35xx_config *config = dev->config;
	struct i2c_cc35xx_data *data = dev->data;
	uint32_t mode;

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		mode = I2C_MODE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		mode = I2C_MODE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		mode = I2C_MODE_FAST_PLUS;
		break;
	default:
		return -EIO;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	I2CControllerDisable(config->base);
	I2CControllerInit(config->base, I2C_CONTROLLER_CONFIG_CLOCK_STRETCHING_DETECTION, mode);
	k_mutex_unlock(&data->mutex);

	return 0;
}

/**
 * Transmit data from internal #dev buffer. Function will put as much
 * data as it can onto i2c fifo buffer and will return once buffer is
 * full or there is no more data to transfer. Function will modify,
 * buffer state in #dev, so function can be called multiple times until
 * all data are transferred out.
 *
 * @return 1 when all data within #dev internal state machine is sent out
 * @return 0 when there is still data in #dev buffers to be sent out. You can
 *         call this again to transmit what is left.
 */
static int i2c_cc35xx_fifo_put(const struct device *dev)
{
	const struct i2c_cc35xx_config *config = dev->config;
	struct i2c_cc35xx_data *data = dev->data;

	for (; data->bufpos < data->buflen; data->bufpos++) {
		if (I2CPutDataNonBlocking(config->base, data->buf[data->bufpos]) == 0) {
			break;
		}
	}

	return data->bufpos == data->buflen;
}

/**
 * Same principle as with i2c_cc35xx_fifo_put but is used to read data from
 * i2c receive buffer
 */
static int i2c_cc35xx_fifo_get(const struct device *dev)
{
	const struct i2c_cc35xx_config *config = dev->config;
	struct i2c_cc35xx_data *data = dev->data;

	for (; data->bufpos < data->buflen; data->bufpos++) {
		if (I2CGetDataNonBlocking(config->base, &data->buf[data->bufpos]) == 0) {
			break;
		}
	}

	return data->bufpos == data->buflen;
}

/**
 * Start single i2c transfer. Function will prepare buffer to receive/send
 * data, enable interrupts, and will wait until i2c transfer completes. Once
 * transfer is started and function waits for semaphore, all logic happens
 * via interrupts at that point.
 *
 * @return 0 transfer was completed without error
 * @return -EIO error during transfer like not getting ACK from peripheral.
 * @return -ETIMEDOUT is returned when we don't get interrupt from i2c after
 *         transfer is started
 */
static int i2c_cc35xx_prime_transfer(const struct device *dev, uint8_t *buf, uint32_t len,
				     uint16_t addr, int flags)
{
	struct i2c_cc35xx_data *data = dev->data;
	const struct i2c_cc35xx_config *config = dev->config;
	bool is_read = flags & I2C_MSG_READ;
	bool is_start = flags & I2C_MSG_RESTART;
	uint32_t cmd = 0;
	uint32_t direction = is_read ? I2C_CONTROLLER_DIR_RECEIVE : I2C_CONTROLLER_DIR_TRANSMIT;
	uint32_t addr_mode = flags & I2C_MSG_ADDR_10_BITS ? I2C_CONTROLLER_ADDR_MODE_10_BIT
							  : I2C_CONTROLLER_ADDR_MODE_7_BIT;
	uint32_t enable_interrupt =
		I2C_CONTROLLER_INT_ARB_LOST | I2C_CONTROLLER_INT_STOP | I2C_CONTROLLER_INT_NACK;
	int st;

	data->buf = buf;
	data->buflen = len;
	data->bufpos = 0;
	data->bus_error = 0;
	data->flags = flags;

	if (is_read) {
		cmd = is_start ? I2C_CONTROLLER_CMD_BURST_RECEIVE_START
			       : I2C_CONTROLLER_CMD_BURST_RECEIVE_CONT;
	} else {
		cmd = is_start ? I2C_CONTROLLER_CMD_BURST_SEND_START
			       : I2C_CONTROLLER_CMD_BURST_SEND_CONT;
	}
	if (is_start && len == 0) {
		cmd = I2C_CONTROLLER_CMD_SINGLE_SEND;
	}

	if (is_read) {
		enable_interrupt |= I2C_CONTROLLER_INT_RX_FIFO_TRIGGER | I2C_CONTROLLER_INT_RX_DONE;
	} else {
		i2c_cc35xx_fifo_put(dev);
		enable_interrupt |= I2C_CONTROLLER_INT_TX_FIFO_TRIGGER | I2C_CONTROLLER_INT_TX_DONE;
	}

	I2CEnableInt(config->base, enable_interrupt);
	I2CControllerSetTargetAddr(config->base, addr_mode, addr, direction);
	I2CControllerSetCommand(config->base, cmd, data->buflen);
	/*
	 * TRM §19.4: after issuing CCTR, "Wait until the transmission
	 * completes by polling the CSR register's BUSY bit until it has
	 * been cleared." ~500 us settle is required here for the
	 * controller FSM to progress before the event IRQ takes over.
	 */
	k_busy_wait(500);
	st = k_sem_take(&data->i2c_msg_done, K_MSEC(100));
	if (st) {
		return -ETIMEDOUT;
	}

	return data->bus_error ? -EIO : 0;
}

static int i2c_cc35xx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_cc35xx_data *data = dev->data;
	const struct i2c_cc35xx_config *config = dev->config;
	struct i2c_msg *msg;
	int ret = 0, retries, flags;

	if (I2CControllerIsBusy(config->base)) {
		return -EBUSY;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	k_sem_reset(&data->i2c_msg_done);

	I2CFlushFifos(config->base);
	I2CClearInt(config->base, I2C_INT_ALL); /* 10ms match swuart print delay */

	for (int i = 0; i < num_msgs; i++) {
		msg = msgs + i;
		if (msg->len > I2C_CONTROLLER_TRANSACTION_LENGTH_MAX) {
			LOG_ERR("Single transaction cannot be larger than %d\n",
				I2C_CONTROLLER_TRANSACTION_LENGTH_MAX);
			return -EINVAL;
		}

		flags = msg->flags;
		if (i == 0) {
			flags |= I2C_MSG_RESTART;
		}
		if (i == (num_msgs - 1)) {
			flags |= I2C_MSG_STOP;
		}

		for (retries = 3; retries; retries--) {
			ret = i2c_cc35xx_prime_transfer(dev, msg->buf, msg->len, addr, flags);
			if (ret != -ETIMEDOUT) {
				break;
			}
		}

		if (ret != 0) {
			goto error;
		}
	}

error:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static void i2c_cc35xx_isr(const struct device *dev)
{
	struct i2c_cc35xx_data *data = dev->data;
	const struct i2c_cc35xx_config *config = dev->config;
	uint32_t int_status;
	uint32_t tx_fifo_int = I2C_CONTROLLER_INT_TX_FIFO_TRIGGER;
	uint32_t rx_fifo_int = I2C_CONTROLLER_INT_RX_FIFO_TRIGGER;
	uint32_t finish_int =
		I2C_CONTROLLER_INT_STOP | I2C_CONTROLLER_INT_TX_DONE | I2C_CONTROLLER_INT_RX_DONE;
	uint32_t i2c_error = I2CControllerGetError(config->base);

	if (i2c_error) {
		if (!(i2c_error & I2C_CONTROLLER_ERR_ARB_LOST)) {
			I2CControllerSetCommand(config->base, I2C_CONTROLLER_CMD_BURST_FINISH,
						I2C_CONTROLLER_TRANSACTION_LENGTH_NONE);
		}
		data->bus_error = 1;
		I2CDisableInt(config->base, I2C_INT_ALL);
		I2CClearInt(config->base, I2C_INT_ALL);

		k_sem_give(&data->i2c_msg_done);

		return;
	}

	while ((int_status = I2CIntStatus(config->base, true))) {
		if (int_status & tx_fifo_int) {
			if (i2c_cc35xx_fifo_put(dev)) {
				I2CDisableInt(config->base, I2C_CONTROLLER_INT_TX_FIFO_TRIGGER);
			}

			I2CClearInt(config->base, tx_fifo_int);
		} else if (int_status & rx_fifo_int) {
			if (i2c_cc35xx_fifo_get(dev)) {
				I2CDisableInt(config->base, I2C_CONTROLLER_INT_RX_FIFO_TRIGGER);
			}

			I2CClearInt(config->base, rx_fifo_int);
		} else if (int_status & finish_int) {
			if (int_status & I2C_CONTROLLER_INT_RX_DONE) {
				i2c_cc35xx_fifo_get(dev);
			}

			data->bus_error = 0;
			I2CDisableInt(config->base, I2C_INT_ALL);
			I2CClearInt(config->base, I2C_INT_ALL);
			if (data->flags & I2C_MSG_STOP) {
				I2CControllerSetCommand(config->base,
							I2C_CONTROLLER_CMD_BURST_FINISH,
							I2C_CONTROLLER_TRANSACTION_LENGTH_NONE);
			}

			k_sem_give(&data->i2c_msg_done);
		} else {
			__ASSERT(false, "Unhandled I2C Interrupt");
			data->bus_error = 1;
			I2CControllerSetCommand(config->base, I2C_CONTROLLER_CMD_BURST_FINISH,
						I2C_CONTROLLER_TRANSACTION_LENGTH_NONE);
			I2CDisableInt(config->base, I2C_INT_ALL);
			I2CClearInt(config->base, I2C_INT_ALL);

			k_sem_give(&data->i2c_msg_done);
		}
	}
}

static int i2c_cc35xx_init(const struct device *dev)
{
	const struct i2c_cc35xx_config *config = dev->config;
	struct i2c_cc35xx_data *data = dev->data;
	int error;

	error = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (error < 0) {
		return error;
	}

	k_mutex_init(&data->mutex);
	k_sem_init(&data->i2c_msg_done, 0, 1);

	sys_write32(I2C_CLKCFG_ENABLE_EN, config->base + I2C_O_CLKCFG);
	I2CControllerDisable(config->base);
	I2CDisableInt(config->base, I2C_INT_ALL);
	config->irq_config_func();
	I2CFlushFifos(config->base);
	I2CSetTxFifoTrigger(config->base, I2C_TX_FIFO_SIZE / 2);
	I2CSetRxFifoTrigger(config->base, I2C_RX_FIFO_SIZE / 2);
	I2CControllerInit(config->base, I2C_CONTROLLER_CONFIG_CLOCK_STRETCHING_DETECTION,
			  I2C_MODE_STANDARD);
	I2CClearInt(config->base, I2C_INT_ALL);

	return 0;
}

static DEVICE_API(i2c, i2c_cc35xx_driver_api) = {
	.configure = i2c_cc35xx_configure,
	.transfer = i2c_cc35xx_transfer,
};

#define I2C_CC35XX_DEVICE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2c_cc35xx_irq_config_##n(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_cc35xx_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct i2c_cc35xx_data i2c_cc35xx_data_##n;                                         \
	static const struct i2c_cc35xx_config i2c_cc35xx_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = i2c_cc35xx_irq_config_##n,                                      \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_cc35xx_init, NULL, &i2c_cc35xx_data_##n,                  \
				  &i2c_cc35xx_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
				  &i2c_cc35xx_driver_api)

DT_INST_FOREACH_STATUS_OKAY(I2C_CC35XX_DEVICE);
