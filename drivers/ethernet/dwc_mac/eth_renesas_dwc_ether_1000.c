/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_rz_ethernet_gmac

#include <sys/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "eth_dwmac_priv.h"
#include "../eth.h"

/* The DMA bus master interface is a 32-bit AXI interface on this IP */
#define DATA_BUS_WIDTH 32

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii) ||
		     DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii) ||
		     DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rgmii),
	     "Unsupported PHY connection type");

/* AXI Bus Mode register */
#define DWMAC_DMAAXIBMR        (DWMAC_DMA_OFFSET + 0x0028)
#define DWMAC_DMAAXIBMR_BLEN16 BIT(3)

/* M-MPU2 */
#define M_MPU2_STADD_DISABLE_RW_PROTECTION  0x00000000U
#define M_MPU2_ENDADD_DISABLE_RW_PROTECTION 0x00000C00U

/* Ethernet subsystem (ETHSS) */
#define ETHSS_PRCMD_UNLOCK1 0x000000A5U
#define ETHSS_PRCMD_UNLOCK2 0x00000001U
#define ETHSS_PRCMD_UNLOCK3 0x0000FFFEU
#define ETHSS_PRCMD_UNLOCK4 0x00000001U
#define ETHSS_PRCMD_LOCK    0x00000000U

/* MODCTRL.SW_MODE = 001b is the only setting that connects the GMAC on port 2 */
#define ETHSS_SW_MODE_GMAC_PORT2 1U
#define ETHSS_GMAC_PORT          2U

#define ETHSS_CONVCTRL_MODE            GENMASK(4, 0)
#define ETHSS_CONVCTRL_MODE_MII        0x00U
#define ETHSS_CONVCTRL_MODE_RMII_10M   0x04U
#define ETHSS_CONVCTRL_MODE_RMII_100M  0x05U
#define ETHSS_CONVCTRL_MODE_RGMII_10M  0x08U
#define ETHSS_CONVCTRL_MODE_RGMII_100M 0x09U
#define ETHSS_CONVCTRL_MODE_RGMII_1000 0x0AU
#define ETHSS_CONVCTRL_FULLD           BIT(8)

/* The low two bits of the mode field select the speed once the mode is set */
#define ETHSS_CONVCTRL_SPEED      GENMASK(1, 0)
#define ETHSS_CONVCTRL_SPEED_10M  0x0U
#define ETHSS_CONVCTRL_SPEED_100M 0x1U
#define ETHSS_CONVCTRL_SPEED_1000 0x2U

