/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_max14917);

#include <zephyr/drivers/gpio/gpio_utils.h>

#include "gpio_max14917.h"

#define DT_DRV_COMPAT adi_max14917_gpio

static int max14917_reg_trans_spi_diag(const struct device *dev)
{
	int ret = 0;
	uint8_t crc;

	uint8_t local_tx_buff[MAX14917_MAX_PKT_SIZE] = {0};
	uint8_t local_rx_buff[MAX14917_MAX_PKT_SIZE] = {0};

	struct max14917_data *data = dev->data;
	const struct max14917_config *config = dev->config;

	struct spi_buf tx_buf = {
		.buf = &local_tx_buff,
		.len = config->pkt_size,
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf = {
		.buf = &local_rx_buff,
		.len = config->pkt_size,
	};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	local_tx_buff[0] = data->gpios_ON;

	/* If CRC enabled calculate it */
	if (config->crc_en) {
		crc = crc8(&local_tx_buff[0], 1, MAX14917_CRC_POLY, MAX14917_CRC_INI_VAL, false);
		local_tx_buff[1] = (crc & MAX14917_CRC_MASK);
	}

	/* Perform SPI transaction */
	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI transfer failed");
		return ret;
	}

	/* If CRC enabled check it */
	if (config->crc_en) {
		crc = crc8(&local_tx_buff[0], 1, MAX14917_CRC_POLY, MAX14917_CRC_INI_VAL, false);
		crc = (crc & MAX14917_CRC_MASK);
		if (crc != (local_rx_buff[1] & 0x1F)) {
			LOG_ERR("READ CRC ERR (%d)-(%d)\n", crc, (local_rx_buff[1] & 0x1F));
			return -EINVAL;
		}
		/* Set error flags in device data */
		data->comm_err = (local_rx_buff[1] & MAX14917_COMM_ERR);
		data->verr = (local_rx_buff[1] & MAX14917_VERR);
		data->therm_err = (local_rx_buff[1] & MAX14917_THERM_ERR);
	}

	data->gpios_fault = local_rx_buff[0];

	return 0;
}

static int max14917_fault_check(const struct device *dev)
{
	int ret;
	const struct max14917_data *data = dev->data;
	const struct max14917_config *config = dev->config;

	if (gpio_pin_get_dt(&config->fault_gpio)) {
		LOG_DBG("FAULT GPIO is high");
	}

	/* Update error flags */
	ret = max14917_reg_trans_spi_diag(dev);
	if (ret) {
		return ret;
	}

	if (data->comm_err) {
		LOG_DBG("COMMERR flag is active");
	}
	if (data->verr) {
		LOG_DBG("VERR flag is active");
	}
	if (data->therm_err) {
		LOG_DBG("THERMERR flag is active");
	}

	for (int i = 0; i < MAX14917_CHANNELS; i++) {
		if (data->gpios_fault & BIT(i)) {
			LOG_DBG("Channel %d has a fault", i);
		}
	}

	return 0;
}

static int gpio_max14917_init(const struct device *dev)
{
	struct max14917_data *data = dev->data;
	const struct max14917_config *config = dev->config;
	int err = 0;

	LOG_DBG(" --- GPIO max14917 init IN ---");

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready\n");
		return -ENODEV;
	}

	/* Output GPIOS */
	/* setup EN gpio - normal low */
	if (!gpio_is_ready_dt(&config->en_gpio)) {
		LOG_ERR("EN GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->en_gpio, GPIO_OUTPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure EN GPIO");
		return err;
	}

	/* setup SYNC gpio - normal low */
	if (!gpio_is_ready_dt(&config->sync_gpio)) {
		LOG_ERR("SYNC GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->sync_gpio, GPIO_OUTPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure SYNC GPIO");
		return err;
	}
	/* setup CRCEN gpio - normal low */
	if (!gpio_is_ready_dt(&config->crcen_gpio)) {
		LOG_ERR("CRCEN GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->crcen_gpio, GPIO_OUTPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure CRCEN GPIO");
		return err;
	}
	/* Input GPIOS */
	/* setup VDDOK gpio - normal low */
	if (!gpio_is_ready_dt(&config->vddok_gpio)) {
		LOG_ERR("VDDOK GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->vddok_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure VDDOK GPIO");
		return err;
	}
	/* setup READY gpio - normal low */
	if (!gpio_is_ready_dt(&config->ready_gpio)) {
		LOG_ERR("VDDOK READY device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->ready_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure READY GPIO");
		return err;
	}
	/* setup COMERR gpio - normal low */
	if (!gpio_is_ready_dt(&config->comerr_gpio)) {
		LOG_ERR("COMERR GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->comerr_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure COMERR GPIO");
		return err;
	}
	/* setup FAULT gpio - normal low */
	if (!gpio_is_ready_dt(&config->fault_gpio)) {
		LOG_ERR("FAULT GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->fault_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure FAULT GPIO");
		return err;
	}

	err = gpio_pin_set_dt(&config->en_gpio, 1);
	if (err) {
		return err;
	}
	err = gpio_pin_set_dt(&config->sync_gpio, 1);
	if (err) {
		return err;
	}

	if (config->crc_en) {
		err = gpio_pin_set_dt(&config->crcen_gpio, 1);
	} else {
		err = gpio_pin_set_dt(&config->crcen_gpio, 0);
	}

	if (err) {
		return err;
	}

	/* Initialize satus and fault flags to 0 */
	data->gpios_ON = 0;
	data->gpios_fault = 0;

	err = max14917_fault_check(dev);

	return err;
}

