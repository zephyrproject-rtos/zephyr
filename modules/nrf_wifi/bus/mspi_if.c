/*
 * Copyright (c) 2026 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing MSPI device interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/drivers/mspi/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>

#include "mspi_if.h"

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#define NRF7002_NODE DT_NODELABEL(nrf70)

/* RPU opcodes, see spi_if.c / qspi_if.c for the SPI/QSPI equivalents. */
#define MSPI_IF_OPCODE_PP       0x02 /* Single line page program */
#define MSPI_IF_OPCODE_PP4IO    0x38 /* Quad (1-4-4) page program */
#define MSPI_IF_OPCODE_FASTREAD 0x0B /* Single line fast read */
#define MSPI_IF_OPCODE_READ4IO  0xEB /* Quad (1-4-4) fast read */
#define MSPI_IF_OPCODE_RDSR1    0x1F
#define MSPI_IF_OPCODE_RDSR2    0x2F
#define MSPI_IF_OPCODE_WRSR2    0x3F

#define MSPI_IF_ADDR_LEN        3
#define MSPI_IF_READ_DUMMY      8  /* Single line FASTREAD dummy cycles */
#define MSPI_IF_READ4IO_DUMMY   10 /* Quad READ4IO dummy cycles (RDC4IO = 0xA) */
#define MSPI_IF_XFER_TIMEOUT_MS 10

static const struct device *mspi_bus = DEVICE_DT_GET(DT_BUS(NRF7002_NODE));
static const struct mspi_dev_id dev_id = MSPI_DEVICE_ID_DT(NRF7002_NODE);

/* Target (block transfer) configuration, as described in devicetree. */
static struct mspi_dev_cfg data_cfg = MSPI_DEVICE_CONFIG_DT(NRF7002_NODE);

/* Register access configuration: RDSR1/RDSR2/WRSR2 have no address phase
 * and are always issued single line, regardless of the block transfer mode.
 */
static struct mspi_dev_cfg reg_cfg;

/* NULL until the first transfer, then always points at data_cfg or reg_cfg,
 * whichever was applied to the controller last.
 */
static struct mspi_dev_cfg *active_cfg;

static struct qspi_config *mspi_if_config;

static bool mspi_if_is_quad(void)
{
	return data_cfg.io_mode != MSPI_IO_MODE_SINGLE;
}

/* Acquire the controller and make sure "cfg" is the configuration currently
 * applied to it, reconfiguring only when switching between the register
 * and data classes (mirrors the flash_mspi_nor.c partial-reconfig idiom).
 */
static int mspi_if_acquire(struct mspi_dev_cfg *cfg)
{
	enum mspi_dev_cfg_mask mask;
	int ret;

	k_sem_take(&mspi_if_config->lock, K_FOREVER);

	if (cfg == active_cfg) {
		mask = MSPI_DEVICE_CONFIG_NONE;
	} else {
		mask = MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_DATA_RATE |
		       MSPI_DEVICE_CONFIG_RX_DUMMY | MSPI_DEVICE_CONFIG_TX_DUMMY |
		       MSPI_DEVICE_CONFIG_CMD_LEN | MSPI_DEVICE_CONFIG_ADDR_LEN;
	}

	ret = mspi_dev_config(mspi_bus, &dev_id, mask, cfg);
	if (ret) {
		LOG_ERR("mspi_dev_config failed: %d", ret);
		k_sem_give(&mspi_if_config->lock);
		return ret;
	}

	active_cfg = cfg;

	return 0;
}

static void mspi_if_release(void)
{
	while (mspi_get_channel_status(mspi_bus, 0)) {
		;
	}

	k_sem_give(&mspi_if_config->lock);
}

