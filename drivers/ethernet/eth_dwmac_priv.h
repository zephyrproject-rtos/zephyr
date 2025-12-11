/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Definitions in this file are based on:
 *
 *   DesignWare Cores Ethernet Quality-of-Service Databook
 *   Version 5.10a, December 2017
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_DWMAC_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_DWMAC_PRIV_H_

/*
 * Global driver parameters
 */

/* number of hardware descriptors in uncached memory */
#define NB_TX_DESCS		CONFIG_DWMAC_NB_TX_DESCS
#define NB_RX_DESCS		CONFIG_DWMAC_NB_RX_DESCS

/* stack size for RX refill thread */
#define RX_REFILL_STACK_SIZE	1024

/*
 * Common structure definitions
 */

/* hardware descriptor representation */
struct dwmac_dma_desc {
	uint32_t des0;
	uint32_t des1;
	uint32_t des2;
	uint32_t des3;
};

/* our private instance structure */
struct dwmac_priv {
	mem_addr_t base_addr;
	struct net_if *iface;
	const struct device *clock;

	uint8_t mac_addr[6];

	uint32_t feature0;
	uint32_t feature1;
	uint32_t feature2;
	uint32_t feature3;

	struct dwmac_dma_desc *tx_descs, *rx_descs;
	struct k_sem free_tx_descs, free_rx_descs;
	unsigned int tx_desc_head, tx_desc_tail;
	unsigned int rx_desc_head, rx_desc_tail;

#ifdef CONFIG_MMU
	uintptr_t tx_descs_phys, rx_descs_phys;
#endif

	struct net_buf *tx_frags[NB_TX_DESCS]; /* index shared with tx_descs */
	struct net_buf *rx_frags[NB_RX_DESCS]; /* index shared with rx_descs */

	struct net_pkt *rx_pkt;
	unsigned int rx_bytes;

	K_KERNEL_STACK_MEMBER(rx_refill_thread_stack, RX_REFILL_STACK_SIZE);
	struct k_thread rx_refill_thread;
};

/*
 * Handy register accessors
 */

#define REG_READ(r) sys_read32(p->base_addr + (r))
#define REG_WRITE(r, v) sys_write32((v), p->base_addr + (r))

/*
 * Shared declarations between core and platform glue code
 */

int dwmac_probe(const struct device *dev);
int dwmac_bus_init(struct dwmac_priv *p);
void dwmac_platform_init(struct dwmac_priv *p);
void dwmac_isr(const struct device *ddev);
extern const struct ethernet_api dwmac_api;

/*
 * MAC Register Definitions
 */

/* 17.1.1 */

#define MAC_CONF				0x0000

#define MAC_CONF_ARPEN					BIT(31)
#define MAC_CONF_SARC				GENMASK(30, 28)
#define MAC_CONF_IPC				BIT(27)
#define MAC_CONF_IPG				GENMASK(26, 24)
#define MAC_CONF_GPSLCE				BIT(23)
#define MAC_CONF_S2KP				BIT(22)
#define MAC_CONF_CST				BIT(21)
#define MAC_CONF_ACS				BIT(20)
#define MAC_CONF_WD				BIT(19)
#define MAC_CONF_BE				BIT(18)
#define MAC_CONF_JD				BIT(17)
#define MAC_CONF_JE				BIT(16)
#define MAC_CONF_PS				BIT(15)
#define MAC_CONF_FES				BIT(14)
#define MAC_CONF_DM				BIT(13)
#define MAC_CONF_LM				BIT(12)
#define MAC_CONF_ECRSFD				BIT(11)
#define MAC_CONF_DO				BIT(10)
#define MAC_CONF_DCRS				BIT(9)
#define MAC_CONF_DR				BIT(8)
#define MAC_CONF_BL				GENMASK(6, 5)
#define MAC_CONF_DC				BIT(4)
#define MAC_CONF_PRELEN				GENMASK(3, 2)
#define MAC_CONF_TE				BIT(1)
#define MAC_CONF_RE				BIT(0)

/* 17.1.2 */

#define MAC_EXT_CONF				0x0004

#define MAC_EXT_CONF_FHE			BIT(31)
#define MAC_EXT_CONF_EIPG			GENMASK(29, 25)
#define MAC_EXT_CONF_EIPGEN			BIT(24)
#define MAC_EXT_CONF_HDSMS			GENMASK(22, 20)
#define MAC_EXT_CONF_PDC			BIT(19)
#define MAC_EXT_CONF_USP			BIT(18)
#define MAC_EXT_CONF_SPEN			BIT(17)
#define MAC_EXT_CONF_DCRCC			BIT(16)
#define MAC_EXT_CONF_GPSL			GENMASK(13, 0)

/* 17.1.3 */

#define MAC_PKT_FILTER				0x0008

#define MAC_PKT_FILTER_RA			BIT(31)
#define MAC_PKT_FILTER_DNTU			BIT(21)
#define MAC_PKT_FILTER_IPFE			BIT(20)
#define MAC_PKT_FILTER_VTFE			BIT(16)
#define MAC_PKT_FILTER_HPF			it(10)
#define MAC_PKT_FILTER_SAF			BIT(9)
#define MAC_PKT_FILTER_SAIF			BIT(8)
#define MAC_PKT_FILTER_PCF			GENMASK(7, 6)
#define MAC_PKT_FILTER_DBF			BIT(5)
#define MAC_PKT_FILTER_PM			BIT(4)
#define MAC_PKT_FILTER_DAIF			BIT(3)
#define MAC_PKT_FILTER_HMC			BIT(2)
#define MAC_PKT_FILTER_HUC			BIT(1)
#define MAC_PKT_FILTER_PR			BIT(0)

/* 17.1.4 */

#define MAC_WDOG_TIMEOUT			0x000c

#define MAC_WDOG_TIMEOUT_PWE			BIT(8)
#define MAC_WDOG_TIMEOUT_WTO			GENMASK(3, 0)

/* 17.1.5 ... 17.1.12 */

#define MAC_HASH_TABLE(n)			(0x0010 + 4 * (n))

/* 17.1.13 */

#define MAC_VLAN_TAG				0x0050

/* 17.1.14 */

#define MAC_VLAN_TAG_CTRL			0x0050

