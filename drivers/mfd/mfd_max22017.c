/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max22017

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(mfd_max22017, CONFIG_MFD_LOG_LEVEL);

#include <zephyr/drivers/mfd/max22017.h>

struct max22017_config {
	struct spi_dt_spec spi;
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_int;
	bool crc_mode;
};

int max22017_reg_read(const struct device *dev, uint8_t addr, uint16_t *value)
{
	int ret;
	const struct max22017_config *config = dev->config;
	const struct max22017_data *data = dev->data;
	uint8_t rxbuffer[4] = {0};
	size_t recv_len = 3;

	*value = 0;
	addr = FIELD_PREP(MAX22017_SPI_TRANS_ADDR, addr) | FIELD_PREP(MAX22017_SPI_TRANS_DIR, 1);

	if (data->crc_enabled) {
		recv_len += 1;
	}

	const struct spi_buf txb[] = {
		{
			.buf = &addr,
			.len = 1,
		},
	};
	const struct spi_buf rxb[] = {
		{
			.buf = rxbuffer,
			.len = recv_len,
		},
	};

	struct spi_buf_set tx = {
		.buffers = txb,
		.count = ARRAY_SIZE(txb),
	};

	struct spi_buf_set rx = {
		.buffers = rxb,
		.count = ARRAY_SIZE(rxb),
	};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		return ret;
	}

	if (!data->crc_enabled) {
		goto out_skip_crc;
	}

	uint8_t crc_in[] = {addr, rxbuffer[1], rxbuffer[2]};

	ret = crc8(crc_in, 3, MAX22017_CRC_POLY, 0, true);

	if (ret != rxbuffer[3]) {
		LOG_ERR("Reg read: CRC Mismatch calculated / read: %x / %x", ret, rxbuffer[3]);
		return -EINVAL;
	}

out_skip_crc:
	*value = rxbuffer[1] | rxbuffer[2] << 8;
	*value = sys_be16_to_cpu(*value);

	return 0;
}