static int mspi_if_xfer(struct mspi_dev_cfg *cfg, enum mspi_xfer_direction dir, uint32_t cmd,
			uint32_t addr, uint8_t addr_length, uint16_t dummy, void *data,
			uint32_t len)
{
	struct mspi_xfer_packet packet = {
		.dir = dir,
		.cmd = cmd,
		.address = addr,
		.num_bytes = len,
		.data_buf = data,
	};
	struct mspi_xfer xfer = {
		.async = false,
		.xfer_mode = MSPI_PIO,
		.tx_dummy = (dir == MSPI_TX) ? dummy : 0,
		.rx_dummy = (dir == MSPI_RX) ? dummy : 0,
		.cmd_length = 1,
		.addr_length = addr_length,
		.hold_ce = false,
		.packets = &packet,
		.num_packet = 1,
		.timeout = MSPI_IF_XFER_TIMEOUT_MS,
	};
	int ret;

	ret = mspi_if_acquire(cfg);
	if (ret) {
		return ret;
	}

	ret = mspi_transceive(mspi_bus, &dev_id, &xfer);
	if (ret) {
		LOG_ERR("mspi_transceive failed: %d", ret);
	}

	mspi_if_release();

	return ret;
}

static int mspi_if_reg_xfer(enum mspi_xfer_direction dir, uint32_t cmd, uint8_t *value)
{
	return mspi_if_xfer(&reg_cfg, dir, cmd, 0, 0, 0, value, 1);
}

int mspi_if_RDSR1(const struct device *dev, uint8_t *rdsr1)
{
	ARG_UNUSED(dev);

	return mspi_if_reg_xfer(MSPI_RX, MSPI_IF_OPCODE_RDSR1, rdsr1);
}

int mspi_if_RDSR2(const struct device *dev, uint8_t *rdsr2)
{
	ARG_UNUSED(dev);

	return mspi_if_reg_xfer(MSPI_RX, MSPI_IF_OPCODE_RDSR2, rdsr2);
}

int mspi_if_WRSR2(const struct device *dev, const uint8_t wrsr2)
{
	uint8_t value = wrsr2;

	ARG_UNUSED(dev);

	return mspi_if_reg_xfer(MSPI_TX, MSPI_IF_OPCODE_WRSR2, &value);
}

int mspi_if_read_reg_wrapper(const struct device *dev, uint8_t reg_addr, uint8_t *reg_value)
{
	ARG_UNUSED(dev);

	return mspi_if_reg_xfer(MSPI_RX, reg_addr, reg_value);
}

int mspi_if_write_reg_wrapper(const struct device *dev, uint8_t reg_addr, uint8_t reg_value)
{
	uint8_t value = reg_value;

	ARG_UNUSED(dev);

	return mspi_if_reg_xfer(MSPI_TX, reg_addr, &value);
}

static void mspi_if_addr_check(unsigned int addr, const void *data, unsigned int len)
{
	if ((addr % 4 != 0) || (((unsigned int)data) % 4 != 0) || (len % 4 != 0)) {
		LOG_ERR("%s : Unaligned address %x %x %d %x %x", __func__, addr, (unsigned int)data,
			(addr % 4 != 0), (((unsigned int)data) % 4 != 0), (len % 4 != 0));
	}
}

int mspi_if_write(unsigned int addr, const void *data, int len)
{
	uint32_t cmd = mspi_if_is_quad() ? MSPI_IF_OPCODE_PP4IO : MSPI_IF_OPCODE_PP;

	mspi_if_addr_check(addr, data, len);

	addr |= mspi_if_config->addrmask;

	return mspi_if_xfer(&data_cfg, MSPI_TX, cmd, addr, MSPI_IF_ADDR_LEN, 0, (void *)data, len);
}

int mspi_if_read(unsigned int addr, void *data, int len)
{
	uint32_t cmd = mspi_if_is_quad() ? MSPI_IF_OPCODE_READ4IO : MSPI_IF_OPCODE_FASTREAD;
	uint16_t dummy = mspi_if_is_quad() ? MSPI_IF_READ4IO_DUMMY : MSPI_IF_READ_DUMMY;

	mspi_if_addr_check(addr, data, len);

	addr |= mspi_if_config->addrmask;

	return mspi_if_xfer(&data_cfg, MSPI_RX, cmd, addr, MSPI_IF_ADDR_LEN, dummy, data, len);
}

/* High-latency read: same command/address as a normal read, but with extra
 * dummy cycles proportional to qspi_slave_latency, issued one word at a time
 * (see spim_hl_readw / qspi_hl_readw).
 */
