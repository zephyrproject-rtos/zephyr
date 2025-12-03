/*
 * Copyright (c) 2025, tinyvision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_sc18is606_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sc18is606, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include "spi_context.h"

#define SC18IS606_CONFIG_SPI 0xF0
#define CLEAR_INTERRUPT      0xF1
#define IDLE_MODE            0xF2
#define SC18IS606_LSB_MASK   GENMASK(5, 5)
#define SC18IS606_MODE_MASK  GENMASK(3, 2)
#define SC18IS606_FREQ_MASK  GENMASK(1, 0)

#include "spi_sc18is606.h"

struct nxp_sc18is606_data {
	struct k_mutex bridge_lock;
	struct spi_context ctx;
	uint8_t frequency_idx;
	uint8_t spi_mode;
	struct gpio_callback int_cb;
	struct k_sem int_sem;
};

struct nxp_sc18is606_config {
	const struct i2c_dt_spec i2c_controller;
	const struct gpio_dt_spec reset_gpios;
	const struct gpio_dt_spec int_gpios;
};

int nxp_sc18is606_claim(const struct device *dev)
{
	struct nxp_sc18is606_data *data = dev->data;

	return k_mutex_lock(&data->bridge_lock, K_FOREVER);
}

int nxp_sc18is606_release(const struct device *dev)
{
	struct nxp_sc18is606_data *data = dev->data;

	return k_mutex_unlock(&data->bridge_lock);
}

int nxp_sc18is606_transfer(const struct device *dev, const uint8_t *tx_data, uint8_t tx_len,
			   uint8_t *rx_data, uint8_t rx_len, uint8_t *id_buf)
{
	struct nxp_sc18is606_data *data = dev->data;
	const struct nxp_sc18is606_config *info = dev->config;
	int ret;

	ret = k_mutex_lock(&data->bridge_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	if (tx_data != NULL) {
		if (id_buf != NULL) {
			struct i2c_msg tx_msg[2] = {
				{
					.buf = id_buf,
					.len = 1,
					.flags = I2C_MSG_WRITE,
				},
				{
					.buf = (uint8_t *)tx_data,
					.len = tx_len,
					.flags = I2C_MSG_WRITE,
				},
			};

			ret = i2c_transfer_dt(&info->i2c_controller, tx_msg, 2);
		} else {
			struct i2c_msg tx_msg[1] = {{
				.buf = (uint8_t *)tx_data,
				.len = tx_len,
				.flags = I2C_MSG_WRITE,
			}};

			ret = i2c_transfer_dt(&info->i2c_controller, tx_msg, 1);
		}

		if (ret != 0) {
			LOG_ERR("SPI write failed: %d", ret);
			goto out;
		}
	}

	/*If interrupt pin is used wait before next transaction*/
	if (info->int_gpios.port) {
		ret = k_sem_take(&data->int_sem, K_MSEC(5));
		if (ret != 0) {
			LOG_WRN("Interrupt semaphore timedout, proceeding with read");
		}
	}

	if (rx_data != NULL) {
		/*What is the time*/
		k_timepoint_t end;

		/*Set a deadline in a second*/
		end = sys_timepoint_calc(K_MSEC(1));

		do {
			ret = i2c_read(info->i2c_controller.bus, rx_data, rx_len,
				       info->i2c_controller.addr);
			if (ret >= 0) {
				break;
			}
		} while (!sys_timepoint_expired(end)); /*Keep reading while in the deadline*/

		if (ret < 0) {
			LOG_ERR("Failed to read data (%d)", ret);
			goto out;
		}
	}

	ret = 0;

out:
	k_mutex_unlock(&data->bridge_lock);
	return ret;
}

static int sc18is606_spi_configure(const struct device *dev, const struct spi_config *config)
{
	struct nxp_sc18is606_data *data = dev->data;
	uint8_t cfg_byte = 0;
	uint8_t buffer[2];

	if ((config->operation & SPI_OP_MODE_SLAVE) != 0U) {
		LOG_ERR("SC18IS606 does not support Slave mode");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Unsupported line configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if (bits > 8) {
		LOG_ERR("Word sizes > 8 bits not supported");
		return -ENOTSUP;
	}

	/* Build SC18IS606  configuration byte*/
	cfg_byte |= FIELD_PREP(SC18IS606_LSB_MASK, (config->operation & SPI_TRANSFER_LSB) >> 4);

	cfg_byte |= FIELD_PREP(SC18IS606_MODE_MASK, (SPI_MODE_GET(config->operation) >> 1));

	cfg_byte |= FIELD_PREP(SC18IS606_FREQ_MASK, config->frequency);

	data->ctx.config = config;

	buffer[0] = SC18IS606_CONFIG_SPI;
	buffer[1] = cfg_byte;
	cfg_byte |= ((config->operation & SPI_TRANSFER_LSB) >> 4) << 5;

	return nxp_sc18is606_transfer(dev, buffer, sizeof(buffer), NULL, 0, NULL);
}

static int sc18is606_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_buffer_set,
				    const struct spi_buf_set *rx_buffer_set)
{
	int ret;

	ret = sc18is606_spi_configure(dev, spi_cfg);
	if (ret < 0) {
		return ret;
	}

	if (!tx_buffer_set && !rx_buffer_set) {
		LOG_ERR("SC18IS606 at least one buffer_set should be set");
		return -EINVAL;
	}

	/* CS line to be Used */
	uint8_t ss_idx = spi_cfg->slave;

	if (ss_idx > 2) {
		LOG_ERR("SC18IS606: Invalid SS Index (%u) must be 0-2", ss_idx);
		return -EINVAL;
	}

	uint8_t function_id = (1 << ss_idx) & 0x07;

	if (tx_buffer_set && tx_buffer_set->buffers && tx_buffer_set->count > 0) {
		for (size_t i = 0; i < tx_buffer_set->count; i++) {
			const struct spi_buf *tx_buf = &tx_buffer_set->buffers[i];

			uint8_t id_buf[1] = {function_id};

			ret = nxp_sc18is606_transfer(dev, tx_buf->buf, tx_buf->len, NULL, 0,
						     id_buf);
			if (ret < 0) {
				LOG_ERR("SC18IS606: TX of size: %d failed %s", tx_buf->len,
					dev->name);
				return ret;
			}
		}
	}

	if (rx_buffer_set && rx_buffer_set->buffers && rx_buffer_set->count > 0) {
		for (size_t i = 0; i < rx_buffer_set->count; i++) {
			/* Function ID first to select the device */
			uint8_t cmd_buf[1] = {function_id};

			const struct spi_buf *rx_buf = &rx_buffer_set->buffers[i];

			ret = nxp_sc18is606_transfer(dev, cmd_buf, sizeof(cmd_buf), rx_buf->buf,
						     rx_buf->len, NULL);

			if (ret < 0) {
				LOG_ERR("SC18IS606: RX of size: %d failed on  (%s)", rx_buf->len,
					dev->name);
				return ret;
			}
		}
	}

	return ret;
}