#if DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
#define ETHSS_CONVCTRL_MODE_INIT ETHSS_CONVCTRL_MODE_MII
#elif DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)
#define ETHSS_CONVCTRL_MODE_INIT ETHSS_CONVCTRL_MODE_RMII_100M
#else
#define ETHSS_CONVCTRL_MODE_INIT ETHSS_CONVCTRL_MODE_RGMII_1000
#endif

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static void eth_renesas_gmac_init(void)
{
	/* Let the GMAC access the whole address space through the M-MPU2 */
	uint8_t region_num = (BSP_FEATURE_BSP_MASTER_MPU_REGION_TYPE == 1) ? 8 : 16;

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SYSTEM);

	for (uint8_t i = 0; i < region_num; i++) {
		R_MPU2->RGN[i].STADD = M_MPU2_STADD_DISABLE_RW_PROTECTION;
		R_MPU2->RGN[i].ENDADD = M_MPU2_ENDADD_DISABLE_RW_PROTECTION;
	}

	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SYSTEM);

	/* Take the GMAC out of module stop and release its resets. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	R_BSP_MODULE_START(FSP_IP_GMAC, 0);
	R_BSP_ModuleResetDisable(BSP_MODULE_RESET_GMAC0_PCLKH);
	R_BSP_ModuleResetDisable(BSP_MODULE_RESET_GMAC0_PCLKM);
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);
}

static void eth_renesas_ethss_unlock(void)
{
	R_ETHSS->PRCMD = ETHSS_PRCMD_UNLOCK1;
	R_ETHSS->PRCMD = ETHSS_PRCMD_UNLOCK2;
	R_ETHSS->PRCMD = ETHSS_PRCMD_UNLOCK3;
	R_ETHSS->PRCMD = ETHSS_PRCMD_UNLOCK4;
}

static void eth_renesas_ethss_lock(void)
{
	R_ETHSS->PRCMD = ETHSS_PRCMD_LOCK;
}

static void eth_renesas_ethss_set_converter(uint8_t port, enum phy_link_speed speed)
{
	uint32_t convctrl;

	convctrl = R_ETHSS->CONVCTRL[port];
	convctrl &= ~(ETHSS_CONVCTRL_SPEED | ETHSS_CONVCTRL_FULLD);

	if (PHY_LINK_IS_SPEED_1000M(speed)) {
		convctrl |= FIELD_PREP(ETHSS_CONVCTRL_SPEED, ETHSS_CONVCTRL_SPEED_1000);
	} else if (PHY_LINK_IS_SPEED_100M(speed)) {
		convctrl |= FIELD_PREP(ETHSS_CONVCTRL_SPEED, ETHSS_CONVCTRL_SPEED_100M);
	} else {
		convctrl |= FIELD_PREP(ETHSS_CONVCTRL_SPEED, ETHSS_CONVCTRL_SPEED_10M);
	}

	if (PHY_LINK_IS_FULL_DUPLEX(speed)) {
		convctrl |= ETHSS_CONVCTRL_FULLD;
	}

	R_ETHSS->CONVCTRL[port] = convctrl;
}

static void eth_renesas_ethss_init(void)
{
	uint32_t convctrl;

	/* Power the subsystem on and release it, and the MII converter, from reset. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	R_BSP_MODULE_START(FSP_IP_ETHSS, 0);
	R_BSP_ModuleResetDisable(BSP_MODULE_RESET_ESC_ETH_SUBSYSTEM);
	R_BSP_ModuleResetDisable(BSP_MODULE_RESET_MII);
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);

	/* Enable write access */
	eth_renesas_ethss_unlock();

	/* MODCTRL.SW_MODE = 001b is the only setting that connects the GMAC on port 2 */
	R_ETHSS->MODCTRL_b.SW_MODE = ETHSS_SW_MODE_GMAC_PORT2;

	/*
	 * Set the converter for the PHY connection type, at full duplex.
	 * dwmac_platform_link_speed_changed() sets the speed and duplex once
	 * the link is up.
	 */
	convctrl = R_ETHSS->CONVCTRL[ETHSS_GMAC_PORT];
	convctrl &= ~(ETHSS_CONVCTRL_MODE | ETHSS_CONVCTRL_FULLD);
	convctrl |= ETHSS_CONVCTRL_MODE_INIT | ETHSS_CONVCTRL_FULLD;
	R_ETHSS->CONVCTRL[ETHSS_GMAC_PORT] = convctrl;

	/* Release the converter of this port from reset. */
	R_ETHSS->CONVRST_b.PHYIR |= BIT(ETHSS_GMAC_PORT);

	/* Disable write access */
	eth_renesas_ethss_lock();
}

void dwmac_platform_link_speed_changed(const struct device *dev __unused, enum phy_link_speed speed)
{
	if (DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)) {
		/* The converter runs in through mode, there is nothing to retune */
		return;
	}

	/* Set the converter on every speed change */
	eth_renesas_ethss_unlock();
	eth_renesas_ethss_set_converter(ETHSS_GMAC_PORT, speed);
	eth_renesas_ethss_lock();
}

int dwmac_bus_init(const struct device *dev __unused)
{
	int ret;

	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins (%d)", ret);
		return ret;
	}

	eth_renesas_gmac_init();
	eth_renesas_ethss_init();

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

	DWMAC_REG_WRITE(DWMAC_DMABMR, DWMAC_DMABMR_AAL | DWMAC_DMABMR_FB);
	DWMAC_REG_WRITE(DWMAC_DMAAXIBMR, DWMAC_REG_READ(DWMAC_DMAAXIBMR) | DWMAC_DMAAXIBMR_BLEN16);

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, sbd, irq), DT_INST_IRQ_BY_NAME(0, sbd, priority),
		    dwmac_isr, DEVICE_DT_INST_GET(0), DT_INST_IRQ_BY_NAME(0, sbd, flags));
	irq_enable(DT_INST_IRQ_BY_NAME(0, sbd, irq));

	return net_eth_mac_load(&mac_cfg, p->mac_addr);
}

static const struct dwmac_config dwmac_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle)),
	.clock = NULL,
	.mac_clk = NULL,
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
