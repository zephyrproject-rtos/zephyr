/*
 * Copyright (c) 2025 Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_dac161s997

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dac/dac161s997.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(dac_dac161s997, CONFIG_DAC_LOG_LEVEL);

#define DAC161S997_CHANNELS   1
#define DAC161S997_RESOLUTION 16

enum dac161s997_reg {
	DAC161S997_REG_XFER = 1,
	DAC161S997_REG_NOP,
	DAC161S997_REG_WR_MODE,
	DAC161S997_REG_DACCODE,
	DAC161S997_REG_ERR_CONFIG,
	DAC161S997_REG_ERR_LOW,
	DAC161S997_REG_ERR_HIGH,
	DAC161S997_REG_RESET,
	DAC161S997_REG_STATUS,
};

struct dac161s997_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_errb;
};

struct dac161s997_data {
	const struct device *dev;
	struct k_sem lock;
	struct gpio_callback gpio_errb_cb;
	struct k_work gpio_errb_work;
	dac161s997_error_callback_t error_cb;
};

int dac161s997_set_error_callback(const struct device *dev, dac161s997_error_callback_t cb)
{
	const struct dac161s997_config *config = dev->config;
	struct dac161s997_data *data = dev->data;
	int ret;

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (config->gpio_errb.port != NULL) {
		data->error_cb = cb;
	} else {
		ret = -ENOTSUP;
	}

	k_sem_give(&data->lock);

	return ret;
}

static int dac161s997_read_reg(const struct device *dev, enum dac161s997_reg reg, uint16_t *val)
{
	const struct dac161s997_config *config = dev->config;
	int res;

	uint8_t reg_read = BIT(7) | reg;
	uint8_t tx_buf[3] = {reg_read};
	const struct spi_buf tx_buffers[] = {{.buf = tx_buf, .len = ARRAY_SIZE(tx_buf)}};
	const struct spi_buf_set tx_bufs = {.buffers = tx_buffers, .count = ARRAY_SIZE(tx_buffers)};
	uint8_t rx_buf[3];
	const struct spi_buf rx_buffers[] = {{.buf = rx_buf, .len = ARRAY_SIZE(rx_buf)}};
	const struct spi_buf_set rx_bufs = {.buffers = rx_buffers, .count = ARRAY_SIZE(rx_buffers)};

	res = spi_write_dt(&config->bus, &tx_bufs);
	if (res != 0) {
		LOG_ERR("Read 0x%02x setup failed: %d", reg, res);
		return res;
	}

	tx_buf[0] = DAC161S997_REG_NOP;
	res = spi_transceive_dt(&config->bus, &tx_bufs, &rx_bufs);
	if (res != 0) {
		LOG_ERR("Read from 0x%02x failed: %d", reg, res);
		return res;
	}

	if (reg_read != rx_buf[0]) {
		LOG_ERR("Read 0x%02x addr mismatch: 0x%02x", reg_read, rx_buf[0]);
		return -EIO;
	}

	*val = sys_get_be16(&rx_buf[1]);

	LOG_DBG("Reg 0x%02x: 0x%02x", reg, *val);

	return 0;
}

static int dac161s997_write_reg(const struct device *dev, enum dac161s997_reg reg, uint16_t val)
{
	const struct dac161s997_config *config = dev->config;

	uint8_t tx_buf[3] = {reg, val >> 8, val};
	const struct spi_buf tx_buffers[] = {{.buf = tx_buf, .len = ARRAY_SIZE(tx_buf)}};
	const struct spi_buf_set tx_bufs = {.buffers = tx_buffers, .count = ARRAY_SIZE(tx_buffers)};

	int ret = spi_write_dt(&config->bus, &tx_bufs);

	if (ret != 0) {
		LOG_ERR("Write to reg 0x%02x failed: %i", reg, ret);
		return ret;
	}

	return 0;
}

static int dac161s997_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id >= DAC161S997_CHANNELS) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != DAC161S997_RESOLUTION) {
		LOG_ERR("Only %d bit resolution is supported", DAC161S997_RESOLUTION);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int dac161s997_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dac161s997_data *data = dev->data;
	int ret;

	if (channel >= DAC161S997_CHANNELS) {
		LOG_ERR("Channel %d is not valid", channel);
		return -EINVAL;
	}

	if (value > BIT(DAC161S997_RESOLUTION) - 1) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret != 0) {
		LOG_WRN("Write value lock failed: %d", ret);
		return ret;
	}

	ret = dac161s997_write_reg(dev, DAC161S997_REG_DACCODE, value);

	k_sem_give(&data->lock);

	return ret;
}

static int dac161s997_read_status(const struct device *dev, union dac161s997_status *status)
{
	int ret;
	uint16_t tmp;

	ret = dac161s997_read_reg(dev, DAC161S997_REG_STATUS, &tmp);
	if (ret == 0) {
		status->raw = tmp;
	}

	return ret;
}

static void dac161s997_gpio_errb_work_handler(struct k_work *work)
{
	struct dac161s997_data *data = CONTAINER_OF(work, struct dac161s997_data, gpio_errb_work);
	const struct device *dev = data->dev;
	union dac161s997_status status;
	int ret;

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret != 0) {
		LOG_WRN("ERRB handler take lock failed: %d", ret);
		return;
	}

	ret = dac161s997_read_status(dev, &status);

	if (data->error_cb != NULL) {
		data->error_cb(dev, ret == 0 ? &status : NULL);
	}

	k_sem_give(&data->lock);
}

static void dac161s997_gpio_errb_cb(const struct device *dev, struct gpio_callback *cb,
				    gpio_port_pins_t pins)
{
	struct dac161s997_data *data = CONTAINER_OF(cb, struct dac161s997_data, gpio_errb_cb);
	int ret;

	ret = k_work_submit(&data->gpio_errb_work);
	if (ret != 1) {
		LOG_WRN("ERRB work not queued: %d", ret);
	}
}

static int dac161s997_init(const struct device *dev)
{
	const struct dac161s997_config *config = dev->config;
	struct dac161s997_data *data = dev->data;
	union dac161s997_status status;
	int ret;

	data->dev = dev;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	ret = dac161s997_write_reg(dev, DAC161S997_REG_RESET, 0xc33c);
	if (ret != 0) {
		return ret;
	}
	ret = dac161s997_write_reg(dev, DAC161S997_REG_NOP, 0);
	if (ret != 0) {
		return ret;
	}

	/* Read status to clear any sticky error caused during boot or reboot */
	ret = dac161s997_read_status(dev, &status);
	if (ret != 0) {
		return ret;
	}

	/* Check that DAC_RES bits are all set */
	if (status.dac_resolution != 0x7) {
		LOG_ERR("Unexpected DAC resolution value: 0x%02x", status.dac_resolution);
		return ret;
	}

	if (config->gpio_errb.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_errb)) {
			LOG_ERR("ERRB GPIO is not ready");
			return -ENODEV;
		}

		k_work_init(&data->gpio_errb_work, dac161s997_gpio_errb_work_handler);

		gpio_init_callback(&data->gpio_errb_cb, dac161s997_gpio_errb_cb,
				   BIT(config->gpio_errb.pin));

		ret = gpio_pin_configure_dt(&config->gpio_errb, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Configure ERRB GPIO failed: %d", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->gpio_errb, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret) {
			LOG_ERR("Configure ERRB interrupt failed: %d", ret);
			return ret;
		}

		ret = gpio_add_callback_dt(&config->gpio_errb, &data->gpio_errb_cb);
		if (ret != 0) {
			LOG_ERR("Configure ERRB callback failed: %d", ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(dac, dac161s997_driver_api) = {
	.channel_setup = dac161s997_channel_setup,
	.write_value = dac161s997_write_value,
};

#define DAC_DAC161S997_INIT(n)                                                                     \
	static const struct dac161s997_config dac161s997_config_##n = {                            \
		.bus = SPI_DT_SPEC_INST_GET(n, SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),             \
		.gpio_errb = GPIO_DT_SPEC_INST_GET_OR(n, errb_gpios, {0}),                         \
	};                                                                                         \
                                                                                                   \
	static struct dac161s997_data dac161s997_data_##n;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, dac161s997_init, NULL, &dac161s997_data_##n,                      \
			      &dac161s997_config_##n, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,       \
			      &dac161s997_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_DAC161S997_INIT);
