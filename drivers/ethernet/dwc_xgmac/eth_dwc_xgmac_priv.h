/*
 * Intel Hard Processor System 10 Giga bit TSN Ethernet Media Access controller (XGMAC) driver
 *
 * Driver private data declarations
 *
 * Copyright (c) 2024 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_ETHERNET_ETH_DWC_XGMAC_PRIV_H_
#define _ZEPHYR_DRIVERS_ETHERNET_ETH_DWC_XGMAC_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include "../eth.h"

#define SET_BIT   1
#define RESET_BIT 0

#define READ_BIT(var, bit) ((var >> bit) & 1u)

/* Offset addresses of the register sets in XGMAC
 */
#define XGMAC_CORE_BASE_ADDR_OFFSET         (0x0000u)
#define XGMAC_MTL_BASE_ADDR_OFFSET          (0x1000u)
#define XGMAC_MTL_TCQ_BASE_ADDR_OFFSET      (0x1100u)
#define XGMAC_DMA_BASE_ADDR_OFFSET          (0x3000u)
#define XGMAC_DMA_CHNL_BASE_ADDR_OFFSET     (0x3100u)
#define XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(x) (XGMAC_DMA_CHNL_BASE_ADDR_OFFSET + (x * 0x80u))
#define XGMAC_MTL_TCQx_BASE_ADDR_OFFSET(x)  (XGMAC_MTL_TCQ_BASE_ADDR_OFFSET + (x * 0x80u))
#define XGMAC_CORE_ADDRx_HIGH(x)            (CORE_MAC_ADDRESS0_HIGH_OFST + (x) * 0x8)
#define XGMAC_CORE_ADDRx_LOW(x)             (CORE_MAC_ADDRESS0_LOW_OFST + (x) * 0x8)

#define XGMAC_DESC_OWNED_BY_DMA (1u)

#define NUM_OF_RxQs_PER_DMA_MAP_REG                (4u)
#define MTL_RXQ_DMA_MAP_Qx_MSK(q_pos)              (~(0xffu << (q_pos * 8u)))
#define MTL_RXQ_DMA_MAP_QxDDMACH_SET(q_pos, value) ((value & 0x1u) << (8u * q_pos + 7u))
#define MTL_RXQ_DMA_MAP_QxMDMACH_SET(q_pos, value) ((value & 0x7u) << (8u * q_pos))

#define NUM_OF_TCs_PER_TC_PRTY_MAP_REG (4u)
#define TC_PRTY_MAP_FEILD_SIZE_IN_BITS (8u)
#define MTL_TCx_PRTY_MAP_MSK(TCx_pos)  (~(0xff << (TCx_pos * TC_PRTY_MAP_FEILD_SIZE_IN_BITS)))

#define MTL_TCx_PRTY_MAP_PSTC_SET(TCx_pos, prio) ((prio & 0xff) << (8u * TCx_pos))

#define DMA_MODE_OFST (0x0)

#define DMA_MODE_SWR_SET(value) ((value) & 0x00000001)

#define DMA_MODE_SWR_SET_MSK (0x00000001)

#define DMA_MODE_INTM_CLR_MSK (0xffffcfff)

#define DMA_MODE_INTM_SET(value) (((value) << 12) & 0x00003000)

#define DMA_SYSBUS_MODE_OFST (0x4)

#define DMA_SYSBUS_MODE_RD_OSR_LMT_SET(value) (((value) << 16) & 0x001f0000)

#define DMA_SYSBUS_MODE_WR_OSR_LMT_SET(value) (((value) << 24) & 0x1f000000)

#define DMA_SYSBUS_MODE_AAL_SET(value) (((value) << 12) & 0x00001000)

#define DMA_SYSBUS_MODE_EAME_SET(value) (((value) << 11) & 0x00000800)

#define DMA_SYSBUS_MODE_BLEN4_SET(value) (((value) << 1) & 0x00000002)

#define DMA_SYSBUS_MODE_BLEN8_SET(value) (((value) << 2) & 0x00000004)

