#ifndef ETH_CYCLONEV_HEADER
#define ETH_CYCLONEV_HEADER
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2022, Intel Corporation
 * Description:
 * Driver for the Synopsys DesignWare
 * 3504-0 Universal 10/100/1000 Ethernet MAC (DWC_gmac)
 * specifically designed for Cyclone V SoC DevKit use only.
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define alt_replbits_word(dest, msk, src)                                                          \
	(sys_write32((sys_read32(dest) & ~(msk)) | ((src) & (msk)), dest))

#define NB_TX_DESCS CONFIG_ETH_CVSX_NB_TX_DESCS
#define NB_RX_DESCS CONFIG_ETH_CVSX_NB_RX_DESCS

#define ETH_BUFFER_SIZE 1536

/* Descriptor Structure */
struct eth_cyclonev_dma_desc {
	uint32_t status;		 /*!< Status */
	uint32_t control_buffer_size;	 /*!< Control and Buffer1, Buffer2 sizes */
	uint32_t buffer1_addr;		 /*!< Buffer1 address pointer */
	uint32_t buffer2_next_desc_addr; /*!< Buffer2 or next desc address pointer */
};

struct eth_cyclonev_priv {
	mem_addr_t base_addr; /* Base address */
	uint8_t mac_addr[6];
	uint32_t interrupt_mask;
	struct net_if *iface; /* Zephyr net_if Interface Struct (for interface initialisation) */
	uint32_t tx_current_desc_number;
	uint32_t rx_current_desc_number;
	uint32_t tx_tail;

	uint32_t feature; /* HW feature register */
	/* Tx/Rx Descriptor Rings */
	struct eth_cyclonev_dma_desc tx_desc_ring[NB_TX_DESCS], rx_desc_ring[NB_RX_DESCS];
	uint32_t rxints;			       /* Tx stats */
	uint32_t txints;			       /* Rx stats */
	uint8_t rx_buf[ETH_BUFFER_SIZE * NB_RX_DESCS]; /* Receive Buffer */
	uint8_t tx_buf[ETH_BUFFER_SIZE * NB_TX_DESCS]; /* Transmit Buffer */

	struct k_sem free_tx_descs;
	uint8_t running;     /* Running state flag */
	uint8_t initialised; /* Initialised state flag */
};

/*
 * Reset Manager Regs
 */
/* The base address of the Rstmgr register group. */
#define RSTMGR_BASE 0xffd05000

/* The byte offset of the ALT_RSTMGR_PERMODRST register from the beginning of
 * the component.
 */
#define RSTMGR_PERMODRST_OFST	       0x14
/* The address of the ALT_RSTMGR_PERMODRST register. */
#define RSTMGR_PERMODRST_ADDR	       0xFFD05014
/* The mask used to set the ALT_RSTMGR_PERMODRST_EMAC0 register field value. */
#define RSTMGR_PERMODRST_EMAC0_SET_MSK 0x00000001
/* The mask used to set the ALT_RSTMGR_PERMODRST_EMAC1 register field value. */
#define RSTMGR_PERMODRST_EMAC1_SET_MSK 0x00000002

/*
 * System Manager Regs
 */
#define SYSMGR_BASE		   0xffd08000
#define SYSMGR_EMAC_ADDR	   0xffd08060
#define SYSMGR_FPGAINTF_INDIV_ADDR 0xffd08004

/* The byte offset of the SYSMGR_EMAC register from the beginning of the
 * component.
 */
#define SYSMGR_EMAC_OFST		    0x60
/* The byte offset of the SYSMGR_FPGAINTF_INDIV register from the beginning of
 * the component.
 */
#define SYSMGR_FPGAINTF_INDIV_OFST	    0x4
/*
 * Enumerated value for register field ALT_SYSMGR_EMACn_PHY_INTF_SEL
 *
 */
#define SYSMGR_EMAC_PHY_INTF_SEL_E_GMII_MII 0x0
/*
 * Enumerated value for register field ALT_SYSMGR_EMACn_PHY_INTF_SEL
 *
 */
