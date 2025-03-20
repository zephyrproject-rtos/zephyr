/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_litei2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_litex_litei2c, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#include <soc.h>

#define MASTER_STATUS_TX_READY_OFFSET 0x0
#define MASTER_STATUS_RX_READY_OFFSET 0x1
#define MASTER_STATUS_NACK_OFFSET     0x8

struct i2c_litex_litei2c_config {
	uint32_t phy_speed_mode_addr;
	uint32_t master_active_addr;
	uint32_t master_settings_addr;
	uint32_t master_addr_addr;
	uint32_t master_rxtx_addr;
	uint32_t master_status_addr;
	uint32_t bitrate;
};

struct i2c_litex_litei2c_data {
	struct k_mutex mutex;
};

static int i2c_litex_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;

	if (I2C_ADDR_10_BITS & dev_config) {
		return -ENOTSUP;
	}

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Setup speed to use */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		litex_write8(0, config->phy_speed_mode_addr);
		break;
	case I2C_SPEED_FAST:
		litex_write8(1, config->phy_speed_mode_addr);
		break;
	case I2C_SPEED_FAST_PLUS:
		litex_write8(2, config->phy_speed_mode_addr);
		break;
	default:
		k_mutex_unlock(&data->mutex);
		return -ENOTSUP;
	}

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int i2c_litex_get_config(const struct device *dev, uint32_t *config)
{
	const struct i2c_litex_litei2c_config *dev_config = dev->config;

	*config = I2C_MODE_CONTROLLER;

	switch (litex_read8(dev_config->phy_speed_mode_addr)) {
	case 0:
		*config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case 1:
		*config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case 2:
		*config |= I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	default:
		break;
	}

	return 0;
}

static int i2c_litex_write_settings(const struct device *dev, uint8_t len_tx, uint8_t len_rx,
				    bool recover)
{
	const struct i2c_litex_litei2c_config *config = dev->config;

	uint32_t settings = len_tx | (len_rx << 8) | (recover << 16);

	litex_write32(settings, config->master_settings_addr);

	return 0;
}

static int i2c_litex_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;
	uint32_t len_tx_buf = 0;
	uint32_t len_rx_buf = 0;
	uint8_t len_tx = 0;
	uint8_t len_rx = 0;

	uint8_t *tx_buf_ptr;
	uint8_t *rx_buf_ptr;

	uint32_t tx_buf;
	uint32_t rx_buf;

	uint32_t tx_j = 0;
	uint32_t rx_j = 0;

	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	litex_write8(1, config->master_active_addr);

	LOG_DBG("addr: 0x%x", addr);
	litex_write8((uint8_t)addr, config->master_addr_addr);

	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			len_tx_buf = 0;
			len_rx_buf = msgs[i].len;
			rx_buf_ptr = msgs[i].buf;
			tx_buf_ptr = NULL;
		} else {
			len_tx_buf = msgs[i].len;
			tx_buf_ptr = msgs[i].buf;
			if (!(msgs[i].flags & I2C_MSG_STOP) && (i + 1 < num_msgs) &&
			    (msgs[i + 1].flags & I2C_MSG_READ) &&
			    (msgs[i + 1].flags & I2C_MSG_RESTART)) {
				i++;
				len_rx_buf = msgs[i].len;
				rx_buf_ptr = msgs[i].buf;
			} else {
				len_rx_buf = 0;
				rx_buf_ptr = NULL;
			}
		}

		LOG_HEXDUMP_DBG(tx_buf_ptr, len_tx_buf, "tx_buf");

		tx_j = 0;
		rx_j = 0;
		do {

			if (len_tx_buf > (tx_j + 4)) {
				len_tx = 5;
				len_rx = 0;
			} else {
				len_tx = len_tx_buf - tx_j;

				if (len_rx_buf > (rx_j + 4)) {
					len_rx = 5;
				} else {
					len_rx = len_rx_buf - rx_j;
				}
			}

			tx_buf = 0;

			switch (len_tx) {
			case 5:
			case 4:
				tx_buf |= tx_buf_ptr[0 + tx_j] << 24;
				tx_buf |= tx_buf_ptr[1 + tx_j] << 16;
				tx_buf |= tx_buf_ptr[2 + tx_j] << 8;
				tx_buf |= tx_buf_ptr[3 + tx_j];
				tx_j += 4;
				break;
			case 3:
				tx_buf |= tx_buf_ptr[0 + tx_j] << 16;
				tx_buf |= tx_buf_ptr[1 + tx_j] << 8;
				tx_buf |= tx_buf_ptr[2 + tx_j];
				tx_j += 3;
				break;
			case 2:
				tx_buf |= tx_buf_ptr[0 + tx_j] << 8;
				tx_buf |= tx_buf_ptr[1 + tx_j];
				tx_j += 2;
				break;
			case 1:
				tx_buf |= tx_buf_ptr[0 + tx_j];
				tx_j += 1;
				break;
			default:
				break;
			}

			LOG_DBG("len_tx: %d, len_rx: %d", len_tx, len_rx);
			i2c_litex_write_settings(dev, len_tx, len_rx, false);

			while (!(litex_read8(config->master_status_addr) &
				 BIT(MASTER_STATUS_TX_READY_OFFSET))) {
				;
			}

			LOG_DBG("tx_buf: 0x%x", tx_buf);
			litex_write32(tx_buf, config->master_rxtx_addr);

			while (!(litex_read8(config->master_status_addr) &
				 BIT(MASTER_STATUS_RX_READY_OFFSET))) {
				;
			}

			if (litex_read16(config->master_status_addr) &
			    BIT(MASTER_STATUS_NACK_OFFSET)) {
				LOG_DBG("NACK received (addr: 0x%x)", addr);
				ret = -EIO;
			}

			rx_buf = litex_read32(config->master_rxtx_addr);
			LOG_DBG("rx_buf: 0x%x", rx_buf);

			switch (len_rx) {
			case 5:
			case 4:
				rx_buf_ptr[0 + rx_j] = rx_buf >> 24;
				rx_buf_ptr[1 + rx_j] = rx_buf >> 16;
				rx_buf_ptr[2 + rx_j] = rx_buf >> 8;
				rx_buf_ptr[3 + rx_j] = rx_buf;
				rx_j += 4;
				break;
			case 3:
				rx_buf_ptr[0 + rx_j] = rx_buf >> 16;
				rx_buf_ptr[1 + rx_j] = rx_buf >> 8;
				rx_buf_ptr[2 + rx_j] = rx_buf;
				rx_j += 3;
				break;
			case 2:
				rx_buf_ptr[0 + rx_j] = rx_buf >> 8;
				rx_buf_ptr[1 + rx_j] = rx_buf;
				rx_j += 2;
				break;
			case 1:
				rx_buf_ptr[0 + rx_j] = rx_buf;
				rx_j += 1;
				break;
			default:
				break;
			}

			if (ret < 0) {
				goto transfer_end;
			}

		} while ((tx_j < len_tx_buf) || (rx_j < len_rx_buf));

		LOG_HEXDUMP_DBG(rx_buf_ptr, len_rx_buf, "rx_buf");
	}

