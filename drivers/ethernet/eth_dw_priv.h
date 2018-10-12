/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_DW_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_DW_PRIV_H_

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*eth_config_irq_t)(struct device *port);

struct eth_config {
	u32_t irq_num;
	eth_config_irq_t config_func;

#ifdef CONFIG_ETH_DW_SHARED_IRQ
	char *shared_irq_dev_name;
#endif  /* CONFIG_ETH_DW_SHARED_IRQ */
};

/* Refer to Intel Quark SoC X1000 Datasheet, Chapter 15 for more details on
 * Ethernet device operation.
 *
 * This driver puts the Ethernet device into a very simple and space-efficient
 * mode of operation.  It only allocates a single packet descriptor for each of
 * the transmit and receive directions, computes checksums on the CPU, and
 * enables store-and-forward mode for both transmit and receive directions.
 */

/* Transmit descriptor */
struct eth_tx_desc {
	/* First word of transmit descriptor */
	union {
		struct {
			/* Only valid in half-duplex mode. */
			u32_t deferred_bit      : 1;
			u32_t err_underflow     : 1;
			u32_t err_excess_defer  : 1;
			u32_t coll_cnt_slot_num : 4;
			u32_t vlan_frm          : 1;
			u32_t err_excess_coll   : 1;
			u32_t err_late_coll     : 1;
			u32_t err_no_carrier    : 1;
			u32_t err_carrier_loss  : 1;
			u32_t err_ip_payload    : 1;
			u32_t err_frm_flushed   : 1;
			u32_t err_jabber_tout   : 1;
			/* OR of all other error bits. */
			u32_t err_summary       : 1;
			u32_t err_ip_hdr        : 1;
			u32_t tx_timestamp_stat : 1;
			u32_t vlan_ins_ctrl     : 2;
			u32_t addr2_chained     : 1;
			u32_t tx_end_of_ring    : 1;
			u32_t chksum_ins_ctrl   : 2;
			u32_t replace_crc       : 1;
			u32_t tx_timestamp_en   : 1;
			u32_t dis_pad           : 1;
			u32_t dis_crc           : 1;
			u32_t first_seg_in_frm  : 1;
			u32_t last_seg_in_frm   : 1;
			u32_t intr_on_complete  : 1;
			/* When set, descriptor is owned by DMA. */
			u32_t own               : 1;
		};
		u32_t tdes0;
	};
	/* Second word of transmit descriptor */
	union {
		struct {
			u32_t tx_buf1_sz        : 13;
			u32_t                   : 3;
			u32_t tx_buf2_sz        : 13;
			u32_t src_addr_ins_ctrl : 3;
		};
		u32_t tdes1;
	};
	/* Pointer to frame data buffer */
	u8_t *buf1_ptr;
	/* Unused, since this driver initializes only a single descriptor for each
	 * direction.
	 */
	u8_t *buf2_ptr;
};

/* Transmit descriptor */
struct eth_rx_desc {
	/* First word of receive descriptor */
	union {
		struct {
			u32_t ext_stat          : 1;
			u32_t err_crc           : 1;
			u32_t err_dribble_bit   : 1;
			u32_t err_rx_mii        : 1;
			u32_t err_rx_wdt        : 1;
			u32_t frm_type          : 1;
			u32_t err_late_coll     : 1;
			u32_t giant_frm         : 1;
			u32_t last_desc         : 1;
			u32_t first_desc        : 1;
			u32_t vlan_tag          : 1;
			u32_t err_overflow      : 1;
			u32_t length_err        : 1;
			u32_t s_addr_filt_fail  : 1;
			u32_t err_desc          : 1;
			u32_t err_summary       : 1;
			u32_t frm_len           : 14;
			u32_t d_addr_filt_fail  : 1;
			u32_t own               : 1;
		};
		u32_t rdes0;
	};
	/* Second word of receive descriptor */
	union {
		struct {
			u32_t rx_buf1_sz        : 13;
			u32_t                   : 1;
			u32_t addr2_chained     : 1;
			u32_t rx_end_of_ring    : 1;
			u32_t rx_buf2_sz        : 13;
			u32_t                   : 2;
			u32_t dis_int_compl     : 1;
		};
		u32_t rdes1;
	};
	/* Pointer to frame data buffer */
	u8_t *buf1_ptr;
	/* Unused, since this driver initializes only a single descriptor for each
	 * direction.
	 */
	u8_t *buf2_ptr;
};

#define ETH_DW_MTU 1500

/* Driver metadata associated with each Ethernet device */
struct eth_runtime {
	u32_t base_addr;
	struct net_if *iface;
#ifdef CONFIG_PCI
	struct pci_dev_info pci_dev;
#endif  /* CONFIG_PCI */
	/* Transmit descriptor */
	volatile struct eth_tx_desc tx_desc;
	/* Receive descriptor */
	volatile struct eth_rx_desc rx_desc;
	/* Receive DMA packet buffer */
	volatile u8_t rx_buf[ETH_DW_MTU];

	union {
		struct {
			u8_t bytes[6];
			u8_t pad[2];
		} __packed;
		u32_t words[2];
	} mac_addr;
};

#define MMC_DEFAULT_MASK               0xffffffff

#define MAC_CONF_14_RMII_100M          BIT(14)
#define MAC_CONF_11_DUPLEX             BIT(11)
#define MAC_CONF_3_TX_EN               BIT(3)
#define MAC_CONF_2_RX_EN               BIT(2)
#define MAC_FILTER_4_PM                BIT(4)

#define STATUS_NORMAL_INT              BIT(16)
#define STATUS_RX_INT                  BIT(6)

#define OP_MODE_25_RX_STORE_N_FORWARD  BIT(25)
#define OP_MODE_21_TX_STORE_N_FORWARD  BIT(21)
#define OP_MODE_13_START_TX            BIT(13)
#define OP_MODE_1_START_RX             BIT(1)

#define INT_ENABLE_NORMAL              BIT(16)
#define INT_ENABLE_RX                  BIT(6)

#define REG_ADDR_MAC_CONF              0x0000
#define REG_ADDR_MAC_FRAME_FILTER      0x0004
#define REG_ADDR_MACADDR_HI            0x0040
#define REG_ADDR_MACADDR_LO            0x0044

#define REG_MMC_RX_INTR_MASK           0x010c
#define REG_MMC_TX_INTR_MASK           0x0110
#define REG_MMC_RX_IPC_INTR_MASK       0x0200

#define REG_ADDR_TX_POLL_DEMAND        0x1004
#define REG_ADDR_RX_POLL_DEMAND        0x1008
#define REG_ADDR_RX_DESC_LIST          0x100C
#define REG_ADDR_TX_DESC_LIST          0x1010
#define REG_ADDR_STATUS                0x1014
#define REG_ADDR_DMA_OPERATION         0x1018
#define REG_ADDR_INT_ENABLE            0x101C

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_DW_PRIV_H_ */