#define SYSMGR_EMAC0_PHY_INTF_SEL_E_RGMII   0x1
#define SYSMGR_EMAC1_PHY_INTF_SEL_E_RGMII   0x4
/*
 * Enumerated value for register field ALT_SYSMGR_EMACn_PHY_INTF_SEL
 *
 */
#define SYSMGR_EMAC_PHY_INTF_SEL_E_RMII	    0x2

/* The mask used to set the ALT_SYSMGR_EMACn_PHY_INTF_SEL register field value.
 */
#define SYSMGR_EMAC0_PHY_INTF_SEL_SET_MSK 0x00000003
#define SYSMGR_EMAC1_PHY_INTF_SEL_SET_MSK 0x0000000c

/* The mask used to set the ALT_SYSMGR_FPGAINTF_MODULE_EMAC_0 register field
 * value.
 */
#define SYSMGR_FPGAINTF_MODULE_EMAC_0_SET_MSK 0x00000004

/* The mask used to set the ALT_SYSMGR_FPGAINTF_MODULE_EMAC_1 register field
 * value.
 */
#define SYSMGR_FPGAINTF_MODULE_EMAC_1_SET_MSK 0x00000008

/*
 * Emac Registers
 */
/* Macros */
#define EMAC_BASE_ADDRESS DT_INST_REG_ADDR(0)

#define EMAC_DMAGRP_BUS_MODE_ADDR(base)	      (uint32_t)((base) + EMAC_DMA_MODE_OFST) /* Bus Mode */
#define EMAC_DMA_RX_DESC_LIST_ADDR(base)      (uint32_t)((base) + EMAC_DMA_RX_DESC_LIST_OFST)
/* Receive Descriptor Address List */
#define EMAC_DMA_TX_DESC_LIST_ADDR(base)      (uint32_t)((base) + EMAC_DMA_TX_DESC_LIST_OFST)
/* Transceive Descriptor Address List */
#define EMAC_DMAGRP_OPERATION_MODE_ADDR(base) (uint32_t)((base) + EMAC_DMAGRP_OPERATION_MODE_OFST)
/* Operation Mode */
#define EMAC_DMAGRP_STATUS_ADDR(base)    (uint32_t)((base) + EMAC_DMAGRP_STATUS_OFST) /* Status */
#define EMAC_DMAGRP_DEBUG_ADDR(base)     (uint32_t)((base) + EMAC_DMAGRP_DEBUG_OFST) /* Debug */
#define EMAC_DMA_INT_EN_ADDR(base)	      (uint32_t)((base) + EMAC_DMA_INT_EN_OFST)
/* Interrupt Enable */
#define EMAC_DMAGRP_AXI_BUS_MODE_ADDR(base)   (uint32_t)((base) + EMAC_DMAGRP_AXI_BUS_MODE_OFST)
/* AXI Bus Mode */
#define EMAC_DMAGRP_AHB_OR_AXI_STATUS_ADDR(base)                                                   \
	(uint32_t)((base) + EMAC_DMAGRP_AHB_OR_AXI_STATUS_OFST)
/* AHB or AXI Status */
#define GMACGRP_CONTROL_STATUS_ADDR(base)                                                          \
	(uint32_t)((base) +                                                                        \
		   EMAC_GMACGRP_SGMII_RGMII_SMII_CONTROL_STATUS_OFST)           \
			/* SGMII RGMII SMII Control Status */
#define EMAC_GMAC_INT_MSK_ADDR(base)  (uint32_t)((base) + EMAC_GMAC_INT_MSK_OFST)
/* Interrupt Mask */
#define EMAC_GMAC_INT_STAT_ADDR(base) (uint32_t)((base) + EMAC_GMAC_INT_STAT_OFST)
/* Interrupt Status */
#define GMACGRP_MAC_CONFIG_ADDR(base) (uint32_t)((base) + EMAC_GMACGRP_MAC_CONFIGURATION_OFST)
/* MAC Configuration */
#define EMAC_GMACGRP_MAC_FRAME_FILTER_ADDR(base)                                                   \
	(uint32_t)((base) + EMAC_GMACGRP_MAC_FRAME_FILTER_OFST)
