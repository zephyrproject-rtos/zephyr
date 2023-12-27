/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bd8lb600fs

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/bd8lb600fs.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(rohm_bd8lb600fs, CONFIG_MFD_LOG_LEVEL);

#define OUTPUT_OFF_WITH_OPEN_LOAD_DETECTION  0b11
#define OUTPUT_ON                            0b10
#define WAIT_TIME_RESET_ACTIVE_IN_US         1000
#define WAIT_TIME_RESET_INACTIVE_TO_CS_IN_US 10

struct bd8lb600fs_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_reset;
	size_t instance_count;
};

struct bd8lb600fs_data {
	/* each bit is one output channel, bit 0 = channel 1, ... */
	uint32_t state;
	/* each bit defines if an open load was detected, see state */
	uint32_t old;
	/* each bit defines if an over current or over temperature was detected, see state */
	uint32_t ocp_or_tsd;
	struct k_mutex lock;
};

static void bd8lb600fs_fill_tx_buffer(const struct device *dev, uint8_t *buffer, size_t buffer_size)
{
	const struct bd8lb600fs_config *config = dev->config;
	struct bd8lb600fs_data *data = dev->data;
	uint16_t state_converted = 0;

	LOG_DBG("%s: writing state 0x%08X to BD8LB600FS", dev->name, data->state);

	memset(buffer, 0x00, buffer_size);

	for (size_t j = 0; j < config->instance_count; ++j) {
		int instance_position = (config->instance_count - j - 1) * 2;

		state_converted = 0;

		for (size_t i = 0; i < 8; ++i) {
			if ((data->state & BIT(i + j * 8)) == 0) {
				state_converted |= OUTPUT_OFF_WITH_OPEN_LOAD_DETECTION << (i * 2);
			} else {
				state_converted |= OUTPUT_ON << (i * 2);
			}
		}

		LOG_DBG("%s: configuration for instance %zu: %04X (position %i)", dev->name, j,
			state_converted, instance_position);
		sys_put_be16(state_converted, buffer + instance_position);
	}
}

static void bd8lb600fs_parse_rx_buffer(const struct device *dev, uint8_t *buffer)
{
	const struct bd8lb600fs_config *config = dev->config;
	struct bd8lb600fs_data *data = dev->data;

	data->old = 0;
	data->ocp_or_tsd = 0;

	for (size_t j = 0; j < config->instance_count; ++j) {
		int instance_position = (config->instance_count - j - 1) * 2;
		uint16_t current = sys_get_be16(buffer + instance_position);

		for (size_t i = 0; i < 8; ++i) {
			if ((BIT(2 * i + 1) & current) != 0) {
				WRITE_BIT(data->old, i + j * 8, 1);
			}
			if ((BIT(2 * i) & current) != 0) {
				WRITE_BIT(data->ocp_or_tsd, i + j * 8, 1);
			}
		}
	}

	LOG_DBG("%s: received 0x%08X open load state from BD8LB600FS", dev->name, data->old);
	LOG_DBG("%s: received 0x%08X OCP or TSD state from BD8LB600FS", dev->name,
		data->ocp_or_tsd);
}

static int bd8lb600fs_transceive_state(const struct device *dev)
{
	const struct bd8lb600fs_config *config = dev->config;

	uint8_t buffer_tx[8];
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = config->instance_count * sizeof(uint16_t),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	uint8_t buffer_rx[8];
	const struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = config->instance_count * sizeof(uint16_t),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	bd8lb600fs_fill_tx_buffer(dev, buffer_tx, ARRAY_SIZE(buffer_tx));

	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	bd8lb600fs_parse_rx_buffer(dev, buffer_rx);

	return 0;
}

static int bd8lb600fs_write_state(const struct device *dev)
{
	const struct bd8lb600fs_config *config = dev->config;

	uint8_t buffer_tx[8];
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = config->instance_count * sizeof(uint16_t),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	bd8lb600fs_fill_tx_buffer(dev, buffer_tx, ARRAY_SIZE(buffer_tx));

	int result = spi_write_dt(&config->bus, &tx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	return 0;
}

int mfd_bd8lb600fs_set_outputs(const struct device *dev, uint32_t values)
{
	struct bd8lb600fs_data *data = dev->data;
	int result;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->state = values;
	result = bd8lb600fs_write_state(dev);
	k_mutex_unlock(&data->lock);

	return result;
}

int mfd_bd8lb600fs_get_output_diagnostics(const struct device *dev, uint32_t *old,
					  uint32_t *ocp_or_tsd)
{
	struct bd8lb600fs_data *data = dev->data;
	int result;

	k_mutex_lock(&data->lock, K_FOREVER);
	result = bd8lb600fs_transceive_state(dev);
	*old = data->old;
	*ocp_or_tsd = data->ocp_or_tsd;
	k_mutex_unlock(&data->lock);

	return result;
}

static int bd8lb600fs_init(const struct device *dev)
{
	const struct bd8lb600fs_config *config = dev->config;
	struct bd8lb600fs_data *data = dev->data;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->gpio_reset)) {
		LOG_ERR("%s: reset GPIO is not ready", dev->name);
		return -ENODEV;
	}

	int result = k_mutex_init(&data->lock);

	if (result != 0) {
		LOG_ERR("unable to initialize mutex");
		return result;
	}

	result = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);

	if (result != 0) {
		LOG_ERR("failed to initialize GPIO for reset");
		return result;
	}

	k_busy_wait(WAIT_TIME_RESET_ACTIVE_IN_US);
	gpio_pin_set_dt(&config->gpio_reset, 0);
	k_busy_wait(WAIT_TIME_RESET_INACTIVE_TO_CS_IN_US);

	return 0;
}

#define BD8LB600FS_INIT(inst)                                                                      \
	static const struct bd8lb600fs_config bd8lb600fs_##inst##_config = {                       \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst, SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),            \
		.gpio_reset = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), reset_gpios, 0),          \
		.instance_count = DT_INST_PROP(inst, instance_count),                              \
	};                                                                                         \
                                                                                                   \
	static struct bd8lb600fs_data bd8lb600fs_##inst##_data = {                                 \
		.state = 0x00,                                                                     \
	};                                                                                         \
                                                                                                   \
	/* This has to be initialized after the SPI peripheral. */                                 \
	DEVICE_DT_INST_DEFINE(inst, bd8lb600fs_init, NULL, &bd8lb600fs_##inst##_data,              \
			      &bd8lb600fs_##inst##_config, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,  \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(BD8LB600FS_INIT)