#define MAC_VLAN_TAG_CTRL_EIVLRXS		BIT(31)
#define MAC_VLAN_TAG_CTRL_EIVLS			GENMASK(29, 28)
#define MAC_VLAN_TAG_CTRL_ERIVLT		BIT(27)
#define MAC_VLAN_TAG_CTRL_EDVLP			BIT(26)
#define MAC_VLAN_TAG_CTRL_VTHM			BIT(25)
#define MAC_VLAN_TAG_CTRL_EVLRXS		BIT(24)
#define MAC_VLAN_TAG_CTRL_EVLS			GENMASK(22, 21)
#define MAC_VLAN_TAG_CTRL_DOVLTC		BIT(20)
#define MAC_VLAN_TAG_CTRL_ERSVLM		BIT(19)
#define MAC_VLAN_TAG_CTRL_ESVL			BIT(18)
#define MAC_VLAN_TAG_CTRL_VTIM			BIT(17)
#define MAC_VLAN_TAG_CTRL_ETV			BIT(16)
#define MAC_VLAN_TAG_CTRL_VL			GENMASK(15, 0)
#define MAC_VLAN_TAG_CTRL_OFS			GENMASK(6, 2)
#define MAC_VLAN_TAG_CTRL_CT			BIT(1)
#define MAC_VLAN_TAG_CTRL_OB			BIT(0)

/* 17.1.15 */

#define MAC_VLAN_TAG_DATA			0x0054

/* 17.1.17 */

#define MAC_VLAN_HASH_TBL			0x0058

/* 17.1.19 */

#define MAC_VLAN_INCL				0x0060

/* 17.1.20 */

#define MAC_INNER_VLAN_INCL			0x0064

/* 17.1.21 */

#define MAC_Qn_TX_FLOW_CTRL(n)			(0x0070 + 4 * (n))

#define MAC_Qn_TX_FLOW_CTRL_PT			GENMASK(31, 16)
#define MAC_Qn_TX_FLOW_CTRL_DZPQ		BIT(7)
#define MAC_Qn_TX_FLOW_CTRL_PLT			GENMASK(6, 4)
#define MAC_Qn_TX_FLOW_CTRL_TFE			BIT(1)
#define MAC_Qn_TX_FLOW_CTRL_FCB_BPA		BIT(0)

/* 17.1.23 */

#define MAC_RX_FLOW_CTRL			0x0090

#define MAC_RX_FLOW_CTRL_PFCE			BIT(8)
#define MAC_RX_FLOW_CTRL_UP			BIT(1)
#define MAC_RX_FLOW_CTRL_RFE			BIT(0)

/* 17.1.24 */

#define MAC_RXQ_CTRL4				0x0094

/* 17.1.5 */

#define MAC_TXQ_PRTY_MAP0			0x0098

/* 17.1.26 */

#define MAC_TXQ_PRTY_MAP1			0x009c

/* 17.1.27 */

#define MAC_RXQ_CTRL0				0x00a0

/* 17.1.28 */

#define MAC_RXQ_CTRL1				0x00a4

/* 17.1.29 */

#define MAC_RXQ_CTRL2				0x00a8

/* 17.1.30 */

#define MAC_RXQ_CTRL3				0x00ac

/* 17.1.31 */

#define MAC_IRQ_STATUS				0x00b0

#define MAC_IRQ_STATUS_MFRIS			BIT(20)
#define MAC_IRQ_STATUS_MFTIS			BIT(19)
#define MAC_IRQ_STATUS_MDIOIS			BIT(18)
#define MAC_IRQ_STATUS_FPEIS			BIT(17)
#define MAC_IRQ_STATUS_GPIIS			BIT(15)
#define MAC_IRQ_STATUS_RXSTSIS			BIT(14)
#define MAC_IRQ_STATUS_TXSTSIS			BIT(13)
#define MAC_IRQ_STATUS_TSIS			BIT(12)
#define MAC_IRQ_STATUS_MMCRXIPIS		BIT(11)
#define MAC_IRQ_STATUS_MMCTXIS			BIT(10)
#define MAC_IRQ_STATUS_MMCRXIS			BIT(9)
#define MAC_IRQ_STATUS_MMCIS			BIT(8)
#define MAC_IRQ_STATUS_LPIIS			BIT(5)
#define MAC_IRQ_STATUS_PMTIS			BIT(4)
#define MAC_IRQ_STATUS_PHYIS			BIT(3)
#define MAC_IRQ_STATUS_PCSANCIS			BIT(2)
#define MAC_IRQ_STATUS_PCSLCHGIS		BIT(1)
#define MAC_IRQ_STATUS_RGSMIIIS			BIT(0)

/* 17.1.32 */

#define MAC_IRQ_ENABLE				0x00b4

#define MAC_IRQ_ENABLE_MDIOIE			BIT(18)
#define MAC_IRQ_ENABLE_FPEIE			BIT(17)
#define MAC_IRQ_ENABLE_RXSTSIE			BIT(14)
#define MAC_IRQ_ENABLE_TXSTSIE			BIT(13)
#define MAC_IRQ_ENABLE_TSIE			BIT(12)
#define MAC_IRQ_ENABLE_LPIIE			BIT(5)
#define MAC_IRQ_ENABLE_PMTIE			BIT(4)
#define MAC_IRQ_ENABLE_PHYIE			BIT(3)
#define MAC_IRQ_ENABLE_PCSANCIE			BIT(2)
#define MAC_IRQ_ENABLE_PCSLCHGIE		BIT(1)
#define MAC_IRQ_ENABLE_RGSMIIIE			BIT(0)

/* 17.1.33 */

#define MAC_RX_TX_STATUS			0x00b8

#define MAC_RX_TX_STATUS_WT			BIT(8)
#define MAC_RX_TX_STATUS_EXCOL			BIT(5)
#define MAC_RX_TX_STATUS_LCOL			BIT(4)
#define MAC_RX_TX_STATUS_EXDEF			BIT(3)
#define MAC_RX_TX_STATUS_LCARR			BIT(2)
#define MAC_RX_TX_STATUS_NCARR			BIT(1)
#define MAC_RX_TX_STATUS_TJT			BIT(0)

/* 17.1.34 */

#define MAC_PMT_CTRL_STATUS			0x00c0

