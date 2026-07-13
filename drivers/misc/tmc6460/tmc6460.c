/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc6460

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/misc/tmc6460/tmc6460.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc6460, CONFIG_TMC6460_LOG_LEVEL);

/* TMC6460 Protocol Constants */

/* SPI protocol */
#define TMC6460_SPI_WRITE_BIT     0x80U
#define TMC6460_SPI_DATAGRAM_SIZE 6U

/* UART protocol */
#define TMC6460_UART_SYNC         0x42U
#define TMC6460_UART_WRITE_BIT    0x08U
#define TMC6460_UART_STREAM_WRITE 0x41U

/* Maximum UART datagram sizes */
#define TMC6460_UART_WR_SIZE_CRC   7U /* write with CRC */
#define TMC6460_UART_WR_SIZE       6U /* write without CRC */
#define TMC6460_UART_RD_REQ_CRC    3U /* read request with CRC */
#define TMC6460_UART_RD_REQ        2U /* read request without CRC */
#define TMC6460_UART_RD_RESP_CRC   7U /* read response with CRC */
#define TMC6460_UART_RD_RESP       6U /* read response without CRC */
#define TMC6460_UART_RTMI_SIZE_CRC 6U /* RTMI packet with CRC */
#define TMC6460_UART_RTMI_SIZE     5U /* RTMI packet without CRC */

/* UART RX timeout */
#define TMC6460_UART_RX_TIMEOUT_MS 100U

/* CRC8 Lookup Table - polynomial x^8 + x^4 + x^3 + x + 1 (reflected) */
/* Source: analogdevicesinc/TMC-API tmc/ic/TMC6460/TMC6460.c */
#ifdef CONFIG_TMC6460_UART
static const uint8_t tmc6460_crc_table[256] = {
	0x00, 0x64, 0xC8, 0xAC, 0xE1, 0x85, 0x29, 0x4D,
	0xB3, 0xD7, 0x7B, 0x1F, 0x52, 0x36, 0x9A, 0xFE,
	0x17, 0x73, 0xDF, 0xBB, 0xF6, 0x92, 0x3E, 0x5A,
	0xA4, 0xC0, 0x6C, 0x08, 0x45, 0x21, 0x8D, 0xE9,
	0x2E, 0x4A, 0xE6, 0x82, 0xCF, 0xAB, 0x07, 0x63,
	0x9D, 0xF9, 0x55, 0x31, 0x7C, 0x18, 0xB4, 0xD0,
	0x39, 0x5D, 0xF1, 0x95, 0xD8, 0xBC, 0x10, 0x74,
	0x8A, 0xEE, 0x42, 0x26, 0x6B, 0x0F, 0xA3, 0xC7,
	0x5C, 0x38, 0x94, 0xF0, 0xBD, 0xD9, 0x75, 0x11,
	0xEF, 0x8B, 0x27, 0x43, 0x0E, 0x6A, 0xC6, 0xA2,
	0x4B, 0x2F, 0x83, 0xE7, 0xAA, 0xCE, 0x62, 0x06,
	0xF8, 0x9C, 0x30, 0x54, 0x19, 0x7D, 0xD1, 0xB5,
	0x72, 0x16, 0xBA, 0xDE, 0x93, 0xF7, 0x5B, 0x3F,
	0xC1, 0xA5, 0x09, 0x6D, 0x20, 0x44, 0xE8, 0x8C,
	0x65, 0x01, 0xAD, 0xC9, 0x84, 0xE0, 0x4C, 0x28,
	0xD6, 0xB2, 0x1E, 0x7A, 0x37, 0x53, 0xFF, 0x9B,
	0xB8, 0xDC, 0x70, 0x14, 0x59, 0x3D, 0x91, 0xF5,
	0x0B, 0x6F, 0xC3, 0xA7, 0xEA, 0x8E, 0x22, 0x46,
	0xAF, 0xCB, 0x67, 0x03, 0x4E, 0x2A, 0x86, 0xE2,
	0x1C, 0x78, 0xD4, 0xB0, 0xFD, 0x99, 0x35, 0x51,
	0x96, 0xF2, 0x5E, 0x3A, 0x77, 0x13, 0xBF, 0xDB,
	0x25, 0x41, 0xED, 0x89, 0xC4, 0xA0, 0x0C, 0x68,
	0x81, 0xE5, 0x49, 0x2D, 0x60, 0x04, 0xA8, 0xCC,
	0x32, 0x56, 0xFA, 0x9E, 0xD3, 0xB7, 0x1B, 0x7F,
	0xE4, 0x80, 0x2C, 0x48, 0x05, 0x61, 0xCD, 0xA9,
	0x57, 0x33, 0x9F, 0xFB, 0xB6, 0xD2, 0x7E, 0x1A,
	0xF3, 0x97, 0x3B, 0x5F, 0x12, 0x76, 0xDA, 0xBE,
	0x40, 0x24, 0x88, 0xEC, 0xA1, 0xC5, 0x69, 0x0D,
	0xCA, 0xAE, 0x02, 0x66, 0x2B, 0x4F, 0xE3, 0x87,
	0x79, 0x1D, 0xB1, 0xD5, 0x98, 0xFC, 0x50, 0x34,
	0xDD, 0xB9, 0x15, 0x71, 0x3C, 0x58, 0xF4, 0x90,
	0x6E, 0x0A, 0xA6, 0xC2, 0x8F, 0xEB, 0x47, 0x23,
};

