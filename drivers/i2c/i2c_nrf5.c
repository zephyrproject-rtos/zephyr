/*
 * Copyright (c) 2016-2017 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <i2c.h>
#include <soc.h>
#include <nrf.h>
#include <misc/util.h>
#include <gpio.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

/* @todo
 *
 * Only one instance of twi0 and spi0 may be active at any point in time.
 * Only one instance of twi1, spi1 and spis1 may be active at a time.
 */

#define NRF5_TWI_INT_STOPPED \
	(TWI_INTENSET_STOPPED_Set << TWI_INTENSET_STOPPED_Pos)
#define NRF5_TWI_INT_RXDREADY \
	(TWI_INTENSET_RXDREADY_Set << TWI_INTENSET_RXDREADY_Pos)
#define NRF5_TWI_INT_TXDSENT \
	(TWI_INTENSET_TXDSENT_Set << TWI_INTENSET_TXDSENT_Pos)
#define NRF5_TWI_INT_ERROR \
	(TWI_INTENSET_ERROR_Set << TWI_INTENSET_ERROR_Pos)


struct i2c_nrf5_config {
	volatile NRF_TWI_Type *base;
	void (*irq_config_func)(struct device *dev);
	u32_t default_cfg;
};


struct i2c_nrf5_data {
	struct k_sem sem;
	u32_t rxd:1;
	u32_t txd:1;
	u32_t err:1;
	u32_t stopped:1;
	struct device *gpio;
};


static int i2c_nrf5_configure(struct device *dev, u32_t dev_config_raw)
{
	const struct i2c_nrf5_config *config = dev->config->config_info;
	volatile NRF_TWI_Type *twi = config->base;

	SYS_LOG_DBG("");

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		twi->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K100;
		break;
	case I2C_SPEED_FAST:
		twi->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K400;
		break;
	default:
		SYS_LOG_ERR("unsupported speed");
		return -EINVAL;
	}

	return 0;
}

static int i2c_nrf5_read(struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_nrf5_config *config = dev->config->config_info;
	struct i2c_nrf5_data *data = dev->driver_data;
	volatile NRF_TWI_Type *twi = config->base;

	__ASSERT_NO_MSG(msg->len);

	if (msg->flags & I2C_MSG_RESTART) {
		/* No special behaviour required for
		 * repeated start.
		 */
	}

	for (int offset = 0; offset < msg->len; offset++) {
		if (offset == msg->len-1) {
			SYS_LOG_DBG("SHORTS=2");
			twi->SHORTS = 2; /* BB->STOP */
		} else {
			SYS_LOG_DBG("SHORTS=1");
			twi->SHORTS = 1; /* BB->SUSPEND */
		}

		if (offset == 0) {
			SYS_LOG_DBG("STARTRX");
			twi->TASKS_STARTRX = 1;
		} else {
			SYS_LOG_DBG("RESUME");
			twi->TASKS_RESUME = 1;
		}

		k_sem_take(&data->sem, K_FOREVER);

		if (data->err) {
			data->err = 0;
			SYS_LOG_DBG("rx error 0x%x", twi->ERRORSRC);
			twi->TASKS_STOP = 1;
			twi->ENABLE = TWI_ENABLE_ENABLE_Disabled;
			return -EIO;
		}

		__ASSERT_NO_MSG(data->rxd);

		SYS_LOG_DBG("RXD");
		data->rxd = 0;
		msg->buf[offset] = twi->RXD;
	}

	if (msg->flags & I2C_MSG_STOP) {
		SYS_LOG_DBG("TASK_STOP");
		k_sem_take(&data->sem, K_FOREVER);
		SYS_LOG_DBG("err=%d txd=%d rxd=%d stopped=%d errsrc=0x%x",
			    data->err, data->txd, data->rxd,
			    data->stopped, twi->ERRORSRC);
		__ASSERT_NO_MSG(data->stopped);
		data->stopped = 0;
	}

	return 0;
}

static int i2c_nrf5_write(struct device *dev,
			  struct i2c_msg *msg)
{
	const struct i2c_nrf5_config *config = dev->config->config_info;
	struct i2c_nrf5_data *data = dev->driver_data;
	volatile NRF_TWI_Type *twi = config->base;

	__ASSERT_NO_MSG(msg->len);

	SYS_LOG_DBG("");

	data->stopped = 0;
	data->txd = 0;