#define MAC_PMT_CTRL_STATUS_RWKFILTRST		BIT(31)
#define MAC_PMT_CTRL_STATUS_RWKPTR		GENMASK(28, 24)
#define MAC_PMT_CTRL_STATUS_RWKPFE		BIT(10)
#define MAC_PMT_CTRL_STATUS_GLBLUCAST		BIT(9)
#define MAC_PMT_CTRL_STATUS_RWKPRCVD		BIT(6)
#define MAC_PMT_CTRL_STATUS_MGKPRCVD		BIT(5)
#define MAC_PMT_CTRL_STATUS_RWKPKTEN		BIT(2)
#define MAC_PMT_CTRL_STATUS_MGKPKTEN		BIT(1)
#define MAC_PMT_CTRL_STATUS_PWRDWN		BIT(0)

/* 17.1.35 */

#define MAC_RWK_PKT_FILTER			0x00c4

/* 17.1.40 */

#define MAC_LPI_CTRL_STATUS			0x00d0

#define MAC_LPI_CTRL_STATUS_LPITCSE		BIT(21)
#define MAC_LPI_CTRL_STATUS_LPIATE		BIT(20)
#define MAC_LPI_CTRL_STATUS_LPITXA		BIT(19)
#define MAC_LPI_CTRL_STATUS_PLSEN		BIT(18)
#define MAC_LPI_CTRL_STATUS_PLS			BIT(17)
#define MAC_LPI_CTRL_STATUS_LPIEN		BIT(16)
#define MAC_LPI_CTRL_STATUS_RLPIST		BIT(9)
#define MAC_LPI_CTRL_STATUS_TLPIST		BIT(8)
#define MAC_LPI_CTRL_STATUS_RLPIEX		BIT(3)
#define MAC_LPI_CTRL_STATUS_RLPIEN		BIT(2)
#define MAC_LPI_CTRL_STATUS_TLPIEX		BIT(1)
#define MAC_LPI_CTRL_STATUS_TLPIEN		BIT(0)

/* 17.1.41 */

#define MAC_LPI_TIMERS_CTRL			0x00d4

/* 17.1.42 */

#define MAC_LPI_ENTRY_TIMER			0x00d8

/* 17.1.43 */

#define MAC_1US_TIC_COUNTERR			0x00dc

/* 17.1.44 */

#define MAC_AN_CTRL				0x00e0

#define MAC_AN_CTRL_SGMRAL			BIT(18)
#define MAC_AN_CTRL_LR				BIT(17)
#define MAC_AN_CTRL_ECD				BIT(16)
#define MAC_AN_CTRL_ELE				BIT(14)
#define MAC_AN_CTRL_ANE				BIT(12)
#define MAC_AN_CTRL_RAN				BIT(9)

/* 17.1.45 */

#define MAC_AN_STATUS				0x00e4

#define MAC_AN_STATUS_ES			BIT(8)
#define MAC_AN_STATUS_ANC			BIT(5)
#define MAC_AN_STATUS_ANA			BIT(3)
#define MAC_AN_STATUS_LS			BIT(2)

/* 17.1.46 */

#define MAC_AN_ADVERT				0x00e8

#define MAC_AN_ADVERT_NP			BIT(15)
#define MAC_AN_ADVERT_RFE			GENMASK(13, 12)
#define MAC_AN_ADVERT_PSE			GENMASK(8, 7)
#define MAC_AN_ADVERT_HD			BIT(6)
#define MAC_AN_ADVERT_FD			BIT(5)

/* 17.1.47 */

#define MAC_AN_LINK_PRTNR			0x00ec

#define MAC_AN_LINK_PRTNR_NP			BIT(15)
#define MAC_AN_LINK_PRTNR_ACK			BIT(14)
#define MAC_AN_LINK_PRTNR_RFE			GENMASK(13, 12)
#define MAC_AN_LINK_PRTNR_PSE			GENMASK(8, 7)
#define MAC_AN_LINK_PRTNR_HD			BIT(6)
#define MAC_AN_LINK_PRTNR_FD			BIT(5)

/* 17.1.48 */

#define MAC_AN_EXPANSION			0x00f0

#define MAC_AN_EXPANSION_NPA			BIT(2)
#define MAC_AN_EXPANSION_NPR			BIT(1)

/* 17.1.49 */

#define MAC_TBI_EXT_STATUS			0x00f4

#define MAC_TBI_EXT_STATUS_GFD			BIT(15)
#define MAC_TBI_EXT_STATUS_GHD			BIT(14)

/* 17.1.50 */

#define MAC_PHYIF_CTRL_STATUS			0x00f8

#define MAC_PHYIF_CTRL_STATUS_FALSCARDET	BIT(21)
#define MAC_PHYIF_CTRL_STATUS_JABTO		BIT(20)
#define MAC_PHYIF_CTRL_STATUS_LNKSTS		BIT(19)
#define MAC_PHYIF_CTRL_STATUS_LNKSPEED		GENMASK(18, 17)
#define MAC_PHYIF_CTRL_STATUS_LNKMOD		BIT(16)
#define MAC_PHYIF_CTRL_STATUS_SMIDRXS		BIT(4)
#define MAC_PHYIF_CTRL_STATUS_SFTERR		BIT(2)
#define MAC_PHYIF_CTRL_STATUS_LUD		BIT(1)
#define MAC_PHYIF_CTRL_STATUS_TC		BIT(0)

/* 17.1.51 */

#define MAC_VERSION				0x0110

#define MAC_VERSION_USERVER			GENMASK(15, 8)
#define MAC_VERSION_SNPSVER			GENMASK(7, 0)

/* 17.1.52 */

#define MAC_DEBUG				0x0114

/* 17.1.53 */

#define MAC_HW_FEATURE0				0x011c

