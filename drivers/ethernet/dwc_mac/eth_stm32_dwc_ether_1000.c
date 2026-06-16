/*
 * Driver for Synopsys DesignWare MAC
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_ethernet

#include <sys/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/crc.h>
#include <zephyr/irq.h>
#include <stm32_ll_system.h>

#include "eth_dwmac_priv.h"

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

/* The DMA bus master interface is 32-bit on this IP */
#define DATA_BUS_WIDTH 32

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii) ||
		     DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii),
	     "Unsupported PHY connection type");

#ifdef CONFIG_SOC_SERIES_STM32F1X
#define STM32_CONFIGURE_ETH_PHY_MODE()                                                             \
	WRITE_BIT(AFIO->MAPR, AFIO_MAPR_MII_RMII_SEL_Pos,                                          \
		  DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii))
#else /* CONFIG_SOC_SERIES_STM32F1X */
#if DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
#define PHY_MODE LL_SYSCFG_PMC_ETHMII
#else
#define PHY_MODE LL_SYSCFG_PMC_ETHRMII
#endif

#define STM32_CONFIGURE_ETH_PHY_MODE()                                                             \
	do {                                                                                       \
		__HAL_RCC_SYSCFG_CLK_ENABLE();                                                     \
		LL_SYSCFG_SetPHYInterface(PHY_MODE);                                               \
	} while (0)
#endif /* CONFIG_SOC_SERIES_STM32F1X */

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);
static struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);

int dwmac_bus_init(const struct device *dev)
{
	const struct dwmac_config *cfg = dev->config;
	int ret;

	for (size_t n = 0; n < ARRAY_SIZE(pclken); n++) {
		if (IN_RANGE(pclken[n].bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
			ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&pclken[n]);
		} else {
			ret = clock_control_configure(cfg->clock,
						      (clock_control_subsys_t)&pclken[n], NULL);
		}

		if (ret != 0) {
			LOG_ERR("Failed to setup ethernet clock #%zu", n);
			return -EIO;
		}
	}

	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	STM32_CONFIGURE_ETH_PHY_MODE();

	return 0;
}

#define DESCRIPTOR_ALIGNMENT ((DATA_BUS_WIDTH) / (BITS_PER_BYTE))
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm)) && defined(CONFIG_SOC_SERIES_STM32F7X)
#define __desc_mem __dtcm_noinit_section __aligned(DESCRIPTOR_ALIGNMENT)
#elif defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache_noinit __aligned(DESCRIPTOR_ALIGNMENT)
#else
#define __desc_mem __noinit __aligned(DESCRIPTOR_ALIGNMENT)
#endif

static struct dwmac_dma_desc dwmac_tx_descs[NB_TX_DESCS] __desc_mem;
static struct dwmac_dma_desc dwmac_rx_descs[NB_RX_DESCS] __desc_mem;

int dwmac_platform_init(const struct device *dev)
{
	struct dwmac_priv *p = dev->data;
	int ret;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	ret = net_eth_mac_load(&mac_cfg, p->mac_addr);
	if (ret == -ENODATA) {
		uint8_t unique_device_ID_12_bytes[12];
		uint32_t result_mac_32_bits;

		p->mac_addr[0] = ST_OUI_B0 | 0x02;
		p->mac_addr[1] = ST_OUI_B1;
		p->mac_addr[2] = ST_OUI_B2;

		hwinfo_get_device_id(unique_device_ID_12_bytes, 12);
		result_mac_32_bits = crc32_ieee((uint8_t *)unique_device_ID_12_bytes, 12);
		memcpy(&p->mac_addr[3], &result_mac_32_bits, 3);

		ret = 0;
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
	.clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
	.mac_clk = (clock_control_subsys_t)&pclken[0],
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
