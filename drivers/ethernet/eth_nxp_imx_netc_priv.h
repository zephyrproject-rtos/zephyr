/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_PRIV_H_

#include "fsl_netc_endpoint.h"
#include "fsl_msgintr.h"

/* Buffer and descriptor alignment */
#define NETC_BD_ALIGN   128
#define NETC_BUFF_ALIGN 64
#define NETC_RX_RING_BUF_SIZE_ALIGN                                                                \
	SDK_SIZEALIGN(CONFIG_ETH_NXP_IMX_RX_RING_BUF_SIZE, NETC_BUFF_ALIGN)

/* MSIX definitions */
#define NETC_TX_MSIX_ENTRY_IDX 0
#define NETC_RX_MSIX_ENTRY_IDX 1
#define NETC_MSIX_ENTRY_NUM    2

#define NETC_MSIX_EVENTS_COUNT      NETC_MSIX_ENTRY_NUM
#define NETC_TX_INTR_MSG_DATA_START 0
#define NETC_RX_INTR_MSG_DATA_START 16
#define NETC_DRV_MAX_INST_SUPPORT   16

/* MSGINTR */
#define NETC_MSGINTR_CHANNEL 0

#if (CONFIG_ETH_NXP_IMX_MSGINTR == 1)
#define NETC_MSGINTR     MSGINTR1
#define NETC_MSGINTR_IRQ MSGINTR1_IRQn
#elif (CONFIG_ETH_NXP_IMX_MSGINTR == 2)
#define NETC_MSGINTR     MSGINTR2
#define NETC_MSGINTR_IRQ MSGINTR2_IRQn
#else
#error "Current CONFIG_ETH_NXP_IMX_MSGINTR not support"
#endif

/* Timeout for various operations */
#define NETC_TIMEOUT K_MSEC(20)

#define NETC_PHY_MODE(node_id)                                                                     \
	(DT_ENUM_HAS_VALUE(node_id, phy_connection_type, mii)                                      \
		 ? kNETC_MiiMode                                                                   \
		 : (DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rmii)                          \
			    ? kNETC_RmiiMode                                                       \
			    : (DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii)              \
				       ? kNETC_RgmiiMode                                           \
				       : (DT_ENUM_HAS_VALUE(node_id, phy_connection_type, gmii)    \
						  ? kNETC_GmiiMode                                 \
						  : kNETC_RmiiMode))))

/* Helper macros to convert from Zephyr PHY speed to NETC speed/duplex types */
#define PHY_TO_NETC_SPEED(x)                                                                       \
	(PHY_LINK_IS_SPEED_1000M(x)                                                                \
		 ? kNETC_MiiSpeed1000M                                                             \
		 : (PHY_LINK_IS_SPEED_100M(x) ? kNETC_MiiSpeed100M : kNETC_MiiSpeed10M))

#define PHY_TO_NETC_DUPLEX_MODE(x)                                                                 \
	(PHY_LINK_IS_FULL_DUPLEX(x) ? kNETC_MiiFullDuplex : kNETC_MiiHalfDuplex)

/* Helper function to generate an Ethernet MAC address for a given ENETC instance */
#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

#define _NETC_GENERATE_MAC_ADDRESS_RANDOM                                                          \
	gen_random_mac(mac_addr, FREESCALE_OUI_B0, FREESCALE_OUI_B1, FREESCALE_OUI_B2)

#define _NETC_GENERATE_MAC_ADDRESS_UNIQUE(n)                                                       \
	do {                                                                                       \
		uint32_t id = 0x001100;                                                            \
                                                                                                   \
		mac_addr[0] = FREESCALE_OUI_B0;                                                    \
		mac_addr[1] = FREESCALE_OUI_B1;                                                    \
		/* Set MAC address locally administered bit (LAA) */                               \
		mac_addr[2] = FREESCALE_OUI_B2 | 0x02;                                             \
		mac_addr[3] = (id >> 16) & 0xff;                                                   \
		mac_addr[4] = (id >> 8) & 0xff;                                                    \
		mac_addr[5] = (id + n) & 0xff;                                                     \
	} while (0)

#define NETC_GENERATE_MAC_ADDRESS(n)                                                               \
	static void netc_eth##n##_generate_mac(uint8_t mac_addr[6])                                \
	{                                                                                          \
		COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address),                            \
			    (_NETC_GENERATE_MAC_ADDRESS_RANDOM),                                   \
			    (COND_CODE_0(DT_INST_NODE_HAS_PROP(n, local_mac_address),              \
					 (_NETC_GENERATE_MAC_ADDRESS_UNIQUE(n)),                   \
					 (ARG_UNUSED(mac_addr)))));                      \
	}

struct netc_eth_config {
	uint16_t si_idx;
	const struct device *phy_dev;
	netc_hw_mii_mode_t phy_mode;
	void (*generate_mac)(uint8_t *mac_addr);
	void (*bdr_init)(netc_bdr_config_t *bdr_config, netc_rx_bdr_config_t *rx_bdr_config,
			 netc_tx_bdr_config_t *tx_bdr_config);
	const struct pinctrl_dev_config *pincfg;
	uint8_t tx_intr_msg_data;
	uint8_t rx_intr_msg_data;
};

typedef uint8_t rx_buffer_t[NETC_RX_RING_BUF_SIZE_ALIGN];

struct netc_eth_data {
	ep_handle_t handle;
	struct net_if *iface;
	uint8_t mac_addr[6];
	/* TX */
	struct k_mutex tx_mutex;
	netc_tx_frame_info_t tx_info;
	uint8_t *tx_buff;
	volatile bool tx_done;
	/* RX */
	struct k_sem rx_sem;
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_NXP_IMX_RX_THREAD_STACK_SIZE);
	uint8_t *rx_frame;
};

int netc_eth_init_common(const struct device *dev);
int netc_eth_tx(const struct device *dev, struct net_pkt *pkt);
enum ethernet_hw_caps netc_eth_get_capabilities(const struct device *dev);
int netc_eth_set_config(const struct device *dev, enum ethernet_config_type type,
			const struct ethernet_config *config);

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_PRIV_H_ */