/**
 * @brief Compute CRC8 over a buffer using the TMC6460 reflected CRC table.
 *
 * After the table lookup the result byte is bit-reversed (reflected).
 * This matches the TMC-API implementation in TMC6460.c.
 */
static uint8_t tmc6460_crc8(const uint8_t *data, uint32_t len)
{
	uint8_t crc = 0U;

	while (len--) {
		crc = tmc6460_crc_table[crc ^ *data++];
	}

	/* Flip the result around (bit-reverse the byte) */
	crc = (uint8_t)(((crc >> 1) & 0x55U) | ((crc & 0x55U) << 1));
	crc = (uint8_t)(((crc >> 2) & 0x33U) | ((crc & 0x33U) << 2));
	crc = (uint8_t)(((crc >> 4) & 0x0FU) | ((crc & 0x0FU) << 4));

	return crc;
}
#endif /* CONFIG_TMC6460_UART */

/* Device Structures */
struct tmc6460_config {
	/* SPI bus (only valid when on SPI) */
	struct spi_dt_spec spi;
#ifdef CONFIG_TMC6460_UART
	/* UART device (only valid when on UART) */
	const struct device *uart;
	bool use_crc;
#endif
};

struct tmc6460_data {
	struct k_sem sem;
};

/* SPI Implementation */
#ifdef CONFIG_TMC6460_SPI

static int tmc6460_spi_transceive(const struct device *dev,
				  uint8_t *tx, uint8_t *rx, size_t len)
{
	const struct tmc6460_config *config = dev->config;

	struct spi_buf tx_buf = { .buf = tx, .len = len };
	struct spi_buf rx_buf = { .buf = rx, .len = len };
	struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

	return spi_transceive_dt(&config->spi, &tx_set, &rx_set);
}

static int tmc6460_spi_write(const struct device *dev,
			     uint16_t reg_addr, uint32_t reg_val)
{
	struct tmc6460_data *data = dev->data;
	uint8_t tx[TMC6460_SPI_DATAGRAM_SIZE] = {0};
	uint8_t rx[TMC6460_SPI_DATAGRAM_SIZE] = {0};
	int err;

	tx[0] = TMC6460_SPI_WRITE_BIT | ((reg_addr >> 8) & 0x03U);
	tx[1] = reg_addr & 0xFFU;
	tx[2] = (uint8_t)(reg_val >> 24);
	tx[3] = (uint8_t)(reg_val >> 16);
	tx[4] = (uint8_t)(reg_val >> 8);
	tx[5] = (uint8_t)(reg_val);

	k_sem_take(&data->sem, K_FOREVER);
	err = tmc6460_spi_transceive(dev, tx, rx, TMC6460_SPI_DATAGRAM_SIZE);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("SPI write reg 0x%04x failed: %d", reg_addr, err);
	}

	return err;
}