#define DMA_SYSBUS_MODE_BLEN16_SET(value) (((value) << 3) & 0x00000008)

#define DMA_SYSBUS_MODE_BLEN32_SET(value) (((value) << 4) & 0x00000010)

#define DMA_SYSBUS_MODE_BLEN64_SET(value) (((value) << 5) & 0x00000020)

#define DMA_SYSBUS_MODE_BLEN128_SET(value) (((value) << 6) & 0x00000040)

#define DMA_SYSBUS_MODE_BLEN256_SET(value) (((value) << 7) & 0x00000080)

#define DMA_SYSBUS_MODE_UNDEF_SET(value) ((value) & 0x00000001)

#define DMA_TX_EDMA_CONTROL_OFST (0x40)

#define DMA_TX_EDMA_CONTROL_TDPS_SET(value) ((value) & 0x00000003)

#define DMA_RX_EDMA_CONTROL_OFST (0x44)

#define DMA_RX_EDMA_CONTROL_RDPS_SET(value) ((value) & 0x00000003)

#define DMA_INTERRUPT_STATUS_OFST (0x8)

#define DMA_CHx_STATUS_OFST (0x60)

#define DMA_CHx_STATUS_TI_SET_MSK (0x00000001)

#define DMA_CHx_STATUS_RI_SET_MSK (0x00000040)

#define DMA_CHx_STATUS_TPS_SET_MSK (0x00000002)

#define DMA_CHx_STATUS_TBU_SET_MSK (0x00000004)

#define DMA_CHx_STATUS_RBU_SET_MSK (0x00000080)

#define DMA_CHx_STATUS_RPS_SET_MSK (0x00000100)

#define DMA_CHx_STATUS_DDE_SET_MSK (0x00000200)

#define DMA_CHx_STATUS_FBE_SET_MSK (0x00001000)

#define DMA_CHx_STATUS_CDE_SET_MSK (0x00002000)

#define DMA_CHx_STATUS_AIS_SET_MSK (0x00004000)

#define DMA_CHx_STATUS_NIS_SET_MSK (0x00008000)

#define DMA_CHx_CONTROL_OFST (0x0)

#define DMA_CHx_CONTROL_SPH_SET(value) (((value) << 24) & 0x01000000)

#define DMA_CHx_CONTROL_PBLX8_SET(value) (((value) << 16) & 0x00010000)

#define DMA_CHx_CONTROL_MSS_SET(value) ((value) & 0x00003fff)

#define DMA_CHx_TX_CONTROL_OFST (0x4)

#define DMA_CHx_TX_CONTROL_TXPBL_SET(value) (((value) << 16) & 0x003f0000)

#define DMA_CHx_TX_CONTROL_TSE_SET(value) (((value) << 12) & 0x00001000)

#define DMA_CHx_TX_CONTROL_RESERVED_OSP_SET(value) (((value) << 4) & 0x00000010)

#define DMA_CHx_TX_CONTROL_ST_CLR_MSK (0xfffffffe)

#define DMA_CHx_RX_CONTROL_OFST (0x8)

#define DMA_CHx_RX_CONTROL_RPF_SET(value) (((value) << 31) & 0x80000000)

#define DMA_CHx_RX_CONTROL_RXPBL_SET(value) (((value) << 16) & 0x003f0000)

#define DMA_CHx_RX_CONTROL_RBSZ_SET(value) ((value << 1) & 0x00007ff0)

#define DMA_CHx_RX_CONTROL_SR_CLR_MSK (0xfffffffe)

#define DMA_CHx_TXDESC_LIST_HADDRESS_OFST (0x10)

#define DMA_CHx_TXDESC_LIST_HADDRESS_TDESHA_SET(value) ((value) & 0x000000ff)

#define DMA_CHx_TXDESC_LIST_LADDRESS_OFST (0x14)

#define DMA_CHx_RXDESC_LIST_HADDRESS_OFST (0x18)