transfer_end:

	litex_write8(0, config->master_active_addr);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int i2c_litex_recover_bus(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	litex_write8(1, config->master_active_addr);

	i2c_litex_write_settings(dev, 0, 0, true);

	while (!(litex_read8(config->master_status_addr) & BIT(MASTER_STATUS_TX_READY_OFFSET))) {
		;
	}

	litex_write32(0, config->master_rxtx_addr);

	while (!(litex_read8(config->master_status_addr) & BIT(MASTER_STATUS_RX_READY_OFFSET))) {
		;
	}

	(void)litex_read32(config->master_rxtx_addr);

	litex_write8(0, config->master_active_addr);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int i2c_litex_init(const struct device *dev)
{
	const struct i2c_litex_litei2c_config *config = dev->config;
	struct i2c_litex_litei2c_data *data = dev->data;
	int ret;

	k_mutex_init(&data->mutex);

	ret = i2c_litex_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (ret != 0) {
		LOG_ERR("failed to configure I2C: %d", ret);
	}

	return ret;
}

static DEVICE_API(i2c, i2c_litex_litei2c_driver_api) = {
	.configure = i2c_litex_configure,
	.get_config = i2c_litex_get_config,
	.transfer = i2c_litex_transfer,
	.recover_bus = i2c_litex_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* Device Instantiation */

#define I2C_LITEX_INIT(n)                                                                          \
	static struct i2c_litex_litei2c_data i2c_litex_litei2c_data_##n;                           \
												   \
	static const struct i2c_litex_litei2c_config i2c_litex_litei2c_config_##n = {              \
		.phy_speed_mode_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_speed_mode),                \
		.master_active_addr = DT_INST_REG_ADDR_BY_NAME(n, master_active),                  \
		.master_settings_addr = DT_INST_REG_ADDR_BY_NAME(n, master_settings),              \
		.master_addr_addr = DT_INST_REG_ADDR_BY_NAME(n, master_addr),                      \
		.master_rxtx_addr = DT_INST_REG_ADDR_BY_NAME(n, master_rxtx),                      \
		.master_status_addr = DT_INST_REG_ADDR_BY_NAME(n, master_status),                  \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_litex_init, NULL, &i2c_litex_litei2c_data_##n,            \
				  &i2c_litex_litei2c_config_##n, POST_KERNEL,                      \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_litex_litei2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_LITEX_INIT)