static int tmc6460_spi_read(const struct device *dev,
			    uint16_t reg_addr, uint32_t *reg_val)
{
	struct tmc6460_data *data = dev->data;
	uint8_t tx[TMC6460_SPI_DATAGRAM_SIZE] = {0};
	uint8_t rx[TMC6460_SPI_DATAGRAM_SIZE] = {0};
	int err;

	tx[0] = (reg_addr >> 8) & 0x03U;
	tx[1] = reg_addr & 0xFFU;

	k_sem_take(&data->sem, K_FOREVER);

	/* First transfer: send read address */
	err = tmc6460_spi_transceive(dev, tx, rx, TMC6460_SPI_DATAGRAM_SIZE);
	if (err < 0) {
		k_sem_give(&data->sem);
		LOG_ERR("SPI read reg 0x%04x request failed: %d", reg_addr, err);
		return err;
	}

	/* Second transfer: clock out response */
	memset(tx, 0, TMC6460_SPI_DATAGRAM_SIZE);
	tx[0] = (reg_addr >> 8) & 0x03U;
	tx[1] = reg_addr & 0xFFU;

	err = tmc6460_spi_transceive(dev, tx, rx, TMC6460_SPI_DATAGRAM_SIZE);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("SPI read reg 0x%04x response failed: %d", reg_addr, err);
		return err;
	}

	*reg_val = ((uint32_t)rx[2] << 24) |
		   ((uint32_t)rx[3] << 16) |
		   ((uint32_t)rx[4] << 8) |
		   ((uint32_t)rx[5]);

	return 0;
}
#endif /* CONFIG_TMC6460_SPI */

/* UART Implementation */
#ifdef CONFIG_TMC6460_UART

static int tmc6460_uart_tx(const struct device *uart,
			   const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart, buf[i]);
	}

	return 0;
}