static int gpio_max14917_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	int err = 0;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INT_ENABLE) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_OUTPUT:
		break;
	case GPIO_INPUT:
	default:
		LOG_ERR("NOT SUPPORTED OPTION!");
		return -ENOTSUP;
	}

	return err;
}

static int gpio_max14917_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	int ret;
	const struct max14917_data *data = dev->data;

	ret = max14917_fault_check(dev);
	if (ret) {
		return ret;
	}

	*value = data->gpios_ON;

	return 0;
}

static int gpio_max14917_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	struct max14917_data *data = dev->data;

	ret = max14917_fault_check(dev);
	if (ret) {
		return ret;
	}

	data->gpios_ON = data->gpios_ON | pins;

	return max14917_reg_trans_spi_diag(dev);
}

static int gpio_max14917_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	struct max14917_data *data = dev->data;

	ret = max14917_fault_check(dev);
	if (ret) {
		return ret;
	}

	data->gpios_ON = data->gpios_ON & ~pins;

	return max14917_reg_trans_spi_diag(dev);
}

static int gpio_max14917_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	struct max14917_data *data = dev->data;

	ret = max14917_fault_check(dev);
	if (ret) {
		return ret;
	}

	data->gpios_ON ^= pins;

	return max14917_reg_trans_spi_diag(dev);
}

static DEVICE_API(gpio, gpio_max14917_api) = {
	.pin_configure = gpio_max14917_config,
	.port_get_raw = gpio_max14917_port_get_raw,
	.port_set_bits_raw = gpio_max14917_port_set_bits_raw,
	.port_clear_bits_raw = gpio_max14917_port_clear_bits_raw,
	.port_toggle_bits = gpio_max14917_port_toggle_bits,
};

#define GPIO_MAX14917_DEVICE(id)                                                                   \
	static const struct max14917_config max14917_##id##_cfg = {                                \
		.spi = SPI_DT_SPEC_INST_GET(id, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),        \
		.vddok_gpio = GPIO_DT_SPEC_INST_GET(id, vddok_gpios),                              \
		.ready_gpio = GPIO_DT_SPEC_INST_GET(id, ready_gpios),                              \
		.comerr_gpio = GPIO_DT_SPEC_INST_GET(id, comerr_gpios),                            \
		.fault_gpio = GPIO_DT_SPEC_INST_GET(id, fault_gpios),                              \
		.en_gpio = GPIO_DT_SPEC_INST_GET(id, en_gpios),                                    \
		.sync_gpio = GPIO_DT_SPEC_INST_GET(id, sync_gpios),                                \
		.crcen_gpio = GPIO_DT_SPEC_INST_GET(id, crcen_gpios),                              \
		.crc_en = DT_INST_PROP(id, crc_en),                                                \
		.pkt_size = (DT_INST_PROP(id, crc_en) & 0x1) ? 2 : 1,                              \
	};                                                                                         \
                                                                                                   \
	static struct max14917_data max14917_##id##_data;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &gpio_max14917_init, NULL, &max14917_##id##_data,                \
			      &max14917_##id##_cfg, POST_KERNEL,                                   \
			      CONFIG_GPIO_MAX14917_INIT_PRIORITY, &gpio_max14917_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX14917_DEVICE)
