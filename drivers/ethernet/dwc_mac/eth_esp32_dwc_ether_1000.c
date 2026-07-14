/*
 * Driver for Synopsys DesignWare MAC
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_eth

#include <sys/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/crc.h>
#include <zephyr/irq.h>

#include "eth_dwmac_priv.h"

#include <esp_mac.h>
#include <hal/emac_ll.h>

#include "../eth_esp32_priv.h"

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii) ||
		     DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii),
	     "Unsupported PHY interface type. Only RMII and MII are supported.");

/* The DMA bus master interface is 32-bit on this IP */
#define DATA_BUS_WIDTH 32

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

PINCTRL_DT_INST_DEFINE(0);

#ifdef CONFIG_SOC_SERIES_ESP32
#define EMAC_EXT_ADDR (&EMAC_EXT)
#else
#define EMAC_EXT_ADDR (NULL)
#endif

int dwmac_bus_init(const struct device *dev)
{
	const struct dwmac_config *cfg = dev->config;
	const struct pinctrl_dev_config *pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);
	int ret;

	ret = clock_control_on(cfg->clock, cfg->mac_clk);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to setup ethernet clock");
		return ret;
	}

	if (DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)) {
		int rmii_clk_gpio = -1;

		ret = esp32_emac_iomux_init_rmii_pinctrl(pcfg, &rmii_clk_gpio);
		if (ret != 0) {
			return -EIO;
		}
#if DT_INST_NODE_HAS_PROP(0, ref_clk_output_gpios)
		BUILD_ASSERT(DT_INST_GPIO_PIN(0, ref_clk_output_gpios) == 0 ||
				     DT_INST_GPIO_PIN(0, ref_clk_output_gpios) == 16 ||
				     DT_INST_GPIO_PIN(0, ref_clk_output_gpios) == 17,
			     "Only GPIO0/16/17 are allowed as a GPIO REF_CLK source!");
		int ref_clk_gpio = DT_INST_GPIO_PIN(0, ref_clk_output_gpios);

		esp32_emac_iomux_rmii_clk_output(ref_clk_gpio);

		/* Configure REF_CLK output when GPIO0 is used */
		if (ref_clk_gpio == 0) {
			REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 6);
		}

		emac_ll_clock_enable_rmii_output(EMAC_EXT_ADDR);
		esp_clk_tree_enable_src(SOC_MOD_CLK_APLL, true);
		ret = esp32_emac_config_apll_clock();
		if (ret != 0) {
			return -EIO;
		}
#else
		esp32_emac_iomux_rmii_clk_input(rmii_clk_gpio);
		emac_ll_clock_enable_rmii_input(EMAC_EXT_ADDR);
#endif
	} else { /* phy_connection_type: mii */
		esp32_emac_iomux_init_mii();
		emac_ll_clock_enable_mii(EMAC_EXT_ADDR);
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

int dwmac_platform_init(const struct device *dev)
{
	const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);
	struct dwmac_priv *p = dev->data;
	int ret;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

#ifdef CONFIG_SOC_SERIES_ESP32P4
	/* Deactivates store and forward mode for rx and tx */
	/* Enable OSF (Operate on Second Frame) to improve performance */
	DWMAC_REG_WRITE(DWMAC_DMAOMR, DWMAC_DMAOMR_OSF | FIELD_PREP(DWMAC_DMAOMR_RTC, 0x3));
#endif

	/* Configure ISR */
	ret = esp_intr_alloc(DT_INST_IRQ_BY_IDX(0, 0, irq),
			     ESP_PRIO_TO_FLAGS(DT_INST_IRQ_BY_IDX(0, 0, priority)) |
				     ESP_INT_FLAGS_CHECK(DT_INST_IRQ_BY_IDX(0, 0, flags)),
			     (intr_handler_t)dwmac_isr, (void *)(uintptr_t)dev, NULL);
	if (ret != 0) {
		return -EIO;
	}

	ret = net_eth_mac_load(&mac_cfg, p->mac_addr);
	if (ret == -ENODATA) {
		if (esp_read_mac(p->mac_addr, ESP_MAC_ETH) != ESP_OK) {
			ret = -EIO;
		} else {
			ret = 0;
		}
	}

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
	.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.mac_clk = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
};

static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0, dwmac_probe, NULL, &dwmac_instance, &dwmac_config,
			      CONFIG_ETH_INIT_PRIORITY, &dwmac_api, NET_ETH_MTU);
