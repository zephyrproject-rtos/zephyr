/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing QSPI device specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>
#include <stdio.h>
#include <string.h>

#include "spi_if.h"

static struct qspi_config config;

#if defined(CONFIG_NRF70_ON_QSPI)
static struct qspi_dev qspi = {.init = qspi_init,
			       .deinit = qspi_deinit,
			       .read = qspi_read,
			       .write = qspi_write,
			       .hl_read = qspi_hl_read};
#else
static struct qspi_dev spim = {.init = spim_init,
			       .deinit = spim_deinit,
			       .read = spim_read,
			       .write = spim_write,
			       .hl_read = spim_hl_read};
#endif

struct qspi_config *qspi_defconfig(void)
{
	memset(&config, 0, sizeof(struct qspi_config));
#if defined(CONFIG_NRF70_ON_QSPI)
	config.addrmode = NRF_QSPI_ADDRMODE_24BIT;
	config.RDC4IO = 0xA0;
	config.easydma = true;
	config.quad_spi = true;
#endif
	config.addrmask = 0x800000; /* set bit23 (incr. addr mode) */

	config.test_name = "QSPI TEST";
	config.test_hlread = false;
	config.test_iteration = 0;

	config.qspi_slave_latency = 0;

	config.encryption = config.CMD_CNONCE = false;

#if defined(CONFIG_NRF70_ON_QSPI) && (NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC)

	/*For #Bit 6 Enable below: i.e ALL Ones for QSPI Key*/
	memset(&config.p_cfg.key, 0xff, sizeof(config.p_cfg.key));

	config.p_cfg.nonce[0] = 0x16181648;
	config.p_cfg.nonce[1] = 0x0;
	config.p_cfg.nonce[2] = 0x1;

#endif /*CONFIG_NRF70_ON_QSPI && (NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC)*/

	return &config;
}

struct qspi_config *qspi_get_config(void)
{
	return &config;
}

struct qspi_dev *qspi_dev(void)
{
#if CONFIG_NRF70_ON_QSPI
	return &qspi;
#else
	return &spim;
#endif
}
