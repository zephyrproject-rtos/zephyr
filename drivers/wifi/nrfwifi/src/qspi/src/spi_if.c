/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing SPI device interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <string.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "qspi_if.h"
#include "spi_if.h"

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUS_LOG_LEVEL);

#define NRF7002_NODE DT_NODELABEL(nrf70)

static struct qspi_config *spim_config;

static const struct spi_dt_spec spi_spec =
SPI_DT_SPEC_GET(NRF7002_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);

static int spim_xfer_tx(unsigned int addr, void *data, unsigned int len)
{
	int err;
	uint8_t hdr[4] = {
		0x02, /* PP opcode */
		(((addr >> 16) & 0xFF) | 0x80),
		(addr >> 8) & 0xFF,
		(addr & 0xFF)
	};

	const struct spi_buf tx_buf[] = {
		{.buf = hdr,  .len = sizeof(hdr) },
		{.buf = data, .len = len },
	};
	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };


	err = spi_transceive_dt(&spi_spec, &tx, NULL);

	return err;
}


static int spim_xfer_rx(unsigned int addr, void *data, unsigned int len, unsigned int discard_bytes)
{
	uint8_t hdr[] = {
		0x0b, /* FASTREAD opcode */
		(addr >> 16) & 0xFF,
		(addr >> 8) & 0xFF,
		addr & 0xFF,
		0 /* dummy byte */
	};

	const struct spi_buf tx_buf[] = {
		{.buf = hdr,  .len = sizeof(hdr) },
		{.buf = NULL, .len = len },
	};

	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };

	const struct spi_buf rx_buf[] = {
		{.buf = NULL,  .len = sizeof(hdr) + discard_bytes},
		{.buf = data, .len = len },
	};

	const struct spi_buf_set rx = { .buffers = rx_buf, .count = 2 };

	return spi_transceive_dt(&spi_spec, &tx, &rx);
}

int spim_read_reg(uint32_t reg_addr, uint8_t *reg_value)
{
	int err;
	uint8_t tx_buffer[6] = { reg_addr };

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = sizeof(tx_buffer)
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	uint8_t sr[6];

	struct spi_buf rx_buf = {
		.buf = &sr,
		.len = sizeof(sr),
	};
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	err = spi_transceive_dt(&spi_spec, &tx, &rx);

	LOG_DBG("err: %d -> %x %x %x %x %x %x", err, sr[0], sr[1], sr[2], sr[3], sr[4], sr[5]);

	if (err == 0) {
		*reg_value = sr[1];
	}

	return err;
}

int spim_write_reg(uint32_t reg_addr, const uint8_t reg_value)
{
	int err;
	uint8_t tx_buffer[] = { reg_addr, reg_value };

	const struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = spi_transceive_dt(&spi_spec, &tx, NULL);

	if (err) {
		LOG_ERR("SPI error: %d", err);
	}

	return err;
}


int spim_RDSR1(const struct device *dev, uint8_t *rdsr1)
{
	uint8_t val = 0;

	return spim_read_reg(0x1F, &val);
}

int spim_RDSR2(const struct device *dev, uint8_t *rdsr1)
{
	uint8_t val = 0;

	return spim_read_reg(0x2F, &val);
}

int spim_WRSR2(const struct device *dev, const uint8_t wrsr2)
{
	return spim_write_reg(0x3F, wrsr2);
}

int _spim_wait_while_rpu_awake(void)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {

		ret = spim_read_reg(0x1F, &val);

		LOG_DBG("RDSR1 = 0x%x", val);

		if (!ret && (val & RPU_AWAKE_BIT)) {
			break;
		}

		k_msleep(1);
	}

	if (ret || !(val & RPU_AWAKE_BIT)) {
		LOG_ERR("RPU is not awake even after 10ms");
		return -1;
	}

	return val;
}

