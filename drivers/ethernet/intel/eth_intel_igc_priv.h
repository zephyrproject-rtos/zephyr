/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_INTEL_IGC_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_INTEL_IGC_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/ethernet/eth_intel_plat.h>

#define ETH_MAX_FRAME_SZ       2048
#define INTEL_IGC_MAX_QCNT     4
#define ETH_IGC_NUM_MISC       1
#define RAH_QSEL_SHIFT         18
#define RAH_QSEL_ENABLE        BIT(28)
#define RAH_QSEL_MASK          GENMASK(19, 18)
#define RAH_ASEL_MASK          GENMASK(17, 16)
#define RAH_ASEL_SRC_ADDR      BIT(16)
#define INTEL_IGC_RAH_AV       BIT(31)
#define INTEL_IGC_DEF_MAC_ADDR 0xC9A000

/* Device Control Register */
#define INTEL_IGC_CTRL                    0x00000
#define INTEL_IGC_CTRL_GIO_MASTER_DISABLE BIT(2)
#define INTEL_IGC_CTRL_SLU                BIT(6)
#define INTEL_IGC_CTRL_FRCSPD             BIT(11)
#define INTEL_IGC_CTRL_FRCDPX             BIT(12)
#define INTEL_IGC_CTRL_RST                BIT(26)
#define INTEL_IGC_CTRL_RFCE               BIT(27)
#define INTEL_IGC_CTRL_TFCE               BIT(28)
#define INTEL_IGC_CTRL_EXT_DRV_LOAD       BIT(28)
#define INTEL_IGC_CTRL_DEV_RST            BIT(29)
#define INTEL_IGC_CTRL_VME                BIT(30)
#define INTEL_IGC_CTRL_PHY_RST            BIT(31)

/* Device Status Register */
#define INTEL_IGC_STATUS                   0x00008
#define INTEL_IGC_STATUS_FD                BIT(0)
#define INTEL_IGC_STATUS_LU                BIT(1)
#define INTEL_IGC_STATUS_TXOFF             BIT(4)
#define INTEL_IGC_STATUS_SPEED_100         BIT(6)
#define INTEL_IGC_STATUS_SPEED_1000        BIT(7)
#define INTEL_IGC_STATUS_GIO_MASTER_ENABLE BIT(19)
#define INTEL_IGC_STATUS_SPEED_MASK        GENMASK(7, 6)

/* Extended Device Control Register */
#define INTEL_IGC_CTRL_EXT          0x00018
#define INTEL_IGC_CTRL_EXT_DRV_LOAD BIT(28)

/* Internal Rx Packet Buffer Size */
#define INTEL_IGC_RXPBS                  0x02404
#define INTEL_IGC_RXPBS_RXPBSIZE_MASK    GENMASK(5, 0)
#define INTEL_IGC_RXPBS_RXPBSIZE_DEFAULT 0x000000A2

/* Internal Tx Packet Buffer Size*/
#define INTEL_IGC_TXPBS                  0x03404
#define INTEL_IGC_TXPBS_TXPBSIZE_DEFAULT 0x04000014

/* Interrupt Cause Read */
#define INTEL_IGC_ICR 0x01500

/* Interrupt Cause Set */
#define INTEL_IGC_ICS 0x01504

/* Interrupt Mask Set/Read */
#define INTEL_IGC_IMS 0x01508

/* Interrupt Mask Clear */
#define INTEL_IGC_IMC 0x0150C

#define INTEL_IGC_TXDW      BIT(0)
#define INTEL_IGC_LSC       BIT(2)
#define INTEL_IGC_RXDMT0    BIT(4)
#define INTEL_IGC_RX_MISS   BIT(6)
#define INTEL_IGC_RXDW      BIT(7)
#define INTEL_IGC_TIME_SYNC BIT(19)
#define INTEL_IGC_DRSTA     BIT(30)
#define INTEL_IGC_INTA      BIT(31)

/* General Purpose Interrupt Enable */
#define INTEL_IGC_GPIE           0x01514
#define INTEL_IGC_GPIE_NSICR     BIT(0)
#define INTEL_IGC_GPIE_MSIX_MODE BIT(4)
#define INTEL_IGC_GPIE_EIAME     BIT(30)
#define INTEL_IGC_GPIE_PBA       BIT(31)

