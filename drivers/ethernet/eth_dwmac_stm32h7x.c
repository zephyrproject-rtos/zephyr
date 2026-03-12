/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM32H7X specific glue.
 */


#define LOG_MODULE_NAME dwmac_plat
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* be compatible with the HAL-based driver here */
#define DT_DRV_COMPAT st_stm32_ethernet

#include <sys/types.h>
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

#if defined(CONFIG_SOC_SERIES_STM32H5X)

#if DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
#define PHY_MODE LL_SBS_ETH_MII
#elif DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)
#define PHY_MODE LL_SBS_ETH_RMII
#else
#error "Unsupported PHY connection type"
#endif
#define STM32_CONFIGURE_ETH_PHY_MODE() do {                                                        \
	__HAL_RCC_SBS_CLK_ENABLE();                                                                \
	LL_SBS_SetPHYInterface(PHY_MODE);                                                          \
} while (0)

#elif defined(CONFIG_SOC_SERIES_STM32H7X)

#if DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
#define PHY_MODE LL_SYSCFG_ETH_MII
#elif DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)
#define PHY_MODE LL_SYSCFG_ETH_RMII
#else
#error "Unsupported PHY connection type"
#endif
#define STM32_CONFIGURE_ETH_PHY_MODE() do {                                                        \
	__HAL_RCC_SYSCFG_CLK_ENABLE();                                                             \
	LL_SYSCFG_SetPHYInterface(PHY_MODE);                                                       \
} while (0)

#elif defined(CONFIG_SOC_SERIES_STM32H7RSX)

#if DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
#define PHY_MODE LL_SBS_ETH_PHYSEL_GMII_MII
#elif DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)
#define PHY_MODE LL_SBS_ETH_PHYSEL_RMII
#else
#error "Unsupported PHY connection type"
#endif
#define STM32_CONFIGURE_ETH_PHY_MODE() do {                                                        \
	__HAL_RCC_SBS_CLK_ENABLE();                                                                \
	LL_SBS_SetEthernetPhy(PHY_MODE);                                                           \
} while (0)

#endif

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg =
	PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_INST_PARENT(0));
static struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);

int dwmac_bus_init(struct dwmac_priv *p)
{
	int ret;

	p->clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	for (size_t n = 0; n < ARRAY_SIZE(pclken); n++) {
		ret  = clock_control_on(p->clock, (clock_control_subsys_t)&pclken[n]);
		if (ret) {
			LOG_ERR("Failed to enable ethernet clock #%zu", n);
			return -EIO;
		}
	}

	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	STM32_CONFIGURE_ETH_PHY_MODE();

	p->base_addr = DT_REG_ADDR(DT_INST_PARENT(0));
	return 0;
}

#if defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache __aligned(4)
#else
#define __desc_mem __aligned(4)
#endif

/* Descriptor rings in uncached memory */
static struct dwmac_dma_desc dwmac_tx_descs[NB_TX_DESCS] __desc_mem;
static struct dwmac_dma_desc dwmac_rx_descs[NB_RX_DESCS] __desc_mem;

int dwmac_platform_init(struct dwmac_priv *p)
{
	int ret;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	/* basic configuration for this platform */
	REG_WRITE(MAC_CONF,
		  MAC_CONF_PS |
		  MAC_CONF_FES |
		  MAC_CONF_DM);
	REG_WRITE(DMA_SYSBUS_MODE,
		  DMA_SYSBUS_MODE_AAL |
		  DMA_SYSBUS_MODE_FB);

	/* set up IRQs (still masked for now) */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* retrieve MAC address */
	ret = net_eth_mac_load(&mac_cfg, p->mac_addr);
	if (ret == -ENODATA) {
		uint8_t unique_device_ID_12_bytes[12];
		uint32_t result_mac_32_bits;

		/**
		 * Set MAC address locally administered bit (LAA) as this is not assigned by the
		 * manufacturer
		 */
		p->mac_addr[0] = ST_OUI_B0 | 0x02;
		p->mac_addr[1] = ST_OUI_B1;
		p->mac_addr[2] = ST_OUI_B2;

		/* Nothing defined by the user, use device id */
		hwinfo_get_device_id(unique_device_ID_12_bytes, 12);
		result_mac_32_bits = crc32_ieee((uint8_t *)unique_device_ID_12_bytes, 12);
		memcpy(&p->mac_addr[3], &result_mac_32_bits, 3);

		ret = 0;
	}

	if (ret < 0) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
		return ret;
	}

	return 0;
}

/* Our private device instance */
static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0,
			      dwmac_probe,
			      NULL,
			      &dwmac_instance,
			      NULL,
			      CONFIG_ETH_INIT_PRIORITY,
			      &dwmac_api,
			      NET_ETH_MTU);