/* Wait until RDSR2 confirms RPU_WAKEUP_NOW write is successful */
int spim_wait_while_rpu_wake_write(void)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {

		ret = spim_read_reg(0x2F, &val);

		LOG_DBG("RDSR2 = 0x%x", val);

		if (!ret && (val & RPU_WAKEUP_NOW)) {
			break;
		}

		k_msleep(1);
	}

	if (ret || !(val & RPU_WAKEUP_NOW)) {
		LOG_ERR("RPU wakeup write ACK failed even after 10ms");
		return -1;
	}

	return ret;
}

int spim_cmd_rpu_wakeup(uint32_t data)
{
	return spim_write_reg(0x3F, data);
}

unsigned int spim_cmd_sleep_rpu(void)
{
	int err;
	uint8_t tx_buffer[] = { 0x3f, 0x0 };

	const struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	err = spi_transceive_dt(&spi_spec, &tx, NULL);

	if (err) {
		LOG_ERR("SPI error: %d", err);
	}

	return 0;
}

int spim_init(struct qspi_config *config)
{
	if (!spi_is_ready_dt(&spi_spec)) {
		LOG_ERR("Device %s is not ready", spi_spec.bus->name);
		return -ENODEV;
	}

	spim_config = config;

	k_sem_init(&spim_config->lock, 1, 1);

	if (spi_spec.config.frequency >= MHZ(16)) {
		spim_config->qspi_slave_latency = 1;
	}

	LOG_INF("SPIM %s: freq = %d MHz", spi_spec.bus->name,
		spi_spec.config.frequency / MHZ(1));
	LOG_INF("SPIM %s: latency = %d", spi_spec.bus->name, spim_config->qspi_slave_latency);

	return 0;
}

int spim_deinit(void)
{
	LOG_DBG("TODO : %s", __func__);

	return 0;
}

static void spim_addr_check(unsigned int addr, const void *data, unsigned int len)
{
	if ((addr % 4 != 0) || (((unsigned int)data) % 4 != 0) || (len % 4 != 0)) {
		LOG_ERR("%s : Unaligned address %x %x %d %x %x", __func__, addr,
		       (unsigned int)data, (addr % 4 != 0), (((unsigned int)data) % 4 != 0),
		       (len % 4 != 0));
	}
}

int spim_write(unsigned int addr, const void *data, int len)
{
	int status;

	spim_addr_check(addr, data, len);

	addr |= spim_config->addrmask;

	k_sem_take(&spim_config->lock, K_FOREVER);

	status = spim_xfer_tx(addr, (void *)data, len);

	k_sem_give(&spim_config->lock);

	return status;
}

int spim_read(unsigned int addr, void *data, int len)
{
	int status;

	spim_addr_check(addr, data, len);

	addr |= spim_config->addrmask;

	k_sem_take(&spim_config->lock, K_FOREVER);

	status = spim_xfer_rx(addr, data, len, 0);

	k_sem_give(&spim_config->lock);

	return status;
}

static int spim_hl_readw(unsigned int addr, void *data)
{
	int status = -1;

	k_sem_take(&spim_config->lock, K_FOREVER);

	status = spim_xfer_rx(addr, data, 4, 4 * spim_config->qspi_slave_latency);

	k_sem_give(&spim_config->lock);

	return status;
}

int spim_hl_read(unsigned int addr, void *data, int len)
{
	int count = 0;

	spim_addr_check(addr, data, len);

	while (count < (len / 4)) {
		spim_hl_readw(addr + (4 * count), (char *)data + (4 * count));
		count++;
	}

	return 0;
}

/* ------------------------------added for wifi utils -------------------------------- */

int spim_cmd_rpu_wakeup_fn(uint32_t data)
{
	return spim_cmd_rpu_wakeup(data);
}

int spim_cmd_sleep_rpu_fn(void)
{
	return spim_cmd_sleep_rpu();
}

int spim_wait_while_rpu_awake(void)
{
	return _spim_wait_while_rpu_awake();
}

int spi_validate_rpu_wake_writecmd(void)
{
	return spim_wait_while_rpu_wake_write();
}