/* MAC Frame Filter */
#define EMAC_GMAC_MAC_ADDR0_HIGH_ADDR(base)   (uint32_t)((base) + EMAC_GMAC_MAC_ADDR0_HIGH_OFST)
/* MAC Address 0 High */
#define EMAC_GMAC_MAC_ADDR0_LOW_ADDR(base)    (uint32_t)((base) + EMAC_GMAC_MAC_ADDR0_LOW_OFST)
/* MAC Address 0 Low */
#define EMAC_GMAC_MAC_ADDR_HIGH_ADDR(base, n) (uint32_t)((base) + EMAC_GMAC_MAC_ADDR_HIGH_OFST(n))
/* MAC Address 0 High */
#define EMAC_GMAC_MAC_ADDR_LOW_ADDR(base, n)  (uint32_t)((base) + EMAC_GMAC_MAC_ADDR_LOW_OFST(n))
/* MAC Address 0 High */
#define EMAC_GMAC_GMII_ADDR_ADDR(base)	      (uint32_t)((base) + EMAC_GMAC_GMII_ADDR_OFST)
/* GMII Address */
#define EMAC_GMAC_GMII_DATA_ADDR(base)	      (uint32_t)((base) + EMAC_GMAC_GMII_DATA_OFST)
/* GMII Data */
#define EMAC_DMA_TX_POLL_DEMAND_ADDR(base)    (uint32_t)((base) + EMAC_DMA_TX_POLL_DEMAND_OFST)
/* Transmit Poll Demand */
#define EMAC_DMA_RX_POLL_DEMAND_ADDR(base)    (uint32_t)((base) + EMAC_DMA_RX_POLL_DEMAND_OFST)
/* Receive Poll Demand */
#define EMAC_DMA_CURR_HOST_TX_DESC_ADDR(base) (uint32_t)((base) + EMAC_DMA_CURR_HOST_TX_DESC_OFST)
/* Current Host Transmit Descriptor */
#define EMAC_DMA_CURR_HOST_RX_DESC_ADDR(base) (uint32_t)((base) + EMAC_DMA_CURR_HOST_RX_DESC_OFST)
/* Current Host Receive Descriptor */
#define EMAC_DMA_CURR_HOST_TX_BUFF_ADDR(base) (uint32_t)((base) + EMAC_DMA_CURR_HOST_TX_BUFF_OFST)
/* Current Host Transmit Buffer Address */
#define EMAC_DMA_CURR_HOST_RX_BUFF_ADDR(base) (uint32_t)((base) + EMAC_DMA_CURR_HOST_RX_BUFF_OFST)
/* Current Host Receive Buffer Address */
#define EMAC_DMA_HW_FEATURE_ADDR(base)	      (uint32_t)((base) + EMAC_DMA_HW_FEATURE_OFST)
/* HW Feature */

/* Bus Mode */
#define EMAC_DMA_MODE_OFST		   0x1000
#define EMAC_DMA_MODE_SWR_SET_MSK	   0x00000001
#define EMAC_DMA_MODE_SWR_GET(value)	   (((value)&0x00000001) >> 0)
#define EMAC_DMA_MODE_FB_SET_MSK	   0x00010000
#define EMAC_DMA_MODE_RPBL_SET(value)	   (((value) << 17) & 0x007e0000)
#define EMAC_DMA_MODE_PBL_SET(value)	   (((value) << 8) & 0x00003f00)
#define EMAC_DMA_MODE_EIGHTXPBL_SET(value) (((value) << 24) & 0x01000000)
#define EMAC_DMA_MODE_AAL_SET_MSK	   0x02000000
#define EMAC_DMA_MODE_USP_SET_MSK	   0x00800000

/* Receive Descriptor Address List */
#define EMAC_DMA_RX_DESC_LIST_OFST 0x100c

/* Transceive Descriptor Address List */
#define EMAC_DMA_TX_DESC_LIST_OFST 0x1010