#define MAC_HW_FEATURE0_ACTPHYSEL		GENMASK(30, 28)
#define MAC_HW_FEATURE0_SAVLANINS		BIT(27)
#define MAC_HW_FEATURE0_TSSTSSEL		GENMASK(26, 25)
#define MAC_HW_FEATURE0_MACADR64SEL		BIT(24)
#define MAC_HW_FEATURE0_MACADR32SEL		BIT(23)
#define MAC_HW_FEATURE0_ADDMACADRSEL		GENMASK(22, 18)
#define MAC_HW_FEATURE0_RXCOESEL		BIT(16)
#define MAC_HW_FEATURE0_TXCOESEL		BIT(14)
#define MAC_HW_FEATURE0_EEESEL			BIT(13)
#define MAC_HW_FEATURE0_TSSEL			BIT(12)
#define MAC_HW_FEATURE0_ARPOFFSEL		BIT(9)
#define MAC_HW_FEATURE0_MMCSEL			BIT(8)
#define MAC_HW_FEATURE0_MGKSEL			BIT(7)
#define MAC_HW_FEATURE0_RWKSEL			BIT(6)
#define MAC_HW_FEATURE0_SMASEL			BIT(5)
#define MAC_HW_FEATURE0_VLHASH			BIT(4)
#define MAC_HW_FEATURE0_PCSSEL			BIT(3)
#define MAC_HW_FEATURE0_HDSEL			BIT(2)
#define MAC_HW_FEATURE0_GMIISEL			BIT(1)
#define MAC_HW_FEATURE0_MIISEL			BIT(0)

/* 17.1.54 */

#define MAC_HW_FEATURE1				0x0120

#define MAC_HW_FEATURE1_L3L4FNUM		GENMASK(30, 27)
#define MAC_HW_FEATURE1_HASHTBLSZ		GENMASK(25, 24)
#define MAC_HW_FEATURE1_POUOST			BIT(23)
#define MAC_HW_FEATURE1_RAVSEL			BIT(21)
#define MAC_HW_FEATURE1_AVSEL			BIT(20)
#define MAC_HW_FEATURE1_DBGMEMA			BIT(19)
#define MAC_HW_FEATURE1_TSOEN			BIT(18)
#define MAC_HW_FEATURE1_SPHEN			BIT(17)
#define MAC_HW_FEATURE1_DCBEN			BIT(16)
#define MAC_HW_FEATURE1_ADDR64			GENMASK(15, 14)
#define MAC_HW_FEATURE1_ADVTHWORD		BIT(13)
#define MAC_HW_FEATURE1_PTOEN			BIT(12)
#define MAC_HW_FEATURE1_OSTEN			BIT(11)
#define MAC_HW_FEATURE1_TXFIFOSIZE		GENMASK(10, 6)
#define MAC_HW_FEATURE1_SPRAM			BIT(5)
#define MAC_HW_FEATURE1_RXFIFOSIZE		GENMASK(4, 0)

/* 17.1.55 */

#define MAC_HW_FEATURE2				0x0124

#define MAC_HW_FEATURE2_AUXSNAPNUM		GENMASK(30, 28)
#define MAC_HW_FEATURE2_PPSOUTNUM		GENMASK(28, 24)
#define MAC_HW_FEATURE2_TXCHCNT			GENMASK(21, 18)
#define MAC_HW_FEATURE2_RXCHCNT			GENMASK(15, 12)
#define MAC_HW_FEATURE2_TXQCNT			GENMASK(9, 6)
#define MAC_HW_FEATURE2_RXQCNT			GENMASK(3, 0)

/* 17.1.56 */

#define MAC_HW_FEATURE3				0x0128

#define MAC_HW_FEATURE3_ASP			GENMASK(29, 28)
#define MAC_HW_FEATURE3_TBSSEL			BIT(27)
#define MAC_HW_FEATURE3_FPESEL			BIT(26)
#define MAC_HW_FEATURE3_ESTWID			GENMASK(21, 20)
#define MAC_HW_FEATURE3_ESTDEP			GENMASK(19, 17)
#define MAC_HW_FEATURE3_ESTSEL			BIT(16)
#define MAC_HW_FEATURE3_FRPES			GENMASK(14, 13)
#define MAC_HW_FEATURE3_FRPBS			GENMASK(12, 11)
#define MAC_HW_FEATURE3_FRPSEL			BIT(10)
#define MAC_HW_FEATURE3_PDUPSEL			BIT(9)
#define MAC_HW_FEATURE3_DVLAN			BIT(5)
#define MAC_HW_FEATURE3_CBTISEL			BIT(4)
#define MAC_HW_FEATURE3_NRVF			GENMASK(2, 0)

/* 17.1.57 */

#define MAC_DPP_FSM_IRQ_STATUS			0x0140

#define MAC_DPP_FSM_IRQ_STATUS_FSMPES		BIT(24)
#define MAC_DPP_FSM_IRQ_STATUS_SLVTES		BIT(17)
#define MAC_DPP_FSM_IRQ_STATUS_MSTTES		BIT(16)
#define MAC_DPP_FSM_IRQ_STATUS_RVCTES		BIT(15)
#define MAC_DPP_FSM_IRQ_STATUS_R125ES		BIT(14)
#define MAC_DPP_FSM_IRQ_STATUS_T125ES		BIT(13)
#define MAC_DPP_FSM_IRQ_STATUS_PTES		BIT(12)
#define MAC_DPP_FSM_IRQ_STATUS_ATES		BIT(11)
#define MAC_DPP_FSM_IRQ_STATUS_CTES		BIT(10)
#define MAC_DPP_FSM_IRQ_STATUS_RTES		BIT(9)
#define MAC_DPP_FSM_IRQ_STATUS_TTES		BIT(8)
#define MAC_DPP_FSM_IRQ_STATUS_ASRPES		BIT(7)
#define MAC_DPP_FSM_IRQ_STATUS_CWPES		BIT(6)
#define MAC_DPP_FSM_IRQ_STATUS_ARPES		BIT(5)
#define MAC_DPP_FSM_IRQ_STATUS_MTSPES		BIT(4)
#define MAC_DPP_FSM_IRQ_STATUS_MPES		BIT(3)
#define MAC_DPP_FSM_IRQ_STATUS_RDPES		BIT(2)
#define MAC_DPP_FSM_IRQ_STATUS_TPES		BIT(1)
#define MAC_DPP_FSM_IRQ_STATUS_ATPES		BIT(0)

/* 17.1.58 */

#define MAC_AXI_SLV_DPE_ADDR_STATUS		0x0144

#define MAC_AXI_SLV_DPE_ADDR_STATUS_ASPEAS	GENMASK(13, 0)

/* 17.1.59 */

#define MAC_FSM_CTRL				0x0148

