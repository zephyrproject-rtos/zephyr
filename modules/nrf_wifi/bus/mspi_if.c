/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing MSPI device interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>

#include "mspi_if.h"

#define NRF7002_NODE DT_NODELABEL(nrf70)

BUILD_ASSERT(DT_ENUM_IDX(NRF7002_NODE, mspi_io_mode) == MSPI_IO_MODE_QUAD_1_4_4,
	     "Only quad 1-4-4 IO mode supported");

static const struct device *bus = DEVICE_DT_GET(DT_BUS(NRF7002_NODE));
static const struct mspi_dev_id mspi_id = MSPI_DEVICE_ID_DT(NRF7002_NODE);
static const struct mspi_dev_cfg mspi_cfg = MSPI_DEVICE_CONFIG_DT(NRF7002_NODE);
static struct qspi_config *config;

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

void mspi_addr_check(unsigned int addr, const void *data, unsigned int len)
{
	if ((addr % 4 != 0) || (((unsigned int)data) % 4 != 0) || (len % 4 != 0)) {
		LOG_ERR("%s : Unaligned address %x %x %d %x %x", __func__, addr, (unsigned int)data,
			(addr % 4 != 0), (((unsigned int)data) % 4 != 0), (len % 4 != 0));
	}
}

static int mspi_config_apply(enum mspi_io_mode io_mode, uint32_t frequency)
{
	const struct mspi_dev_cfg cfg = {
		.io_mode = io_mode,
		.freq = frequency,
	};
	enum mspi_dev_cfg_mask mask = MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_FREQUENCY;
	int rc;

	/* Apply IO mode and frequency */
	LOG_DBG("Applying mode %d, frequency %d MHz", io_mode, frequency / MHZ(1));
	rc = mspi_dev_config(bus, &mspi_id, mask, &cfg);
	if (rc < 0) {
		LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
		return rc;
	}
	return 0;
}

static int mspi_xfer_tx(unsigned int addr, void *data, unsigned int len)
{
	struct mspi_xfer_packet packet = {
		.dir = MSPI_TX,
		.cmd = 0x38, /* PP4IO opcode*/
		.address = addr,
		.num_bytes = len,
		.data_buf = data,
	};
	struct mspi_xfer xfer = {
		.xfer_mode = MSPI_PIO,
		.tx_dummy = 0,
		.cmd_length = 1,
		.addr_length = 3,
		.packets = &packet,
		.num_packet = 1,
		.timeout = 1000,
	};
	int rc;

	LOG_DBG("Writing %d bytes to 0x%06x", len, addr);

	/* Full IO lanes, full frequency */
	rc = mspi_config_apply(mspi_cfg.io_mode, mspi_cfg.freq);
	if (rc < 0) {
		return rc;
	}
	rc = mspi_transceive(bus, &mspi_id, &xfer);
	if (rc < 0) {
		LOG_ERR("%s: transceive() failed: %d", __func__, rc);
		return rc;
	}
	return 0;
}

static int mspi_xfer_rx(unsigned int addr, void *data, unsigned int len, bool extra_rx_latency)
{
	struct mspi_xfer_packet packet = {
		.dir = MSPI_RX,
		.cmd = 0xEB, /* READ4IO opcode*/
		.address = addr << 8,
		.num_bytes = len,
		.data_buf = data,
	};
	struct mspi_xfer xfer = {
		.xfer_mode = MSPI_PIO,
		.tx_dummy = 0,
		.rx_dummy = 8,
		.cmd_length = 1,
		.addr_length = 4,
		.packets = &packet,
		.num_packet = 1,
		.timeout = 1000,
	};
	int rc;

	LOG_DBG("Reading %d bytes from 0x%06x", len, addr);

	/* Extra data latency requested.
	 * The values for "rx_dummy" (both with and without extra latency)
	 * don't match the datasheet diagrams for READ4IO, but neither does
	 * the QSPI implementation. The important thing is that it works.
	 */
	if (extra_rx_latency) {
		xfer.rx_dummy += 8;
	}

	/* Full IO lanes, full frequency */
	rc = mspi_config_apply(mspi_cfg.io_mode, mspi_cfg.freq);
	if (rc < 0) {
		return rc;
	}
	rc = mspi_transceive(bus, &mspi_id, &xfer);
	if (rc < 0) {
		LOG_ERR("%s: transceive() failed: %d", __func__, rc);
		return rc;
	}
	return 0;
}

