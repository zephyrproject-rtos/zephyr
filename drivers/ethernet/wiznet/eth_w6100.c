/*
 * SPDX-FileCopyrightText: Copyright 2020 Linumiz
 * SPDX-FileCopyrightText: Copyright 2026 Sayed Naser Moravej
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w6100, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#include "eth_w6100_priv.h"
#include "eth_wiznet.h"

#define W6100_SPI_BLOCK_SELECT(addr)  (((addr) >> 16) & 0x1f)
#define W6100_SPI_READ_CONTROL(addr)  (W6100_SPI_BLOCK_SELECT(addr) << 3)
#define W6100_SPI_WRITE_CONTROL(addr) ((W6100_SPI_BLOCK_SELECT(addr) << 3) | BIT(2))

static int w6100_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t cmd[3] = {addr >> 8, addr, W6100_SPI_READ_CONTROL(addr)};
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

static int w6100_spi_write(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t cmd[3] = {addr >> 8, addr, W6100_SPI_WRITE_CONTROL(addr)};
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

static int w6100_soft_reset(const struct device *dev)
{
	uint8_t cmd = CHPLCKR_UNLOCK;
	uint8_t mask = 0;
	int ret;

	/* SYCR0 and SYCR1 only accept writes while CHPLCKR is unlocked */
	w6100_spi_write(dev, W6100_CHPLCKR, &cmd, 1);
	cmd = SYCR0_RST;
	w6100_spi_write(dev, W6100_SYCR0, &cmd, 1);
	cmd = SYCR0_NORMAL;
	w6100_spi_write(dev, W6100_SYCR0, &cmd, 1);
	cmd = CHPLCKR_LOCK;
	ret = w6100_spi_write(dev, W6100_CHPLCKR, &cmd, 1);
	if (ret < 0) {
		return ret;
	}

	/* disable interrupt */
	return w6100_spi_write(dev, W6100_SIMR, &mask, 1);
}

static void w6100_set_macaddr(const struct device *dev)
{
	struct wiznet_runtime *ctx = dev->data;
	uint8_t lock = NETLCKR_UNLOCK;

	w6100_spi_write(dev, W6100_NETLCKR, &lock, 1);
	w6100_spi_write(dev, W6100_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
	lock = NETLCKR_LOCK;
	w6100_spi_write(dev, W6100_NETLCKR, &lock, 1);
}

static void w6100_update_link_status(const struct device *dev)
{
	struct wiznet_runtime *ctx = dev->data;
	enum phy_link_speed speed;
	uint8_t physr;

	if (w6100_spi_read(dev, W6100_PHYSR, &physr, 1) < 0) {
		return;
	}

	if (IS_BIT_SET(physr, W6100_PHYSR_LNK_BIT)) {
		if (ctx->state.is_up != true) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		if (IS_BIT_SET(physr, W6100_PHYSR_SPD_BIT)) {
			speed = IS_BIT_SET(physr, W6100_PHYSR_DPX_BIT) ? LINK_FULL_100BASE
								       : LINK_HALF_100BASE;
		} else {
			speed = IS_BIT_SET(physr, W6100_PHYSR_DPX_BIT) ? LINK_FULL_10BASE
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

static void w6100_memory_configure(const struct device *dev)
{
	uint8_t mem = 0x10;

	/* Configure RX & TX memory to 16K */
	w6100_spi_write(dev, W6100_Sn_RXMEM_SIZE(0), &mem, 1);
	w6100_spi_write(dev, W6100_Sn_TXMEM_SIZE(0), &mem, 1);

	mem = 0;
	for (int i = 1; i < 8; i++) {
		w6100_spi_write(dev, W6100_Sn_RXMEM_SIZE(i), &mem, 1);
		w6100_spi_write(dev, W6100_Sn_TXMEM_SIZE(i), &mem, 1);
	}
}

static void w6100_clear_pending(const struct device *dev)
{
	uint8_t slir;

	w6100_spi_read(dev, W6100_SLIR, &slir, 1);

	/* SLIRCLR is write-1-to-clear; nothing to do when slir == 0 */
	if (slir != 0U) {
		w6100_spi_write(dev, W6100_SLIRCLR, &slir, 1);
	}
}

static const struct wiznet_chip_ops w6100_ops = {
	.spi_read = w6100_spi_read,
	.spi_write = w6100_spi_write,
	.soft_reset = w6100_soft_reset,
	.set_macaddr = w6100_set_macaddr,
	.update_link_status = w6100_update_link_status,
	.memory_configure = w6100_memory_configure,
	.clear_pending = w6100_clear_pending,
};

static const struct wiznet_regs w6100_regs = {
	.shar = W6100_SHAR,
	.imr = W6100_SIMR,
	.s0_mr = W6100_S0_MR,
	.s0_cr = W6100_S0_CR,
	.s0_ir = W6100_S0_IR,
	.s0_irclr = W6100_S0_IRCLR,
	.s0_tx_wr = W6100_S0_TX_WR,
	.s0_rx_rsr = W6100_S0_RX_RSR,
	.s0_rx_rd = W6100_S0_RX_RD,
	.tx_mem_start = W6100_Sn_TX_MEM_START,
	.tx_mem_size = W6100_TX_MEM_SIZE,
	.rx_mem_start = W6100_Sn_RX_MEM_START,
	.rx_mem_size = W6100_RX_MEM_SIZE,
	.s0_mr_macraw = S0_MR_MACRAW,
	.s0_mr_mf_bit = W6100_S0_MR_MF,
};

static const struct ethernet_api w6100_api_funcs = {
	.iface_api.init = wiznet_iface_init,
	.get_capabilities = wiznet_get_capabilities,
	.set_config = wiznet_set_config,
	.start = wiznet_hw_start,
	.stop = wiznet_hw_stop,
	.get_phy = wiznet_get_phy,
	.send = wiznet_tx,
};

static DEVICE_API(ethphy, w6100_phy_driver_api) = {
	.get_link = wiznet_get_link_state,
};

static int w6100_init(const struct device *dev)
{
	uint8_t rtr[2];
	int err;

	err = wiznet_init(dev);
	if (err != 0) {
		return err;
	}

	/* check retry time value */
	w6100_spi_read(dev, W6100_RTR, rtr, 2);
	if (sys_get_be16(rtr) != RTR_DEFAULT) {
		LOG_ERR("Unable to read RTR register");
		return -ENODEV;
	}

	LOG_INF("W6100 Initialized");

	return 0;
}

#define W6100_DEFINE(node)                                                                         \
	WIZNET_DEVICE_DEFINE(node, w6100_init, &w6100_api_funcs, &w6100_phy_driver_api,            \
			     &w6100_ops, &w6100_regs, CONFIG_ETH_W6100_RX_THREAD_STACK_SIZE,       \
			     CONFIG_ETH_W6100_RX_THREAD_PRIO, CONFIG_ETH_W6100_TIMEOUT,            \
			     CONFIG_ETH_W6100_POLL_PERIOD, CONFIG_ETH_W6100_MONITOR_PERIOD,        \
			     T_RST_US, T_STA_mS)

DT_FOREACH_STATUS_OKAY(wiznet_w6100, W6100_DEFINE)