#define DMA_CHx_RXDESC_LIST_LADDRESS_OFST (0x1c)

#define DMA_CHx_TXDESC_TAIL_LPOINTER_OFST (0x24)

#define DMA_CHx_TXDESC_TAIL_LPOINTER_TDT_SET(value) ((value) & 0xfffffff8)

#define DMA_CHx_RXDESC_TAIL_LPOINTER_OFST (0x2c)

#define DMA_CHx_RXDESC_TAIL_LPOINTER_RDT_SET(value) ((value) & 0xfffffff8)

#define DMA_CHx_TX_CONTROL2_OFST (0x30)

#define DMA_CHx_TX_CONTROL2_TDRL_SET(value) (((value) << 0) & 0x0000ffff)

#define DMA_CHx_RX_CONTROL2_OFST (0x34)

#define DMA_CHx_RX_CONTROL2_RDRL_SET(value) (((value) << 0) & 0x0000ffff)

#define DMA_CHx_TX_CONTROL_ST_SET_MSK (0x00000001)

#define DMA_CHx_RX_CONTROL_SR_SET_MSK (0x00000001)

#define DMA_CHx_INTERRUPT_ENABLE_OFST (0x38)

#define DMA_CHx_INTERRUPT_ENABLE_NIE_SET(value) (((value) << 15) & 0x00008000)

#define DMA_CHx_INTERRUPT_ENABLE_AIE_SET(value) (((value) << 14) & 0x00004000)

#define DMA_CHx_INTERRUPT_ENABLE_CDEE_SET(value) (((value) << 13) & 0x00002000)

#define DMA_CHx_INTERRUPT_ENABLE_FBEE_SET(value) (((value) << 12) & 0x00001000)

#define DMA_CHx_INTERRUPT_ENABLE_DDEE_SET(value) (((value) << 9) & 0x00000200)

#define DMA_CHx_INTERRUPT_ENABLE_RSE_SET(value) (((value) << 8) & 0x00000100)

#define DMA_CHx_INTERRUPT_ENABLE_RBUE_SET(value) (((value) << 7) & 0x00000080)

#define DMA_CHx_INTERRUPT_ENABLE_RIE_SET(value) (((value) << 6) & 0x00000040)

#define DMA_CHx_INTERRUPT_ENABLE_TBUE_SET(value) (((value) << 2) & 0x00000004)

#define DMA_CHx_INTERRUPT_ENABLE_TXSE_SET(value) (((value) << 1) & 0x00000002)

#define DMA_CHx_INTERRUPT_ENABLE_TIE_SET(value) (((value) << 0) & 0x00000001)

#define MTL_OPERATION_MODE_OFST (0x0)

#define MTL_OPERATION_MODE_ETSALG_SET(value) (((value) << 5) & 0x00000060)

#define MTL_OPERATION_MODE_RAA_SET(value) (((value) << 2) & 0x00000004)

#define MTL_TC_PRTY_MAP0_OFST (0x40)

#define MTL_RXQ_DMA_MAP0_OFST (0x30)

#define MTL_TCQx_MTL_TXQx_OPERATION_MODE_OFST (0x0)

#define MTL_TCQx_MTL_TXQx_OPERATION_MODE_TQS_SET(value) (((value) << 16) & 0x007f0000)

#define MTL_TCQx_MTL_TXQx_OPERATION_MODE_Q2TCMAP_SET(value) (((value) << 8) & 0x00000700)

#define MTL_TCQx_MTL_TXQx_OPERATION_MODE_TTC_SET(value) (((value) << 4) & 0x00000070)

#define MTL_TCQx_MTL_TXQx_OPERATION_MODE_TXQEN_SET(value) (((value) << 2) & 0x0000000c)

#define MTL_TCQx_MTL_TXQx_OPERATION_MODE_TSF_SET(value) (((value) << 1) & 0x00000002)

#define MTL_TCQx_MTC_TCx_ETS_CONTROL_OFST (0x10)