static int mspi_reg_write(uint8_t reg, uint8_t val, uint32_t frequency)
{
	struct mspi_xfer_packet packet = {
		.dir = MSPI_TX,
		.cmd = reg,
		.num_bytes = sizeof(val),
		.data_buf = &val,
	};
	struct mspi_xfer xfer = {
		.xfer_mode = MSPI_PIO,
		.cmd_length = 1,
		.addr_length = 0,
		.packets = &packet,
		.num_packet = 1,
		.timeout = 1000,
	};
	int rc;

	LOG_DBG("Writing 0x%02x to 0x%02x", val, reg);

	/* Single IO with the requested frequency */
	rc = mspi_config_apply(MSPI_IO_MODE_SINGLE, frequency);
	if (rc < 0) {
		return rc;
	}
	rc = mspi_transceive(bus, &mspi_id, &xfer);
	if (rc < 0) {
		LOG_ERR("%s: transceive() failed: %d", __func__, rc);
		return rc;
	}
	return 0;
}

static int mspi_reg_read(uint8_t reg, uint8_t *val)
{
	struct mspi_xfer_packet packet = {
		.dir = MSPI_RX,
		.cmd = reg,
		.num_bytes = sizeof(*val),
		.data_buf = val,
	};
	struct mspi_xfer xfer = {
		.xfer_mode = MSPI_PIO,
		.cmd_length = 1,
		.addr_length = 0,
		.packets = &packet,
		.num_packet = 1,
		.timeout = 1000,
	};
	int rc;

	LOG_DBG("Reading 0x%02x", reg);

	/* Single IO with the full frequency */
	rc = mspi_config_apply(MSPI_IO_MODE_SINGLE, mspi_cfg.freq);
	if (rc < 0) {
		return rc;
	}
	rc = mspi_transceive(bus, &mspi_id, &xfer);
	if (rc < 0) {
		LOG_ERR("%s: transceive() failed: %d", __func__, rc);
		return rc;
	}
	return 0;
}

int mspi_init(struct qspi_config *config_ptr)
{
	int rc;

	config = config_ptr;
	k_sem_init(&config->lock, 1, 1);

	LOG_INF("MSPI %s: freq = %d MHz", bus->name, mspi_cfg.freq / MHZ(1));

	/* Power up the MSPI bus */
	rc = pm_device_runtime_get(bus);
	if (rc < 0) {
	}

	/* Apply the based configuration on boot */
	return mspi_dev_config(bus, &mspi_id, MSPI_DEVICE_CONFIG_ALL, &mspi_cfg);
}

int mspi_deinit(void)
{
	return pm_device_runtime_put(bus);
}

int mspi_write(unsigned int addr, const void *data, int len)
{
	int status;

	mspi_addr_check(addr, data, len);

	addr |= config->addrmask;

	k_sem_take(&config->lock, K_FOREVER);

	status = mspi_xfer_tx(addr, (void *)data, len);

	k_sem_give(&config->lock);

	return status;
}

int mspi_read(unsigned int addr, void *data, int len)
{
	int status;

	mspi_addr_check(addr, data, len);

	addr |= config->addrmask;

	k_sem_take(&config->lock, K_FOREVER);

	status = mspi_xfer_rx(addr, data, len, false);

	k_sem_give(&config->lock);

	return status;
}

static int mspi_hl_readw(unsigned int addr, void *data)
{
	int status = -1;

	k_sem_take(&config->lock, K_FOREVER);

	status = mspi_xfer_rx(addr, data, 4, mspi_cfg.freq >= MHZ(16));

	k_sem_give(&config->lock);

	return status;
}

int mspi_hl_read(unsigned int addr, void *data, int len)
{
	int count = 0;

	mspi_addr_check(addr, data, len);

	while (count < (len / 4)) {
		mspi_hl_readw(addr + (4 * count), (char *)data + (4 * count));
		count++;
	}

	return 0;
}

int mspi_cmd_sleep_rpu_fn(void)
{
	/* Sleep command can handle full frequency */
	return mspi_reg_write(0x3F, 0x00, mspi_cfg.freq);
}

int mspi_validate_rpu_wake_writecmd(void)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {

		ret = mspi_reg_read(0x2F, &val);

		LOG_DBG("RDSR2 = 0x%02x", val);

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

int mspi_cmd_rpu_wakeup_fn(uint8_t data)
{
	/* Wakeup is only reliable at 8MHz */
	return mspi_reg_write(0x3F, data, MHZ(8));
}

int mspi_wait_while_rpu_awake(void)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {

		ret = mspi_reg_read(0x1F, &val);

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

int mspi_write_reg(uint8_t reg_addr, const uint8_t reg_value)
{
	return mspi_reg_write(reg_addr, reg_value, mspi_cfg.freq);
}

int mspi_read_reg(uint8_t reg_addr, uint8_t *reg_value)
{
	return mspi_reg_read(reg_addr, reg_value);
}