/* Operation Mode */
#define EMAC_DMAGRP_OPERATION_MODE_OFST	       0x1018
#define EMAC_DMAGRP_OPERATION_MODE_OSF_SET_MSK 0x00000004 /* Operate on Second Frame */
#define EMAC_DMAGRP_OPERATION_MODE_TSF_SET_MSK 0x00200000 /* Transmit Store and Forward */
#define EMAC_DMAGRP_OPERATION_MODE_RSF_SET_MSK 0x02000000 /* Receive Store and Forward */
#define EMAC_DMAGRP_OPERATION_MODE_FTF_SET_MSK 0x00100000 /* Receive Store and Forward */
#define EMAC_DMAGRP_OPERATION_MODE_ST_SET_MSK  0x00002000
#define EMAC_DMAGRP_OPERATION_MODE_SR_SET_MSK  0x00000002
#define EMAC_DMAGRP_OPERATION_MODE_DT_SET_MSK  0x04000000 /* Ignore frame errors */

/* Interrupt Enable */
#define EMAC_DMA_INT_EN_OFST	    0x101C
#define EMAC_DMA_INT_EN_NIE_SET_MSK 0x00010000
#define EMAC_DMA_INT_EN_AIE_SET_MSK 0x00008000
#define EMAC_DMA_INT_EN_ERE_SET_MSK 0x00004000
#define EMAC_DMA_INT_EN_FBE_SET_MSK 0x00002000
#define EMAC_DMA_INT_EN_ETE_SET_MSK 0x00000400
#define EMAC_DMA_INT_EN_RWE_SET_MSK 0x00000200
#define EMAC_DMA_INT_EN_RSE_SET_MSK 0x00000100
#define EMAC_DMA_INT_EN_RUE_SET_MSK 0x00000080
#define EMAC_DMA_INT_EN_RIE_SET_MSK 0x00000040
#define EMAC_DMA_INT_EN_UNE_SET_MSK 0x00000020
#define EMAC_DMA_INT_EN_OVE_SET_MSK 0x00000010
#define EMAC_DMA_INT_EN_TJE_SET_MSK 0x00000008
#define EMAC_DMA_INT_EN_TUE_SET_MSK 0x00000004
#define EMAC_DMA_INT_EN_TSE_SET_MSK 0x00000002
#define EMAC_DMA_INT_EN_TIE_SET_MSK 0x00000001

/* Status */
#define EMAC_DMAGRP_STATUS_OFST	       0x1014
#define EMAC_DMAGRP_STATUS_TS_SET_MSK  0x00700000
#define EMAC_DMAGRP_STATUS_TS_E_SUSPTX 0x00600000
#define EMAC_DMAGRP_STATUS_RS_SET_MSK  0x000e0000
#define EMAC_DMAGRP_STATUS_RS_E_SUSPRX 0x00080000

#define EMAC_DMAGRP_DEBUG_OFST		    0x24
#define EMAC_DMAGRP_DEBUG_TWCSTS	    0x00400000
#define EMAC_DMAGRP_DEBUG_RWCSTS	    0x00000010
#define EMAC_DMAGRP_DEBUG_RXFSTS_GET(value) (((value)&0x00000300) >> 8)

/* AXI Bus Mode */
#define EMAC_DMAGRP_AXI_BUS_MODE_OFST		0x1028
#define EMAC_DMAGRP_AXI_BUS_MODE_BLEN16_SET_MSK 0x00000008

/* AHB or AXI Status */
#define EMAC_DMAGRP_AHB_OR_AXI_STATUS_OFST 0x102c

/* MAC Configuration */

#define EMAC_GMACGRP_MAC_CONFIGURATION_OFST	   0x0000
#define EMAC_GMACGRP_MAC_CONFIGURATION_IPC_SET_MSK 0x00000400
#define EMAC_GMACGRP_MAC_CONFIGURATION_JD_SET_MSK  0x00400000 /* Jabber Disable */
#define EMAC_GMACGRP_MAC_CONFIGURATION_PS_SET_MSK  0x00008000 /* Port Select = MII */
#define EMAC_GMACGRP_MAC_CONFIGURATION_BE_SET_MSK  0x00200000 /* Frame Burst Enable */
#define EMAC_GMACGRP_MAC_CONFIGURATION_WD_SET_MSK  0x00800000 /* Watchdog Disable */
#define EMAC_GMACGRP_MAC_CONFIGURATION_DO_SET_MSK  0x00002000
#define EMAC_GMACGRP_MAC_CONFIGURATION_TE_SET_MSK  0x00000008
#define EMAC_GMACGRP_MAC_CONFIGURATION_RE_SET_MSK  0x00000004
#define EMAC_GMACGRP_MAC_CONFIGURATION_TC_SET_MSK  0x01000000

