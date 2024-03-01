/* NXP ENET QOS Header
 *
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet_qos.h>

/* shorthands */
#define NUM_TX_BUFDESC CONFIG_ETH_NXP_ENET_QOS_TX_BUFFER_DESCRIPTORS
#define NUM_RX_BUFDESC CONFIG_ETH_NXP_ENET_QOS_RX_BUFFER_DESCRIPTORS
#define LAST_TX_DESC_INDEX NUM_TX_BUFDESC - 1
#define LAST_RX_DESC_INDEX NUM_RX_BUFDESC - 1

/* NXP Organizational Unique Identifier */
#define NXP_OUI_BYTE_0 0xAC
#define NXP_OUI_BYTE_1 0x9A
#define NXP_OUI_BYTE_2 0x22

#define FIRST_TX_DESCRIPTOR_FLAG BIT(29)
#define LAST_TX_DESCRIPTOR_FLAG BIT(28)
#define OWN_FLAG BIT(31)
#define RX_INTERRUPT_ON_COMPLETE_FLAG BIT(30)
#define TX_INTERRUPT_ON_COMPLETE_FLAG BIT(31)
#define BUF1_ADDR_VALID_FLAG BIT(24)
#define DESC_RX_PKT_LEN GENMASK(14, 0)

#define ENET_QOS_MAX_NORMAL_FRAME_LEN 1518

#define NUM_SWR_WAIT_CHUNKS 5

struct nxp_enet_qos_tx_read_desc {
	union {
		uint32_t buf1_addr;
		uint32_t head_addr;
	};
	union {
		uint32_t buf2_addr;
		uint32_t buf1_addr_alt;
	};
	uint32_t control1;
	uint32_t control2;
};

struct nxp_enet_qos_tx_write_desc {
	uint32_t timestamp_low;
	uint32_t timestamp_high;
	uint32_t reserved;
	uint32_t status;
};

union nxp_enet_qos_tx_desc {
	struct nxp_enet_qos_tx_read_desc read;
	struct nxp_enet_qos_tx_write_desc write;
};

struct nxp_enet_qos_rx_read_desc {
	union {
		uint32_t buf1_addr;
		uint32_t head_addr;
	};
	uint32_t reserved;
	uint32_t buf2_addr;
	uint32_t control;
};

struct nxp_enet_qos_rx_write_desc {
	uint32_t vlan_tag;
	uint32_t control1;
	uint32_t control2;
	uint32_t control3;
};

union nxp_enet_qos_rx_desc {
	struct nxp_enet_qos_rx_read_desc read;
	struct nxp_enet_qos_rx_write_desc write;
};

struct nxp_enet_qos_hw_info {
	uint16_t max_frame_len;
};

struct nxp_enet_qos_mac_config {
	const struct device *enet_dev;
	const struct device *phy_dev;
	enet_qos_t *base;
	struct nxp_enet_qos_hw_info hw_info;
	void (*irq_config_func)(void);
	bool random_mac;
};

struct nxp_enet_qos_tx_data {
	struct k_sem tx_sem;
	struct net_pkt *pkt;
	struct k_work tx_done_work;
	struct net_buf *tx_header;
	volatile union nxp_enet_qos_tx_desc descriptors[NUM_TX_BUFDESC];
};

struct nxp_enet_qos_rx_data {
	struct k_work rx_work;
	volatile union nxp_enet_qos_rx_desc descriptors[NUM_RX_BUFDESC];
	struct net_buf *reserved_bufs[NUM_RX_BUFDESC];
};

struct nxp_enet_qos_mac_data {
	struct net_if *iface;
	struct net_eth_addr mac_addr;
	struct nxp_enet_qos_tx_data tx;
	struct nxp_enet_qos_rx_data rx;
};