/* Extended Interrupt Cause Set */
#define INTEL_IGC_EICS 0x01520

/* Extended Interrupt Mask Set/Read */
#define INTEL_IGC_EIMS 0x01524

/* Extended Interrupt Mask Clear */
#define INTEL_IGC_EIMC 0x01528

/* Extended Interrupt Auto Clear */
#define INTEL_IGC_EIAC 0x0152C

/* Extended Interrupt Auto Mask */
#define INTEL_IGC_EIAM 0x01530

/* Extended Interrupt Cause read */
#define INTEL_IGC_EICR 0x01580

/* Interrupt Throttle */
#define INTEL_IGC_EITR_BASE_ADDR 0x01680
#define INTEL_IGC_EITR(n)        (INTEL_IGC_EITR_BASE_ADDR + (n * 4))

/* Interrupt Vector Allocation */
#define INTEL_IGC_IVAR_BASE_ADDR 0x01700
#define INTEL_IGC_IVAR(n)        (INTEL_IGC_IVAR_BASE_ADDR + (n * 4))

/* Interrupt Vector Allocation MISC */
#define INTEL_IGC_IVAR_MISC              0x01740
#define INTEL_IGC_IVAR_INT_VALID_BIT     BIT(7)
#define INTEL_IGC_IVAR_MSI_CLEAR_RX0_RX2 0xFFFFFF00
#define INTEL_IGC_IVAR_MSI_CLEAR_TX0_TX2 0xFFFF00FF
#define INTEL_IGC_IVAR_MSI_CLEAR_RX1_RX3 0xFF00FFFF
#define INTEL_IGC_IVAR_MSI_CLEAR_TX1_TX3 0x00FFFFFF

/* Receive Control */
#define INTEL_IGC_RCTL          0x00100
#define INTEL_IGC_RCTL_EN       BIT(1)
#define INTEL_IGC_RCTL_SBP      BIT(2)
#define INTEL_IGC_RCTL_UPE      BIT(3)
#define INTEL_IGC_RCTL_MPE      BIT(4)
#define INTEL_IGC_RCTL_LPE      BIT(5)
#define INTEL_IGC_RCTL_LBM_MAC  BIT(6)
#define INTEL_IGC_RCTL_BAM      BIT(15)
#define INTEL_IGC_RCTL_VFE      BIT(18)
#define INTEL_IGC_RCTL_CFIEN    BIT(19)
#define INTEL_IGC_RCTL_PADSMALL BIT(21)
#define INTEL_IGC_RCTL_DPF      BIT(22)
#define INTEL_IGC_RCTL_PMCF     BIT(23)
#define INTEL_IGC_RCTL_SECRC    BIT(26)
#define INTEL_IGC_RCTL_MO_SHIFT 12
#define INTEL_IGC_RCTL_SZ_2048  0x0
#define INTEL_IGC_RCTL_SZ_1024  GENMASK(16, 16)
#define INTEL_IGC_RCTL_SZ_512   GENMASK(17, 17)
#define INTEL_IGC_RCTL_SZ_256   GENMASK(17, 16)
#define INTEL_IGC_RCTL_LBM_TCVR GENMASK(7, 6)

/* Split and Replication Receive Control */
#define INTEL_IGC_SRRCTL_BASE          0x0C00C
#define INTEL_IGC_SRRCTL_OFFSET        0x40
#define INTEL_IGC_SRRCTL(n)            (INTEL_IGC_SRRCTL_BASE + (INTEL_IGC_SRRCTL_OFFSET * (n)))
#define INTEL_IGC_SRRCTL_BSIZEPKT_MASK GENMASK(6, 0)

/* Convert as 1024 Bytes resolution */
#define INTEL_IGC_SRRCTL_BSIZEPKT(x)   FIELD_PREP(INTEL_IGC_SRRCTL_BSIZEPKT_MASK, (x) / 1024)
#define INTEL_IGC_SRRCTL_BSIZEHDR_MASK GENMASK(13, 8)