#define MAC_FSM_CTRL_RVCLGRNML			BIT(31)
#define MAC_FSM_CTRL_R125LGRNML			BIT(30)
#define MAC_FSM_CTRL_T125LGRNML			BIT(29)
#define MAC_FSM_CTRL_PLGRNML			BIT(28)
#define MAC_FSM_CTRL_ALGRNML			BIT(27)
#define MAC_FSM_CTRL_CLGRNML			BIT(26)
#define MAC_FSM_CTRL_RLGRNML			BIT(25)
#define MAC_FSM_CTRL_TLGRNML			BIT(24)
#define MAC_FSM_CTRL_RVCPEIN			BIT(23)
#define MAC_FSM_CTRL_R125PEIN			BIT(22)
#define MAC_FSM_CTRL_T125PEIN			BIT(21)
#define MAC_FSM_CTRL_PPEIN			BIT(20)
#define MAC_FSM_CTRL_APEIN			BIT(19)
#define MAC_FSM_CTRL_CPEIN			BIT(18)
#define MAC_FSM_CTRL_RPEIN			BIT(17)
#define MAC_FSM_CTRL_TPEIN			BIT(16)
#define MAC_FSM_CTRL_RVCTEIN			BIT(15)
#define MAC_FSM_CTRL_R125TEIN			BIT(14)
#define MAC_FSM_CTRL_T125TEIN			BIT(13)
#define MAC_FSM_CTRL_PTEIN			BIT(12)
#define MAC_FSM_CTRL_ATEIN			BIT(11)
#define MAC_FSM_CTRL_CTEIN			BIT(10)
#define MAC_FSM_CTRL_RTEIN			BIT(9)
#define MAC_FSM_CTRL_TTEIN			BIT(8)
#define MAC_FSM_CTRL_PRTYEN			BIT(1)
#define MAC_FSM_CTRL_TMOUTEN			BIT(0)

/* 17.1.60 */

#define MAC_FSM_ACT_TIMER			0x014c

#define MAC_FSM_ACT_TIMER_LTMRMD		GENMASK(23, 20)
#define MAC_FSM_ACT_TIMER_NTMRMD		GENMASK(19, 16)
#define MAC_FSM_ACT_TIMER_TMR			GENMASK(9, 0)

/* 17.1.62 */

#define MAC_MDIO_ADDRESS			0x0200

#define MAC_MDIO_ADDRESS_PSE			BIT(27)
#define MAC_MDIO_ADDRESS_BTB			BIT(26)
#define MAC_MDIO_ADDRESS_PA			GENMASK(25, 21)
#define MAC_MDIO_ADDRESS_RDA			GENMASK(20, 16)
#define MAC_MDIO_ADDRESS_NTC			GENMASK(14, 12)
#define MAC_MDIO_ADDRESS_CR			BIT(11, 8)
#define MAC_MDIO_ADDRESS_SKAP			BIT(4)
#define MAC_MDIO_ADDRESS_GOC_1			BIT(3)
#define MAC_MDIO_ADDRESS_GOC_0			BIT(2)
#define MAC_MDIO_ADDRESS_GOC_C45E			BIT(1)
#define MAC_MDIO_ADDRESS_GOC_GB			BIT(0)

/* 17.1.63 */

#define MAC_MDIO_DATA				0x0204

#define MAC_MDIO_DATA_RA			GENMASK(31, 16)
#define MAC_MDIO_DATA_GD			GENMASK(15, 0)

/* 17.1.64 */

#define MAC_GPIO_CTRL				0x0208

/* 17.1.65 */

#define MAC_GPIO_STATUS				0x020c

/* 17.1.66 */

#define MAC_ARP_ADDRESS				0x0210

/* 17.1.67 */

#define MAC_CSR_SW_CTRL				0x0230

/* 17.1.68 */

#define MAC_FPE_CTRL_STS			0x0234

/* 17.1.69 */

#define MAC_EXT_CFG1				0x0238

#define MAC_EXT_CFG1_SPLM			GENMASK(9, 8)
#define MAC_EXT_CFG1_SPLOFST			GENMASK(6, 0)

/* 17.1.70 */

#define MAC_PRESN_TIME_NS			0x0240

/* 17.1.71 */

#define MAC_PRESN_TIME_UPDT			0x0244

/* 17.1.72, 17.1.74 */

#define MAC_ADDRESS_HIGH(n)			(0x0300 + 8 * (n))

#define MAC_ADDRESS_HIGH_AE			BIT(31)

/* 17.1.73, 17.1.75 */

#define MAC_ADDRESS_LOW(n)			(0x0304 + 8 * (n))

/*
 * MTL Register Definitions
 */

/* 17.2.1 */

#define MTL_OPERATION_MODE			0x0c00

/* 17.2.2 */

#define MTL_DBG_CTL				0x0c08

/* 17.2.3 */

#define MTL_DBG_STS				0x0c0c

/* 17.2.4 */

#define MTL_FIFO_DEBUG_DATA			0x0c10

/* 17.2.5 */

#define MTL_IRQ_STATUS				0x0c20

#define MTL_IRQ_STATUS_MTLPIS			BIT(23)
#define MTL_IRQ_STATUS_ESTIS			BIT(18)
#define MTL_IRQ_STATUS_DBGIS			BIT(17)
#define MTL_IRQ_STATUS_MACIS			BIT(16)
#define MTL_IRQ_STATUS_Q7IS			BIT(7)
#define MTL_IRQ_STATUS_Q6IS			BIT(6)
#define MTL_IRQ_STATUS_Q5IS			BIT(5)
#define MTL_IRQ_STATUS_Q4IS			BIT(4)
#define MTL_IRQ_STATUS_Q3IS			BIT(3)
#define MTL_IRQ_STATUS_Q2IS			BIT(2)
#define MTL_IRQ_STATUS_Q1IS			BIT(1)
#define MTL_IRQ_STATUS_Q0IS			BIT(0)

/* 17.2.6 */

#define MTL_RXQ_DMA_MAP0			0x0c30

/* 17.2.7 */

#define MTL_RXQ_DMA_MAP1			0x0c34

/* 17.2.8 */

#define MTL_TBS_CTRL				0x0c40

/* 17.2.9 */

#define MTL_EST_CTRL				0x0c50

/* 17.2.10 */

#define MTL_EST_STATUS				0x0c58

/* 17.2.11 */

#define MTL_EST_SCH_ERROR			0x0c60

/* 17.2.12 */

#define MTL_EST_FRM_SIZE_ERROR			0x0c64

/* 17.2.13 */

#define MTL_EST_FRM_SIZE_CAPTURE		0x0c68

/* 17.2.14 */

