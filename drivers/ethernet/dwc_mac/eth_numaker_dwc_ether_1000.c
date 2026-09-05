/*
 * Driver for Synopsys DesignWare MAC
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT nuvoton_numaker_ethernet

#include <sys/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <NuMicro.h>

#include "eth_dwmac_priv.h"

/* The DMA bus master interface is 32-bit on this IP */
#define DATA_BUS_WIDTH 32

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii), "The EMAC only supports RMII");

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static const struct reset_dt_spec eth_reset = RESET_DT_SPEC_INST_GET(0);

static struct numaker_scc_subsys eth_clk_subsys = {
	.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC,
	.pcc = {
		.clk_modidx = DT_INST_CLOCKS_CELL(0, clock_module_index),
		.clk_src = DT_INST_CLOCKS_CELL(0, clock_source),
		.clk_div = DT_INST_CLOCKS_CELL(0, clock_divider),
	},
};

/*
 * The reset bit of the MAC sits in a write protected register and the reset
 * controller does not unlock it, so do that here.
 */
static int eth_numaker_reset(bool assert_reset)
{
	int ret;

	SYS_UnlockReg();
	ret = assert_reset ? reset_line_assert_dt(&eth_reset) : reset_line_deassert_dt(&eth_reset);
	SYS_LockReg();

	return ret;
}

int dwmac_bus_init(const struct device *dev)
{
	const struct dwmac_config *cfg = dev->config;
	int ret;

	/* Hold the MAC in reset while the pins and the clock are configured. */
	ret = eth_numaker_reset(true);
	if (ret != 0) {
		LOG_ERR("Could not assert ethernet reset (%d)", ret);
		return ret;
	}

	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins (%d)", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clock, cfg->mac_clk);
	if (ret < 0) {
		LOG_ERR("Failed to setup ethernet clock (%d)", ret);
		return ret;
	}

	ret = eth_numaker_reset(false);
	if (ret != 0) {
		LOG_ERR("Could not deassert ethernet reset (%d)", ret);
		return ret;
	}

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

/*
 * Spread the three words of the unique ID over the address the way the
 * NuMaker Ethernet driver does, so a board keeps the address it had before
 * it moved to this driver. Bit 9 of the first word carries the locally
 * administered bit and bit 8 the unicast one.
 */
static void eth_numaker_mac_from_uid(const uint8_t *uid, uint8_t *mac_addr)
{
	uint32_t uid0 = sys_get_be32(&uid[0]);
	uint32_t uid1 = sys_get_be32(&uid[4]);
	uint32_t uid2 = sys_get_be32(&uid[8]);
	uint32_t word0, word1;

	word1 = (uid1 & 0x003FFFFFU) | ((uid1 & 0x00030000U) >> 2);
	word1 = (word1 | 0x00000200U) & 0x0000FEFFU;

	word0 = ((uid0 >> 4) << 20) | ((uid1 & 0x000000FFU) << 12) | (uid2 & 0x00000FFFU);

	mac_addr[0] = FIELD_GET(0x0000FF00U, word1);
	mac_addr[1] = FIELD_GET(0x000000FFU, word1);
	mac_addr[2] = FIELD_GET(0xFF000000U, word0);
	mac_addr[3] = FIELD_GET(0x00FF0000U, word0);
	mac_addr[4] = FIELD_GET(0x0000FF00U, word0);
	mac_addr[5] = FIELD_GET(0x000000FFU, word0);
}

static int eth_numaker_mac_load(const struct net_eth_mac_config *cfg, uint8_t *mac_addr)
{
	uint8_t uid[12];
	ssize_t uid_length;
	int ret;

	ret = net_eth_mac_load(cfg, mac_addr);
	if (ret != -ENODATA) {
		return ret;
	}

	/* Nothing defined by the user, derive one from the chip's unique ID. */
	uid_length = hwinfo_get_device_id(uid, sizeof(uid));
	if (uid_length < 0) {
		return (int)uid_length;
	}

	if (uid_length != (ssize_t)sizeof(uid)) {
		/*
		 * A short read would leave part of the address unset, and every
		 * affected board would answer to the same one.
		 */
		return -ENODATA;
	}

	eth_numaker_mac_from_uid(uid, mac_addr);

	return 0;
}

int dwmac_platform_init(const struct device *dev)
{
	const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);
	struct dwmac_priv *p = dev->data;
	int ret;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	ret = eth_numaker_mac_load(&mac_cfg, p->mac_addr);
	if (ret < 0) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
		return ret;
	}

	LOG_DBG("MAC address %02x:%02x:%02x:%02x:%02x:%02x", p->mac_addr[0], p->mac_addr[1],
		p->mac_addr[2], p->mac_addr[3], p->mac_addr[4], p->mac_addr[5]);

	return 0;
}

static const struct dwmac_config dwmac_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle)),
	.clock = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(0))),
	.mac_clk = (clock_control_subsys_t)&eth_clk_subsys,
#if defined(CONFIG_PTP_CLOCK_DWC_MAC)
	.ptp_clock = DEVICE_DT_GET(DT_INST_CHILD(0, ptp_clock)),
	/* The timestamp unit is clocked from the same clock as the MAC. */
	.ptp_clk = (clock_control_subsys_t)&eth_clk_subsys,
#endif
};

static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0, dwmac_probe, NULL, &dwmac_instance, &dwmac_config,
			      CONFIG_ETH_INIT_PRIORITY, &dwmac_api, NET_ETH_MTU);