static int tmc6460_uart_rx(const struct device *uart,
			   uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		k_timepoint_t end = sys_timepoint_calc(
			K_MSEC(TMC6460_UART_RX_TIMEOUT_MS));
		int err;

		do {
			err = uart_poll_in(uart, &buf[i]);
		} while (err == -1 && !sys_timepoint_expired(end));

		if (err != 0) {
			LOG_ERR("UART RX timeout at byte %zu", i);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

/**
 * @brief Check if a received byte is an RTMI datagram header.
 *
 * Per TMC-API TMC6460.c: a byte is an RTMI header if bit 0 is set
 * and bit 7 is clear.  This matches stream-write headers of the form
 * 0x41 | ((index << 1) & 0x0E) => 0x41, 0x43, ..., 0x4F.
 */
static inline bool tmc6460_is_rtmi_header(uint8_t byte)
{
	return (byte & 0x81U) == 0x01U;
}

static int tmc6460_uart_write(const struct device *dev,
			      uint16_t reg_addr, uint32_t reg_val)
{
	const struct tmc6460_config *config = dev->config;
	struct tmc6460_data *data = dev->data;
	uint8_t buf[TMC6460_UART_WR_SIZE_CRC];
	size_t write_size;
	int err;

	/*
	 * UART write datagram:
	 * Byte 0: sync | write_bit | addr[9:8]
	 * Byte 1: addr[7:0]
	 * Bytes 2-5: data (MSB first)
	 * Byte 6 (optional): CRC8
	 */
	buf[0] = TMC6460_UART_SYNC | TMC6460_UART_WRITE_BIT |
		 ((reg_addr >> 4) & 0x30U);
	buf[1] = reg_addr & 0xFFU;
	buf[2] = (uint8_t)(reg_val >> 24);
	buf[3] = (uint8_t)(reg_val >> 16);
	buf[4] = (uint8_t)(reg_val >> 8);
	buf[5] = (uint8_t)(reg_val);

	if (config->use_crc) {
		buf[6] = tmc6460_crc8(buf, 6);
		write_size = TMC6460_UART_WR_SIZE_CRC;
	} else {
		write_size = TMC6460_UART_WR_SIZE;
	}

	k_sem_take(&data->sem, K_FOREVER);
	err = tmc6460_uart_tx(config->uart, buf, write_size);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("UART write reg 0x%04x failed: %d", reg_addr, err);
	}

	return err;
}

static int tmc6460_uart_read(const struct device *dev,
			     uint16_t reg_addr, uint32_t *reg_val)
{
	const struct tmc6460_config *config = dev->config;
	struct tmc6460_data *data = dev->data;
	uint8_t tx_buf[TMC6460_UART_RD_REQ_CRC];
	uint8_t rx_buf[TMC6460_UART_RD_RESP_CRC];
	size_t write_size;
	size_t read_size;
	size_t rtmi_size;
	int err;

	/*
	 * UART read request:
	 * Byte 0: sync | addr[9:8]   (write bit clear)
	 * Byte 1: addr[7:0]
	 * Byte 2 (optional): CRC8
	 */
	tx_buf[0] = TMC6460_UART_SYNC | ((reg_addr >> 4) & 0x30U);
	tx_buf[1] = reg_addr & 0xFFU;

	if (config->use_crc) {
		tx_buf[2] = tmc6460_crc8(tx_buf, 2);
		write_size = TMC6460_UART_RD_REQ_CRC;
		read_size = TMC6460_UART_RD_RESP_CRC;
		rtmi_size = TMC6460_UART_RTMI_SIZE_CRC;
	} else {
		write_size = TMC6460_UART_RD_REQ;
		read_size = TMC6460_UART_RD_RESP;
		rtmi_size = TMC6460_UART_RTMI_SIZE;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Send read request */
	err = tmc6460_uart_tx(config->uart, tx_buf, write_size);
	if (err < 0) {
		k_sem_give(&data->sem);
		LOG_ERR("UART read req 0x%04x TX failed: %d", reg_addr, err);
		return err;
	}

	/*
	 * Receive response. RTMI packets may be interleaved.
	 * Read response starts with a non-RTMI header byte.
	 * RTMI packets start with 0x41|index and are 5 or 6 bytes.
	 */
	while (true) {
		/* Read first byte to determine packet type */
		err = tmc6460_uart_rx(config->uart, &rx_buf[0], 1);
		if (err < 0) {
			k_sem_give(&data->sem);
			return err;
		}

		if (tmc6460_is_rtmi_header(rx_buf[0])) {
			/* RTMI packet: read remaining bytes and discard */
			err = tmc6460_uart_rx(config->uart,
					     &rx_buf[1], rtmi_size - 1);
			if (err < 0) {
				k_sem_give(&data->sem);
				return err;
			}

			if (config->use_crc) {
				uint8_t crc = tmc6460_crc8(rx_buf,
							   rtmi_size - 1);
				if (crc != rx_buf[rtmi_size - 1]) {
					LOG_WRN("RTMI CRC mismatch");
				}
			}
			/* Discard and continue waiting for read response */
			continue;
		}

		/* Not RTMI: this is our read response. Read remaining. */
		err = tmc6460_uart_rx(config->uart,
				     &rx_buf[1], read_size - 1);
		if (err < 0) {
			k_sem_give(&data->sem);
			return err;
		}
		break;
	}

	k_sem_give(&data->sem);

	/* Validate CRC if enabled */
	if (config->use_crc) {
		uint8_t crc = tmc6460_crc8(rx_buf, read_size - 1);

		if (crc != rx_buf[read_size - 1]) {
			LOG_ERR("UART read CRC mismatch for reg 0x%04x",
				reg_addr);
			return -EIO;
		}
	}

	/*
	 * Read response layout (after header bytes):
	 * Byte 0: echo of sync+addr[9:8]
	 * Byte 1: echo of addr[7:0]
	 * Bytes 2-5: data (MSB first)
	 * Byte 6 (optional): CRC8
	 */
	*reg_val = ((uint32_t)rx_buf[2] << 24) |
		   ((uint32_t)rx_buf[3] << 16) |
		   ((uint32_t)rx_buf[4] << 8) |
		   ((uint32_t)rx_buf[5]);

	return 0;
}

/**
 * @brief Write to an RTMI streamed register slot.
 *
 * Uses fire-and-forget stream write protocol:
 * Byte 0: 0x41 | ((rtmi_index << 1) & 0x0E)
 * Bytes 1-4: data (MSB first)
 * Byte 5 (optional): CRC8
 */
int tmc6460_rtmi_stream_write(const struct device *dev,
			      uint8_t rtmi_index, uint32_t value)
{
	const struct tmc6460_config *config = dev->config;
	struct tmc6460_data *data = dev->data;
	uint8_t buf[TMC6460_UART_RTMI_SIZE_CRC];
	size_t write_size;
	int err;

	buf[0] = TMC6460_UART_STREAM_WRITE | ((rtmi_index << 1) & 0x0EU);
	buf[1] = (uint8_t)(value >> 24);
	buf[2] = (uint8_t)(value >> 16);
	buf[3] = (uint8_t)(value >> 8);
	buf[4] = (uint8_t)(value);

	if (config->use_crc) {
		buf[5] = tmc6460_crc8(buf, 5);
		write_size = TMC6460_UART_RTMI_SIZE_CRC;
	} else {
		write_size = TMC6460_UART_RTMI_SIZE;
	}

	k_sem_take(&data->sem, K_FOREVER);
	err = tmc6460_uart_tx(config->uart, buf, write_size);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("RTMI stream write index %u failed: %d",
			rtmi_index, err);
	}

	return err;
}
#endif /* CONFIG_TMC6460_UART */

/* Public Register Access Functions */
int tmc6460_write(const struct device *dev, uint16_t reg_addr,
		  uint32_t reg_val)
{
#ifdef CONFIG_TMC6460_UART
	const struct tmc6460_config *config = dev->config;

	if (config->uart != NULL) {
		return tmc6460_uart_write(dev, reg_addr, reg_val);
	}
#endif
#ifdef CONFIG_TMC6460_SPI
	return tmc6460_spi_write(dev, reg_addr, reg_val);
#else
	return -ENOTSUP;
#endif
}

int tmc6460_read(const struct device *dev, uint16_t reg_addr,
		 uint32_t *reg_val)
{
#ifdef CONFIG_TMC6460_UART
	const struct tmc6460_config *config = dev->config;

	if (config->uart != NULL) {
		return tmc6460_uart_read(dev, reg_addr, reg_val);
	}
#endif
#ifdef CONFIG_TMC6460_SPI
	return tmc6460_spi_read(dev, reg_addr, reg_val);
#else
	return -ENOTSUP;
#endif
}

int tmc6460_enable(const struct device *dev)
{
	uint32_t reg_val;
	int err;

	err = tmc6460_read(dev, TMC6460_MCC_CONFIG_GDRV, &reg_val);
	if (err != 0) {
		return -EIO;
	}

	/* Set DRV_EN bit to enable the gate driver */
	reg_val |= TMC6460_MCC_CONFIG_GDRV_DRV_EN_BIT_MASK;

	return tmc6460_write(dev, TMC6460_MCC_CONFIG_GDRV, reg_val);
}

int tmc6460_disable(const struct device *dev)
{
	uint32_t reg_val;
	int err;

	err = tmc6460_read(dev, TMC6460_MCC_CONFIG_GDRV, &reg_val);
	if (err != 0) {
		return -EIO;
	}

	/* Clear DRV_EN bit to disable the gate driver */
	reg_val &= ~TMC6460_MCC_CONFIG_GDRV_DRV_EN_BIT_MASK;

	return tmc6460_write(dev, TMC6460_MCC_CONFIG_GDRV, reg_val);
}

int tmc6460_poll_flag(const struct device *dev, uint16_t reg_addr,
		      uint32_t flag_mask, uint32_t poll_interval_ms,
		      uint32_t timeout_ms)
{
	uint32_t reg_val;
	int err;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(timeout_ms));

	while (true) {
		err = tmc6460_read(dev, reg_addr, &reg_val);
		if (err != 0) {
			return -EIO;
		}

		if ((reg_val & flag_mask) == flag_mask) {
			return 0;
		}

		if (sys_timepoint_expired(end)) {
			return -ETIMEDOUT;
		}

		if (poll_interval_ms != 0U) {
			k_msleep(poll_interval_ms);
		}
	}
}

