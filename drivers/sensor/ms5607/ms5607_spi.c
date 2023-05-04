/*
 * Copyright (c) 2019 Thomas Schmid <tom@lfence.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5607

#include <string.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include "ms5607.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ms5607);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

static int ms5607_spi_raw_cmd(const struct ms5607_config *config, uint8_t cmd)
{
	const struct spi_buf buf = {
		.buf = &cmd,
		.len = 1,
	};

	const struct spi_buf_set buf_set = {
		.buffers = &buf,
		.count = 1,
	};

	return spi_write_dt(&config->bus_cfg.spi, &buf_set);
}

static int ms5607_spi_reset(const struct ms5607_config *config)
{
	int err = ms5607_spi_raw_cmd(config, MS5607_CMD_RESET);

	if (err < 0) {
		return err;
	}

	k_sleep(K_MSEC(3));
	return 0;
}

static int ms5607_spi_read_prom(const struct ms5607_config *config, uint8_t cmd,
				uint16_t *val)
{
	int err;

	uint8_t tx[3] = { cmd, 0, 0 };
	const struct spi_buf tx_buf = {
		.buf = tx,
		.len = 3,
	};

	union {
		struct {
			uint8_t pad;
			uint16_t prom_value;
		} __packed;
		uint8_t rx[3];
	} rx;


	const struct spi_buf rx_buf = {
		.buf = &rx,
		.len = 3,
	};

	const struct spi_buf_set rx_buf_set = {
		.buffers = &rx_buf,
		.count = 1,
	};

	const struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	err = spi_transceive_dt(&config->bus_cfg.spi, &tx_buf_set, &rx_buf_set);
	if (err < 0) {
		return err;
	}

	*val = sys_be16_to_cpu(rx.prom_value);

	return 0;
}


static int ms5607_spi_start_conversion(const struct ms5607_config *config, uint8_t cmd)
{
	return ms5607_spi_raw_cmd(config, cmd);
}

static int ms5607_spi_read_adc(const struct ms5607_config *config, uint32_t *val)
{
	int err;

	uint8_t tx[4] = { MS5607_CMD_CONV_READ_ADC, 0, 0, 0 };
	const struct spi_buf tx_buf = {
		.buf = tx,
		.len = 4,
	};

	union {
		struct {
			uint32_t adc_value;
		} __packed;
		uint8_t rx[4];
	} rx;

	const struct spi_buf rx_buf = {
		.buf = &rx,
		.len = 4,
	};

	const struct spi_buf_set rx_buf_set = {
		.buffers = &rx_buf,
		.count = 1,
	};

	const struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	err = spi_transceive_dt(&config->bus_cfg.spi, &tx_buf_set, &rx_buf_set);
	if (err < 0) {
		return err;
	}

	rx.rx[0] = 0;
	*val = sys_be32_to_cpu(rx.adc_value);
	return 0;
}

static int ms5607_spi_check(const struct ms5607_config *config)
{
	if (!spi_is_ready_dt(&config->bus_cfg.spi)) {
		LOG_DBG("SPI bus not ready");
		return -ENODEV;
	}

	return 0;
}

const struct ms5607_transfer_function ms5607_spi_transfer_function = {
	.bus_check = ms5607_spi_check,
	.reset = ms5607_spi_reset,
	.read_prom = ms5607_spi_read_prom,
	.start_conversion = ms5607_spi_start_conversion,
	.read_adc = ms5607_spi_read_adc,
};

#endif