#define MTL_EST_IRQ_ENABLE			0x0c70

/* 17.2.15 */

#define MTL_EST_GCL_CONTROL			0x0c80

/* 17.2.16 */

#define MTL_EST_GCL_DATA			0x0c84

/* 17.2.17 */

#define MTL_FPE_CTRL_STS			0x0c90

/* 17.2.18 */

#define MTL_FPE_ADVANCE				0x0c94

/* 17.2.19 */

#define MTL_RXP_CTRL_STATUS			0x0ca0

/* 17.2.20 */

#define MTL_RXP_IRQ_CTRL_STATUS			0x0ca4

/* 17.2.21 */

#define MTL_RXP_DROP_CNT			0x0ca8

/* 17.2.22 */

#define MTL_RXP_ERROR_CNT			0x0cac

/* 17.2.23 */

#define MTL_RXP_INDIRECT_ACC_CTRL_STATUS	0x0cb0

/* 17.2.24 */

#define MTL_RXP_INDIRECT_ACC_DATA		0x0cb4

/* 17.2.25 */

#define MTL_ECC_CTRL				0x0cc0

/* 17.2.26 */

#define MTL_SAFETY_IRQ_STATUS			0x0cc4

/* 17.2.27 */

#define MTL_ECC_IRQ_ENABLE			0x0cc8

/* 17.2.28 */

#define MTL_ECC_IRQ_STATUS			0x0ccc

/* 17.2.29 */

#define MTL_ECC_ERR_STS_RCTL			0x0cd0

/* 17.2.30 */

#define MTL_ECC_ERR_ADDR_STATUS			0x0cd4

/* 17.2.31 */

#define MTL_ECC_ERR_CNTR_STATUS			0x0cd8

/* 17.2.32 */

#define MTL_DPP_CTRL				0x0ce0

/* 17.3.1, 17.4.1 */

#define MTL_TXQn_OPERATION_MODE(n)		(0x0d00 + 0x40 * (n))

/* 17.3.2, 17.4.2 */

#define MTL_TXQn_UNDERFLOW(n)			(0x0d04 + 0x40 * (n))

/* 17.3.3, 17.4.3 */

#define MTL_TXQn_DEBUG(n)			(0x0d08 + 0x40 * (n))

/* 17.4.4 */

#define MTL_TXQn_ETS_CTRL(n)			(0x0d10 + 0x40 * (n))

/* 17.3.4, 17.4.5 */

#define MTL_TXQn_ETS_STATUS(n)			(0x0d14 + 0x40 * (n))

/* 17.3.5, 17.4.6 */

#define MTL_TXQn_QUANTUM_WEIGHT(n)		(0x0d18 + 0x40 * (n))

/* 17.4.7 */

#define MTL_TXQn_SENDSLOPECREDIT(n)		(0x0d1c + 0x40 * (n))

/* 17.4.8 */

#define MTL_TXQn_HICREDIT(n)			(0x0d20 + 0x40 * (n))

/* 17.4.9 */

#define MTL_TXQn_LOCREDIT(n)			(0x0d24 + 0x40 * (n))

/* 17.3.6, 17.4.10 */

#define MTL_Qn_IRQ_CTRL_STATUS(n)		(0x0d2c + 0x40 * (n))

/* 17.3.7, 17.4.11 */

#define MTL_RXQn_OPERATION_MODE(n)		(0x0d30 + 0x40 * (n))

/* 17.3.8, 17.4.12 */

#define MTL_RXQn_MISSED_PKT_OVFL_CNT(n)		(0x0d34 + 0x40 * (n))

/* 17.3.9, 17.4.13 */

#define MTL_RXQn_DEBUG(n)			(0x0d38 + 0x40 * (n))

/* 17.3.10, 17.4.14 */

#define MTL_RXQn_CTRL(n)			(0x0d3c + 0x40 * (n))

/*
 * DMA Register Definitions
 */

/* 17.5.1 */

#define DMA_MODE				0x1000

#define DMA_MODE_INTM				GENMASK(17, 16)
#define DMA_MODE_PR				GENMASK(14, 12)
#define DMA_MODE_TXPR				BIT(12)
#define DMA_MODE_ARBC				BIT(9)
#define DMA_MODE_DSPW				BIT(8)
#define DMA_MODE_TAA				GENMASK(4, 2)
#define DMA_MODE_DA				BIT(1)
#define DMA_MODE_SWR				BIT(0)

/* 17.5.2 */

#define DMA_SYSBUS_MODE				0x1004

#define DMA_SYSBUS_MODE_EN_LPI			BIT(31)
#define DMA_SYSBUS_MODE_LPI_XIT_PKT		BIT(30)
#define DMA_SYSBUS_MODE_WR_OSR_LMT		GENMASK(27, 24)
#define DMA_SYSBUS_MODE_RD_OSR_LMT		GENMASK(19, 16)
#define DMA_SYSBUS_MODE_RB			BIT(15)
#define DMA_SYSBUS_MODE_MB			BIT(14)
#define DMA_SYSBUS_MODE_ONEKBBE			BIT(13)
#define DMA_SYSBUS_MODE_AAL			BIT(12)
#define DMA_SYSBUS_MODE_EAME			BIT(11)
#define DMA_SYSBUS_MODE_AALE			BIT(10)
#define DMA_SYSBUS_MODE_BLEN256			BIT(7)
#define DMA_SYSBUS_MODE_BLEN128			BIT(6)
#define DMA_SYSBUS_MODE_BLEN64			BIT(5)
#define DMA_SYSBUS_MODE_BLEN32			BIT(4)
#define DMA_SYSBUS_MODE_BLEN16			BIT(3)
#define DMA_SYSBUS_MODE_BLEN8			BIT(2)
#define DMA_SYSBUS_MODE_BLEN4			BIT(1)
#define DMA_SYSBUS_MODE_FB			BIT(0)

/* 17.5.3 */

#define DMA_IRQ_STATUS				0x1008

#define DMA_IRQ_STATUS_MACIS			BIT(17)
#define DMA_IRQ_STATUS_MTLIS			BIT(16)
#define DMA_IRQ_STATUS_DC7IS			BIT(7)
#define DMA_IRQ_STATUS_DC6IS			BIT(6)
#define DMA_IRQ_STATUS_DC5IS			BIT(5)
#define DMA_IRQ_STATUS_DC4IS			BIT(4)
#define DMA_IRQ_STATUS_DC3IS			BIT(3)
#define DMA_IRQ_STATUS_DC2IS			BIT(2)
#define DMA_IRQ_STATUS_DC1IS			BIT(1)
#define DMA_IRQ_STATUS_DC0IS			BIT(0)