#define MTL_TCQx_MTC_TCx_ETS_CONTROL_TSA_SET(value) (((value) << 0) & 0x00000003)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_OFST (0x40)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_RQS_SET(value) (((value) << 16) & 0x003f0000)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_EHFC_SET(value) (((value) << 7) & 0x00000080)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_DIS_TCP_EF_SET(value) (((value) << 6) & 0x00000040)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_RSF_SET(value) (((value) << 5) & 0x00000020)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_FEF_SET(value) (((value) << 4) & 0x00000010)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_FUF_SET(value) (((value) << 3) & 0x00000008)

#define MTL_TCQx_MTL_RXQx_OPERATION_MODE_RTC_SET(value) (((value) << 0) & 0x00000003)

#define MTL_INTERRUPT_STATUS_OFST (0x20)

#define CORE_MAC_ADDRESSx_HIGH_SA_SET(value) (((value) << 30) & 0x40000000)

#define CORE_MAC_ADDRESS1_HIGH_AE_SET_MSK (0x80000000)

#define CORE_MAC_ADDRESS0_HIGH_OFST (0x300)

#define CORE_MAC_ADDRESS0_LOW_OFST (0x304)

#define CORE_MAC_TX_CONFIGURATION_OFST (0x0)

#define CORE_MAC_TX_CONFIGURATION_SS_CLR_MSK (0x1fffffff)

#define CORE_MAC_TX_CONFIGURATION_SS_SET(value) (((value) << 29) & 0xe0000000)

#define CORE_MAC_TX_CONFIGURATION_JD_SET(value) (((value) << 16) & 0x00010000)

#define CORE_MAC_RXQ_CTRL0_OFST (0xa0)

#define CORE_MAC_RX_CONFIGURATION_OFST (0x4)

#define CORE_MAC_RX_CONFIGURATION_GPSLCE_SET(value) (((value) << 6) & 0x00000040)

#define CORE_MAC_RX_CONFIGURATION_WD_SET(value) (((value) << 7) & 0x00000080)

#define CORE_MAC_RX_CONFIGURATION_JE_SET(value) (((value) << 8) & 0x00000100)

#define CORE_MAC_RX_CONFIGURATION_ARPEN_SET(value) (((value) << 31) & 0x80000000)

#define CORE_MAC_RX_CONFIGURATION_GPSL_SET(value) (((value) << 16) & 0x3fff0000)

#define CORE_MAC_TX_CONFIGURATION_TE_SET(value) (((value) << 0) & 0x00000001)

#define CORE_MAC_RX_CONFIGURATION_RE_SET(value) (((value) << 0) & 0x00000001)

#define CORE_MAC_TX_CONFIGURATION_TE_CLR_MSK (0xfffffffe)

#define CORE_MAC_TX_CONFIGURATION_SS_10MHZ (0x07)

#define CORE_MAC_TX_CONFIGURATION_SS_100MHZ (0x04)

#define CORE_MAC_TX_CONFIGURATION_SS_1000MHZ (0x03)

#define CORE_MAC_TX_CONFIGURATION_SS_2500MHZ (0x06)

#define CORE_MAC_RX_CONFIGURATION_RE_CLR_MSK (0xfffffffe)

#define CORE_MAC_INTERRUPT_STATUS_OFST (0xb0)

#define CORE_MAC_INTERRUPT_ENABLE_OFST (0xb4)

#define CORE_MAC_INTERRUPT_ENABLE_LSIE_SET(value) (((value) << 0) & 0x00000001)

#define CORE_MAC_PACKET_FILTER_OFST (0x8)

#define CORE_MAC_PACKET_FILTER_IPFE_SET(value) (((value) << 20) & 0x00100000)

#define CORE_MAC_PACKET_FILTER_HPF_SET(value) (((value) << 10) & 0x00000400)

#define CORE_MAC_PACKET_FILTER_HMC_SET(value) (((value) << 2) & 0x00000004)

