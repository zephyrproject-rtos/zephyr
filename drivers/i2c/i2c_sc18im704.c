/*
 * Copyright (c), 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sc18im704_i2c

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_sc18im, CONFIG_I2C_LOG_LEVEL);

#include "i2c_sc18im704.h"

struct i2c_sc18im_config {
	const struct device *bus;
	uint32_t bus_speed;
	const struct gpio_dt_spec reset_gpios;
};

struct i2c_sc18im_data {
	struct k_mutex lock;
	uint32_t i2c_config;
};

int sc18im704_claim(const struct device *dev)
{
	struct i2c_sc18im_data *data = dev->data;

	return k_mutex_lock(&data->lock, K_FOREVER);
}

int sc18im704_release(const struct device *dev)
{
	struct i2c_sc18im_data *data = dev->data;

	return k_mutex_unlock(&data->lock);
}

int sc18im704_transfer(const struct device *dev,
		       const uint8_t *tx_data, uint8_t tx_len,
		       uint8_t *rx_data, uint8_t rx_len)
{
	const struct i2c_sc18im_config *cfg = dev->config;
	struct i2c_sc18im_data *data = dev->data;
	int ret = 0;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	if (tx_data != NULL) {
		for (uint8_t i = 0; i < tx_len; ++i)  {
			uart_poll_out(cfg->bus, tx_data[i]);
		}
	}

	if (rx_data != NULL) {
		k_timepoint_t end;

		for (uint8_t i = 0; i < rx_len && ret == 0; ++i)  {
			/* Make sure we don't wait forever */
			end = sys_timepoint_calc(K_SECONDS(1));

			do {
				ret = uart_poll_in(cfg->bus, &rx_data[i]);
			} while (ret == -1 && !sys_timepoint_expired(end));
		}

		/* -1 indicates we timed out */
		ret = ret == -1 ? -EAGAIN : ret;

		if (ret < 0) {
			LOG_ERR("Failed to read data (%d)", ret);
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int i2c_sc18im_configure(const struct device *dev, uint32_t config)
{
	struct i2c_sc18im_data *data = dev->data;

	if (!(I2C_MODE_CONTROLLER & config)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & config) {
		return -EINVAL;
	}

	if (I2C_SPEED_GET(config) != I2C_SPEED_GET(data->i2c_config)) {
		uint8_t buf[] = {
			SC18IM704_CMD_WRITE_REG,
			SC18IM704_REG_I2C_CLK_L,
			0,
			SC18IM704_CMD_STOP,
		};
		int ret;

		/* CLK value is calculated as 15000000 / (8 * freq), see datasheet */
		switch (I2C_SPEED_GET(config)) {
		case I2C_SPEED_STANDARD:
			buf[2] = 0x13; /* 99 kHz */
			break;
		case I2C_SPEED_FAST:
			buf[2] = 0x05; /* 375 kHz */
			break;
		default:
			return -EINVAL;
		}

		ret = sc18im704_transfer(dev, buf, sizeof(buf), NULL, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set I2C speed (%d)", ret);
			return -EIO;
		}
	}

	data->i2c_config = config;

	return 0;
}

static int i2c_sc18im_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_sc18im_data *data = dev->data;

	*config = data->i2c_config;

	return 0;
}