/* 17.5.4 */

#define DMA_DEBUG_STATUS0			0x100c

/* 17.5.5 */

#define DMA_DEBUG_STATUS1			0x1010

/* 17.5.6 */

#define DMA_DEBUG_STATUS2			0x1014

/* 17.5.7 */

#define AXI4_TX_AR_ACE_CTRL			0x1020

/* 17.5.8 */

#define AXI4_RX_AW_ACE_CTRL			0x1024

/* 17.5.9 */

#define AXI4_TXRX_AWAR_ACE_CTRL			0x1028

/* 17.5.10 */

#define AXI_LPI_ENTRY_INTERVAL			0x1040

/* 17.5.11 */

#define DMA_TBS_CTRL				0x1050

/* 17.5.12 */

#define DMA_SAFETY_IRQ_STATUS			0x1080

/* 17.5.13 */

#define DMA_ECC_IRQ_ENABLE			0x1084

/* 17.5.14 */

#define DMA_ECC_IRQ_STATUS			0x1088

/* 17.6.1 */

#define DMA_CHn_CTRL(n)				(0x1100 + 0x80 * (n))

#define DMA_CHn_CTRL_SPH			BIT(24)
#define DMA_CHn_CTRL_DSL			GENMASK(20, 18)
#define DMA_CHn_CTRL_PBLx8			BIT(16)
#define DMA_CHn_CTRL_MSS			GENMASK(13, 0)

/* 17.6.2 */

#define DMA_CHn_TX_CTRL(n)			(0x1104 + 0x80 * (n))

#define DMA_CHn_TX_CTRL_EDSE			BIT(28)
#define DMA_CHn_TX_CTRL_TQOS			GENMASK(27, 24)
#define DMA_CHn_TX_CTRL_ETIC			BIT(22)
#define DMA_CHn_TX_CTRL_PBL			GENMASK(21, 16)
#define DMA_CHn_TX_CTRL_IPBL			BIT(15)
#define DMA_CHn_TX_CTRL_TSE_MODE		GENMASK(14, 13)
#define DMA_CHn_TX_CTRL_TSE			BIT(12)
#define DMA_CHn_TX_CTRL_OSF			BIT(4)
#define DMA_CHn_TX_CTRL_TCW			GENMASK(3, 1)
#define DMA_CHn_TX_CTRL_St			BIT(0)

/* 17.6.3 */

#define DMA_CHn_RX_CTRL(n)			(0x1108 + 0x80 * (n))

#define DMA_CHn_RX_CTRL_RPF			BIT(31)
#define DMA_CHn_RX_CTRL_RQOS			GENMASK(27, 24)
#define DMA_CHn_RX_CTRL_ERIC			BIT(22)
#define DMA_CHn_RX_CTRL_PBL			GENMASK(21, 16)
#define DMA_CHn_RX_CTRL_RBSZ			GENMASK(14, 1)
#define DMA_CHn_RX_CTRL_SR			BIT(0)

/* 17.6.4 */

#define DMA_CHn_TXDESC_LIST_HADDR(n)		(0x1110 + 0x80 * (n))

/* 17.6.5 */

#define DMA_CHn_TXDESC_LIST_ADDR(n)		(0x1114 + 0x80 * (n))

/* 17.6.6 */

#define DMA_CHn_RXDESC_LIST_HADDR(n)		(0x1118 + 0x80 * (n))

/* 17.6.7 */

#define DMA_CHn_RXDESC_LIST_ADDR(n)		(0x111c + 0x80 * (n))

/* 17.6.8 */

#define DMA_CHn_TXDESC_TAIL_PTR(n)		(0x1120 + 0x80 * (n))

/* 17.6.9 */

#define DMA_CHn_RXDESC_TAIL_PTR(n)		(0x1128 + 0x80 * (n))

/* 17.6.10 */

#define DMA_CHn_TXDESC_RING_LENGTH(n)		(0x112c + 0x80 * (n))

/* 17.6.11 */

#define DMA_CHn_RXDESC_RING_LENGTH(n)		(0x1130 + 0x80 * (n))

/* 17.6.12 */

#define DMA_CHn_IRQ_ENABLE(n)			(0x1134 + 0x80 * (n))

#define DMA_CHn_IRQ_ENABLE_NIE			BIT(15)
#define DMA_CHn_IRQ_ENABLE_AIE			BIT(14)
#define DMA_CHn_IRQ_ENABLE_CDEE			BIT(13)
#define DMA_CHn_IRQ_ENABLE_FBEE			BIT(12)
#define DMA_CHn_IRQ_ENABLE_ERIE			BIT(11)
#define DMA_CHn_IRQ_ENABLE_ETIE			BIT(10)
#define DMA_CHn_IRQ_ENABLE_RWTE			BIT(9)
#define DMA_CHn_IRQ_ENABLE_RSE			BIT(8)
#define DMA_CHn_IRQ_ENABLE_RBUE			BIT(7)
#define DMA_CHn_IRQ_ENABLE_RIE			BIT(6)
#define DMA_CHn_IRQ_ENABLE_TBUE			BIT(2)
#define DMA_CHn_IRQ_ENABLE_TXSE			BIT(1)
#define DMA_CHn_IRQ_ENABLE_TIE			BIT(0)

/* 17.6.13 */

#define DMA_CHn_RX_IRQ_WATCHDOG_TIMER(n)	(0x1138 + 0x80 * (n))

/* 17.6.14 */

#define DMA_CHn_SLOT_FN_CTRL_STATUS(n)		(0x113c + 0x80 * (n))

/* 17.6.15 */

#define DMA_CHn_CURR_APP_TXDESC(n)		(0x1144 + 0x80 * (n))

/* 17.6.16 */

#define DMA_CHn_CURR_APP_RXDESC(n)		(0x114c + 0x80 * (n))

/* 17.6.17 */

#define DMA_CHn_CURR_APP_TX_BUF_H(n)		(0x1150 + 0x80 * (n))

/* 17.6.18 */

#define DMA_CHn_CURR_APP_TX_BUF(n)		(0x1154 + 0x80 * (n))