/* Covert as 64 Bytes resolution */
#define INTEL_IGC_SRRCTL_BSIZEHDR(x)         FIELD_PREP(INTEL_IGC_SRRCTL_BSIZEHDR_MASK, (x) / 64)
#define INTEL_IGC_RXBUFFER_256               256
#define INTEL_IGC_SRRCTL_DESCTYPE_ADV_ONEBUF BIT(25)
#define INTEL_IGC_SRRCTL_DROP_EN             BIT(31)

/* Receive Descriptor Base Address Low */
#define INTEL_IGC_RDBAL_BASE_ADDR 0x0C000
#define INTEL_IGC_RDBAL_OFFSET    0x40
#define INTEL_IGC_RDBAL(n)        (INTEL_IGC_RDBAL_BASE_ADDR + (INTEL_IGC_RDBAL_OFFSET * (n)))

/* Receive Descriptor Base Address High */
#define INTEL_IGC_RDBAH_BASE_ADDR 0x0C004
#define INTEL_IGC_RDBAH_OFFSET    0x40
#define INTEL_IGC_RDBAH(n)        (INTEL_IGC_RDBAH_BASE_ADDR + (INTEL_IGC_RDBAH_OFFSET * (n)))

/* Receive Descriptor Ring Length */
#define INTEL_IGC_RDLEN_BASE_ADDR 0x0C008
#define INTEL_IGC_RDLEN_OFFSET    0x40
#define INTEL_IGC_RDLEN(n)        (INTEL_IGC_RDLEN_BASE_ADDR + (INTEL_IGC_RDLEN_OFFSET * (n)))

/* Receive Descriptor Head */
#define INTEL_IGC_RDH_BASE_ADDR 0x0C010
#define INTEL_IGC_RDH_OFFSET    0x40
#define INTEL_IGC_RDH(n)        (INTEL_IGC_RDH_BASE_ADDR + (INTEL_IGC_RDH_OFFSET * (n)))

/* Receive Descriptor Tail */
#define INTEL_IGC_RDT_BASE_ADDR 0x0C018
#define INTEL_IGC_RDT_OFFSET    0x40
#define INTEL_IGC_RDT(n)        (INTEL_IGC_RDT_BASE_ADDR + (INTEL_IGC_RDT_OFFSET * (n)))

/* Receive Descriptor Control */
#define INTEL_IGC_RXDCTL_BASE_ADDR    0x0C028
#define INTEL_IGC_RXDCTL_OFFSET       0x40
#define INTEL_IGC_RXDCTL(n)           (INTEL_IGC_RXDCTL_BASE_ADDR + (INTEL_IGC_RXDCTL_OFFSET * (n)))
#define INTEL_IGC_RXDCTL_QUEUE_ENABLE BIT(25)
#define INTEL_IGC_RXDCTL_SWFLUSH      BIT(26)

#define INTEL_IGC_RX_THRESH_RESET  GENMASK(31, 21)
#define INTEL_IGC_RX_PTHRESH_VAL   8
#define INTEL_IGC_RX_HTHRESH_VAL   8
#define INTEL_IGC_RX_WTHRESH_VAL   8
#define INTEL_IGC_RX_PTHRESH_SHIFT 0
#define INTEL_IGC_RX_HTHRESH_SHIFT 8
#define INTEL_IGC_RX_WTHRESH_SHIFT 16

/* Receive Queue Drop Packet Count  */
#define INTEL_IGC_RQDPC_BASE_ADDR 0x0C030
#define INTEL_IGC_RQDPC_OFFSET    0x40
#define INTEL_IGC_RQDPC(n)        (INTEL_IGC_RQDPC_BASE_ADDR + (INTEL_IGC_RQDPC_OFFSET * (n)))

/* Receive Checksum Control */
#define INTEL_IGC_RXCSUM        0x05000
#define INTEL_IGC_RXCSUM_CRCOFL BIT(11)
#define INTEL_IGC_RXCSUM_PCSD   BIT(13)

/* Receive Long Packet Maximum Length */
#define INTEL_IGC_RLPML 0x05004

