/* W5100S Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w5100s, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#include "eth_w5100s_priv.h"
#include "eth_wiznet.h"

#define W5100S_SPI_READ_OPCODE  0x0F
#define W5100S_SPI_WRITE_OPCODE 0xF0

/*
 * W5100S SPI frame: opcode (read/write) + 16-bit offset address + N data bytes.
 * The internal address auto-increments, so a single frame streams a burst.
 */
static int w5100s_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t cmd[3] = {
		W5100S_SPI_READ_OPCODE,
		addr >> 8,
		addr,
	};
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = ARRAY_SIZE(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	/* skip the bytes clocked out during the command phase */
	const struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = ARRAY_SIZE(cmd)},
		{.buf = data, .len = len},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int w5100s_spi_write(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t cmd[3] = {
		W5100S_SPI_WRITE_OPCODE,
		addr >> 8,
		addr,
	};
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

static int w5100s_soft_reset(const struct device *dev)
{
	uint8_t mask = 0;
	uint8_t tmp = MR_RST;
	int ret;

	ret = w5100s_spi_write(dev, W5100S_MR, &tmp, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);

	/* disable interrupt */
	return w5100s_spi_write(dev, W5100S_IMR, &mask, 1);
}

static void w5100s_set_macaddr(const struct device *dev)
{
	struct wiznet_runtime *ctx = dev->data;
	uint8_t unlock = NETLCKR_UNLOCK;

	/* GWR/SUBR/SHAR/SIPR are writable only while NETLCKR is unlocked. */
	w5100s_spi_write(dev, W5100S_NETLCKR, &unlock, 1);
	w5100s_spi_write(dev, W5100S_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
}

static void w5100s_update_link_status(const struct device *dev)
{
	struct wiznet_runtime *ctx = dev->data;
	enum phy_link_speed speed;
	uint8_t physr0;
	bool is_100m;
	bool is_full;

	if (w5100s_spi_read(dev, W5100S_PHYSR0, &physr0, 1) < 0) {
		return;
	}

	if (IS_BIT_SET(physr0, W5100S_PHYSR0_LNK_BIT)) {
		if (ctx->state.is_up != true) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		/* PHYSR0 flags: FSPD 1 = 10Mbps, FDPX 1 = half duplex. */
		is_100m = !IS_BIT_SET(physr0, W5100S_PHYSR0_FSPD_BIT);
		is_full = !IS_BIT_SET(physr0, W5100S_PHYSR0_FDPX_BIT);

		if (is_100m) {
			speed = is_full ? LINK_FULL_100BASE : LINK_HALF_100BASE;
		} else {
			speed = is_full ? LINK_FULL_10BASE : LINK_HALF_10BASE;
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

static void w5100s_memory_configure(const struct device *dev)
{
	uint8_t mem = W5100S_RX_MEM_SIZE / 1024;

	/* Assign the whole 8 KB RX/TX memory to socket 0 */
	w5100s_spi_write(dev, W5100S_Sn_RXBUF_SIZE(0), &mem, 1);
	w5100s_spi_write(dev, W5100S_Sn_TXBUF_SIZE(0), &mem, 1);

	mem = 0;
	for (int i = 1; i < W5100S_SOCKET_COUNT; i++) {
		w5100s_spi_write(dev, W5100S_Sn_RXBUF_SIZE(i), &mem, 1);
		w5100s_spi_write(dev, W5100S_Sn_TXBUF_SIZE(i), &mem, 1);
	}
}

static const struct wiznet_chip_ops w5100s_ops = {
	.api = {
		.iface_api.init = wiznet_iface_init,
		.get_capabilities = wiznet_get_capabilities,
		.set_config = wiznet_set_config,
		.start = wiznet_hw_start,
		.stop = wiznet_hw_stop,
		.get_phy = wiznet_get_phy,
		.send = wiznet_tx,
	},
	.spi_read = w5100s_spi_read,
	.spi_write = w5100s_spi_write,
	.soft_reset = w5100s_soft_reset,
	.set_macaddr = w5100s_set_macaddr,
	.update_link_status = w5100s_update_link_status,
	.memory_configure = w5100s_memory_configure,
};

static const struct wiznet_regs w5100s_regs = {
	.shar = W5100S_SHAR,
	.imr = W5100S_IMR,
	.s0_mr = W5100S_S0_MR,
	.s0_cr = W5100S_S0_CR,
	.s0_ir = W5100S_S0_IR,
	.s0_irclr = W5100S_S0_IR,
	.s0_tx_wr = W5100S_S0_TX_WR,
	.s0_rx_rsr = W5100S_S0_RX_RSR,
	.s0_rx_rd = W5100S_S0_RX_RD,
	.tx_mem_start = W5100S_S0_TX_MEM_START,
	.tx_mem_size = W5100S_TX_MEM_SIZE,
	.rx_mem_start = W5100S_S0_RX_MEM_START,
	.rx_mem_size = W5100S_RX_MEM_SIZE,
	.s0_mr_macraw = S0_MR_MACRAW,
	.s0_mr_mf_bit = W5100S_S0_MR_MF,
};

static DEVICE_API(ethphy, w5100s_phy_driver_api) = {
	.get_link = wiznet_get_link_state,
};

static int w5100s_init(const struct device *dev)
{
	uint8_t verr;
	int err;

	err = wiznet_init(dev);
	if (err != 0) {
		return err;
	}

	/* verify the chip is present and talking */
	w5100s_spi_read(dev, W5100S_VERR, &verr, 1);
	if (verr != W5100S_VERSION) {
		LOG_ERR("Unexpected chip version 0x%02x", verr);
		return -ENODEV;
	}

	LOG_INF("W5100S Initialized");

	return 0;
}

#define W5100S_DEFINE(node)                                                                        \
	WIZNET_DEVICE_DEFINE(node, w5100s_init, &w5100s_phy_driver_api,         \
			     &w5100s_ops, &w5100s_regs, CONFIG_ETH_WIZNET_RX_THREAD_STACK_SIZE,    \
			     CONFIG_ETH_WIZNET_RX_THREAD_PRIO, CONFIG_ETH_WIZNET_TIMEOUT,          \
			     CONFIG_ETH_WIZNET_POLL_PERIOD, CONFIG_ETH_WIZNET_MONITOR_PERIOD, 500, \
			     100)

DT_FOREACH_STATUS_OKAY(wiznet_w5100s, W5100S_DEFINE)
