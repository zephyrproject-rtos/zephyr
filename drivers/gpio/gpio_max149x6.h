/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MAX149X6_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MAX149X6_H_

#define MAX149x6_READ  0
#define MAX149x6_WRITE 1

#define MAX149X6_GET_BIT(val, i) (0x1 & ((val) >> (i)))
#define PRINT_ERR_BIT(bit1, bit2)                                                                  \
	if ((bit1) & (bit2))                                                                       \
		LOG_ERR("[%s] %d", #bit1, bit1)
#define PRINT_ERR(bit)                                                                             \
	if (bit)                                                                                   \
		LOG_ERR("[DIAG] [%s] %d\n", #bit, bit)
#define PRINT_INF(bit) LOG_INFO("[%s] %d\n", #bit, bit)
#define LOG_DIAG(...)  Z_LOG(LOG_LEVEL_ERR, __VA_ARGS__)

/**
 * @brief Compute the CRC5 value for an array of bytes when writing to MAX149X6
 * @param data - array of data to encode
 * @param encode - action to be performed - true(encode), false(decode)
 * @return the resulted CRC5
 */
static uint8_t max149x6_crc(uint8_t *data, bool encode)
{
	uint8_t crc5_start = 0x1f;
	uint8_t crc5_poly = 0x15;
	uint8_t crc5_result = crc5_start;
	uint8_t extra_byte = 0x00;
	uint8_t data_bit;
	uint8_t result_bit;
	int i;

	/*
	 * This is a custom implementation of a CRC5 algorithm, detailed here:
	 * https://www.analog.com/en/app-notes/how-to-program-the-max14906-quadchannel-
	 *					industrial-digital-output-digital-input.html
	 */

	for (i = (encode) ? 0 : 2; i < 8; i++) {
		data_bit = (data[0] >> (7 - i)) & 0x01;
		result_bit = (crc5_result & 0x10) >> 4;
		if (data_bit ^ result_bit) {
			crc5_result = crc5_poly ^ ((crc5_result << 1) & 0x1f);
		} else {
			crc5_result = (crc5_result << 1) & 0x1f;
		}
	}

	for (i = 0; i < 8; i++) {
		data_bit = (data[1] >> (7 - i)) & 0x01;
		result_bit = (crc5_result & 0x10) >> 4;
		if (data_bit ^ result_bit) {
			crc5_result = crc5_poly ^ ((crc5_result << 1) & 0x1f);
		} else {
			crc5_result = (crc5_result << 1) & 0x1f;
		}
	}

	for (i = 0; i < 3; i++) {
		data_bit = (extra_byte >> (7 - i)) & 0x01;
		result_bit = (crc5_result & 0x10) >> 4;
		if (data_bit ^ result_bit) {
			crc5_result = crc5_poly ^ ((crc5_result << 1) & 0x1f);
		} else {
			crc5_result = (crc5_result << 1) & 0x1f;
		}
	}

	return crc5_result;
}

/*
 * @brief Register read/write function for MAX149x6
 *
 * @param dev - MAX149x6 device config.
 * @param addr - Register value to which data is written.
 * @param val - Value which is to be written to requested register.
 * @return 0 in case of success, negative error code otherwise.
 */
static int max149x6_reg_transceive(const struct device *dev, uint8_t addr, uint8_t val,
				   uint8_t *rx_diag_buff, uint8_t rw)
{
	uint8_t crc;
	int ret;

	uint8_t local_rx_buff[MAX149x6_MAX_PKT_SIZE] = {0};
	uint8_t local_tx_buff[MAX149x6_MAX_PKT_SIZE] = {0};

	const struct max149x6_config *config = dev->config;

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

	if (config->crc_en & 0) {
		rx_buf.len++;
	}

	local_tx_buff[0] = FIELD_PREP(MAX149x6_ADDR_MASK, addr) |
			   FIELD_PREP(MAX149x6_CHIP_ADDR_MASK, config->spi_addr) |
			   FIELD_PREP(MAX149x6_RW_MASK, rw & 0x1);
	local_tx_buff[1] = val;

	/* If CRC enabled calculate it */
	if (config->crc_en) {
		local_tx_buff[2] = max149x6_crc(&local_tx_buff[0], true);
	}

	/* write cmd & read resp at once */
	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		LOG_ERR("Err spi_transcieve_dt  [%d]\n", ret);
		return ret;
	}

	/* if CRC enabled check readed */
	if (config->crc_en) {
		crc = max149x6_crc(&local_rx_buff[0], false);
		if (crc != (local_rx_buff[2] & 0x1F)) {
			LOG_ERR("READ CRC ERR (%d)-(%d)\n", crc, (local_rx_buff[2] & 0x1F));
			return -EINVAL;
		}
	}

	if (rx_diag_buff != NULL) {
		rx_diag_buff[0] = local_rx_buff[0];
	}

	/* In case of write we are getting 2 diagnostic bytes - byte0 & byte1
	 * and pass them to diag buffer to be parsed in next stage
	 */
	if ((MAX149x6_WRITE == rw) && (rx_diag_buff != NULL)) {
		rx_diag_buff[1] = local_rx_buff[1];
	} else {
		ret = local_rx_buff[1];
	}

	return ret;
}

#endif
