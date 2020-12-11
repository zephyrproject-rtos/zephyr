/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_GECKO_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_GECKO_PRIV_H_

#include <kernel.h>
#include <zephyr/types.h>

#define ETH_GECKO_MTU         NET_ETH_MTU

#define SILABS_OUI_B0         0x00
#define SILABS_OUI_B1         0x0B
#define SILABS_OUI_B2         0x57

#define ETH_TX_BUF_SIZE       1536
#define ETH_TX_BUF_COUNT      2
#define ETH_RX_BUF_SIZE       128
#define ETH_RX_BUF_COUNT      32

#define ETH_BUF_ALIGNMENT     16

#define ETH_DESC_ALIGNMENT    4

#define ETH_TX_USED           BIT(31)
#define ETH_TX_WRAP           BIT(30)
#define ETH_TX_ERROR          BIT(29)
#define ETH_TX_UNDERRUN       BIT(28)
#define ETH_TX_EXHAUSTED      BIT(27)
#define ETH_TX_NO_CRC         BIT(16)
#define ETH_TX_LAST           BIT(15)
#define ETH_TX_LENGTH         (2048-1)

#define ETH_RX_ADDRESS        ~(ETH_DESC_ALIGNMENT-1)
#define ETH_RX_WRAP           BIT(1)
#define ETH_RX_OWNERSHIP      BIT(0)
#define ETH_RX_BROADCAST      BIT(31)
#define ETH_RX_MULTICAST_HASH BIT(30)
#define ETH_RX_UNICAST_HASH   BIT(29)
#define ETH_RX_EXT_ADDR       BIT(28)
#define ETH_RX_SAR1           BIT(26)
#define ETH_RX_SAR2           BIT(25)
#define ETH_RX_SAR3           BIT(24)
#define ETH_RX_SAR4           BIT(23)
#define ETH_RX_TYPE_ID        BIT(22)
#define ETH_RX_VLAN_TAG       BIT(21)
#define ETH_RX_PRIORITY_TAG   BIT(20)
#define ETH_RX_VLAN_PRIORITY  (0x7UL<<17)
#define ETH_RX_CFI            BIT(16)
#define ETH_RX_EOF            BIT(15)
#define ETH_RX_SOF            BIT(14)
#define ETH_RX_OFFSET         (0x3UL<<12)
#define ETH_RX_LENGTH         (4096-1)

#define ETH_RX_ENABLE(base)   (base->NETWORKCTRL |= ETH_NETWORKCTRL_ENBRX)
#define ETH_RX_DISABLE(base)  (base->NETWORKCTRL &= ~ETH_NETWORKCTRL_ENBRX)

#define ETH_TX_ENABLE(base)   (base->NETWORKCTRL |= ETH_NETWORKCTRL_ENBTX)
#define ETH_TX_DISABLE(base)  (base->NETWORKCTRL &= ~ETH_NETWORKCTRL_ENBTX)

struct eth_buf_desc {
	uint32_t address;
	uint32_t status;
};

struct eth_gecko_pin_list {
	struct soc_gpio_pin mdio[2];
	struct soc_gpio_pin rmii[7];
};

/* Device constant configuration parameters */
struct eth_gecko_dev_cfg {
	ETH_TypeDef *regs;
	const struct eth_gecko_pin_list *pin_list;
	uint32_t pin_list_size;
	void (*config_func)(void);
	struct phy_gecko_dev phy;
};

/* Device run time data */
struct eth_gecko_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_sem tx_sem;
	struct k_sem rx_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack,
		CONFIG_ETH_GECKO_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
	bool link_up;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((const struct eth_gecko_dev_cfg *)(dev)->config)
#define DEV_DATA(dev) \
	((struct eth_gecko_dev_data *)(dev)->data)

/* PHY Management pins */
#define PIN_PHY_MDC {DT_INST_PROP_BY_IDX(0, location_phy_mdc, 1), \
	DT_INST_PROP_BY_IDX(0, location_phy_mdc, 2), gpioModePushPull,\
	0}
#define PIN_PHY_MDIO {DT_INST_PROP_BY_IDX(0, location_phy_mdio, 1), \
	DT_INST_PROP_BY_IDX(0, location_phy_mdio, 2), gpioModePushPull,\
	0}

#define PIN_LIST_PHY {PIN_PHY_MDC, PIN_PHY_MDIO}

/* RMII pins excluding reference clock, handled by board.c */
#define PIN_RMII_CRSDV {DT_INST_PROP_BY_IDX(0, location_rmii_crs_dv, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_crs_dv, 2), gpioModeInput, 0}

#define PIN_RMII_TXD0 {DT_INST_PROP_BY_IDX(0, location_rmii_txd0, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_txd0, 2), gpioModePushPull, 0}

#define PIN_RMII_TXD1 {DT_INST_PROP_BY_IDX(0, location_rmii_txd1, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_txd1, 2), gpioModePushPull, 0}

#define PIN_RMII_TX_EN {DT_INST_PROP_BY_IDX(0, location_rmii_tx_en, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_tx_en, 2), gpioModePushPull, 0}

#define PIN_RMII_RXD0 {DT_INST_PROP_BY_IDX(0, location_rmii_rxd0, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_rxd0, 2), gpioModeInput, 0}

#define PIN_RMII_RXD1 {DT_INST_PROP_BY_IDX(0, location_rmii_rxd1, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_rxd1, 2), gpioModeInput, 0}

#define PIN_RMII_RX_ER {DT_INST_PROP_BY_IDX(0, location_rmii_rx_er, 1),\
	DT_INST_PROP_BY_IDX(0, location_rmii_rx_er, 2), gpioModeInput, 0}

#define PIN_LIST_RMII {PIN_RMII_CRSDV, PIN_RMII_TXD0, PIN_RMII_TXD1, \
	PIN_RMII_TX_EN, PIN_RMII_RXD0, PIN_RMII_RXD1, PIN_RMII_RX_ER}

/* RMII reference clock is not included in RMII pin set
 * #define PIN_RMII_REFCLK {DT_INST_PROP_BY_IDX(0, location_rmii_refclk, 1),\
 *	DT_INST_PROP_BY_IDX(0, location_rmii_refclk, 2), gpioModePushPull, 0}
 */

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_GECKO_PRIV_H_ */