#define CORE_MAC_PACKET_FILTER_HUC_SET(value) (((value) << 1) & 0x00000002)

#define CORE_MAC_RX_CONFIGURATION_IPC_SET(value) (((value) << 9) & 0x00000200)

#define CORE_MAC_ADDRESS1_HIGH_AE_CLR_MSK 0x7fffffff

#define CORE_MAC_ADDRESS1_LOW_ADDRLO_SET_MSK 0xffffffff

#define CORE_MAC_PACKET_FILTER_PR_CLR_MSK 0xfffffffe

#define CORE_MAC_PACKET_FILTER_PR_SET(value) (((value) << 0) & 0x00000001)

#define CORE_MAC_PACKET_FILTER_RA_SET(value) (((value) << 31) & 0x80000000)

#define CORE_MAC_PACKET_FILTER_PM_SET(value) (((value) << 4) & 0x00000010)

/* 0th index mac address is not used for L2 filtering */
#define XGMAC_MAX_MAC_ADDR_COUNT (32u)
#define MAC_ADDR_BYTE_5          (5)
#define MAC_ADDR_BYTE_4          (4)
#define MAC_ADDR_BYTE_3          (3)
#define MAC_ADDR_BYTE_2          (2)
#define MAC_ADDR_BYTE_1          (1)
#define MAC_ADDR_BYTE_0          (0)
#define BIT_OFFSET_8             (8)
#define BIT_OFFSET_16            (16)
#define BIT_OFFSET_24            (24)

#define XGMAC_RXQxEN_DCB       (2u) /* RX queue enabled for Data Center Bridging or Generic */
#define XGMAC_RXQxEN_SIZE_BITS (2u)
#define ETH_MAC_ADDRESS_SIZE   (6u) /*Ethernet MAC address size 6 bytes */

#define XGMAC_TDES2_IOC       BIT(31)
#define XGMAC_TDES3_OWN       BIT(31)
#define XGMAC_TDES3_FD        BIT(29)
#define XGMAC_TDES3_LD        BIT(28)
#define XGMAC_TDES3_CS_EN_MSK (3u << 16u)

#define XGMAC_RDES3_OWN BIT(31)
#define XGMAC_RDES3_IOC BIT(30)
#define XGMAC_RDES3_FD  BIT(29)
#define XGMAC_RDES3_LD  BIT(28)
#define XGMAC_RDES3_ES  BIT(15)
#define XGMAC_RDES3_PL  GENMASK(14, 0)

#define RX_FRAGS_PER_DESC  (2u)
#define XGMAC_POLLING_MODE (2u)
#define RX_FRAG_ONE        (0u)
#define RX_FRAG_TWO        (1u)

#ifdef CONFIG_ETH_DWC_XGMAC_ARP_OFFLOAD
#define ETH_DWC_XGMAC_ARP_OFFLOAD (1u)
#else
#define ETH_DWC_XGMAC_ARP_OFFLOAD (0u)
#endif

#define XGMAC_INTERRUPT_POLING_TIMEOUT_US (500u)

#define ETH_DWC_XGMAC_RESET_STATUS_CHECK_RETRY_COUNT                                               \
	(100)                      /* retry up to 100ms (1 x 100ms poll interval) */
#define XGMAC_REG_SIZE_BYTES (4u)  /*4 Bytes*/
#define XGMAC_REG_SIZE_BITS  (32u) /*4 Bytes*/

#define CHLCNT(n)      DT_INST_PROP(n, num_dma_ch)
#define MAX_TX_RING(n) DT_INST_PROP(n, dma_ch_tdrl)
#define MAX_RX_RING(n) DT_INST_PROP(n, dma_ch_rdrl)

typedef void (*eth_config_irq_t)(const struct device *port);
typedef void (*eth_enable_irq_t)(const struct device *port, bool en);

/**
 * @brief Transmit descriptor
 */