#define EMAC_GMACGRP_MAC_CONFIGURATION_DM_SET_MSK  0x00000800
#define EMAC_GMACGRP_MAC_CONFIGURATION_FES_SET_MSK 0x00004000

/* SGMII RGMII SMII Control Status */
#define EMAC_GMACGRP_SGMII_RGMII_SMII_CONTROL_STATUS_OFST 0x00d8
#define EMAC_GMAC_MII_CTL_STAT_LNKSTS_GET(value)	  (((value)&0x00000008) >> 3)
#define EMAC_GMAC_MII_CTL_STAT_LNKSPEED_GET(value)	  (((value)&0x00000007) >> 1)
#define EMAC_GMAC_MII_CTL_STAT_LNKMOD_GET(value)	  ((value)&0x00000001)

/* Interrupt Mask */
#define EMAC_GMAC_INT_MSK_OFST		    0x003c
#define EMAC_GMAC_INT_STAT_LPIIS_SET_MSK    0x00000400
#define EMAC_GMAC_INT_STAT_TSIS_SET_MSK	    0x00000200
#define EMAC_GMAC_INT_STAT_RGSMIIIS_SET_MSK 0x00000001

/* Interrupt Status (Gmac)*/
#define EMAC_GMAC_INT_STAT_OFST 0x0038

/* MAC Frame Filter */
#define EMAC_GMACGRP_MAC_FRAME_FILTER_OFST	 0x0004
#define EMAC_GMACGRP_MAC_FRAME_FILTER_PR_SET_MSK 0x00000001

/* MAC Address 0 High */
#define EMAC_GMAC_MAC_ADDR0_HIGH_OFST	0x40
#define EMAC_GMAC_MAC_ADDR_HIGH_OFST(n) (0x40 + 8 * (n))
/* MAC Address 0 Low */
#define EMAC_GMAC_MAC_ADDR0_LOW_OFST	0x44
#define EMAC_GMAC_MAC_ADDR_LOW_OFST(n)	(0x44 + 8 * (n))

/* GMII Address */
#define EMAC_GMAC_GMII_ADDR_OFST	  0x10
#define EMAC_GMAC_GMII_ADDR_PA_SET(value) (((value) << 11) & 0x0000f800)
#define EMAC_GMAC_GMII_ADDR_GR_SET(value) (((value) << 6) & 0x000007c0)
#define EMAC_GMAC_GMII_ADDR_GW_SET_MSK	  0x00000002
#define EMAC_GMAC_GMII_ADDR_GW_CLR_MSK	  0xfffffffd
#define EMAC_GMAC_GMII_ADDR_CR_SET(value) (((value) << 2) & 0x0000003c)
#define EMAC_GMAC_GMII_ADDR_GB_SET(value) (((value) << 0) & 0x00000001)
#define EMAC_GMAC_GMII_ADDR_CR_E_DIV102	  0x4
#define EMAC_GMAC_GMII_ADDR_GB_SET_MSK	  0x00000001

/* GMII Data */
#define EMAC_GMAC_GMII_DATA_OFST 0x14

/* Transmit Poll Demand */
#define EMAC_DMA_TX_POLL_DEMAND_OFST 0x1004

/* Receive Poll Demand */
#define EMAC_DMA_RX_POLL_DEMAND_OFST 0x1008

/* Current Host Transmit Descriptor */
#define EMAC_DMA_CURR_HOST_TX_DESC_OFST 0x1048

/* Current Host Receive Descriptor */
#define EMAC_DMA_CURR_HOST_RX_DESC_OFST 0x104C