static int mspi_if_hl_readw(unsigned int addr, void *data)
{
	uint32_t cmd = mspi_if_is_quad() ? MSPI_IF_OPCODE_READ4IO : MSPI_IF_OPCODE_FASTREAD;
	uint16_t dummy = mspi_if_is_quad() ? MSPI_IF_READ4IO_DUMMY : MSPI_IF_READ_DUMMY;

	dummy += 32 * mspi_if_config->qspi_slave_latency;

	return mspi_if_xfer(&data_cfg, MSPI_RX, cmd, addr, MSPI_IF_ADDR_LEN, dummy, data, 4);
}

int mspi_if_hl_read(unsigned int addr, void *data, int len)
{
	int count = 0;

	mspi_if_addr_check(addr, data, len);

	addr |= mspi_if_config->addrmask;

	while (count < (len / 4)) {
		int ret = mspi_if_hl_readw(addr + (4 * count), (char *)data + (4 * count));

		if (ret) {
			return ret;
		}
		count++;
	}

	return 0;
}

int mspi_if_cmd_rpu_wakeup_fn(uint32_t data)
{
	return mspi_if_WRSR2(NULL, (uint8_t)data);
}

int mspi_if_cmd_sleep_rpu_fn(void)
{
	return mspi_if_WRSR2(NULL, 0);
}

int mspi_if_wait_while_rpu_awake(void)
{
	int ret;
	uint8_t val = 0;

	for (int ii = 0; ii < 10; ii++) {
		ret = mspi_if_RDSR1(NULL, &val);

		LOG_DBG("RDSR1 = 0x%x", val);

		if (ret) {
			return ret;
		}

		if (val & RPU_AWAKE_BIT) {
			/* Callers such as rpu_sleep_status() propagate this
			 * return value straight through to hal_rpu_ps_wake(),
			 * which masks it as the raw status register contents
			 * (see qspi_wait_while_rpu_awake()'s matching "return
			 * val;"). Returning 0 here would make that mask check
			 * always fail.
			 */
			return val;
		}

		k_sleep(K_MSEC(1));
	}

	return -ETIMEDOUT;
}

int mspi_if_validate_rpu_wake_writecmd(void)
{
	int ret;
	uint8_t val = 0;

	ret = mspi_if_RDSR2(NULL, &val);
	if (ret) {
		return ret;
	}

	if (!(val & RPU_WAKEUP_NOW)) {
		LOG_ERR("RPU wake-up write cmd validation failed, RDSR2 = 0x%x", val);
		return -EIO;
	}

	return 0;
}

int mspi_if_init(struct qspi_config *config)
{
	if (!device_is_ready(mspi_bus)) {
		LOG_ERR("Device %s is not ready", mspi_bus->name);
		return -ENODEV;
	}

	if ((data_cfg.io_mode != MSPI_IO_MODE_SINGLE) &&
	    (data_cfg.io_mode != MSPI_IO_MODE_QUAD_1_4_4)) {
		LOG_ERR("Unsupported MSPI io-mode %d", data_cfg.io_mode);
		return -EINVAL;
	}

	mspi_if_config = config;

	k_sem_init(&mspi_if_config->lock, 1, 1);

	/* Register frames: single line, no address phase, no dummy cycles. */
	reg_cfg = data_cfg;
	reg_cfg.io_mode = MSPI_IO_MODE_SINGLE;
	reg_cfg.cmd_length = 1;
	reg_cfg.addr_length = 0;
	reg_cfg.rx_dummy = 0;
	reg_cfg.tx_dummy = 0;

	active_cfg = NULL;

	config->quad_spi = mspi_if_is_quad();

	if (data_cfg.freq >= MHZ(16)) {
		mspi_if_config->qspi_slave_latency = 1;
	}

	LOG_INF("MSPI %s: freq = %d MHz, quad = %d", mspi_bus->name, data_cfg.freq / MHZ(1),
		config->quad_spi);
	LOG_INF("MSPI %s: latency = %d", mspi_bus->name, mspi_if_config->qspi_slave_latency);

	return 0;
}

int mspi_if_deinit(void)
{
	return 0;
}