/* Receive Filter Control */
#define INTEL_IGC_RFCTL             0x05008
#define INTEL_IGC_RFCTL_IPV6_EX_DIS BIT(16)
#define INTEL_IGC_RFCTL_LEF         BIT(18)

/* Collision related config parameters */
#define INTEL_IGC_TCTL_CT_SHIFT       4
#define INTEL_IGC_COLLISION_THRESHOLD 15

/* Transmit Control Register */
#define INTEL_IGC_TCTL      0x00400
#define INTEL_IGC_TCTL_EN   BIT(1)
#define INTEL_IGC_TCTL_PSP  BIT(3)
#define INTEL_IGC_TCTL_RTLC BIT(24)
#define INTEL_IGC_TCTL_CT   GENMASK(11, 4)
#define INTEL_IGC_TCTL_COLD GENMASK(21, 12)

/* Transmit Descriptor Base Address Low */
#define INTEL_TDBAL_BASE_ADDR 0x0E000
#define INTEL_TDBAL_OFFSET    0x40
#define INTEL_IGC_TDBAL(n)    (INTEL_TDBAL_BASE_ADDR + (INTEL_TDBAL_OFFSET * (n)))

/* Transmit Descriptor Base Address High */
#define INTEL_TDBAH_BASE_ADDR 0x0E004
#define INTEL_TDBAH_OFFSET    0x40
#define INTEL_IGC_TDBAH(n)    (INTEL_TDBAH_BASE_ADDR + (INTEL_TDBAH_OFFSET * (n)))

/* Transmit Descriptor Ring Length  */
#define INTEL_TDLEN_BASE_ADDR 0x0E008
#define INTEL_TDLEN_OFFSET    0x40
#define INTEL_IGC_TDLEN(n)    (INTEL_TDLEN_BASE_ADDR + (INTEL_TDLEN_OFFSET * (n)))

/* Transmit Descriptor Head */
#define INTEL_TDH_BASE_ADDR 0x0E010
#define INTEL_TDH_OFFSET    0x40
#define INTEL_IGC_TDH(n)    (INTEL_TDH_BASE_ADDR + (INTEL_TDH_OFFSET * (n)))

/* Transmit Descriptor Tail */
#define INTEL_TDT_BASE_ADDR 0x0E018
#define INTEL_TDT_OFFSET    0x40
#define INTEL_IGC_TDT(n)    (INTEL_TDT_BASE_ADDR + (INTEL_TDT_OFFSET * (n)))

/* Transmit Descriptor Control */
#define INTEL_TXDCTL_BASE_ADDR        0x0E028
#define INTEL_TXDCTL_OFFSET           0x40
#define INTEL_IGC_TXDCTL(n)           (INTEL_TXDCTL_BASE_ADDR + (INTEL_TXDCTL_OFFSET * (n)))
#define INTEL_IGC_TXDCTL_QUEUE_ENABLE BIT(25)

#define INTEL_IGC_TX_PTHRESH_VAL   8
#define INTEL_IGC_TX_HTHRESH_VAL   8
#define INTEL_IGC_TX_WTHRESH_VAL   8
#define INTEL_IGC_TX_PTHRESH_SHIFT 0
#define INTEL_IGC_TX_HTHRESH_SHIFT 8
#define INTEL_IGC_TX_WTHRESH_SHIFT 16
#define INTEL_IGC_TX_DESC_TYPE     0x3