struct xgmac_dma_tx_desc {
	/* First word of descriptor */
	uint32_t tdes0;
	/* Second word of descriptor */
	uint32_t tdes1;
	/* Third word of descriptor */
	uint32_t tdes2;
	/* Fourth word of descriptor */
	uint32_t tdes3;
};

/**
 * @brief Receive descriptor
 *
 */
struct xgmac_dma_rx_desc {
	/* First word of descriptor */
	uint32_t rdes0;
	/* Second word of descriptor */
	uint32_t rdes1;
	/* Third word of descriptor */
	uint32_t rdes2;
	/* Fourth word of descriptor */
	uint32_t rdes3;
};

/**
 * @brief TX DMA memory area buffer descriptor ring management structure.
 *
 * The DMA memory area buffer descriptor ring management structure
 * is used to manage either the TX buffer descriptor array.
 * It contains a pointer to the start of the descriptor array, a
 * semaphore as a means of preventing concurrent access, a free entry
 * counter as well as indices used to determine which BD shall be used
 * or evaluated for the next TX operation.
 */
struct xgmac_dma_tx_desc_meta {
	struct k_sem free_tx_descs_sem;
	/* Concurrent modification protection */
	struct k_mutex ring_lock;
	/* Index of the next BD to be used for TX */
	volatile uint16_t next_to_use;
	/* Address of the first descriptor in the TX descriptor ring. This field will be
	 * updated in TX descriptor initialization and consumed by channel initialization.
	 */
	mem_addr_t desc_list_addr;
	/* Address of the last descriptor in the TX descriptor ring. This field will be
	 * updated in TX descriptor initialization and consumed by channel initialization.
	 */
	volatile mem_addr_t desc_tail_addr;
};
/**
 * @brief RX DMA memory area buffer descriptor ring management structure.
 *
 * The DMA memory area buffer descriptor ring management structure
 * is used to manage either the RX buffer descriptor array.
 * It contains a pointer to the start of the descriptor array, a
 * semaphore as a means of preventing concurrent access, a free entry
 * counter as well as indices used to determine which BD shall be used
 * or evaluated for the next RX operation.
 */
struct xgmac_dma_rx_desc_meta {
	/* Index of the next BD to be read for RX */
	volatile uint16_t next_to_read;
	/* Address of the first descriptor in the RX descriptor ring. this field will be
	 * updated in RX descriptor initialization and consumed by channel initialization
	 */
	mem_addr_t desc_list_addr;
	/* Address of the last descriptor in the RX descriptor ring. this field will be
	 * updated in RX descriptor initialization and consumed by channel initialization
	 */
	volatile mem_addr_t desc_tail_addr;
	struct net_pkt *rx_pkt;
};

struct xgmac_tx_cntxt {
	int timeout; /*Time out in sleep intervals count*/
	/* TX packet queue ID */
	uint8_t q_id;
	struct xgmac_dma_tx_desc_meta *descmeta;
	struct xgmac_dma_tx_desc *tx_desc;
	uint16_t pkt_desc_id;
};

struct xgmac_dma_cfg {
	/* Software configured maximum number of AXI data writing requests */
	uint8_t wr_osr_lmt;
	/* Software configured maximum number of AXI data reading requests */
	uint8_t rd_osr_lmt;
	/* This field controls the threshold in the Descriptor cache after which
	 * the EDMA starts pre-fetching the TxDMA descriptors
	 */
	uint8_t edma_tdps;
	/* This field controls the threshold in the Descriptor cache after which
	 * the EDMA starts pre-fetching the RxDMA descriptors
	 */
	uint8_t edma_rdps;
	/* Mixed burst: AXI master can perform burst transfers that are equal to or
	 * less than the maximum allowed burst length programmed
	 */
	bool ubl;
	/* burst length 4bytes */
	bool blen4;
	/* burst length 8bytes */
	bool blen8;
	/* burst length 16bytes */
	bool blen16;
	/* burst length 32bytes */
	bool blen32;
	/* burst length 64bytes */
	bool blen64;
	/* burst length 128bytes */
	bool blen128;
	/* burst length 256bytes */
	bool blen256;
	/* Address-Aligned Beats. When this bit is set to 1, the AXI master performs
	 * address-aligned burst transfers on Read and Write channels
	 */
	bool aal;
	/* Enhanced Address Mode Enable: e DMA engine uses either the 40- or 48-bit address,
	 * depending on the configuration. This bit is valid only when Address Width is greater than
	 * 32
	 */
	bool eame;
};