/* Current Host Transmit Buffer Address */
#define EMAC_DMA_CURR_HOST_TX_BUFF_OFST 0x1050

/* Current Host Receive Buffer Address */
#define EMAC_DMA_CURR_HOST_RX_BUFF_OFST 0x1054

/* HW Feature */
#define EMAC_DMA_HW_FEATURE_OFST      0x1058
#define EMAC_DMA_HW_FEATURE_MIISEL    0x00000001 /* 10/100 Mbps support */
#define EMAC_DMA_HW_FEATURE_GMIISEL   0x00000002 /* 1000 Mbps support */
#define EMAC_DMA_HW_FEATURE_HDSEL     0x00000004 /* Half-Duplex support */
#define EMAC_DMA_HW_FEATURE_RXTYP2COE 0x00040000 /* IP Checksum Offload (Type 2) in Rx */
#define EMAC_DMA_HW_FEATURE_RXTYP1COE 0x00020000 /* IP Checksum Offload (Type 1) in Rx */
#define EMAC_DMA_HW_FEATURE_TXOESEL   0x00010000 /* Checksum Offload in Tx */

/*
 * DMA Descriptor Flag Definitions
 */

/*
 * DMA Rx Descriptor
 * -------------------------------------------------------------------------------------------
 * RDES0 | OWN(31) |		Status [30:0]		  |
 * -------------------------------------------------------------------------------------------
 * RDES1 |CTRL(31)|Reserv[30:29]|Buff2ByteCt[28:16]|CTRL[15:14]
 * Reservr(13)|Buff1ByteCt[12:0]|
 * -------------------------------------------------------------------------------------------
 * RDES2 |		   Buffer1 Address [31:0]		 |
 * --------------------------------------------------------------------------------------------
 * RDES3 |	  Buffer2 Address [31:0] / Next Descriptor Address [31:0]
 * |
 * --------------------------------------------------------------------------------------------
 */

/*	Bit definition of RDES0 register: DMA Rx descriptor status register  */
#define ETH_DMARXDESC_OWN     ((uint32_t)0x80000000)
/*!< OWN bit: descriptor is owned by DMA engine  */
#define ETH_DMARXDESC_AFM     ((uint32_t)0x40000000)
/*!< DA Filter Fail for the rx frame  */
#define ETH_DMARXDESC_FL      ((uint32_t)0x3FFF0000)
/*!< Receive descriptor frame length  */
#define ETH_DMARXDESC_ES      ((uint32_t)0x00008000)
/*!< Error summary: OR of the following bits:
 * DE || OE || IPC || LC || RWT || RE || CE
 */
#define ETH_DMARXDESC_DE      ((uint32_t)0x00004000)
/*!< Descriptor error: no more descriptors for receive frame  */
#define ETH_DMARXDESC_SAF     ((uint32_t)0x00002000)
/*!< SA Filter Fail for the received frame */
#define ETH_DMARXDESC_LE      ((uint32_t)0x00001000)
/*!< Frame size not matching with length field */
#define ETH_DMARXDESC_OE      ((uint32_t)0x00000800)
/*!< Overflow Error: Frame was damaged due to buffer overflow */
#define ETH_DMARXDESC_VLAN    ((uint32_t)0x00000400)
/*!< VLAN Tag: received frame is a VLAN frame */
#define ETH_DMARXDESC_FS      ((uint32_t)0x00000200)
/*!< First descriptor of the frame  */
#define ETH_DMARXDESC_LS      ((uint32_t)0x00000100)
/*!< Last descriptor of the frame  */
#define ETH_DMARXDESC_IPV4HCE ((uint32_t)0x00000080)
/*!< IPC Checksum Error: Rx Ipv4 header checksum error   */
#define ETH_DMARXDESC_LC      ((uint32_t)0x00000040)
/*!< Late collision occurred during reception   */
#define ETH_DMARXDESC_FT      ((uint32_t)0x00000020)
/*!< Frame type - Ethernet, otherwise 802.3	*/
#define ETH_DMARXDESC_RWT     ((uint32_t)0x00000010)
/*!< Receive Watchdog Timeout: watchdog timer expired during reception	*/
#define ETH_DMARXDESC_RE      ((uint32_t)0x00000008)
/*!< Receive error: error reported by MII interface  */
#define ETH_DMARXDESC_DBE     ((uint32_t)0x00000004)
/*!< Dribble bit error: frame contains non int multiple of 8 bits  */
#define ETH_DMARXDESC_CE      ((uint32_t)0x00000002)
/*!< CRC error */
#define ETH_DMARXDESC_MAMPCE  ((uint32_t)0x00000001)
/* !< Rx MAC Address/Payload Checksum Error: Rx MAC address matched/
 * Rx Payload Checksum Error
 */