/* Statistics Register Descriptions */
#define INTEL_IGC_CRCERRS      0x04000
#define INTEL_IGC_ALGNERRC     0x04004
#define INTEL_IGC_RXERRC       0x0400C
#define INTEL_IGC_MPC          0x04010
#define INTEL_IGC_SCC          0x04014
#define INTEL_IGC_ECOL         0x04018
#define INTEL_IGC_MCC          0x0401C
#define INTEL_IGC_LATECOL      0x04020
#define INTEL_IGC_COLC         0x04028
#define INTEL_IGC_RERC         0x0402C
#define INTEL_IGC_DC           0x04030
#define INTEL_IGC_TNCRS        0x04034
#define INTEL_IGC_HTDPMC       0x0403C
#define INTEL_IGC_RLEC         0x04040
#define INTEL_IGC_XONRXC       0x04048
#define INTEL_IGC_XONTXC       0x0404C
#define INTEL_IGC_XOFFRXC      0x04050
#define INTEL_IGC_XOFFTXC      0x04054
#define INTEL_IGC_FCRUC        0x04058
#define INTEL_IGC_PRC64        0x0405C
#define INTEL_IGC_PRC127       0x04060
#define INTEL_IGC_PRC255       0x04064
#define INTEL_IGC_PRC511       0x04068
#define INTEL_IGC_PRC1023      0x0406C
#define INTEL_IGC_PRC1522      0x04070
#define INTEL_IGC_GPRC         0x04074
#define INTEL_IGC_BPRC         0x04078
#define INTEL_IGC_MPRC         0x0407C
#define INTEL_IGC_GPTC         0x04080
#define INTEL_IGC_GORCL        0x04088
#define INTEL_IGC_GORCH        0x0408C
#define INTEL_IGC_GOTCL        0x04090
#define INTEL_IGC_GOTCH        0x04094
#define INTEL_IGC_RNBC         0x040A0
#define INTEL_IGC_RUC          0x040A4
#define INTEL_IGC_RFC          0x040A8
#define INTEL_IGC_ROC          0x040AC
#define INTEL_IGC_RJC          0x040B0
#define INTEL_IGC_MGTPRC       0x040B4
#define INTEL_IGC_MGTPDC       0x040B8
#define INTEL_IGC_MGTPTC       0x040BC
#define INTEL_IGC_TORL         0x040C0
#define INTEL_IGC_TORH         0x040C4
#define INTEL_IGC_TOTL         0x040C8
#define INTEL_IGC_TOTH         0x040CC
#define INTEL_IGC_TPR          0x040D0
#define INTEL_IGC_TPT          0x040D4
#define INTEL_IGC_PTC64        0x040D8
#define INTEL_IGC_PTC127       0x040DC
#define INTEL_IGC_PTC255       0x040E0
#define INTEL_IGC_PTC511       0x040E4
#define INTEL_IGC_PTC1023      0x040E8
#define INTEL_IGC_PTC1522      0x040EC
#define INTEL_IGC_MPTC         0x040F0
#define INTEL_IGC_BPTC         0x040F4
#define INTEL_IGC_TSCTC        0x040F8
#define INTEL_IGC_IAC          0x04100
#define INTEL_IGC_RPTHC        0x04104
#define INTEL_IGC_TLPIC        0x04148
#define INTEL_IGC_RLPIC        0x0414C
#define INTEL_IGC_HGPTC        0x04118
#define INTEL_IGC_RXDMTC       0x04120
#define INTEL_IGC_HGORCL       0x04128
#define INTEL_IGC_HGORCH       0x0412C
#define INTEL_IGC_HGOTCL       0x04130
#define INTEL_IGC_HGOTCH       0x04134
#define INTEL_IGC_LENERRS      0x04138
#define INTEL_IGC_TQDPC_BASE   0x0E030
#define INTEL_IGC_TQDPC_OFFSET 0x40
#define INTEL_IGC_TQDPC(n)     (INTEL_IGC_TQDPC_BASE + (INTEL_IGC_TQDPC_OFFSET * (n)))

#define INTEL_IGC_GIO_MASTER_DISABLE_TIMEOUT 800
#define INTEL_IGC_RESET_DELAY                1

#define INTEL_IGC_RAL(i) (((i) <= 15) ? (0x05400 + ((i) * 8)) : (0x054E0 + ((i - 16) * 8)))
#define INTEL_IGC_RAH(i) (((i) <= 15) ? (0x05404 + ((i) * 8)) : (0x054E4 + ((i - 16) * 8)))

typedef void (*eth_config_irq_t)(const struct device *);

struct eth_intel_igc_intr_info {
	const struct device *mac;
	msi_vector_t msi_vector;
	uint8_t id;
};

enum eth_igc_mac_filter_mode {
	DEST_ADDR, /* normal mode */
	SRC_ADDR
};