struct xgmac_dma_chnl_config {
	/* This field specifies the maximum segment size that should be used while
	 * segmenting the Transmit packet. not applicable when TSO is disabled
	 */
	uint16_t mss;
	/* Transmit Descriptor Ring Length. This field sets the maximum number of Tx
	 * descriptors in the circular descriptor ring. The maximum number of descriptors
	 * is limited to 65536 descriptors
	 */
	uint16_t tdrl;
	/* Receive Descriptor Ring Length. This field sets the maximum number of Rx
	 * descriptors in the circular descriptor ring. The maximum number of descriptors
	 * is limited to 65536 descriptors
	 */
	uint16_t rdrl;
	/* Alternate Receive Buffer Size Indicates size for Buffer 1 when ARBS is
	 * programmed to a non-zero value (when split header feature is not enabled).
	 * When split header feature is enabled, ARBS indicates the buffer size for
	 * header data. It is recommended to use this field when split header feature is
	 * enabled.
	 */
	uint8_t arbs;
	/* maximum receive burst length */
	uint8_t rxpbl;
	/* maximum transmit burst length */
	uint8_t txpbl;
	/* When this bit is set, the DMA splits the header and payload in
	 * the Receive path and stores into the buffer1 and buffer 2 respectively
	 */
	bool sph;
	/* When this is set, the PBL value programmed in Tx_control is multiplied
	 * eight times
	 */
	bool pblx8;
	/* TCP Segmentation Enabled.When this bit is set, the DMA performs the TCP
	 * segmentation for packets in Channel. not applicable when TSO is disabled
	 */
	bool tse;
	/* Operate on Second Packet. When this bit is set, it instructs the DMA to process
	 * the second packet of the Transmit data even before closing the descriptor of the
	 * first packet. not applicable when edma is enabled
	 */
	bool osp;
};

struct xgmac_mtl_config {
	/* Receive Arbitration Algorithm.This field is used to select the arbitration algorithm
	 * for the RX side.
	 * 0: Strict Priority (SP): Queue 0 has the lowest priority and the last queue has the
	 * highest priority. 1: Weighted Strict Priority (WSP)
	 */
	bool raa;
	/* ETS Algorithm. This field selects the type of ETS algorithm to be applied for
	 * traffic classes whose transmission selection algorithm (TSA) is set to ETS:
	 * 0: WRR algorithm
	 * 1: WFQ algorithm
	 * 2: DWRR algorithm
	 */
	uint8_t etsalg;
};

struct xgmac_mac_config {
	/* Giant Packet Size Limit
	 */
	uint32_t gpsl;
	/* ARP offload is enabled/disabled.
	 */
	bool arp_offload_en;
	/* jumbo packet is enabled/disabled.
	 */
	bool je;
};