/*	Bit definition of RDES1 register		  */
#define ETH_DMARXDESC_DIC  ((uint32_t)0x80000000) /*!< Disable Interrupt on Completion */
#define ETH_DMARXDESC_RBS2 ((uint32_t)0x1FFF0000) /*!< Receive Buffer2 Size */
#define ETH_DMARXDESC_RER  ((uint32_t)0x00008000) /*!< Receive End of Ring */
#define ETH_DMARXDESC_RCH                                                                          \
	((uint32_t)0x00004000)			  /*!< Second Address Chained                      \
						   */
#define ETH_DMARXDESC_RBS1 ((uint32_t)0x00001FFF) /*!< Receive Buffer1 Size */

/*
 *DMA Tx Descriptor
 *-----------------------------------------------------------------------------------------------
 *TDES0 | OWN(31) | CTRL[30:26] | Reserved[25:24] | CTRL[23:20] |
 *Reserved[19:17] | Status[16:0] |
 *-----------------------------------------------------------------------------------------------
 *TDES1 | Reserved[31:29] | Buffer2 ByteCount[28:16] | Reserved[15:13] | Buffer1
 *ByteCount[12:0] |
 *-----------------------------------------------------------------------------------------------
 *TDES2 |			 Buffer1 Address [31:0]
 *|
 *-----------------------------------------------------------------------------------------------
 *TDES3 |		   Buffer2 Address [31:0] / Next Descriptor Address [31:0]
 *|
 *-----------------------------------------------------------------------------------------------
 */

/* Bit definition of TDES0 register: DMA Tx descriptor status register */
#define ETH_DMATXDESC_OWN		     ((uint32_t)0x80000000)
/*!< OWN bit: descriptor is owned by DMA engine */
#define ETH_DMATXDESC_IC		     ((uint32_t)0x40000000)
/*!< Interrupt on Completion */
#define ETH_DMATXDESC_LS		     ((uint32_t)0x20000000)
/*!< Last Segment */
#define ETH_DMATXDESC_FS		     ((uint32_t)0x10000000)
/*!< First Segment */
#define ETH_DMATXDESC_DC		     ((uint32_t)0x08000000)
/*!< Disable CRC */
#define ETH_DMATXDESC_DP		     ((uint32_t)0x04000000)
/*!< Disable Padding */
#define ETH_DMATXDESC_TTSE		     ((uint32_t)0x02000000)
/*!< Transmit Time Stamp Enable */
#define ETH_DMATXDESC_CIC		     ((uint32_t)0x00C00000)
/*!< Checksum Insertion Control: 4 cases */
#define ETH_DMATXDESC_CIC_BYPASS	     ((uint32_t)0x00000000)
/*!< Do Nothing: Checksum Engine is bypassed */
#define ETH_DMATXDESC_CIC_IPV4HEADER	     ((uint32_t)0x00400000)
/*!< IPV4 header Checksum Insertion */
#define ETH_DMATXDESC_CIC_TCPUDPICMP_SEGMENT ((uint32_t)0x00800000)
/*!< TCP/UDP/ICMP Checksum Insertion calculated over segment only */
#define ETH_DMATXDESC_CIC_TCPUDPICMP_FULL    ((uint32_t)0x00C00000)
/*!< TCP/UDP/ICMP Checksum Insertion fully calculated */
#define ETH_DMATXDESC_TER		     ((uint32_t)0x00200000)
/*!< Transmit End of Ring */
#define ETH_DMATXDESC_TCH		     ((uint32_t)0x00100000)
/*!< Second Address Chained */
#define ETH_DMATXDESC_TTSS		     ((uint32_t)0x00020000)
/*!< Tx Time Stamp Status */
#define ETH_DMATXDESC_IHE		     ((uint32_t)0x00010000)
/*!< IP Header Error */
#define ETH_DMATXDESC_ES		     ((uint32_t)0x00008000)
/*!< Error summary: OR of the following bits: UE||ED||EC||LCO||NC||LCA||FF||JT
 */
