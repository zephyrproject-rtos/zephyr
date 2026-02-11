/**
 *
 * @copyright Copyright (c) 2026 William Fish (Manulytica ltd)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <math.h>
#include "ad74416h_priv.h"

LOG_MODULE_REGISTER(AD74416H, CONFIG_SENSOR_LOG_LEVEL);

/* --- SPI & CRC Core --- */

static uint8_t ad74416h_crc8(const uint8_t *data, size_t len)
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

static int ad74416h_access(const struct device *dev, uint8_t reg, uint16_t val_in,
			   uint16_t *val_out)
{
	const struct ad74416h_config *config = dev->config;
	uint8_t tx[5] = {0, reg, (val_in >> 8), (val_in & 0xFF), 0};
	uint8_t rx[5] = {0};
	tx[4] = ad74416h_crc8(tx, 4);

	struct spi_buf tx_b = {.buf = tx, .len = 5};
	struct spi_buf rx_b = {.buf = rx, .len = 5};
	struct spi_buf_set tx_s = {.buffers = &tx_b, .count = 1};
	struct spi_buf_set rx_s = {.buffers = &rx_b, .count = 1};

	int ret = spi_transceive_dt(&config->spi, &tx_s, &rx_s);
	if (ret) {
		return ret;
	}

	if (val_out) {
		if (rx[4] != ad74416h_crc8(rx, 4)) {
			return -EIO;
		}
		*val_out = (rx[2] << 8) | rx[3];
	}
	return 0;
}

static int ad74416h_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	ad74416h_access(dev, 0x6E, reg, NULL); // READ_SELECT
	return ad74416h_access(dev, AD74416H_REG_NOP, 0x0000, val);
}

/* --- GPIO API (Digital I/O) --- */

static int ad74416h_gpio_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	if (flags & GPIO_OUTPUT) {
		ad74416h_access(dev, AD74416H_REG_CH_FUNC_SETUP(pin), 0x0000, NULL); // HIGH_Z
		return ad74416h_access(dev, AD74416H_REG_DO_EXT_CONFIG(pin), 0x0001,
				       NULL); // Sourcing
	} else {
		return ad74416h_access(dev, AD74416H_REG_CH_FUNC_SETUP(pin), 0x0008,
				       NULL); // Digital In
	}
}

static int ad74416h_gpio_set_bits(const struct device *dev, uint32_t mask, uint32_t value)
{
	for (int i = 0; i < 4; i++) {
		if (mask & BIT(i)) {
			uint16_t reg = 0;
			ad74416h_read(dev, AD74416H_REG_DO_EXT_CONFIG(i), &reg);
			if (value & BIT(i)) {
				reg |= BIT(7);
			} else {
				reg &= ~BIT(7);
			}
		}
		ad74416h_access(dev, AD74416H_REG_DO_EXT_CONFIG(i), reg, NULL);
	}
	return 0;
}

static const struct gpio_driver_api ad74416h_gpio_api = {
	.pin_configure = ad74416h_gpio_config,
	.port_set_bits_raw = &ad74416h_gpio_set_bits,
};

/* --- DAC API --- */

static int ad74416h_dac_write(const struct device *dev, uint8_t channel, uint32_t value)
{
	return ad74416h_access(dev, AD74416H_REG_DAC_CODE(channel), (uint16_t)value, NULL);
}

static const struct dac_driver_api ad74416h_dac_api = {.write_value = ad74416h_dac_write};

/* --- Sensor API (ADC & RTD) --- */

static int ad74416h_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ad74416h_data *data = dev->data;
	const struct ad74416h_config *config = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);
	ad74416h_access(dev, AD74416H_REG_ADC_CONV_CTRL, 0x010F, NULL); // Start single sequence

	if (k_sem_take(&data->adc_sem, K_MSEC(200))) {
		k_mutex_unlock(&data->lock);
		return -EIO;
	}

	for (int i = 0; i < 4; i++) {
		uint16_t upr, lwr;
		ad74416h_read(dev, AD74416H_REG_ADC_RESULT_UPR(i), &upr);
		ad74416h_read(dev, AD74416H_REG_ADC_RESULT(i), &lwr);
		uint32_t raw = ((upr & 0xFF) << 16) | lwr;
		data->raw_results[i] =
			(raw & 0x800000) ? (int32_t)(raw | 0xFF000000U) : (int32_t)raw;
	}
	k_mutex_unlock(&data->lock);
	return 0;
}

static int ad74416h_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct ad74416h_data *data = dev->data;
	int ch = chan - SENSOR_CHAN_VOLTAGE;
	if (ch < 0 || ch > 3) {
		return -ENOTSUP;
	}

	double v = (double)data->raw_results[ch] * 12.0 / 16777216.0;
	v = (v * data->cal[ch].gain) + data->cal[ch].offset;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* Callendar-Van Dusen Math */
		double R = (v / (4.0)) * 2012.0; // Simplified R_RTD calculation
		double celsius =
			(-3.9083e-3 + sqrt(pow(3.9083e-3, 2) - 4 * -5.775e-7 * (1 - R / 1000.0))) /
			(2 * -5.775e-7);
		sensor_value_from_double(val, celsius);
	} else {
		sensor_value_from_double(val, v);
	}
	return 0;
}

static DEVICE_API(sensor, ad74416h_sensor_api) = {
	.sample_fetch = ad74416h_sample_fetch,
	.channel_get = ad74416h_channel_get,
};

/* --- Initialization --- */

static void ad74416h_rdy_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct ad74416h_data *data = CONTAINER_OF(cb, struct ad74416h_data, adc_rdy_cb);
	k_sem_give(&data->adc_sem);
}

static int ad74416h_init(const struct device *dev)
{
	const struct ad74416h_config *config = dev->config;
	struct ad74416h_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_sem_init(&data->adc_sem, 0, 1);

	if (config->reset_gpio.port) {
		gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 0);
	}

	if (config->adc_rdy_gpio.port) {
		gpio_pin_configure_dt(&config->adc_rdy_gpio, GPIO_INPUT);
		gpio_init_callback(&data->adc_rdy_cb, ad74416h_rdy_handler,
				   BIT(config->adc_rdy_gpio.pin));
		gpio_add_callback(config->adc_rdy_gpio.port, &data->adc_rdy_cb);
		gpio_pin_interrupt_configure_dt(&config->adc_rdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	/* Verify Silicon */
	uint16_t rev;
	ad74416h_read(dev, AD74416H_REG_SILICON_REV, &rev);
	if (rev != 0x0002) {
		return -ENODEV;
	}

	for (int i = 0; i < 4; i++) {
		data->cal[i].gain = 1.0;
		data->cal[i].offset = 0.0;
		uint16_t out_cfg = config->adaptive_power ? (0x2 << 14) : 0;
		ad74416h_access(dev, AD74416H_REG_OUTPUT_CONFIG(i), out_cfg, NULL);
	}

	return 0;
}

#define AD74416H_DEFINE(n)                                                                         \
	static struct ad74416h_data ad74416h_data_##n;                                             \
	static const struct ad74416h_config ad74416h_config_##n = {                                \
		.spi = SPI_DT_SPEC_INST_GET(                                                       \
			n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),            \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
		.adc_rdy_gpio = GPIO_DT_SPEC_INST_GET_OR(n, adc_rdy_gpios, {0}),                   \
		.adaptive_power = DT_INST_PROP(n, adi_adaptive_power),                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ad74416h_init, NULL, &ad74416h_data_##n, &ad74416h_config_##n,    \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ad74416h_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(AD74416H_DEFINE)