int sc18is606_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct nxp_sc18is606_data *data = dev->data;

	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static DEVICE_API(spi, sc18is606_api) = {
	.transceive = sc18is606_spi_transceive,
	.release = sc18is606_spi_release,
};

static void sc18is606_int_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct nxp_sc18is606_data *data = CONTAINER_OF(cb, struct nxp_sc18is606_data, int_cb);

	k_sem_give(&data->int_sem);
}

static int int_gpios_setup(const struct device *dev)
{
	struct nxp_sc18is606_data *data = dev->data;
	const struct nxp_sc18is606_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpios)) {
		LOG_ERR("SC18IS606 Int GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpios, GPIO_INPUT);
	if (ret != 0U) {
		LOG_ERR("Failed to configure SC18IS606 int gpio (%d)", ret);
		return ret;
	}

	ret = k_sem_init(&data->int_sem, 0, 1);
	if (ret != 0U) {
		LOG_ERR("Failed to Initialize Interrupt Semaphore (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_cb, sc18is606_int_isr, BIT(cfg->int_gpios.pin));

	ret = gpio_add_callback(cfg->int_gpios.port, &data->int_cb);
	if (ret != 0U) {
		LOG_ERR("Failed to assign the Interrupt callback (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpios, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0U) {
		LOG_ERR("Failed to configure the GPIO interrupt edge (%d)", ret);
		return ret;
	}

	return ret;
}

static int sc18is606_init(const struct device *dev)
{
	const struct nxp_sc18is606_config *cfg = dev->config;
	struct nxp_sc18is606_data *data = dev->data;
	int ret;

	struct spi_config my_config = {
		.frequency = data->frequency_idx,
		.operation = data->spi_mode,
		.slave = 0,
	};

	if (!device_is_ready(cfg->i2c_controller.bus)) {
		LOG_ERR("I2C controller %s not found", cfg->i2c_controller.bus->name);
		return -ENODEV;
	}

	LOG_INF("Using I2C controller: %s", cfg->i2c_controller.bus->name);

	ret = sc18is606_spi_configure(dev, &my_config);
	if (ret != 0) {
		LOG_ERR("Failed to CONFIGURE the SC18IS606: %d", ret);
		return ret;
	}

	if (cfg->reset_gpios.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpios)) {
			LOG_ERR("SC18IS606 Reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpios, GPIO_OUTPUT_ACTIVE);
		if (ret != 0U) {
			LOG_ERR("Failed to configure SC18IS606 reset GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&cfg->reset_gpios, 0);
		if (ret != 0U) {
			LOG_ERR("Failed to reset Bridge via Reset pin (%d)", ret);
			return ret;
		}
	}

	if (cfg->int_gpios.port) {
		ret = int_gpios_setup(dev);
		if (ret != 0U) {
			LOG_ERR("Could not set up device int_gpios (%d)", ret);
			return ret;
		}
	}
	LOG_INF("SC18IS606 initialized");
	return 0;
}

#define SPI_SC18IS606_DEFINE(inst)                                                                 \
	static struct nxp_sc18is606_data sc18is606_data_##inst = {                                 \
		.frequency_idx = DT_INST_ENUM_IDX(inst, frequency),                                \
		.spi_mode = DT_INST_PROP(inst, spi_mode),                                          \
	};                                                                                         \
	static const struct nxp_sc18is606_config sc18is606_config_##inst = {                       \
		.i2c_controller = I2C_DT_SPEC_GET(DT_PARENT(DT_DRV_INST(inst))),                   \
		.reset_gpios = GPIO_DT_SPEC_GET_OR(DT_INST_PARENT(inst), reset_gpios, {0}),        \
		.int_gpios = GPIO_DT_SPEC_GET_OR(DT_INST_PARENT(inst), int_gpios, {0}),            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sc18is606_init, NULL, &sc18is606_data_##inst,                  \
			      &sc18is606_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
			      &sc18is606_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SC18IS606_DEFINE)