#define ETH_DMATXDESC_JT		     ((uint32_t)0x00004000)
/*!< Jabber Timeout */
#define ETH_DMATXDESC_FF		     ((uint32_t)0x00002000)
/*!< Frame Flushed: DMA/MTL flushed the frame due to SW flush */
#define ETH_DMATXDESC_PCE		     ((uint32_t)0x00001000)
/*!< Payload Checksum Error */
#define ETH_DMATXDESC_LCA		     ((uint32_t)0x00000800)
/*!< Loss of Carrier: carrier lost during transmission */
#define ETH_DMATXDESC_NC		     ((uint32_t)0x00000400)
/*!< No Carrier: no carrier signal from the transceiver */
#define ETH_DMATXDESC_LCO		     ((uint32_t)0x00000200)
/*!< Late Collision: transmission aborted due to collision */
#define ETH_DMATXDESC_EC		     ((uint32_t)0x00000100)
/*!< Excessive Collision: transmission aborted after 16 collisions */
#define ETH_DMATXDESC_VF		     ((uint32_t)0x00000080)
/*!< VLAN Frame */
#define ETH_DMATXDESC_CC		     ((uint32_t)0x00000078)
/*!< Collision Count */
#define ETH_DMATXDESC_ED		     ((uint32_t)0x00000004)
/*!< Excessive Deferral */
#define ETH_DMATXDESC_UF		     ((uint32_t)0x00000002)
/*!< Underflow Error: late data arrival from the memory */
#define ETH_DMATXDESC_DB		     ((uint32_t)0x00000001)
/*!< Deferred Bit */

/*			Bit definition of TDES1 register		  */
#define ETH_DMATXDESC_TBS2                                                                         \
	((uint32_t)0x1FFF0000) /*!< Transmit Buffer2 Size                                          \
				*/
#define ETH_DMATXDESC_TBS1                                                                         \
	((uint32_t)0x00001FFF) /*!< Transmit Buffer1 Size                                          \
				*/

/*			Bit definition of TDES2 register		  */
#define ETH_DMATXDESC_B1AP ((uint32_t)0xFFFFFFFF) /*!< Buffer1 Address Pointer */

/*			Bit definition of TDES3 register		  */
#define ETH_DMATXDESC_B2AP ((uint32_t)0xFFFFFFFF) /*!< Buffer2 Address Pointer */


static const uint32_t Rstmgr_Permodrst_Emac_Set_Msk[] = {RSTMGR_PERMODRST_EMAC0_SET_MSK,
							 RSTMGR_PERMODRST_EMAC1_SET_MSK};

static const uint32_t Sysmgr_Core_Emac_Phy_Intf_Sel_Set_Msk[] = {SYSMGR_EMAC0_PHY_INTF_SEL_SET_MSK,
								 SYSMGR_EMAC1_PHY_INTF_SEL_SET_MSK};

static const uint32_t Sysmgr_Fpgaintf_En_3_Emac_Set_Msk[] = {SYSMGR_FPGAINTF_MODULE_EMAC_0_SET_MSK,
							     SYSMGR_FPGAINTF_MODULE_EMAC_1_SET_MSK};

static const uint32_t Sysmgr_Emac_Phy_Intf_Sel_E_Rgmii[] = {SYSMGR_EMAC0_PHY_INTF_SEL_E_RGMII,
							    SYSMGR_EMAC1_PHY_INTF_SEL_E_RGMII};

#endif