	twi->EVENTS_TXDSENT = 0;
	twi->SHORTS = 0;

	for (int offset = 0; offset < msg->len; offset++) {
		SYS_LOG_DBG("txd=0x%x", msg->buf[offset]);
		twi->TXD = msg->buf[offset];

		if (offset == 0) {
			SYS_LOG_DBG("STARTTX");
			twi->TASKS_STARTTX = 1;
		}

		SYS_LOG_DBG("wait for sync");
		k_sem_take(&data->sem, K_FOREVER);
		SYS_LOG_DBG("err=%d txd=%d stopped=%d errsrc=0x%x",
			    data->err, data->txd,
			    data->stopped, twi->ERRORSRC);

		if (data->err) {
			data->err = 0;
			SYS_LOG_ERR("tx error 0x%x",
				    twi->ERRORSRC);
			twi->ERRORSRC = twi->ERRORSRC;
			twi->TASKS_STOP = 1;
			return -EIO;
		}

		__ASSERT_NO_MSG(data->txd);
		data->txd = 0;
		SYS_LOG_DBG("txdsent arrived");
	}

	if (msg->flags & I2C_MSG_STOP) {
		SYS_LOG_DBG("TASK_STOP");
		twi->TASKS_STOP = 1;
		k_sem_take(&data->sem, K_FOREVER);
		SYS_LOG_DBG("err=%d txd=%d rxd=%d stopped=%d errsrc=0x%x",
			    data->err, data->txd, data->rxd,
			    data->stopped, twi->ERRORSRC);
		data->stopped = 0;
	}

	return 0;
}

static int i2c_nrf5_transfer(struct device *dev, struct i2c_msg *msgs,
			     u8_t num_msgs, u16_t addr)
{
	const struct i2c_nrf5_config *config = dev->config->config_info;
	volatile NRF_TWI_Type *twi = config->base;

	SYS_LOG_DBG("transaction-start addr=0x%x", addr);

	/* @todo The NRF5 imposes constraints on which peripherals can
	 * be simultaneously active.  We should take steps here to
	 * enforce appropriate mutual exclusion between SPI, TWI and
	 * SPIS drivers.
	 */

	twi->ENABLE = TWI_ENABLE_ENABLE_Enabled;
	twi->ADDRESS = addr;
	for (int i = 0; i < num_msgs; i++) {
		int r;

		SYS_LOG_DBG("msg len=%d %s%s%s", msgs[i].len,
			    (msgs[i].flags & I2C_MSG_READ) ? "R":"W",
			    (msgs[i].flags & I2C_MSG_STOP) ? "S":"-",
			    (msgs[i].flags & I2C_MSG_RESTART) ? "+":"-");

		if (msgs[i].flags & I2C_MSG_READ) {
			twi->EVENTS_RXDREADY = 0;
			twi->INTENSET = (NRF5_TWI_INT_TXDSENT
					 | NRF5_TWI_INT_RXDREADY
					 | NRF5_TWI_INT_ERROR
					 | NRF5_TWI_INT_STOPPED);
			r = i2c_nrf5_read(dev, msgs + i);
		} else {
			r = i2c_nrf5_write(dev, msgs + i);
		}

		if (r != 0) {
			twi->ENABLE = TWI_ENABLE_ENABLE_Disabled;
			return r;
		}
	}
	twi->ENABLE = TWI_ENABLE_ENABLE_Disabled;

	return 0;
}

static void i2c_nrf5_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct i2c_nrf5_config *config = dev->config->config_info;
	struct i2c_nrf5_data *data = dev->driver_data;
	volatile NRF_TWI_Type *twi = config->base;

	if (twi->EVENTS_RXDREADY) {
		data->rxd = 1;
		twi->EVENTS_RXDREADY = 0;
		k_sem_give(&data->sem);
	}

	if (twi->EVENTS_TXDSENT) {
		data->txd = 1;
		twi->EVENTS_TXDSENT = 0;
		k_sem_give(&data->sem);
	}

	if (twi->EVENTS_ERROR) {
		data->err = 1;
		twi->EVENTS_ERROR = 0;
		k_sem_give(&data->sem);
	}

	if (twi->EVENTS_STOPPED) {
		data->stopped = 1;
		twi->EVENTS_STOPPED = 0;
		k_sem_give(&data->sem);
	}
}