/* Device Initialization */
static int tmc6460_init(const struct device *dev)
{
	const struct tmc6460_config *config = dev->config;
	struct tmc6460_data *data = dev->data;

	LOG_DBG("Initializing TMC6460 %s", dev->name);

	k_sem_init(&data->sem, 1, 1);

#ifdef CONFIG_TMC6460_UART
	if (config->uart != NULL) {
		if (!device_is_ready(config->uart)) {
			LOG_ERR("UART device not ready");
			return -ENODEV;
		}
		LOG_DBG("TMC6460 %s ready (UART)", dev->name);
		return 0;
	}
#endif

#ifdef CONFIG_TMC6460_SPI
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}
	LOG_DBG("TMC6460 %s ready (SPI)", dev->name);
	return 0;
#else
	LOG_ERR("No bus configured for TMC6460 %s", dev->name);
	return -ENODEV;
#endif
}

/* Device Instantiation Macros */
/*
 * Bus detection uses DT_INST_ON_BUS(inst, spi) to determine bus type.
 * If on SPI: use SPI_DT_SPEC_INST_GET.
 * If on UART: use DEVICE_DT_GET(DT_INST_BUS(inst)) to get the parent UART.
 */

#ifdef CONFIG_TMC6460_SPI
#define TMC6460_CONFIG_SPI(inst)                                            \
	.spi = SPI_DT_SPEC_INST_GET(inst,                                  \
				    (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |   \
				     SPI_MODE_CPHA | SPI_WORD_SET(8))),