static int i2c_sc18im_transfer_msg(const struct device *dev,
				   struct i2c_msg *msg,
				   uint16_t addr)
{
	uint8_t start[] = {
		SC18IM704_CMD_I2C_START,
		0x00,
		0x00,
	};
	uint8_t stop = SC18IM704_CMD_STOP;
	int ret;

	if (msg->flags & I2C_MSG_ADDR_10_BITS || msg->len > UINT8_MAX) {
		return -EINVAL;
	}

	start[1] = addr | (msg->flags & I2C_MSG_RW_MASK);
	start[2] = msg->len;

	ret = sc18im704_transfer(dev, start, sizeof(start), NULL, 0);
	if (ret < 0) {
		return ret;
	}

	if (msg->flags & I2C_MSG_READ) {
		/* Send the stop character before reading */
		ret = sc18im704_transfer(dev, &stop, 1, msg->buf, msg->len);
		if (ret < 0) {
			return ret;
		}

	} else {
		ret = sc18im704_transfer(dev, msg->buf, msg->len, NULL, 0);
		if (ret < 0) {
			return ret;
		}

		if (msg->flags & I2C_MSG_STOP) {
			ret = sc18im704_transfer(dev, &stop, 1, NULL, 0);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int i2c_sc18im_transfer(const struct device *dev,
			       struct i2c_msg *msgs,
			       uint8_t num_msgs, uint16_t addr)
{
	int ret;

	if (num_msgs == 0) {
		return 0;
	}

	ret = sc18im704_claim(dev);
	if (ret < 0) {
		LOG_ERR("Failed to claim I2C bridge (%d)", ret);
		return ret;
	}

	for (uint8_t i = 0; i < num_msgs && ret == 0; ++i) {
		ret = i2c_sc18im_transfer_msg(dev, &msgs[i], addr);
	}

#ifdef CONFIG_I2C_SC18IM704_VERIFY
	if (ret == 0) {
		uint8_t buf[] = {
			SC18IM704_CMD_READ_REG,
			SC18IM704_REG_I2C_STAT,
			SC18IM704_CMD_STOP,
		};
		uint8_t data;

		ret = sc18im704_transfer(dev, buf, sizeof(buf), &data, 1);

		if (ret == 0 && data != SC18IM704_I2C_STAT_OK) {
			ret = -EIO;
		}
	}
#endif /* CONFIG_I2C_SC18IM704_VERIFY */

	sc18im704_release(dev);

	return ret;
}

static int i2c_sc18im_init(const struct device *dev)
{
	const struct i2c_sc18im_config *cfg = dev->config;
	struct i2c_sc18im_data *data = dev->data;
	int ret;

	/* The device baudrate after reset is 9600 */
	struct uart_config uart_cfg = {
		.baudrate = 9600,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("UART bus not ready");
		return -ENODEV;
	}

	ret = uart_configure(cfg->bus, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure UART (%d)", ret);
		return ret;
	}

	if (cfg->reset_gpios.port) {
		uint8_t buf[2];

		if (!gpio_is_ready_dt(&cfg->reset_gpios)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpios, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&cfg->reset_gpios, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set reset GPIO (%d)", ret);
			return ret;
		}

		/* The device sends "OK" */
		ret = sc18im704_transfer(dev, NULL, 0, buf, sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to get OK (%d)", ret);
			return ret;
		}
	}

	if (cfg->bus_speed != 9600) {
		uint16_t brg = (7372800 / cfg->bus_speed) - 16;
		uint8_t buf[] = {
			SC18IM704_CMD_WRITE_REG,
			SC18IM704_REG_BRG0,
			brg & 0xff,
			SC18IM704_REG_BRG1,
			brg >> 8,
			SC18IM704_CMD_STOP,
		};

		ret = sc18im704_transfer(dev, buf, sizeof(buf), NULL, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set baudrate (%d)", ret);
			return ret;
		}

		/* Make sure UART buffer is sent */
		k_msleep(1);

		/* Re-configure the UART controller with the new baudrate */
		uart_cfg.baudrate = cfg->bus_speed;
		ret = uart_configure(cfg->bus, &uart_cfg);
		if (ret < 0) {
			LOG_ERR("Failed to re-configure UART (%d)", ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(i2c, i2c_sc18im_driver_api) = {
	.configure = i2c_sc18im_configure,
	.get_config = i2c_sc18im_get_config,
	.transfer = i2c_sc18im_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_SC18IM_DEFINE(n)									\
												\
	static const struct i2c_sc18im_config i2c_sc18im_config_##n = {				\
		.bus = DEVICE_DT_GET(DT_BUS(DT_INST_PARENT(n))),				\
		.bus_speed = DT_PROP_OR(DT_INST_PARENT(n), target_speed, 9600),			\
		.reset_gpios = GPIO_DT_SPEC_GET_OR(DT_INST_PARENT(n), reset_gpios, {0}),	\
	};											\
	static struct i2c_sc18im_data i2c_sc18im_data_##n = {					\
		.i2c_config = I2C_MODE_CONTROLLER | (I2C_SPEED_STANDARD << I2C_SPEED_SHIFT),	\
	};											\
												\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_sc18im_init, NULL,					\
			      &i2c_sc18im_data_##n, &i2c_sc18im_config_##n,			\
			      POST_KERNEL, CONFIG_I2C_SC18IM704_INIT_PRIORITY,			\
			      &i2c_sc18im_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SC18IM_DEFINE)