/**
 * @brief This Advanced transmit descriptor format is crucial for the transmit DMA.
 * Field misalignment or size change will break the DMA operation. Modify this
 * structure with caution.
 */
union dma_tx_desc {
	struct {
		uint64_t data_buf_addr;

		unsigned int data_len: 16;
		unsigned int ptp1: 4;
		unsigned int desc_type: 4;
		unsigned int eop: 1;
		unsigned int ifcs: 1;
		unsigned int reserved1: 1;
		unsigned int rs: 1;
		unsigned int reserved2: 1;
		unsigned int dext: 1;
		unsigned int vle: 1;
		unsigned int tse: 1;
		unsigned int dd: 1;
		unsigned int ts_stat: 1;
		unsigned int reserved3: 2;
		unsigned int idx: 1;
		unsigned int ptp2: 3;
		unsigned int popts: 6;
		unsigned int payloadlen: 18;
	} read;

	struct {
		uint64_t dma_time_stamp;

		unsigned int reserved1: 32;
		unsigned int dd: 1;
		unsigned int ts_stat: 1;
		unsigned int reserved2: 2;
		unsigned int reserved3: 28;
	} writeback;
};

/**
 * @brief This Advanced receive descriptor format is crucial for the receive DMA.
 * Field misalignment or size change will break the DMA operation. Modify this
 * structure with caution.
 */
union dma_rx_desc {
	struct {
		uint64_t pkt_buf_addr;
		uint64_t hdr_buf_addr;
	} read;

	struct {
		unsigned int rss_type: 4;
		unsigned int pkt_type: 13;
		unsigned int reserved1: 2;
		unsigned int hdr_len: 12;
		unsigned int sph: 1;
		unsigned int rss_has_val: 32;

		/* extended status */
		unsigned int dd: 1;
		unsigned int eop: 1;
		unsigned int reserved2: 1;
		unsigned int vp: 1;
		unsigned int udpcs: 1;
		unsigned int l4cs: 1;
		unsigned int ipcs: 1;
		unsigned int pif: 1;
		unsigned int reserved3: 1;
		unsigned int vext: 1;
		unsigned int udpv: 1;
		unsigned int llint: 1;
		unsigned int crc_strip: 1;
		unsigned int smd_type: 2;
		unsigned int tsip: 1;
		unsigned int reserved4: 3;
		unsigned int mc: 1;

		/* extended error */
		unsigned int reserved5: 3;
		unsigned int hbo: 1;
		unsigned int reserved6: 5;
		unsigned int l4e: 1;
		unsigned int ipe: 1;
		unsigned int rxe: 1;

		unsigned int pkt_len: 16;
		unsigned int vlan_tag: 16;
	} writeback;
};

struct eth_intel_igc_mac_cfg {
	const struct device *const platform;
	const struct device *const phy;
	eth_config_irq_t config_func;
	uint32_t num_tx_desc;
	uint32_t num_rx_desc;
	uint8_t num_queues;
	uint8_t num_msix;
	bool random_mac_address;
};

struct eth_intel_igc_mac_tx {
	union dma_tx_desc *desc;
	struct k_sem *sem;
	uint32_t *ring_wr_ptr;
	uint32_t *ring_rd_ptr;
	struct net_buf **frag;
	struct net_pkt **pkt;
};

struct eth_intel_igc_mac_rx {
	union dma_rx_desc *desc;
	struct k_sem *sem;
	uint32_t *ring_wr_ptr;
	uint32_t *ring_rd_ptr;
	uint8_t *buf;
};

struct eth_intel_igc_mac_data {
	struct net_if *iface;
	const struct device *mac;
	struct eth_intel_igc_mac_tx tx;
	struct eth_intel_igc_mac_rx rx;
	struct k_work tx_work[INTEL_IGC_MAX_QCNT];
	struct k_work_delayable rx_work[INTEL_IGC_MAX_QCNT];
	struct eth_intel_igc_intr_info *intr_info;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	msi_vector_t *msi_vec;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
	mm_reg_t base;
};

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_INTEL_IGC_PRIV_H_*/