struct xgmac_tcq_config {
	/* Receive queue enabled for dynamic DMA channel selection when set, this bit indicates
	 * that each packet received in receive queue is routed to a DMA channel as decided in
	 * the MAC Receiver based on the DMA channel number programmed in the L3-L4 filter
	 * registers, RSS lookup table, the ethernet DA address registers or VLAN filter registers.
	 * When reset, this bit indicates that all packets received in receive queue are routed to
	 * the DMA Channel programmed in the rx_q_dma_chnl_sel field.
	 */
	uint8_t rx_q_ddma_en;
	/* Receive Queue Mapped to DMA Channel. This field is valid when the rx_q_ddma_en field is
	 * reset
	 */
	uint8_t rx_q_dma_chnl_sel[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/* Transmit Queue Size. This field indicates the size of the allocated Transmit queues in
	 * blocks of 256 bytes. Range 0 - 63
	 */
	uint8_t tx_q_size[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/* Queue to Traffic Class Mapping
	 * Range TC0 - TC7 -> 0 to 7
	 */
	uint8_t q_to_tc_map[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/* Transmit Threshold Control. These bits control the threshold level of the MTL TX Queue.
	 * Transmission starts when the packet size within the MTL TX Queue is larger than the
	 * threshold. In addition, full packets with length less than the threshold are also
	 * transmitted 0: 64 2: 96 3: 128 4: 192 5: 256 6: 384 7: 512
	 */
	uint8_t ttc[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/* Receive Queue Size. This field indicates the size of the allocated Receive queues in
	 * blocks of 256 bytes Range: 0 - 127
	 */
	uint8_t rx_q_size[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/* Transmit Store and Forward. When this bit is set, the transmission starts when a full
	 * packet resides in the MTL TX Queue. When this bit is set, the values specified in the TTC
	 * field are ignored
	 */
	uint8_t tsf_en;
	/* Enable Hardware Flow Control. When this bit is set, the flow control signal operation,
	 * based on the fill-level of RX queue, is enabled
	 */
	uint8_t hfc_en;
	/* Disable Dropping of TCP/IP Checksum Error Packets */
	uint8_t cs_err_pkt_drop_dis;
	/* Receive Queue Store and Forward. When this bit is set, DWC_xgmac reads a packet from the
	 * RX queue only after the complete packet has been written to it, ignoring the RTC field of
	 * this register
	 */
	uint8_t rsf_en;
	/* Forward Error Packets.  When this bit is set, all packets except the runt error
	 * packets are forwarded to the application or DMA
	 */
	uint8_t fep_en;
	/* Forward Undersized Good Packets. When this bit is set, the RX queue forwards the
	 * undersized good packets
	 */
	uint8_t fup_en;
	/* Receive Queue Threshold Control. These bits control the threshold level of the MTL Rx
	 * queue in bytes 0: 64 2: 96 3: 128
	 */
	uint8_t rtc[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/* Priorities Mapped to Traffic Class. This field determines if the transmit queues
	 * associated with the traffic class should be blocked from transmitting for the specified
	 * pause time when a PFC packet is received with priorities matching the priorities
	 * programmed in this field
	 */
	uint8_t pstc[CONFIG_ETH_XGMAC_MAX_QUEUES];
	/*
	 * uint8_t slc;
	 * uint8_t cc;
	 * uint8_t cbs_en;
	 */

	/* Transmission Selection Algorithm. This field is used to assign a transmission selection
	 * algorithm for this traffic class.
	 * 0: Strict priority
	 * 1: CBS
	 * 2: ETS
	 */
	uint8_t tsa[CONFIG_ETH_XGMAC_MAX_QUEUES];
};

struct xgmac_irq_cntxt_data {
	const struct device *dev;
	/*
	 * DMA interrupt status value
	 */
	volatile uint32_t dma_interrupt_sts;
	/*
	 * Array pointer all dma channel interrupt status registers values
	 */
	volatile uint32_t *dma_chnl_interrupt_sts;
	/*
	 * MTL interrupt status register value
	 */
	volatile uint32_t mtl_interrupt_sts;
	/*
	 * MAC interrupt status register value
	 */
	volatile uint32_t mac_interrupt_sts;
};

/**
 * @brief Link speed configuration enumeration type.
 *
 * Enumeration type for link speed indication, contains 'link down'
 * plus all link speeds supported by the controller (10/100/1000).
 */
enum eth_dwc_xgmac_link_speed {
	/* The values of this enum are consecutively numbered */
	LINK_DOWN = 0,
	LINK_10MBIT = 10,
	LINK_100MBIT = 100,
	LINK_1GBIT = 1000
};

#endif /* _ZEPHYR_DRIVERS_ETHERNET_ETH_DWC_XGMAC_PRIV_H_ */