#endif

#ifdef CONFIG_TMC6460_UART
#define TMC6460_CONFIG_UART(inst)                                           \
	.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                          \
	.use_crc = DT_INST_PROP_OR(inst, uart_crc, false),
#endif

#if defined(CONFIG_TMC6460_SPI) && defined(CONFIG_TMC6460_UART)
#define TMC6460_BUS_CONFIG(inst)                                            \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),                             \
		    (TMC6460_CONFIG_SPI(inst)),                             \
		    (TMC6460_CONFIG_UART(inst)))
#elif defined(CONFIG_TMC6460_SPI)
#define TMC6460_BUS_CONFIG(inst) TMC6460_CONFIG_SPI(inst)
#elif defined(CONFIG_TMC6460_UART)
#define TMC6460_BUS_CONFIG(inst) TMC6460_CONFIG_UART(inst)
#endif

#define TMC6460_DEFINE(inst)                                                \
	static struct tmc6460_data tmc6460_data_##inst;                     \
                                                                            \
	static const struct tmc6460_config tmc6460_config_##inst = {        \
		TMC6460_BUS_CONFIG(inst)                                    \
	};                                                                  \
                                                                            \
	DEVICE_DT_INST_DEFINE(inst, tmc6460_init, NULL,                     \
			      &tmc6460_data_##inst,                         \
			      &tmc6460_config_##inst,                       \
			      POST_KERNEL,                                  \
			      CONFIG_TMC6460_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TMC6460_DEFINE)
