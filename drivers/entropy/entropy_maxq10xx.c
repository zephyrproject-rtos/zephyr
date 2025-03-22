/*
 * Copyright (c) 2025 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_maxq10xx_trng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/mfd/mfd_maxq10xx.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_maxq10xx, CONFIG_ENTROPY_LOG_LEVEL);

#define MAXQ10XX_CMD_HEADER                0xAA
#define MAXQ10XX_CMD_GET_RANDOM            0xC9
#define MAXQ10XX_CMD_GET_RANDOM_INPUT_DATA 0x02
#define MAXQ10XX_CMD_READ_READY            0x55

#define MAXQ10XX_CRC16_POLYNOMIAL    0xA001
#define MAXQ10XX_CRC16_INITIAL_VALUE 0x0000

#define MAXQ10XX_WAIT_TIME K_MSEC(1)

struct entropy_maxq10xx_config {
	struct spi_dt_spec spi;
	const struct device *parent;
};

static int entropy_maxq10xx_send_cmd(const struct device *dev, uint16_t length)
{
	const struct entropy_maxq10xx_config *config = dev->config;

	uint8_t buffer_tx[9];
	uint16_t crc;
	int ret;

	buffer_tx[0] = MAXQ10XX_CMD_HEADER;
	buffer_tx[1] = 0x00;
	buffer_tx[2] = MAXQ10XX_CMD_GET_RANDOM;
	buffer_tx[3] = 0x00;
	buffer_tx[4] = MAXQ10XX_CMD_GET_RANDOM_INPUT_DATA;

	sys_put_be16(length, &buffer_tx[5]);

	crc = crc16_reflect(MAXQ10XX_CRC16_POLYNOMIAL, MAXQ10XX_CRC16_INITIAL_VALUE, buffer_tx, 7);

	sys_put_le16(crc, &buffer_tx[7]);

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	LOG_HEXDUMP_DBG(buffer_tx, sizeof(buffer_tx), "TX buffer");

	ret = spi_write_dt(&config->spi, &tx);

	return ret;
}

static int entropy_maxq10xx_wait(const struct device *dev)
{
	const struct entropy_maxq10xx_config *config = dev->config;
	uint8_t buffer_rx[1];
	int ret;

	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	while (1) {
		ret = spi_read_dt(&config->spi, &rx);
		if ((ret < 0) || (buffer_rx[0] == MAXQ10XX_CMD_READ_READY)) {
			break;
		}

		k_sleep(MAXQ10XX_WAIT_TIME);
	};

	return ret;
}

static int entropy_maxq10xx_read(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct entropy_maxq10xx_config *config = dev->config;
	uint8_t execution_status[2];
	uint8_t length_data[2];
	uint8_t crc[2];
	uint16_t crc_calc;
	int ret;

	const struct spi_buf rx_buf[] = {
		{
			.buf = execution_status,
			.len = ARRAY_SIZE(execution_status),
		},
		{
			.buf = length_data,
			.len = ARRAY_SIZE(length_data),
		}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_read_dt(&config->spi, &rx);
	if (ret < 0) {
		return ret;
	}

	if (execution_status[0] != 0 || execution_status[1] != 0) {
		LOG_ERR("Execution status: 0x%02X 0x%02X", execution_status[0],
			execution_status[1]);
		return -EIO;
	}

	if (length != sys_get_be16(length_data)) {
		LOG_ERR("Length mismatch: %d != %d", length, sys_get_be16(length_data));
		return -EIO;
	}

	const struct spi_buf rx_data_buf[] = {
		{
			.buf = buffer,
			.len = length,
		},
		{
			.buf = crc,
			.len = sizeof(crc),
		}
	};

	const struct spi_buf_set rx_data = {
		.buffers = rx_data_buf,
		.count = ARRAY_SIZE(rx_data_buf),
	};

	ret = spi_read_dt(&config->spi, &rx_data);
	if (ret < 0) {
		return ret;
	}

	uint8_t header_tx[1] = {MAXQ10XX_CMD_READ_READY};

	crc_calc = crc16_reflect(MAXQ10XX_CRC16_POLYNOMIAL, MAXQ10XX_CRC16_INITIAL_VALUE, header_tx,
				 sizeof(header_tx));
	crc_calc = crc16_reflect(MAXQ10XX_CRC16_POLYNOMIAL, crc_calc, execution_status,
				 sizeof(execution_status));
	crc_calc = crc16_reflect(MAXQ10XX_CRC16_POLYNOMIAL, crc_calc, length_data,
				 sizeof(length_data));
	crc_calc = crc16_reflect(MAXQ10XX_CRC16_POLYNOMIAL, crc_calc, buffer, length);

	if (crc_calc != sys_get_le16(crc)) {
		LOG_ERR("CRC error: 0x%04X != 0x%04X", crc_calc, sys_get_le16(crc));
		return -EIO;
	}

	return ret;
}

static int entropy_maxq10xx_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct entropy_maxq10xx_config *config = dev->config;
	struct k_sem *sem_lock = mfd_maxq10xx_get_lock(config->parent);

	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		return -EAGAIN;
	}

	k_sem_take(sem_lock, K_FOREVER);

	ret = entropy_maxq10xx_send_cmd(dev, length);
	if (ret < 0) {
		LOG_ERR("Failed to send command: %d", ret);
		goto exit;
	}

	ret = entropy_maxq10xx_wait(dev);
	if (ret < 0) {
		LOG_ERR("Failed to wait for ready: %d", ret);
		goto exit;
	}

	ret = entropy_maxq10xx_read(dev, buffer, length);
	if (ret < 0) {
		LOG_ERR("Failed to read data: %d", ret);
	}
exit:
	k_sem_give(sem_lock);
	return ret;
}

static DEVICE_API(entropy, entropy_maxq10xx_api) = {
	.get_entropy = entropy_maxq10xx_get_entropy
};

#define DEFINE_MAXQ10XX_ENTROPY(_num)                                                              \
	static const struct entropy_maxq10xx_config entropy_maxq10xx_config##_num = {              \
		.spi = SPI_DT_SPEC_GET(DT_INST_PARENT(_num), SPI_WORD_SET(8), 0),                  \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(_num)),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_num, NULL, NULL, NULL,                                              \
			      &entropy_maxq10xx_config##_num, POST_KERNEL,                         \
			      CONFIG_MFD_MAXQ10XX_INIT_PRIORITY, &entropy_maxq10xx_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_MAXQ10XX_ENTROPY);
