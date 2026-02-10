/*
 * Copyright (c) 2026 William Fish (Manulytica ltd)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT analog_ad74115h

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(AD74115H, CONFIG_SENSOR_LOG_LEVEL);

#define AD74115H_REG_CH_FUNC_SETUP    0x01
#define AD74115H_REG_ADC_CONFIG       0x02
#define AD74115H_REG_ADC_CONV_CTRL    0x3B
#define AD74115H_REG_ADC_RESULT1      0x44
#define AD74115H_REG_READ_SELECT      0x64

struct ad74115h_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec adc_rdy_gpio;
	bool use_charge_pump;
};

struct ad74115h_data {
	struct k_mutex lock;
	struct k_sem adc_sem;
	uint16_t adc_raw;
	struct gpio_callback adc_rdy_cb;
};

/* CRC-8 calculation: x^8 + x^2 + x^1 + 1 (0x07) */
static uint8_t ad74115h_crc8(const uint8_t *data, size_t len)
{
	uint8_t crc = 0x00;
	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int j = 0; j < 8; j++) {
			crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
		}
	}
	return crc;
}

static int ad74115h_access(const struct device *dev, uint8_t reg, uint16_t val_in, uint16_t *val_out)
{
	const struct ad74115h_config *config = dev->config;
	uint8_t tx[4] = { reg, val_in >> 8, val_in & 0xFF, 0 };
	uint8_t rx[4] = { 0 };
	tx[3] = ad74115h_crc8(tx, 3);

	struct spi_buf tx_buf = { .buf = tx, .len = 4 };
	struct spi_buf rx_buf = { .buf = rx, .len = 4 };
	const struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

	int ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);
	if (ret < 0) return ret;

	if (val_out) {
		if (rx[3] != ad74115h_crc8(rx, 3)) {
			LOG_ERR("CRC Mismatch");
			return -EIO;
		}
		*val_out = sys_get_be16(&rx[1]);
	}
	return 0;
}

static int ad74115h_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	ad74115h_access(dev, AD74115H_REG_READ_SELECT, reg, NULL);
	return ad74115h_access(dev, 0x00, 0x0000, val);
}

static int ad74115h_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ad74115h_data *data = dev->data;
	k_mutex_lock(&data->lock, K_FOREVER);

	/* Trigger conversion */
	ad74115h_access(dev, AD74115H_REG_ADC_CONV_CTRL, 0x1201, NULL);

	if (k_sem_take(&data->adc_sem, K_MSEC(200)) != 0) {
		k_mutex_unlock(&data->lock);
		return -EIO;
	}

	int ret = ad74115h_read(dev, AD74115H_REG_ADC_RESULT1, &data->adc_raw);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int ad74115h_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct ad74115h_data *data = dev->data;
	double v = ((double)data->adc_raw / 65535.0) * 12.0;
	return sensor_value_from_double(val, v);
}

static const struct sensor_driver_api ad74115h_sensor_api = {
	.sample_fetch = ad74115h_sample_fetch,
	.channel_get = ad74115h_channel_get,
};

static void ad74115h_rdy_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct ad74115h_data *data = CONTAINER_OF(cb, struct ad74115h_data, adc_rdy_cb);
	k_sem_give(&data->adc_sem);
}

static int ad74115h_init(const struct device *dev)
{
	const struct ad74115h_config *config = dev->config;
	struct ad74115h_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_sem_init(&data->adc_sem, 0, 1);

	if (!spi_is_ready_dt(&config->spi)) return -ENODEV;

	if (config->reset_gpio.port) {
		gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 0);
	}

	if (config->adc_rdy_gpio.port) {
		gpio_pin_configure_dt(&config->adc_rdy_gpio, GPIO_INPUT);
		gpio_init_callback(&data->adc_rdy_cb, ad74115h_rdy_callback, BIT(config->adc_rdy_gpio.pin));
		gpio_add_callback(config->adc_rdy_gpio.port, &data->adc_rdy_cb);
		gpio_pin_interrupt_configure_dt(&config->adc_rdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	LOG_INF("AD74115H driver initialized");
	return 0;
}

#define AD74115H_DEFINE(n)                                                                         \
	static struct ad74115h_data ad74115h_data_##n;                                             \
	static const struct ad74115h_config ad74115h_config_##n = {                                \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),            \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, { 0 }),                     \
		.adc_rdy_gpio = GPIO_DT_SPEC_INST_GET_OR(n, adc_rdy_gpios, { 0 }),                 \
		.use_charge_pump = DT_INST_PROP(n, analog_ext_charge_pump),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ad74115h_init, NULL, &ad74115h_data_##n, &ad74115h_config_##n,    \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ad74115h_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(AD74115H_DEFINE)
