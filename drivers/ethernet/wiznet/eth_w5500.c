/* W5500 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w5500, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#include "eth_w5500_priv.h"
#include "eth_wiznet.h"

#define W5500_SPI_BLOCK_SELECT(addr)  (((addr) >> 16) & 0x1f)
#define W5500_SPI_READ_CONTROL(addr)  (W5500_SPI_BLOCK_SELECT(addr) << 3)
#define W5500_SPI_WRITE_CONTROL(addr) ((W5500_SPI_BLOCK_SELECT(addr) << 3) | BIT(2))

static int w5500_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t cmd[3] = {addr >> 8, addr, W5500_SPI_READ_CONTROL(addr)};
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = ARRAY_SIZE(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	/* skip the default dummy 0x010203 */
	const struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = 3},
		{.buf = data, .len = len},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int w5500_spi_write(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t cmd[3] = {addr >> 8, addr, W5500_SPI_WRITE_CONTROL(addr)};
	const struct spi_buf tx_buf[2] = {
		{.buf = cmd, .len = ARRAY_SIZE(cmd)},
		{.buf = data, .len = len},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	return spi_write_dt(&cfg->spi, &tx);
}

static int w5500_soft_reset(const struct device *dev)
{
	uint8_t mask = 0;
	uint8_t tmp = MR_RST;
	int ret;

	ret = w5500_spi_write(dev, W5500_MR, &tmp, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);
	tmp = MR_PB;
	w5500_spi_write(dev, W5500_MR, &tmp, 1);

	return w5500_spi_write(dev, W5500_SIMR, &mask, 1);
}

static void w5500_set_macaddr(const struct device *dev)
{
	struct wiznet_runtime *ctx = dev->data;

	w5500_spi_write(dev, W5500_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
}

static void w5500_update_link_status(const struct device *dev)
{
	struct wiznet_runtime *ctx = dev->data;
	enum phy_link_speed speed;
	uint8_t phycfgr;

	if (w5500_spi_read(dev, W5500_PHYCFGR, &phycfgr, 1) < 0) {
		return;
	}

	if (IS_BIT_SET(phycfgr, W5500_PHYCFGR_LNK_BIT)) {
		if (ctx->state.is_up != true) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		if (IS_BIT_SET(phycfgr, W5500_PHYCFGR_SPD_BIT)) {
			speed = IS_BIT_SET(phycfgr, W5500_PHYCFGR_DPX_BIT) ? LINK_FULL_100BASE
									   : LINK_HALF_100BASE;
		} else {
			speed = IS_BIT_SET(phycfgr, W5500_PHYCFGR_DPX_BIT) ? LINK_FULL_10BASE
									   : LINK_HALF_10BASE;
		}

		if (ctx->state.speed != speed) {
			ctx->state.speed = speed;
			LOG_INF("%s: Link speed %s Mb, %s duplex", dev->name,
				PHY_LINK_IS_SPEED_100M(speed) ? "100" : "10",
				PHY_LINK_IS_FULL_DUPLEX(speed) ? "full" : "half");
		}

		return;
	}

	if (ctx->state.is_up) {
		ctx->state.is_up = false;
		ctx->state.speed = 0;
		net_eth_carrier_off(ctx->iface);
	}
}

static void w5500_memory_configure(const struct device *dev)
{
	uint8_t mem = 0x10;
	int i;

	/* Configure RX & TX memory to 16K */
	w5500_spi_write(dev, W5500_Sn_RXMEM_SIZE(0), &mem, 1);
	w5500_spi_write(dev, W5500_Sn_TXMEM_SIZE(0), &mem, 1);

	mem = 0;
	for (i = 1; i < 8; i++) {
		w5500_spi_write(dev, W5500_Sn_RXMEM_SIZE(i), &mem, 1);
		w5500_spi_write(dev, W5500_Sn_TXMEM_SIZE(i), &mem, 1);
	}
}

static const struct wiznet_chip_ops w5500_ops = {
	.api = {
		.iface_api.init = wiznet_iface_init,
		.get_capabilities = wiznet_get_capabilities,
		.set_config = wiznet_set_config,
		.start = wiznet_hw_start,
		.stop = wiznet_hw_stop,
		.get_phy = wiznet_get_phy,
		.send = wiznet_tx,
	},
	.spi_read = w5500_spi_read,
	.spi_write = w5500_spi_write,
	.soft_reset = w5500_soft_reset,
	.set_macaddr = w5500_set_macaddr,
	.update_link_status = w5500_update_link_status,
	.memory_configure = w5500_memory_configure,
};

static const struct wiznet_regs w5500_regs = {
	.shar = W5500_SHAR,
	.imr = W5500_SIMR,
	.s0_mr = W5500_S0_MR,
	.s0_cr = W5500_S0_CR,
	.s0_ir = W5500_S0_IR,
	.s0_irclr = W5500_S0_IR,
	.s0_tx_wr = W5500_S0_TX_WR,
	.s0_rx_rsr = W5500_S0_RX_RSR,
	.s0_rx_rd = W5500_S0_RX_RD,
	.tx_mem_start = W5500_Sn_TX_MEM_START,
	.tx_mem_size = W5500_TX_MEM_SIZE,
	.rx_mem_start = W5500_Sn_RX_MEM_START,
	.rx_mem_size = W5500_RX_MEM_SIZE,
	.s0_mr_macraw = S0_MR_MACRAW,
	.s0_mr_mf_bit = W5500_S0_MR_MF,
};

static DEVICE_API(ethphy, w5500_phy_driver_api) = {
	.get_link = wiznet_get_link_state,
};

static int w5500_init(const struct device *dev)
{
	uint8_t rtr[2];
	int err;

	err = wiznet_init(dev);
	if (err != 0) {
		return err;
	}

	w5500_spi_read(dev, W5500_RTR, rtr, 2);
	if (sys_get_be16(rtr) != RTR_DEFAULT) {
		LOG_ERR("Unable to read RTR register");
		return -ENODEV;
	}

	LOG_INF("W5500 Initialized");

	return 0;
}

#define W5500_DEFINE(node)                                                                         \
	WIZNET_DEVICE_DEFINE(node, w5500_init, &w5500_phy_driver_api,            \
			     &w5500_ops, &w5500_regs, CONFIG_ETH_W5500_RX_THREAD_STACK_SIZE,       \
			     CONFIG_ETH_W5500_RX_THREAD_PRIO, CONFIG_ETH_W5500_TIMEOUT,            \
			     CONFIG_ETH_W5500_POLL_PERIOD, CONFIG_ETH_W5500_MONITOR_PERIOD, 500,   \
			     1)

DT_FOREACH_STATUS_OKAY(wiznet_w5500, W5500_DEFINE)