static int i2c_nrf5_init(struct device *dev)
{
	const struct i2c_nrf5_config *config = dev->config->config_info;
	struct i2c_nrf5_data *data = dev->driver_data;
	volatile NRF_TWI_Type *twi = config->base;
	int status;

	SYS_LOG_DBG("");

	data->gpio = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	k_sem_init(&data->sem, 0, UINT_MAX);

	config->irq_config_func(dev);

	twi->ENABLE = TWI_ENABLE_ENABLE_Disabled;

	status = gpio_pin_configure(data->gpio, CONFIG_I2C_NRF5_GPIO_SCL_PIN,
				    GPIO_DIR_IN
				    | GPIO_PUD_PULL_UP
				    | GPIO_DS_DISCONNECT_HIGH);
	__ASSERT_NO_MSG(status == 0);

	status = gpio_pin_configure(data->gpio, CONFIG_I2C_NRF5_GPIO_SCA_PIN,
				    GPIO_DIR_IN
				    | GPIO_PUD_PULL_UP
				    | GPIO_DS_DISCONNECT_HIGH);
	__ASSERT_NO_MSG(status == 0);

	twi->PSELSCL = CONFIG_I2C_NRF5_GPIO_SCL_PIN;
	twi->PSELSDA = CONFIG_I2C_NRF5_GPIO_SCA_PIN;
	twi->ERRORSRC = twi->ERRORSRC;
	twi->EVENTS_TXDSENT = 0;
	twi->EVENTS_RXDREADY = 0;
	twi->EVENTS_ERROR = 0;
	twi->INTENSET = (NRF5_TWI_INT_TXDSENT
			 | NRF5_TWI_INT_RXDREADY
			 | NRF5_TWI_INT_ERROR
			 | NRF5_TWI_INT_STOPPED);

	status = i2c_nrf5_configure(dev, config->default_cfg);
	if (status) {
		return status;
	}

	return 0;
}

static const struct i2c_driver_api i2c_nrf5_driver_api = {
	.configure = i2c_nrf5_configure,
	.transfer = i2c_nrf5_transfer,
};

/* i2c & spi instance with the same id (e.g. I2C_0 and SPI_0) can NOT be used
 * at the same time on nRF5x chip family.
 */
#if defined(CONFIG_I2C_0) && !defined(CONFIG_SPI_0)
static void i2c_nrf5_config_func_0(struct device *dev);

static const struct i2c_nrf5_config i2c_nrf5_config_0 = {
	.base = NRF_TWI0,
	.irq_config_func = i2c_nrf5_config_func_0,
	.default_cfg = CONFIG_I2C_0_DEFAULT_CFG,
};

static struct i2c_nrf5_data i2c_nrf5_data_0;

DEVICE_AND_API_INIT(i2c_nrf5_0, CONFIG_I2C_0_NAME, i2c_nrf5_init,
		    &i2c_nrf5_data_0, &i2c_nrf5_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &i2c_nrf5_driver_api);

static void i2c_nrf5_config_func_0(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_SPI0_TWI0_IRQn, CONFIG_I2C_0_IRQ_PRI,
		    i2c_nrf5_isr, DEVICE_GET(i2c_nrf5_0), 0);

	irq_enable(NRF5_IRQ_SPI0_TWI0_IRQn);
}
#endif /* CONFIG_I2C_0 && !CONFIG_SPI_0 */

#if defined(CONFIG_I2C_1) && !defined(CONFIG_SPI_1)
static void i2c_nrf5_config_func_1(struct device *dev);

static const struct i2c_nrf5_config i2c_nrf5_config_1 = {
	.base = NRF_TWI1,
	.irq_config_func = i2c_nrf5_config_func_1,
	.default_cfg = CONFIG_I2C_1_DEFAULT_CFG,
};

static struct i2c_nrf5_data i2c_nrf5_data_1;

DEVICE_AND_API_INIT(i2c_nrf5_1, CONFIG_I2C_1_NAME, i2c_nrf5_init,
		    &i2c_nrf5_data_1, &i2c_nrf5_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &i2c_nrf5_driver_api);

static void i2c_nrf5_config_func_1(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_SPI1_TWI1_IRQn, CONFIG_I2C_1_IRQ_PRI,
		    i2c_nrf5_isr, DEVICE_GET(i2c_nrf5_1), 0);

	irq_enable(NRF5_IRQ_SPI1_TWI1_IRQn);
}
#endif /* CONFIG_I2C_1 && !CONFIG_SPI_1 */