int max22017_reg_write(const struct device *dev, uint8_t addr, uint16_t value)
{
	uint8_t crc;
	size_t crc_len = 0;
	const struct max22017_config *config = dev->config;
	const struct max22017_data *data = dev->data;

	addr = FIELD_PREP(MAX22017_SPI_TRANS_ADDR, addr) | FIELD_PREP(MAX22017_SPI_TRANS_DIR, 0);
	value = sys_cpu_to_be16(value);

	if (data->crc_enabled) {
		uint8_t crc_in[] = {addr, ((uint8_t *)&value)[0], ((uint8_t *)&value)[1]};

		crc_len = 1;
		crc = crc8(crc_in, 3, MAX22017_CRC_POLY, 0, true);
	}

	const struct spi_buf buf[] = {
		{
			.buf = &addr,
			.len = 1,
		},
		{
			.buf = &value,
			.len = 2,
		},
		{
			.buf = &crc,
			.len = crc_len,
		},
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	return spi_write_dt(&config->spi, &tx);
}

static int max22017_reset(const struct device *dev)
{
	int ret = 0;
	uint16_t ao_sta;
	const struct max22017_config *config = dev->config;

	if (config->gpio_reset.port != NULL) {
		ret = gpio_pin_set_dt(&config->gpio_reset, 0);
		if (ret) {
			return ret;
		}
		k_sleep(K_MSEC(100));
		ret = gpio_pin_set_dt(&config->gpio_reset, 1);
		k_sleep(K_MSEC(500));
	} else {
		ret = max22017_reg_write(dev, MAX22017_GEN_RST_CTRL_OFF,
					 FIELD_PREP(MAX22017_GEN_RST_CTRL_GEN_RST, 1));
		if (ret) {
			return ret;
		}
		k_sleep(K_MSEC(100));
		ret = max22017_reg_write(dev, MAX22017_GEN_RST_CTRL_OFF,
					 FIELD_PREP(MAX22017_GEN_RST_CTRL_GEN_RST, 0));
		if (ret) {
			return ret;
		}
		k_sleep(K_MSEC(500));
		ret = max22017_reg_read(dev, MAX22017_AO_STA_OFF, &ao_sta);
		if (ret) {
			return ret;
		}
		if (FIELD_GET(MAX22017_AO_STA_BUSY_STA, ao_sta)) {
			ret = -EBUSY;
		}
	}
	return ret;
}

static void max22017_isr(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct max22017_data *data = CONTAINER_OF(gpio_cb, struct max22017_data, callback_int);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = k_work_submit(&data->int_work);
	if (ret < 0) {
		LOG_WRN("Could not submit int work: %d", ret);
	}

	k_mutex_unlock(&data->lock);
}

static void max22017_int_worker(struct k_work *work)
{
	struct max22017_data *data = CONTAINER_OF(work, struct max22017_data, int_work);
	const struct device *dev = data->dev;
	int ret;
	uint16_t gen_int;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_INT_OFF, &gen_int);
	if (ret) {
		LOG_ERR("Unable to read GEN_INT register");
		goto fail;
	}

	if (FIELD_GET(MAX22017_GEN_INT_FAIL_INT, gen_int)) {
		LOG_ERR("Boot failure");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_CONV_OVF_INT, gen_int);
	if (ret) {
		LOG_ERR("Conversion failure on channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_OPENWIRE_DTCT_INT, gen_int);
	if (ret) {
		LOG_ERR("Openwire detected on channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	if (FIELD_GET(MAX22017_GEN_INT_HVDD_INT, gen_int)) {
		LOG_ERR("HVDD/HVSS voltage difference below 1.5V");
	}

	if (FIELD_GET(MAX22017_GEN_INT_TMOUT_INT, gen_int)) {
		LOG_ERR("SPI transaction timeout");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_THSHDN_INT, gen_int);
	if (ret) {
		LOG_ERR("Thermal shutdown AO channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_THWRNG_INT, gen_int);
	if (ret) {
		LOG_ERR("Thermal warning AO channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_OVC_INT, gen_int);
	if (ret) {
		LOG_ERR("Over current on channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	if (FIELD_GET(MAX22017_GEN_INT_CRC_INT, gen_int)) {
		LOG_ERR("CRC Error");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_GPI_INT, gen_int);
	if (ret) {
		LOG_INF("GPI Interrupt: %d", ret);
#ifdef CONFIG_GPIO_MAX22017
		uint16_t gpi_sta, lsb;

		ret = max22017_reg_read(dev, MAX22017_GEN_GPI_INT_STA_OFF, &gpi_sta);
		if (ret) {
			goto fail;
		}

		/* Aggregate both positive and negative edge together */
		gpi_sta = FIELD_GET(MAX22017_GEN_GPI_INT_GPI_NEG_EDGE_INT, gpi_sta) |
			  FIELD_GET(MAX22017_GEN_GPI_INT_GPI_POS_EDGE_INT, gpi_sta);
		while (gpi_sta) {
			lsb = LSB_GET(gpi_sta);
			gpio_fire_callbacks(&data->callbacks_gpi, dev, lsb);
			gpi_sta &= ~lsb;
		}
#endif
	}
fail:
	k_mutex_unlock(&data->lock);
}

static int max22017_init(const struct device *dev)
{
	const struct max22017_config *config = dev->config;
	struct max22017_data *data = dev->data;
	uint16_t version;
	int ret;
	uint16_t gen_cnfg = 0, gen_int_en = 0;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI spi %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		ret = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("failed to initialize GPIO reset pin");
			return ret;
		}
	}

	ret = max22017_reset(dev);
	if (ret) {
		LOG_ERR("failed to reset MAX22017");
		return ret;
	}

	data->dev = dev;
	k_work_init(&data->int_work, max22017_int_worker);
	k_mutex_init(&data->lock);

	if (config->gpio_int.port) {
		ret = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (ret) {
			LOG_ERR("failed to initialize GPIO interrupt pin");
			goto fail;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret) {
			LOG_ERR("failed to configure interrupt pin");
			goto fail;
		}

		gpio_init_callback(&data->callback_int, max22017_isr, BIT(config->gpio_int.pin));
		ret = gpio_add_callback(config->gpio_int.port, &data->callback_int);
		if (ret) {
			LOG_ERR("failed to add data ready callback");
			goto fail;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_write(dev, MAX22017_AO_CNFG_OFF, 0);
	if (ret) {
		return ret;
	}

	ret = max22017_reg_write(dev, MAX22017_AO_DATA_CHn_OFF(0), 0);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(dev, MAX22017_AO_DATA_CHn_OFF(1), 0);
	if (ret) {
		goto fail;
	}

	if (config->crc_mode) {
		gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_CRC_EN, 1);
		gen_int_en |= FIELD_PREP(MAX22017_GEN_INTEN_CRC_INTEN, 1);
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_INTEN_OFF, gen_int_en);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_CNFG_OFF, gen_cnfg);
	if (ret) {
		goto fail;
	}

	if (config->crc_mode) {
		data->crc_enabled = true;
	}

	ret = max22017_reg_read(dev, MAX22017_GEN_ID_OFF, &version);
	if (ret) {
		LOG_ERR("Unable to read MAX22017 version over SPI: %d", ret);
		goto fail;
	}

	LOG_INF("MAX22017 version: 0x%lx 0x%lx", FIELD_GET(MAX22017_GEN_ID_PROD_ID, version),
		FIELD_GET(MAX22017_GEN_ID_REV_ID, version));

fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

#define INST_DT_MAX22017(index)                                                                    \
	static const struct max22017_config max22017_config_##index = {                            \
		.spi = SPI_DT_SPEC_INST_GET(index, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),     \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),                       \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(index, rst_gpios, {0}),                     \
		.crc_mode = DT_INST_PROP_OR(index, crc_mode, 0),                                   \
	};                                                                                         \
                                                                                                   \
	static struct max22017_data max22017_data_##index;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, max22017_init, NULL, &max22017_data_##index,                  \
			      &max22017_config_##index, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_MAX22017);
