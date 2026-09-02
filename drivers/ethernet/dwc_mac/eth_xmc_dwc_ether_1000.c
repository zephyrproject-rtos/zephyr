/*
 * Driver for Synopsys DesignWare MAC
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_xmc4xxx_ethernet

#include <sys/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/random/random.h>
#include <xmc_eth_mac.h>
#include <xmc_scu.h>

#include "eth_dwmac_priv.h"

/* The DMA bus master interface is 32-bit on this IP */
#define DATA_BUS_WIDTH 32

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii) ||
		     DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii),
	     "Unsupported PHY connection type");

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static const XMC_ETH_MAC_PORT_CTRL_t eth_xmc_port_ctrl = {
	.rxd0 = DT_INST_ENUM_IDX(0, rxd0_port_ctrl),
	.rxd1 = DT_INST_ENUM_IDX(0, rxd1_port_ctrl),
	.rxd2 = DT_INST_ENUM_IDX_OR(0, rxd2_port_ctrl, 0),
	.rxd3 = DT_INST_ENUM_IDX_OR(0, rxd3_port_ctrl, 0),
	.clk_rmii = DT_INST_ENUM_IDX(0, rmii_rx_clk_port_ctrl),
	.crs_dv = DT_INST_ENUM_IDX(0, crs_rx_dv_port_ctrl),
	.crs = DT_INST_ENUM_IDX_OR(0, crs_port_ctrl, 0),
	.rxer = DT_INST_ENUM_IDX(0, rxer_port_ctrl),
	.col = DT_INST_ENUM_IDX_OR(0, col_port_ctrl, 0),
	.clk_tx = DT_INST_ENUM_IDX_OR(0, tx_clk_port_ctrl, 0),
	.mode = DT_INST_ENUM_IDX_OR(0, phy_connection_type, 0),
	.mdio = DT_INST_ENUM_IDX_OR(0, mdi_port_ctrl, 0),
};

int dwmac_bus_init(const struct device *dev)
{
	int ret;

	XMC_ETH_MAC_Disable(NULL);
	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	XMC_ETH_MAC_SetPortControl(NULL, eth_xmc_port_ctrl);
	XMC_ETH_MAC_Enable(NULL);

	return 0;
}

#define DESCRIPTOR_ALIGNMENT ((DATA_BUS_WIDTH) / (BITS_PER_BYTE))
#if defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache_noinit __aligned(DESCRIPTOR_ALIGNMENT)
#else
#define __desc_mem __noinit __aligned(DESCRIPTOR_ALIGNMENT)
#endif

static struct dwmac_dma_desc dwmac_tx_descs[NB_TX_DESCS] __desc_mem;
static struct dwmac_dma_desc dwmac_rx_descs[NB_RX_DESCS] __desc_mem;

int dwmac_platform_init(const struct device *dev)
{
	const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);
	struct dwmac_priv *p = dev->data;
	int ret;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	ret = net_eth_mac_load(&mac_cfg, p->mac_addr);
	if (ret == -ENODATA) {
		LOG_DBG("No MAC address configured");
		return 0;
	}

	if (ret < 0) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
		return ret;
	}

	LOG_DBG("MAC address %02x:%02x:%02x:%02x:%02x:%02x", p->mac_addr[0], p->mac_addr[1],
		p->mac_addr[2], p->mac_addr[3], p->mac_addr[4], p->mac_addr[5]);

	return 0;
}

#define XMC_ETH_MAC_CLK 0
#define XMC_ETH_PTP_CLK 1

static int eth_xmc_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
				  uint32_t *rate)
{
	uint32_t clk_id = (uint32_t)sys;

	switch (clk_id) {
	case XMC_ETH_MAC_CLK:
		*rate = XMC_SCU_CLOCK_GetEthernetClockFrequency();
		break;
	case XMC_ETH_PTP_CLK:
		*rate = XMC_SCU_CLOCK_GetSystemClockFrequency();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(clock_control, eth_xmc_clock_api) = {
	.get_rate = eth_xmc_clock_get_rate,
};

DEVICE_DEFINE(eth_xmc_clock, "eth_xmc_clock", NULL, NULL, NULL, NULL,
	      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &eth_xmc_clock_api);

static const struct dwmac_config dwmac_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle)),
	.clock = DEVICE_GET(eth_xmc_clock),
	.mac_clk = (clock_control_subsys_t)XMC_ETH_MAC_CLK,
#if defined(CONFIG_PTP_CLOCK_DWC_MAC)
	.ptp_clock = DEVICE_DT_GET(DT_INST_CHILD(0, ptp_clock)),
	.ptp_clk = (clock_control_subsys_t)XMC_ETH_PTP_CLK,
#endif
};

static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0,
			      dwmac_probe,
			      NULL,
			      &dwmac_instance,
			      &dwmac_config,
			      CONFIG_ETH_INIT_PRIORITY,
			      &dwmac_api,
			      NET_ETH_MTU);