/* 17.6.19 */

#define DMA_CHn_CURR_APP_RX_BUF_H(n)		(0x1158 + 0x80 * (n))

/* 17.6.20 */

#define DMA_CHn_CURR_APP_RX_BUF(n)		(0x115c + 0x80 * (n))

/* 17.6.21 */

#define DMA_CHn_STATUS(n)			(0x1160 + 0x80 * (n))

#define DMA_CHn_STATUS_REB			GENMASK(21, 19)
#define DMA_CHn_STATUS_TEB			GENMASK(18, 16)
#define DMA_CHn_STATUS_NIS			BIT(15)
#define DMA_CHn_STATUS_AIS			BIT(14)
#define DMA_CHn_STATUS_CDE			BIT(13)
#define DMA_CHn_STATUS_FBE			BIT(12)
#define DMA_CHn_STATUS_ERI			BIT(11)
#define DMA_CHn_STATUS_ETI			BIT(10)
#define DMA_CHn_STATUS_RWT			BIT(9)
#define DMA_CHn_STATUS_RPS			BIT(8)
#define DMA_CHn_STATUS_RBU			BIT(7)
#define DMA_CHn_STATUS_RI			BIT(6)
#define DMA_CHn_STATUS_TBU			BIT(2)
#define DMA_CHn_STATUS_TPS			BIT(1)
#define DMA_CHn_STATUS_TI			BIT(0)

/* 17.6.22 */

#define DMA_CHn_MISS_FRAME_CNT(n)		(0x1164 + 0x80 * (n))

/* 17.6.23 */

#define DMA_CHn_RXP_ACCEPT_CNT(n)		(0x1168 + 0x80 * (n))

/* 17.6.24 */

#define DMA_CHn_RX_ERI_CNT(n)			(0x116c + 0x80 * (n))

/*
 * DMA Descriptor Flag Definitions
 */

/* 19.5.1.3 */

#define TDES2_IOC				BIT(31)
#define TDES2_TTSE				BIT(30)
#define TDES2_TMWD				BIT(30)
#define TDES2_B2L				GENMASK(29, 16)
#define TDES2_VTIR				GENMASK(15, 14)
#define TDES2_HL				GENMASK(13, 0)
#define TDES2_B1L				GENMASK(13, 0)

/* 19.5.1.4 */

#define TDES3_OWN				BIT(31)
#define TDES3_CTXT				BIT(30)
#define TDES3_FD				BIT(29)
#define TDES3_LD				BIT(28)
#define TDES3_CPC				GENMASK(27, 26)
#define TDES3_SAIC				GENMASK(25, 23)
#define TDES3_SLOTNUM				GENMASK(22, 19)
#define TDES3_THL				GENMASK(22, 19)
#define TDES3_TSE				BIT(18)
#define TDES3_CIC				GENMASK(17, 16)
#define TDES3_TPL				GENMASK(17, 0)
#define TDES3_FL				GENMASK(14, 0)

/* 19.5.1.9 */

     /* TDES3_OWN				BIT(31) */
#define TDES3_CTXT				BIT(30)
     /* TDES3_FD				BIT(29) */
     /* TDES3_LD				BIT(28) */
#define TDES3_DE				BIT(23)
#define TDES3_TTSS				BIT(17)
#define TDES3_EUE				BIT(16)
#define TDES3_ES				BIT(15)
#define TDES3_JT				BIT(14)
#define TDES3_FF				BIT(13)
#define TDES3_PCE				BIT(12)
#define TDES3_LoC				BIT(11)
#define TDES3_NC				BIT(10)
#define TDES3_LC				BIT(9)
#define TDES3_EC				BIT(8)
#define TDES3_CC				GENMASK(7, 4)
#define TDES3_ED				BIT(3)
#define TDES3_UF				BIT(2)
#define TDES3_DB				BIT(1)
#define TDES3_IHE				BIT(0)

/* 19.6.1.4 */

#define RDES3_OWN				BIT(31)
#define RDES3_IOC				BIT(30)
#define RDES3_BUF2V				BIT(25)
#define RDES3_BUF1V				BIT(24)

/* 19.6.2.1 */

#define RDES0_IVT				GENMASK(31, 16)
#define RDES0_OVT				GENMASK(15, 0)

/* 19.6.2.2 */

#define RDES1_OPC				GENMASK(31, 16)
#define RDES1_TD				BIT(15)
#define RDES1_TSA				BIT(14)
#define RDES1_PV				BIT(13)
#define RDES1_PFT				BIT(12)
#define RDES1_PMT				GENMASK(11, 8)
#define RDES1_ipce				BIT(7)
#define RDES1_IPCB				BIT(6)
#define RDES1_IPV6				BIT(5)
#define RDES1_IPV4				BIT(4)
#define RDES1_IPHE				BIT(3)
#define RDES1_PT				GENMASK(2, 0)

/* 19.6.2.3 */

#define RDES2_L3L4FM				GENMASK(31, 29)
#define RDES2_L4FM				BIT(28)
#define RDES2_L3FM				BIT(27)
#define RDES2_MADRM				GENMASK(26, 19)
#define RDES2_HF				BIT(18)
#define RDES2_DAF				BIT(17)
#define RDES2_RXPI				BIT(17)
#define RDES2_SAF				BIT(16)
#define RDES2_RXPD				BIT(16)
#define RDES2_OTS				BIT(15)
#define RDES2_ITS				BIT(14)
#define RDES2_ARPNR				BIT(10)
#define RDES2_HL				GENMASK(9, 0)

/* 19.6.2.4 */

     /* RDES3_OWN				BIT(31) */
#define RDES3_CTXT				BIT(30)
#define RDES3_FD				BIT(29)
#define RDES3_LD				BIT(28)
#define RDES3_RS2V				BIT(27)
#define RDES3_RS1V				BIT(26)
#define RDES3_RS0V				BIT(25)
#define RDES3_CE				BIT(24)
#define RDES3_GP				BIT(23)
#define RDES3_RWT				BIT(22)
#define RDES3_OE				BIT(21)
#define RDES3_RE				BIT(20)
#define RDES3_DE				BIT(19)
#define RDES3_LT				GENMASK(18, 16)
#define RDES3_ES				BIT(15)
#define RDES3_PL				GENMASK(14, 0)


#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_DWMAC_PRIV_H_ */
