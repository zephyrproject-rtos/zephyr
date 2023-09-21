/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_S32_NETC_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_S32_NETC_PRIV_H_

#define NETC_F3_PSICFGR0_SIVC_CVLAN_BIT	BIT(0)	/* 0x8100 */
#define NETC_F3_PSICFGR0_SIVC_SVLAN_BIT	BIT(1)	/* 0x88A8 */

#define NETC_MIN_RING_LEN	8U
#define NETC_MIN_RING_BUF_SIZE	64U

#define NETC_SWITCH_IDX		0U
#define NETC_SWITCH_PORT_IDX	0U
#define NETC_SWITCH_PORT_AGING	300U
#define NETC_ETH_0_RX_CLK_IDX	49U
#define NETC_ETH_1_RX_CLK_IDX	51U

#define NETC_MSIX_EVENTS_COUNT	2U

/* Timeout for various operations */
#define NETC_TIMEOUT		K_MSEC(20)

/* Helper macros to convert from Zephyr PHY speed to NETC baudrate/duplex types */
#define PHY_TO_NETC_SPEED(x) \
	(PHY_LINK_IS_SPEED_1000M(x) ? ETHTRCV_BAUD_RATE_1000MBIT : \
		(PHY_LINK_IS_SPEED_100M(x) ? ETHTRCV_BAUD_RATE_100MBIT : ETHTRCV_BAUD_RATE_10MBIT))

#define PHY_TO_NETC_DUPLEX_MODE(x) \
	(PHY_LINK_IS_FULL_DUPLEX(x) ? NETC_ETHSWT_PORT_FULL_DUPLEX : NETC_ETHSWT_PORT_HALF_DUPLEX)

/*
 * Get the first MRU mailbox address for an specific mbox handle
 * mbox[0] addr = MRU base addr + (channel × channel offset), with channel=1..N
 */
#define MRU_CHANNEL_OFFSET	0x1000
#define MRU_MBOX_ADDR(node, name)						\
	(DT_REG_ADDR(DT_MBOX_CTLR_BY_NAME(node, name))				\
	 + ((DT_MBOX_CHANNEL_BY_NAME(node, name) + 1) * MRU_CHANNEL_OFFSET))

#define NETC_MSIX(node, name, cb)					\
	{								\
		.handler = cb,						\
		.mbox_channel = MBOX_DT_CHANNEL_GET(node, name),	\
	}

/* Tx/Rx ENETC ring definitions */
#define _NETC_RING(n, idx, len, buf_size, prefix1, prefix2)					\
	static Netc_Eth_Ip_##prefix1##BDRType nxp_s32_eth##n##_##prefix2##ring##idx##_desc[len]	\
		__nocache __aligned(FEATURE_NETC_BUFFDESCR_ALIGNMENT_BYTES);			\
	static uint8_t nxp_s32_eth##n##_##prefix2##ring##idx##_buf[len * buf_size]		\
		__nocache __aligned(FEATURE_NETC_BUFF_ALIGNMENT_BYTES)

#define NETC_RX_RING(n, idx, len, buf_size)	_NETC_RING(n, idx, len, buf_size, Rx, rx)
#define NETC_TX_RING(n, idx, len, buf_size)	_NETC_RING(n, idx, len, buf_size, Tx, tx)

/* Helper function to generate an Ethernet MAC address for a given ENETC instance */
#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

#define _NETC_GENERATE_MAC_ADDRESS_RANDOM	\
	gen_random_mac(mac_addr, FREESCALE_OUI_B0, FREESCALE_OUI_B1, FREESCALE_OUI_B2)

#define _NETC_GENERATE_MAC_ADDRESS_UNIQUE(n)				\
	do {								\
		uint32_t id = 0x001100;					\
									\
		mac_addr[0] = FREESCALE_OUI_B0;				\
		mac_addr[1] = FREESCALE_OUI_B1;				\
		/* Set MAC address locally administered bit (LAA) */	\
		mac_addr[2] = FREESCALE_OUI_B2 | 0x02;			\
		mac_addr[3] = (id >> 16) & 0xff;			\
		mac_addr[4] = (id >> 8) & 0xff;				\
		mac_addr[5] = (id + n) & 0xff;				\
	} while (0)

#define NETC_GENERATE_MAC_ADDRESS(node, n)					\
	static void nxp_s32_eth##n##_generate_mac(uint8_t mac_addr[6])		\
	{									\
		COND_CODE_1(DT_PROP(node, zephyr_random_mac_address),		\
			(_NETC_GENERATE_MAC_ADDRESS_RANDOM),			\
			(COND_CODE_0(DT_NODE_HAS_PROP(node, local_mac_address),	\
				(_NETC_GENERATE_MAC_ADDRESS_UNIQUE(n)),		\
				(ARG_UNUSED(mac_addr)))));			\
	}

/* Helper macros to concatenate tokens that require further expansions */
#define _CONCAT3(a, b, c)	DT_CAT3(a, b, c)

struct nxp_s32_eth_msix {
	void (*handler)(uint8_t chan, const uint32_t *buf, uint8_t buf_size);
	struct mbox_channel mbox_channel;
};

struct nxp_s32_eth_config {
	const Netc_Eth_Ip_ConfigType netc_cfg;

	Netc_Eth_Ip_MACFilterHashTableEntryType *mac_filter_hash_table;

	uint8_t si_idx;
	uint8_t port_idx;
	const struct device *phy_dev;
	uint8_t tx_ring_idx;
	uint8_t rx_ring_idx;
	void (*generate_mac)(uint8_t *mac_addr);
	struct nxp_s32_eth_msix msix[NETC_MSIX_EVENTS_COUNT];
	const struct pinctrl_dev_config *pincfg;
};

struct nxp_s32_eth_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_mutex tx_mutex;
	struct k_sem rx_sem;
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_NXP_S32_RX_THREAD_STACK_SIZE);
};

int nxp_s32_eth_initialize_common(const struct device *dev);
int nxp_s32_eth_tx(const struct device *dev, struct net_pkt *pkt);
enum ethernet_hw_caps nxp_s32_eth_get_capabilities(const struct device *dev);
void nxp_s32_eth_mcast_cb(struct net_if *iface, const struct net_addr *addr, bool is_joined);
int nxp_s32_eth_set_config(const struct device *dev, enum ethernet_config_type type,
			   const struct ethernet_config *config);

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_S32_NETC_PRIV_H_ */
