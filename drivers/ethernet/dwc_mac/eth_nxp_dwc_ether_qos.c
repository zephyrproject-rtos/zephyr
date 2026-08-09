/*
 * Driver for Synopsys DesignWare MAC
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP specific glue.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_enet_qos

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/crc.h>
#include <zephyr/irq.h>
#include <fsl_device_registers.h>

#include "eth_dwmac_priv.h"

/* The DMA bus master interface is 32-bit on this IP */
#define DATA_BUS_WIDTH 32

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

#if DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
#define PHY_MODE 0U
#elif DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)
#define PHY_MODE 1U
#else
#error "Unsupported PHY connection type"
#endif

/*
 * OUI used when devicetree carries no MAC address and one has to be derived
 * from the chip's unique ID. NXP prints a per-board address from this same OUI
 * on the board label, but does not store it anywhere the SoC can read; set
 * local-mac-address in devicetree to use that one instead.
 */
#define NXP_OUI_BYTE_0 0x00
#define NXP_OUI_BYTE_1 0x04
#define NXP_OUI_BYTE_2 0x9f

/* Locally administered address bit of the first MAC octet */
#define ETH_MAC_LAA_BIT 0x02

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

/* The mc_cgm clock cell is itself named "name", hence the repetition. */
#define NXP_ETH_CLOCK_SUBSYS(clk)                                                                 \
	(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(0, clk, name)

static const clock_control_subsys_t eth0_clocks[] = {
	NXP_ETH_CLOCK_SUBSYS(tx),
	NXP_ETH_CLOCK_SUBSYS(rx),
	NXP_ETH_CLOCK_SUBSYS(ptp),
	NXP_ETH_CLOCK_SUBSYS(mac),
};

int dwmac_bus_init(const struct device *dev)
{
	const struct dwmac_config *cfg = dev->config;
	int ret;

	/* Mux the pads first: the clocks below are derived from what they carry. */
	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	/* Select the PHY interface. */
	DCM_GPR->DCMRWF1 = (DCM_GPR->DCMRWF1 & ~DCM_GPR_DCMRWF1_RMII_MII_SEL_MASK) |
			   DCM_GPR_DCMRWF1_RMII_MII_SEL(PHY_MODE);

	/*
	 * The transmit, receive and timestamp clocks are derived from clocks the
	 * PHY drives into the SoC, which is why the pads are muxed first.
	 */
	for (size_t n = 0; n < ARRAY_SIZE(eth0_clocks); n++) {
		ret = clock_control_on(cfg->clock, eth0_clocks[n]);
		if (ret != 0) {
			LOG_ERR("Failed to enable ethernet clock #%zu (%d)", n, ret);
			return ret;
		}
	}

	return 0;
}

#define DESCRIPTOR_ALIGNMENT ((DATA_BUS_WIDTH) / (BITS_PER_BYTE))
#if defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache __aligned(DESCRIPTOR_ALIGNMENT)
#else
/*
 * The core driver maintains the cache for the packet buffers but not for the
 * descriptors, so those have to live in memory the DMA and the CPU see alike.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_DCACHE),
	     "DMA descriptors would be cached; enable CONFIG_ARM_MPU to get a nocache region");
#define __desc_mem __aligned(DESCRIPTOR_ALIGNMENT)
#endif

/* Descriptor rings in uncached memory */
static struct dwmac_dma_desc dwmac_tx_descs[NB_TX_DESCS] __desc_mem;
static struct dwmac_dma_desc dwmac_rx_descs[NB_RX_DESCS] __desc_mem;

static int nxp_load_mac_addr(const struct net_eth_mac_config *cfg, uint8_t *mac_addr)
{
	uint8_t unique_device_id[16] = {0};
	ssize_t uuid_length;
	uint32_t hash;
	int ret;

	ret = net_eth_mac_load(cfg, mac_addr);
	if (ret != -ENODATA) {
		if (ret < 0) {
			LOG_ERR("Failed to load MAC address (%d)", ret);
		}

		return ret;
	}

	/*
	 * Nothing defined by the user, hash the chip's unique ID. Note this is
	 * not universally unique, it just is probably unique on a network.
	 */
	uuid_length = hwinfo_get_device_id(unique_device_id,
					   sizeof(unique_device_id));
	if (uuid_length <= 0) {
		/*
		 * Hashing the empty buffer would give every affected board the
		 * same address, so refuse rather than hand out a duplicate.
		 */
		return (uuid_length < 0) ? (int)uuid_length : -ENODATA;
	}

	hash = crc24_pgp(unique_device_id, (size_t)uuid_length);

	/* Setting LAA bit because it is not guaranteed universally unique */
	mac_addr[0] = NXP_OUI_BYTE_0 | ETH_MAC_LAA_BIT;
	mac_addr[1] = NXP_OUI_BYTE_1;
	mac_addr[2] = NXP_OUI_BYTE_2;
	mac_addr[3] = FIELD_GET(0xFF0000, hash);
	mac_addr[4] = FIELD_GET(0x00FF00, hash);
	mac_addr[5] = FIELD_GET(0x0000FF, hash);

	return 0;
}

#define NXP_ETH_IRQ_CONNECT(name)                                                                  \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, name, irq),                                     \
			    DT_INST_IRQ_BY_NAME(0, name, priority), dwmac_isr,                     \
			    DEVICE_DT_INST_GET(0), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(0, name, irq));                                     \
	} while (0)

int dwmac_platform_init(const struct device *dev)
{
	const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);
	struct dwmac_priv *p = dev->data;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	/* basic configuration for this platform */
	DWMAC_REG_WRITE(MAC_CONF,
			MAC_CONF_PS |
			MAC_CONF_FES |
			MAC_CONF_DM);
	DWMAC_REG_WRITE(DMA_SYSBUS_MODE,
			DMA_SYSBUS_MODE_AAL |
			DMA_SYSBUS_MODE_FB);

	/*
	 * Set up IRQs (still masked for now). The MAC raises DMA transfer
	 * completion on the dedicated tx/rx lines and everything else on the
	 * common line, so all three share the same handler.
	 */
	NXP_ETH_IRQ_CONNECT(common);
	NXP_ETH_IRQ_CONNECT(tx);
	NXP_ETH_IRQ_CONNECT(rx);

	return nxp_load_mac_addr(&mac_cfg, p->mac_addr);
}

/* Our private device instance */
static const struct dwmac_config dwmac_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.phy_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(0, phy_handle)),
	.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.mac_clk = NXP_ETH_CLOCK_SUBSYS(mac),
#if defined(CONFIG_PTP_CLOCK_DWC_MAC)
	.ptp_clock = DEVICE_DT_GET(DT_INST_CHILD(0, ptp_clock)),
	.ptp_clk = NXP_ETH_CLOCK_SUBSYS(ptp),
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
