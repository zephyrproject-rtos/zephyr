/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwcxgmac

#include "eth_dwc_xgmac_priv.h"

#define ETH_XGMAC_IS_INST_RESET_EN(inst) DT_NODE_HAS_PROP(DT_DRV_INST(inst), resets)
#define ETH_XGMAC_CHECK_RESET(inst)      ETH_XGMAC_IS_INST_RESET_EN(inst) ||

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
#include <zephyr/drivers/reset.h>
#endif

/**
 * @brief Run-time device configuration data structure.
 *
 * This struct contains all device configuration data for a XGMAC
 * controller instance which is modifiable at run-time, such as
 * data relating to the attached PHY or the auxiliary thread.
 */
struct eth_dwc_xgmac_dev_data {
	DEVICE_MMIO_RAM;
	/*
	 * Device running status. eth_dwc_xgmac_start_device API will will variable and
	 * eth_dwc_xgmac_stop_device APi will clear this variable.
	 */
	bool dev_started;
	/*
	 * Ethernet interface structure associated with this device.
	 */
	struct net_if *iface;
	/*
	 * Device MAC address
	 */
	uint8_t mac_addr[6];
	/*
	 * Current Ethernet link speed 10Mbps/100Mbps/1Gbps.
	 */
	enum eth_dwc_xgmac_link_speed link_speed;
	/*
	 * This feild specifies the ethernet link type full duplex or half duplex.
	 */
	bool enable_full_duplex;
	/*
	 * Ethernet auto-negotiation status.
	 */
	bool auto_neg;
	/*
	 * Ethernet promiscuous mode status.
	 */
	bool promisc_mode;
	/*
	 * Global pointer to DMA receive descriptors ring.
	 */
	struct xgmac_dma_rx_desc *dma_rx_desc;
	/*
	 * Global pointer to DMA transmit descriptors ring.
	 */
	struct xgmac_dma_tx_desc *dma_tx_desc;
	/*
	 * Global pointer to DMA transmit descriptors ring's meta data.
	 */
	volatile struct xgmac_dma_tx_desc_meta *tx_desc_meta;
	/*
	 * Global pointer to DMA receive descriptors ring's meta data.
	 */
	volatile struct xgmac_dma_rx_desc_meta *rx_desc_meta;
	/*
	 * Array of pointers pointing to Transmit packets  under tranmission.
	 * These pointers will be cleared once the packet transmission is completed.
	 */
	mem_addr_t *tx_pkts;
	/*
	 * Array of pointers pointing to receive fragments reserved for receive data.
	 * Data received by XGMAC will be copied to these fragments. An empty network packet
	 * will reserved as receive packet, these fragments will be added to a the receive packet.
	 * After a fragment is added to the receive packet a new fragment will be reserved
	 * and replaced with the used fragment for future receive data.
	 */
	mem_addr_t *rx_frags;
	/*
	 * A global pointer pointing to XGMAC IRQ context data.
	 */
	struct xgmac_irq_cntxt_data irq_cntxt_data;
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	/*
	 * Ethernet statistics captured by XGMAC driver
	 */
	struct net_stats_eth stats;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
#ifdef CONFIG_ETH_DWC_XGMAC_POLLING_MODE
	/*
	 * timer for interrupt polling
	 */
	struct k_timer isr_polling_timer;
#endif /* CONFIG_ETH_DWC_XGMAC_POLLING_MODE */
#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
	/*
	 * work que item for processing TX interrupt bottom half
	 */
	struct k_work isr_work;
#endif /*CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE*/
	struct k_mutex dev_cfg_lock;
};

/**
 * @brief Constant device configuration data structure.
 *
 * This struct contains all device configuration data for a XGMAC
 * controller instance which is constant. The data herein is
 * either acquired from the generated header file based on the
 * data from Kconfig, or from header file based on the device tree
 * data. Some of the data contained, in particular data relating
 * to clock sources, is specific to the platform, which contain the XGMAC.
 */
struct eth_dwc_xgmac_config {
	DEVICE_MMIO_ROM;
	/*
	 * Number of TX queues configured
	 */
	uint8_t num_tx_Qs;
	/*
	 * Number of RX queues configured
	 */
	uint8_t num_rx_Qs;
	/*
	 * Number of DMA channels configured
	 */
	uint8_t num_dma_chnl;
	/*
	 * Number of traffic classes configured
	 */
	uint8_t num_TCs;
	/*
	 * Maximum transfer unit length configured
	 */
	uint16_t mtu;
	/*
	 * Transmit FIFO size
	 */
	uint32_t tx_fifo_size;
	/*
	 * Receive FIFO size
	 */
	uint32_t rx_fifo_size;
	/*
	 * XGMAC DMA configuration
	 */
	struct xgmac_dma_cfg dma_cfg;
	/*
	 * XGMAC DMA channels configuration
	 */
	struct xgmac_dma_chnl_config dma_chnl_cfg;
	/*
	 * XGMAC MTL configuration
	 */
	struct xgmac_mtl_config mtl_cfg;
	/*
	 * XGMAC core configuration
	 */
	struct xgmac_mac_config mac_cfg;
	/*
	 * Global pointer to traffic classes and queues configuration
	 */
	struct xgmac_tcq_config *tcq_config;
	/*
	 * Ethernet PHY device pointer
	 */
	const struct device *phy_dev;
	/*
	 * Interrupts configuration function pointer
	 */
	eth_config_irq_t irq_config_fn;
	/*
	 * Interrupts enable function pointer
	 */
	eth_enable_irq_t irq_enable_fn;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	/*
	 * XGMAC peripheral reset signal
	 */
	const struct reset_dt_spec reset;
#endif
};

#define LOG_MODULE_NAME eth_dwc_xgmac
#define LOG_LEVEL       CONFIG_ETHERNET_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

static inline mem_addr_t get_reg_base_addr(const struct device *dev)
{
	return (mem_addr_t)DEVICE_MMIO_GET(dev);
}

static inline int dwxgmac_software_reset(mem_addr_t ioaddr)
{
	uint32_t retries = ETH_DWC_XGMAC_RESET_STATUS_CHECK_RETRY_COUNT;
	/* software reset */
	mem_addr_t reg_addr = (mem_addr_t)(ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_MODE_OFST);

	sys_write32(DMA_MODE_SWR_SET(1u), reg_addr);

	while (sys_read32(reg_addr) & DMA_MODE_SWR_SET_MSK) {
		if (retries-- == 0) {
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(1u));
	}

	return 0;
}

static void dwxgmac_dma_init(mem_addr_t ioaddr, const struct xgmac_dma_cfg *const dma_cfg)
{

	mem_addr_t reg_addr =
		(mem_addr_t)(ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_SYSBUS_MODE_OFST);

	uint32_t reg_val = DMA_SYSBUS_MODE_RD_OSR_LMT_SET(dma_cfg->rd_osr_lmt) |
			   DMA_SYSBUS_MODE_WR_OSR_LMT_SET(dma_cfg->wr_osr_lmt) |
			   DMA_SYSBUS_MODE_AAL_SET(dma_cfg->aal) |
			   DMA_SYSBUS_MODE_EAME_SET(dma_cfg->eame) |
			   DMA_SYSBUS_MODE_BLEN4_SET(dma_cfg->blen4) |
			   DMA_SYSBUS_MODE_BLEN8_SET(dma_cfg->blen8) |
			   DMA_SYSBUS_MODE_BLEN16_SET(dma_cfg->blen16) |
			   DMA_SYSBUS_MODE_BLEN32_SET(dma_cfg->blen32) |
			   DMA_SYSBUS_MODE_BLEN64_SET(dma_cfg->blen64) |
			   DMA_SYSBUS_MODE_BLEN128_SET(dma_cfg->blen128) |
			   DMA_SYSBUS_MODE_BLEN256_SET(dma_cfg->blen256) |
			   DMA_SYSBUS_MODE_UNDEF_SET(dma_cfg->ubl);

	sys_write32(reg_val, reg_addr);

	reg_addr = (ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_TX_EDMA_CONTROL_OFST);

	reg_val = DMA_TX_EDMA_CONTROL_TDPS_SET(dma_cfg->edma_tdps);

	sys_write32(reg_val, reg_addr);

	reg_addr = (ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_RX_EDMA_CONTROL_OFST);

	reg_val = DMA_RX_EDMA_CONTROL_RDPS_SET(dma_cfg->edma_rdps);

	sys_write32(reg_val, reg_addr);
}

static void dwxgmac_dma_chnl_init(mem_addr_t ioaddr,
				  const struct eth_dwc_xgmac_config *const config,
				  struct eth_dwc_xgmac_dev_data *const data)
{
	int dma_chnl;
	int max_dma_chnl = config->num_dma_chnl;
	struct xgmac_dma_chnl_config *const dma_chnl_cfg =
		(struct xgmac_dma_chnl_config *)&config->dma_chnl_cfg;
	struct xgmac_dma_tx_desc_meta *tx_desc_meta;
	struct xgmac_dma_rx_desc_meta *rx_desc_meta;
	uint32_t reg_val;
	mem_addr_t reg_addr;

	for (dma_chnl = 0; dma_chnl < max_dma_chnl; dma_chnl++) {
		tx_desc_meta = (struct xgmac_dma_tx_desc_meta *)&data->tx_desc_meta[dma_chnl];
		rx_desc_meta = (struct xgmac_dma_rx_desc_meta *)&data->rx_desc_meta[dma_chnl];

		reg_addr = (mem_addr_t)(ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
					DMA_CHx_CONTROL_OFST);
		reg_val = DMA_CHx_CONTROL_SPH_SET(dma_chnl_cfg->sph) |
			  DMA_CHx_CONTROL_PBLX8_SET(dma_chnl_cfg->pblx8) |
			  DMA_CHx_CONTROL_MSS_SET(dma_chnl_cfg->mss);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TX_CONTROL_OFST);
		reg_val = DMA_CHx_TX_CONTROL_TXPBL_SET(dma_chnl_cfg->txpbl) |
			  DMA_CHx_TX_CONTROL_TSE_SET(dma_chnl_cfg->tse) |
			  DMA_CHx_TX_CONTROL_RESERVED_OSP_SET(dma_chnl_cfg->osp);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RX_CONTROL_OFST);
		reg_val = DMA_CHx_RX_CONTROL_RPF_SET(1u) |
			  DMA_CHx_RX_CONTROL_RXPBL_SET(dma_chnl_cfg->rxpbl) |
			  DMA_CHx_RX_CONTROL_RBSZ_SET(CONFIG_NET_BUF_DATA_SIZE);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TXDESC_LIST_HADDRESS_OFST);
		reg_val = DMA_CHx_TXDESC_LIST_HADDRESS_TDESHA_SET(tx_desc_meta->desc_list_addr >>
								  32u);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TXDESC_LIST_LADDRESS_OFST);
		reg_val = tx_desc_meta->desc_list_addr;
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RXDESC_LIST_HADDRESS_OFST);
		reg_val = rx_desc_meta->desc_list_addr >> 32u;
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RXDESC_LIST_LADDRESS_OFST);
		reg_val = rx_desc_meta->desc_list_addr;
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TXDESC_TAIL_LPOINTER_OFST);
		reg_val = DMA_CHx_TXDESC_TAIL_LPOINTER_TDT_SET(tx_desc_meta->desc_tail_addr);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RXDESC_TAIL_LPOINTER_OFST);
		reg_val = DMA_CHx_RXDESC_TAIL_LPOINTER_RDT_SET(rx_desc_meta->desc_tail_addr);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TX_CONTROL2_OFST);
		reg_val = DMA_CHx_TX_CONTROL2_TDRL_SET((dma_chnl_cfg->tdrl - 1u));
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RX_CONTROL2_OFST);
		reg_val = DMA_CHx_RX_CONTROL2_RDRL_SET((dma_chnl_cfg->rdrl - 1u));
		sys_write32(reg_val, reg_addr);

		/*
		 * Initialize channel metadata
		 */
		tx_desc_meta->next_to_use = 0u;
		rx_desc_meta->next_to_read = 0u;
		rx_desc_meta->rx_pkt = (struct net_pkt *)NULL;
	}
}

static void dwxgmac_dma_desc_init(const struct eth_dwc_xgmac_config *const config,
				  struct eth_dwc_xgmac_dev_data *const data)
{
	const int max_dma_chnl = config->num_dma_chnl;
	int dma_chnl;
	struct xgmac_dma_chnl_config *const dma_chnl_cfg =
		(struct xgmac_dma_chnl_config *)&config->dma_chnl_cfg;

	struct xgmac_dma_rx_desc *rx_desc;
	struct xgmac_dma_tx_desc_meta *tx_desc_meta;
	struct xgmac_dma_rx_desc_meta *rx_desc_meta;

	for (dma_chnl = 0; dma_chnl < max_dma_chnl; dma_chnl++) {
		tx_desc_meta = (struct xgmac_dma_tx_desc_meta *)&data->tx_desc_meta[dma_chnl];
		rx_desc_meta = (struct xgmac_dma_rx_desc_meta *)&data->rx_desc_meta[dma_chnl];

		tx_desc_meta->desc_list_addr =
			POINTER_TO_UINT(data->dma_tx_desc + (dma_chnl * dma_chnl_cfg->tdrl));
		tx_desc_meta->desc_tail_addr = POINTER_TO_UINT(tx_desc_meta->desc_list_addr);

		memset((void *)(tx_desc_meta->desc_list_addr), 0,
		       ((dma_chnl_cfg->tdrl) * sizeof(struct xgmac_dma_tx_desc)));

		rx_desc_meta->desc_list_addr =
			POINTER_TO_UINT(data->dma_rx_desc + (dma_chnl * dma_chnl_cfg->rdrl));
		rx_desc_meta->desc_tail_addr = POINTER_TO_UINT(rx_desc_meta->desc_list_addr);

		memset((void *)(rx_desc_meta->desc_list_addr), 0,
		       ((dma_chnl_cfg->rdrl) * sizeof(struct xgmac_dma_rx_desc)));

		for (int desc_idx = 0; desc_idx < dma_chnl_cfg->rdrl; desc_idx++) {
			rx_desc = (data->dma_rx_desc + (desc_idx + dma_chnl * dma_chnl_cfg->rdrl));
			rx_desc->rdes3.wb.own = XGMAC_DESC_OWNED_BY_DMA;
		}
	}
}

static void dwxgmac_dma_mtl_init(mem_addr_t ioaddr, const struct eth_dwc_xgmac_config *const config)
{
	int max_q_count =
		config->num_tx_Qs > config->num_rx_Qs ? config->num_tx_Qs : config->num_rx_Qs;
	int q_idx;

	struct xgmac_mtl_config *mtl_cfg = (struct xgmac_mtl_config *)&config->mtl_cfg;
	struct xgmac_tcq_config *const tcq_config = (struct xgmac_tcq_config *)config->tcq_config;

	mem_addr_t reg_addr =
		(mem_addr_t)(ioaddr + XGMAC_MTL_BASE_ADDR_OFFSET + MTL_OPERATION_MODE_OFST);
	uint32_t reg_val = MTL_OPERATION_MODE_ETSALG_SET(mtl_cfg->etsalg) |
			   MTL_OPERATION_MODE_RAA_SET(mtl_cfg->raa);
	sys_write32(reg_val, reg_addr);

	for (int tc_id = 0; tc_id < config->num_TCs; tc_id++) {
		reg_addr = (ioaddr + XGMAC_MTL_BASE_ADDR_OFFSET + MTL_TC_PRTY_MAP0_OFST +
			    ((tc_id / NUM_OF_TCs_PER_TC_PRTY_MAP_REG) * XGMAC_REG_SIZE_BYTES));
		reg_val = (sys_read32(reg_addr) &
			   MTL_TCx_PRTY_MAP_MSK(tc_id % NUM_OF_TCs_PER_TC_PRTY_MAP_REG));
		reg_val |= MTL_TCx_PRTY_MAP_PSTC_SET((tc_id % NUM_OF_TCs_PER_TC_PRTY_MAP_REG),
						     tcq_config->pstc[tc_id]);
		sys_write32(reg_val, reg_addr);
	}

	for (q_idx = 0u; q_idx < max_q_count; q_idx++) {
		reg_addr = (ioaddr + XGMAC_MTL_BASE_ADDR_OFFSET + MTL_RXQ_DMA_MAP0_OFST +
			    ((q_idx / NUM_OF_RxQs_PER_DMA_MAP_REG) * XGMAC_REG_SIZE_BYTES));
		reg_val = (sys_read32(reg_addr) &
			   MTL_RXQ_DMA_MAP_Qx_MSK(q_idx % NUM_OF_RxQs_PER_DMA_MAP_REG));
		reg_val |= MTL_RXQ_DMA_MAP_QxDDMACH_SET((q_idx % NUM_OF_RxQs_PER_DMA_MAP_REG),
							READ_BIT(tcq_config->rx_q_ddma_en, q_idx)) |
			   MTL_RXQ_DMA_MAP_QxMDMACH_SET((q_idx % NUM_OF_RxQs_PER_DMA_MAP_REG),
							tcq_config->rx_q_dma_chnl_sel[q_idx]);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_MTL_TCQx_BASE_ADDR_OFFSET(q_idx) +
			    MTL_TCQx_MTL_TXQx_OPERATION_MODE_OFST);
		reg_val = MTL_TCQx_MTL_TXQx_OPERATION_MODE_TQS_SET(tcq_config->tx_q_size[q_idx]) |
			  MTL_TCQx_MTL_TXQx_OPERATION_MODE_Q2TCMAP_SET(
				  tcq_config->q_to_tc_map[q_idx]) |
			  MTL_TCQx_MTL_TXQx_OPERATION_MODE_TTC_SET(tcq_config->ttc[q_idx]) |
			  MTL_TCQx_MTL_TXQx_OPERATION_MODE_TXQEN_SET(2u) |
			  MTL_TCQx_MTL_TXQx_OPERATION_MODE_TSF_SET(
				  READ_BIT(tcq_config->tsf_en, q_idx));
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_MTL_TCQx_BASE_ADDR_OFFSET(q_idx) +
			    MTL_TCQx_MTC_TCx_ETS_CONTROL_OFST);
		reg_val = MTL_TCQx_MTC_TCx_ETS_CONTROL_TSA_SET(tcq_config->tsa[q_idx]);
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_MTL_TCQx_BASE_ADDR_OFFSET(q_idx) +
			    MTL_TCQx_MTL_RXQx_OPERATION_MODE_OFST);
		reg_val = MTL_TCQx_MTL_RXQx_OPERATION_MODE_RQS_SET(tcq_config->rx_q_size[q_idx]) |
			  MTL_TCQx_MTL_RXQx_OPERATION_MODE_EHFC_SET(
				  READ_BIT(tcq_config->hfc_en, q_idx)) |
			  MTL_TCQx_MTL_RXQx_OPERATION_MODE_DIS_TCP_EF_SET(
				  READ_BIT(tcq_config->cs_err_pkt_drop_dis, q_idx)) |
			  MTL_TCQx_MTL_RXQx_OPERATION_MODE_RSF_SET(
				  READ_BIT(tcq_config->rsf_en, q_idx)) |
			  MTL_TCQx_MTL_RXQx_OPERATION_MODE_FEF_SET(
				  READ_BIT(tcq_config->fep_en, q_idx)) |
			  MTL_TCQx_MTL_RXQx_OPERATION_MODE_FUF_SET(
				  READ_BIT(tcq_config->fup_en, q_idx)) |
			  MTL_TCQx_MTL_RXQx_OPERATION_MODE_RTC_SET(tcq_config->rtc[q_idx]);
		sys_write32(reg_val, reg_addr);
	}
}

static void dwxgmac_set_mac_addr_by_idx(mem_addr_t ioaddr, uint8_t *addr, uint8_t idx, bool sa)
{
	uint32_t reg_val;

	reg_val = (addr[5] << 8) | addr[4];
	idx ? (reg_val |= CORE_MAC_ADDRESS1_HIGH_SA_SET(sa)) : ARG_UNUSED(sa);
	sys_write32(reg_val | CORE_MAC_ADDRESS1_HIGH_AE_SET_MSK,
		    ioaddr + XGMAC_CORE_ADDRx_HIGH(idx));

	reg_val = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	sys_write32(reg_val, ioaddr + XGMAC_CORE_ADDRx_LOW(idx));
}

static void eth_dwc_xgmac_update_link_speed(uint32_t ioaddr,
					    enum eth_dwc_xgmac_link_speed link_speed)
{
	uint32_t reg_val;

	reg_val = sys_read32(ioaddr + CORE_MAC_TX_CONFIGURATION_OFST);
	reg_val &= CORE_MAC_TX_CONFIGURATION_SS_CLR_MSK;

	switch (link_speed) {
	case LINK_10MBIT:
		reg_val |= CORE_MAC_TX_CONFIGURATION_SS_SET(CORE_MAC_TX_CONFIGURATION_SS_10MHZ);
		break;
	case LINK_100MBIT:
		reg_val |= CORE_MAC_TX_CONFIGURATION_SS_SET(CORE_MAC_TX_CONFIGURATION_SS_100MHZ);
		break;
	case LINK_1GBIT:
		reg_val |= CORE_MAC_TX_CONFIGURATION_SS_SET(CORE_MAC_TX_CONFIGURATION_SS_1000MHZ);
		break;
	/*
	 * case LINK_2500MBIT:
	 * reg_val |= CORE_MAC_TX_CONFIGURATION_SS_SET(CORE_MAC_TX_CONFIGURATION_SS_2500MHZ);
	 * break;
	 */
	default:
		/*Do nothing*/
	}

	sys_write32(reg_val, ioaddr + CORE_MAC_TX_CONFIGURATION_OFST);
}

static void dwxgmac_mac_init(uint32_t ioaddr, const struct eth_dwc_xgmac_config *const config,
			     struct eth_dwc_xgmac_dev_data *const data)
{
	struct xgmac_mac_config *const mac_cfg = (struct xgmac_mac_config *)&config->mac_cfg;
	uint32_t reg_val;

	/* Enable MAC HASH & MAC Perfect filtering */
	reg_val =
#ifndef CONFIG_ETH_DWC_XGMAC_HW_FILTERING
		CORE_MAC_PACKET_FILTER_RA_SET(SET_BIT) | CORE_MAC_PACKET_FILTER_PM_SET(SET_BIT);
#else
#ifdef CONFIG_ETH__DWC_XGMAC_HW_L3_L4_FILTERING
		CORE_MAC_PACKET_FILTER_IPFE_SET(SET_BIT) |
#endif
		CORE_MAC_PACKET_FILTER_HPF_SET(SET_BIT) | CORE_MAC_PACKET_FILTER_HMC_SET(SET_BIT) |
		CORE_MAC_PACKET_FILTER_HUC_SET(SET_BIT);
#endif

	sys_write32(reg_val, ioaddr + CORE_MAC_PACKET_FILTER_OFST);

	reg_val = 0;
	for (int q = 0; q < config->num_rx_Qs; q++) {
		reg_val |= (XGMAC_RXQxEN_DCB << (q * XGMAC_RXQxEN_SIZE_BITS));
	}
	sys_write32(reg_val, ioaddr + CORE_MAC_RXQ_CTRL0_OFST);

	reg_val = CORE_MAC_TX_CONFIGURATION_JD_SET(SET_BIT);
	sys_write32(reg_val, ioaddr + CORE_MAC_TX_CONFIGURATION_OFST);

	reg_val = CORE_MAC_RX_CONFIGURATION_GPSLCE_SET(SET_BIT) |
#ifdef CONFIG_ETH_DWC_XGMAC_RX_CS_OFFLOAD
		  CORE_MAC_RX_CONFIGURATION_IPC_SET(SET_BIT) |
#endif
		  CORE_MAC_RX_CONFIGURATION_WD_SET(SET_BIT) |
		  CORE_MAC_RX_CONFIGURATION_JE_SET(mac_cfg->je) |
		  CORE_MAC_RX_CONFIGURATION_ARPEN_SET(mac_cfg->arp_offload_en) |
		  CORE_MAC_RX_CONFIGURATION_GPSL_SET(mac_cfg->gpsl);

	sys_write32(reg_val, ioaddr + CORE_MAC_RX_CONFIGURATION_OFST);

	/* Configure MAC link speed */
	eth_dwc_xgmac_update_link_speed(ioaddr, data->link_speed);
}

static inline void dwxgmac_irq_init(const struct device *dev)
{
	struct eth_dwc_xgmac_dev_data *const data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	mem_addr_t reg_addr;
	uint32_t reg_val;
	mem_addr_t ioaddr = get_reg_base_addr(dev);

	reg_addr = (ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_MODE_OFST);
	reg_val = (sys_read32(reg_addr) & DMA_MODE_INTM_CLR_MSK);
	sys_write32(reg_val, reg_addr);
	data->irq_cntxt_data.dev = dev;
}

static inline void add_frags_to_pkt(struct net_pkt *rx_pkt, struct net_buf *frag1,
				    uint16_t frag1_len, struct net_buf *frag2, uint16_t frag2_len)
{
	frag1->len = frag1_len;
	net_pkt_frag_add(rx_pkt, frag1);
	if (frag2_len) {
		frag2->len = frag2_len;
		net_pkt_frag_add(rx_pkt, frag2);
	} else {
		net_pkt_frag_unref(frag2);
	}
}

static void get_and_refill_desc_frags(struct xgmac_dma_rx_desc *rx_desc, uint16_t desc_id,
				      mem_addr_t *rx_frags, struct net_buf **frag1,
				      struct net_buf **frag2)
{
	struct net_buf *new_frag;

	*frag1 = (struct net_buf *)((mem_addr_t)*(rx_frags + (desc_id * RX_FRAGS_PER_DESC)));
	*frag2 = (struct net_buf *)((mem_addr_t)*(rx_frags + (desc_id * RX_FRAGS_PER_DESC) + 1u));
	new_frag = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_FOREVER);
	if (!new_frag) {
		LOG_ERR("Failed to allocate a network buffer to refill the DMA descriptor");
		return;
	}
	*(rx_frags + (desc_id * RX_FRAGS_PER_DESC)) = (mem_addr_t)new_frag;
	rx_desc->rdes0.dword = POINTER_TO_UINT(new_frag->data);
	rx_desc->rdes1.dword = POINTER_TO_UINT(new_frag->data) >> XGMAC_REG_SIZE_BITS;

	new_frag = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_FOREVER);
	if (!new_frag) {
		rx_desc->rdes0.dword = 0u;
		rx_desc->rdes0.dword = 1u;
		net_pkt_frag_unref((struct net_buf *)(*(rx_frags + (desc_id * RX_FRAGS_PER_DESC))));
		*(rx_frags + (desc_id * RX_FRAGS_PER_DESC)) = (mem_addr_t)NULL;
		LOG_ERR("Failed to allocate a network buffer to refill the DMA descriptor");
		return;
	}
	*(rx_frags + (desc_id * RX_FRAGS_PER_DESC) + 1u) = (mem_addr_t)new_frag;
	rx_desc->rdes2.dword = POINTER_TO_UINT(new_frag->data);
	rx_desc->rdes3.dword = XGMAC_RDES3_OWN | XGMAC_RDES3_IOC |
			       (POINTER_TO_UINT(new_frag->data) >> XGMAC_REG_SIZE_BITS);
}

static void eth_dwc_xgmac_rx_irq_work(const struct device *dev, uint32_t dma_chnl)
{
	struct eth_dwc_xgmac_dev_data *const data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	const struct eth_dwc_xgmac_config *const config =
		(struct eth_dwc_xgmac_config *)dev->config;
	struct xgmac_dma_chnl_config *dma_chnl_cfg =
		(struct xgmac_dma_chnl_config *)&config->dma_chnl_cfg;
	struct xgmac_dma_rx_desc_meta *rx_desc_meta =
		(struct xgmac_dma_rx_desc_meta *)&data->rx_desc_meta[dma_chnl];
	struct xgmac_dma_rx_desc *fisrt_rx_desc =
		(struct xgmac_dma_rx_desc *)(data->dma_rx_desc + (dma_chnl * dma_chnl_cfg->rdrl));
	struct xgmac_dma_rx_desc *rx_desc, rx_desc_data;
	struct net_buf *frag1 = NULL, *frag2 = NULL;
	int desc_data_len;
	int err;

	mem_addr_t *rx_frags = (mem_addr_t *)(data->rx_frags + (((dma_chnl * dma_chnl_cfg->rdrl)) *
								RX_FRAGS_PER_DESC));

	rx_desc = (struct xgmac_dma_rx_desc *)(fisrt_rx_desc + rx_desc_meta->next_to_read);
	rx_desc_data = *(rx_desc);
	while (!(rx_desc_data.rdes3.dword & XGMAC_RDES3_OWN)) {
		get_and_refill_desc_frags(rx_desc, rx_desc_meta->next_to_read, rx_frags, &frag1,
					  &frag2);

		if (rx_desc_data.rdes3.dword & XGMAC_RDES3_FD) {
			LOG_DBG("received FD fragment. descriptor indx = %d",
				rx_desc_meta->next_to_read);
			if (rx_desc_meta->rx_pkt) {
				net_pkt_frag_unref(rx_desc_meta->rx_pkt->frags);
				net_pkt_unref(rx_desc_meta->rx_pkt);
			}
			rx_desc_meta->rx_pkt = net_pkt_rx_alloc_on_iface(data->iface, K_NO_WAIT);
			if (!rx_desc_meta->rx_pkt) {
				LOG_ERR("Failed allocate a network packet for receive data");
				/*Error processing*/
				return;
			}
		}

		if (rx_desc_meta->rx_pkt != NULL) {
			if (rx_desc_data.rdes3.dword & XGMAC_RDES3_LD) {
				LOG_DBG("received LD fragment. descriptor indx = %d",
					rx_desc_meta->next_to_read);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
				data->stats.pkts.rx++;
#endif
				if (!(rx_desc_data.rdes3.dword & XGMAC_RDES3_ES)) {
					desc_data_len =
						(rx_desc_data.rdes3.dword & XGMAC_RDES3_PL) %
						(CONFIG_NET_BUF_DATA_SIZE * RX_FRAGS_PER_DESC);

					if (desc_data_len > CONFIG_NET_BUF_DATA_SIZE) {
						add_frags_to_pkt(
							rx_desc_meta->rx_pkt, frag1,
							CONFIG_NET_BUF_DATA_SIZE, frag2,
							(desc_data_len - CONFIG_NET_BUF_DATA_SIZE));
					} else {
						add_frags_to_pkt(rx_desc_meta->rx_pkt, frag1,
								 desc_data_len, frag2, 0u);
					}
					/* Full packet received, submit to net sub system for
					 * further processing
					 */
					err = net_recv_data(data->iface, rx_desc_meta->rx_pkt);
					if (err) {
#ifdef CONFIG_NET_STATISTICS_ETHERNET
						data->stats.errors.rx++;
#endif
						net_pkt_unref(rx_desc_meta->rx_pkt);
						LOG_DBG("received packet dropped %d", err);
					} else {
						LOG_DBG("received a packet");
#ifdef CONFIG_NET_STATISTICS_ETHERNET
						data->stats.bytes.received +=
							net_pkt_get_len(rx_desc_meta->rx_pkt);
#endif
					}
				} else {
					LOG_ERR("rx packet error");
#ifdef CONFIG_NET_STATISTICS_ETHERNET
					data->stats.errors.rx++;
#endif
					net_pkt_unref(rx_desc_meta->rx_pkt);
				}
				rx_desc_meta->rx_pkt = (struct net_pkt *)NULL;
			} else {
				add_frags_to_pkt(rx_desc_meta->rx_pkt, frag1,
						 CONFIG_NET_BUF_DATA_SIZE, frag2,
						 CONFIG_NET_BUF_DATA_SIZE);
			}
		} else {
			LOG_ERR("Received a fragment with no FD fragmanet received in the "
				"sequence");
		}
		rx_desc_meta->next_to_read =
			((rx_desc_meta->next_to_read + 1) % dma_chnl_cfg->rdrl);
		rx_desc = (struct xgmac_dma_rx_desc *)(fisrt_rx_desc + rx_desc_meta->next_to_read);
		rx_desc_data = *(rx_desc);
	}
}

static void eth_dwc_xgmac_tx_irq_work(const struct device *dev, uint32_t dma_chnl)
{
	struct eth_dwc_xgmac_dev_data *const data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	const struct eth_dwc_xgmac_config *const config =
		(struct eth_dwc_xgmac_config *)dev->config;
	struct xgmac_dma_chnl_config *dma_chnl_cfg =
		(struct xgmac_dma_chnl_config *)&config->dma_chnl_cfg;
	struct xgmac_dma_tx_desc_meta *tx_desc_meta =
		(struct xgmac_dma_tx_desc_meta *)&data->tx_desc_meta[dma_chnl];
	struct xgmac_dma_tx_desc *fisrt_tx_desc =
		(struct xgmac_dma_tx_desc *)(data->dma_tx_desc + (dma_chnl * dma_chnl_cfg->tdrl));
	struct xgmac_dma_tx_desc *tx_desc;
	uint16_t desc_idx;
	struct net_pkt *pkt;

	desc_idx =
		((tx_desc_meta->next_to_use + k_sem_count_get(&tx_desc_meta->free_tx_descs_sem)) %
		 dma_chnl_cfg->tdrl);
	for (; desc_idx != tx_desc_meta->next_to_use;
	     desc_idx = ((desc_idx + 1) % dma_chnl_cfg->tdrl)) {
		tx_desc = (struct xgmac_dma_tx_desc *)(fisrt_tx_desc + desc_idx);
		if (!tx_desc->tdes3.wb.own) {
			if (tx_desc->tdes3.wb.ld) {
				pkt = (struct net_pkt *)(*(
					data->tx_pkts +
					((dma_chnl * dma_chnl_cfg->tdrl) + desc_idx)));
				LOG_DBG("dwc xgmac: %p packet unreferenced for after tx", pkt);
				net_pkt_unref(pkt);
				*(data->tx_pkts + ((dma_chnl * dma_chnl_cfg->tdrl) + desc_idx)) =
					(mem_addr_t)NULL;
			}
			tx_desc->tdes0.dword = 0u;
			tx_desc->tdes1.dword = 0u;
			tx_desc->tdes2.dword = 0u;
			tx_desc->tdes3.dword = 0u;
			k_sem_give(&tx_desc_meta->free_tx_descs_sem);
		} else {
			break;
		}
	}
}

static void eth_dwc_xgmac_dmach_isr(const struct device *dev, uint32_t dmach_interrupt_sts,
				    uint32_t dma_chnl)
{
	if (dmach_interrupt_sts & DMA_CHx_STATUS_TI_SET_MSK) {
		/* Tranmit interrupt */
		eth_dwc_xgmac_tx_irq_work(dev, dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_RI_SET_MSK) {
		/* Receive interrupt */
		eth_dwc_xgmac_rx_irq_work(dev, dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_TPS_SET_MSK) {
		/* Transmit process stopped interrupt*/
		LOG_ERR("DMA channel %d Transmit process stopped", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_TBU_SET_MSK) {
		/* Transmit buffer unavailable interrupt*/
		LOG_ERR("DMA channel %d Transmit buffer unavailable", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_RBU_SET_MSK) {
		/* Receive buffer unavailable interrupt*/
		LOG_ERR("DMA channel %d Receive buffer unavailable", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_RPS_SET_MSK) {
		/* Receive process stopped interrupt*/
		LOG_ERR("DMA channel %d Receive process stopped", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_DDE_SET_MSK) {
		/* Descriptor definition error interrupt*/
		LOG_ERR("DMA channel %d  Descriptor definition error", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_FBE_SET_MSK) {
		/* Fatal bus error interrupt*/
		LOG_ERR("DMA channel %d Fatal bus error", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_CDE_SET_MSK) {
		/* Context descriptor error interrupt*/
		LOG_ERR("DMA channel %d Context descriptor error", dma_chnl);
	}
	if (dmach_interrupt_sts & DMA_CHx_STATUS_AIS_SET_MSK) {
		/* Abnormal interrupt status interrupt*/
		LOG_ERR("DMA channel %d Abnormal error", dma_chnl);
	}
}

static inline void eth_dwc_xgmac_mtl_isr(const struct device *dev, uint32_t mtl_interrupt_sts)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mtl_interrupt_sts);
	/*Handle MTL interrupts*/
}

static inline void eth_dwc_xgmac_mac_isr(const struct device *dev, uint32_t mac_interrupt_sts)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mac_interrupt_sts);
	/*Handle MAC interrupts*/
}

#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
/**
 * @brief Handler function for bottom half processing which got
 * submitted to work queue in the interrupt handler.
 *
 * @param item Pointer to the work item
 */
static void eth_dwc_xgmac_irq_work(struct k_work *item)
{
	struct eth_dwc_xgmac_dev_data *const data =
		CONTAINER_OF(item, struct eth_dwc_xgmac_dev_data, isr_work);
	struct xgmac_irq_cntxt_data *cntxt_data =
		(struct xgmac_irq_cntxt_data *)&data->irq_cntxt_data;
	const struct device *dev = cntxt_data->dev;
	const struct eth_dwc_xgmac_config *const config =
		(struct eth_dwc_xgmac_config *)dev->config;
	uint32_t dma_chnl_interrupt_sts = 0u;

	for (uint32_t x = 0; x < config->num_dma_chnl; x++) {
		if (cntxt_data->dma_interrupt_sts & BIT(x)) {
			dma_chnl_interrupt_sts = cntxt_data->dma_chnl_interrupt_sts[x];
			cntxt_data->dma_chnl_interrupt_sts[x] ^= dma_chnl_interrupt_sts;
			eth_dwc_xgmac_dmach_isr(dev, dma_chnl_interrupt_sts, x);
			WRITE_BIT(cntxt_data->dma_interrupt_sts, x, 0);
		}
	}
}
#endif
/**
 * @brief XGMAC interrupt service routine
 * XGMAC interrupt service routine. Checks for indications of errors
 * and either immediately handles RX pending / TX complete notifications
 * or defers them to the system work queue.
 *
 * @param dev Pointer to the ethernet device
 */
static void eth_dwc_xgmac_isr(const struct device *dev)
{
	const struct eth_dwc_xgmac_config *const config =
		(struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *const data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	struct xgmac_irq_cntxt_data *cntxt_data =
		(struct xgmac_irq_cntxt_data *)&data->irq_cntxt_data;
	uint32_t dma_int_status = 0u;
	uint32_t dmach_interrupt_sts = 0u;
	mem_addr_t ioaddr = get_reg_base_addr(dev);
	mem_addr_t reg_addr;
	uint32_t reg_val;

	if (!data->dev_started || data->link_speed == LINK_DOWN ||
	    (!net_if_flag_is_set(data->iface, NET_IF_UP))) {
		dma_int_status =
			sys_read32(ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_INTERRUPT_STATUS_OFST);
		for (int x = 0; x < config->num_dma_chnl; x++) {
			if (dma_int_status & BIT(x)) {
				reg_val = DMA_CHx_STATUS_NIS_SET_MSK | DMA_CHx_STATUS_AIS_SET_MSK |
					  DMA_CHx_STATUS_CDE_SET_MSK | DMA_CHx_STATUS_FBE_SET_MSK |
					  DMA_CHx_STATUS_DDE_SET_MSK | DMA_CHx_STATUS_RPS_SET_MSK |
					  DMA_CHx_STATUS_RBU_SET_MSK | DMA_CHx_STATUS_TBU_SET_MSK |
					  DMA_CHx_STATUS_TPS_SET_MSK | DMA_CHx_STATUS_RI_SET_MSK |
					  DMA_CHx_STATUS_TI_SET_MSK;
				sys_write32(reg_val, (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(x) +
						      DMA_CHx_STATUS_OFST));
			}
		}

		LOG_ERR("%s ignoring xgmac interrupt: device not started,link is down or network "
			"interface is not up",
			dev->name);

		return;
	}

	/* Interrupt Top half processing
	 */
	reg_addr = (ioaddr + XGMAC_DMA_BASE_ADDR_OFFSET + DMA_INTERRUPT_STATUS_OFST);
	/* Only set the interrupt, do not overwrite the interrupt status stored in the context.
	 * The status will be cleared once the corresponding action is completed in the work item
	 */
	cntxt_data->dma_interrupt_sts |= sys_read32(reg_addr);
	for (int x = 0; x < config->num_dma_chnl; x++) {
		if (cntxt_data->dma_interrupt_sts & BIT(x)) {
			reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(x) +
				    DMA_CHx_STATUS_OFST);
			dmach_interrupt_sts = sys_read32(reg_addr);
			sys_write32(dmach_interrupt_sts, reg_addr);
#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
			/* Only set the interrupt, do not overwrite the interrupt status stored in
			 * the context. The status will be cleared once the corresponding action is
			 * done in the work item
			 */
			cntxt_data->dma_chnl_interrupt_sts[x] |= dmach_interrupt_sts;
#else
			eth_dwc_xgmac_dmach_isr(dev, dmach_interrupt_sts, x);
			WRITE_BIT(cntxt_data->dma_interrupt_sts, x, 0);
#endif
		}
	}

	reg_addr = (ioaddr + XGMAC_MTL_BASE_ADDR_OFFSET + MTL_INTERRUPT_STATUS_OFST);
	reg_val = sys_read32(reg_addr);
#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
	cntxt_data->mtl_interrupt_sts |= reg_val;
#else
	eth_dwc_xgmac_mtl_isr(dev, reg_val);
#endif

	reg_addr = (ioaddr + XGMAC_CORE_BASE_ADDR_OFFSET + CORE_MAC_INTERRUPT_STATUS_OFST);
	reg_val = sys_read32(reg_addr);
#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
	cntxt_data->mac_interrupt_sts |= reg_val;
#else
	eth_dwc_xgmac_mac_isr(dev, reg_val);
#endif

	/* submitting work item for interrupt bottom half processing.
	 * Checking if already a work item is waiting in the work queue to process the interrupt
	 * flags
	 */
#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
	k_work_submit(&data->isr_work);
#endif
}
/**
 * @brief This is the expiry function which will be called from the
 * system timer irq handler upon configured isr polling timer expires.
 *
 * @param timer Pointer to the timer object
 */
#ifdef CONFIG_ETH_DWC_XGMAC_POLLING_MODE
static void eth_dwc_xgmac_irq_poll(struct k_timer *timer)
{
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)CONTAINER_OF(
		timer, struct eth_dwc_xgmac_dev_data, isr_polling_timer);
	const struct device *dev = dev_data->irq_cntxt_data.dev;

	eth_dwc_xgmac_isr(dev);
}
#endif /* CONFIG_ETH_DWC_XGMAC_POLLING_MODE */

/**
 * @brief XGMAC device initialization function
 * Initializes the XGMAC itself, the DMA memory area used by the XGMAC.
 *
 * @param dev Pointer to the ethernet device
 * @retval 0 if the device initialization completed successfully
 */
static int eth_dwc_xgmac_dev_init(const struct device *dev)
{
	const struct eth_dwc_xgmac_config *const config =
		(struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *const data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	mem_addr_t ioaddr;
	int ret = 0;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

	if (config->reset.dev != NULL) {
		if (!device_is_ready(config->reset.dev)) {
			LOG_ERR("Reset device is not ready");
			return -ENODEV;
		}
		ret = reset_line_toggle(config->reset.dev, config->reset.id);
		if (ret) {
			LOG_ERR("failed to reset peripheral");
			return ret;
		}
	}

#endif
	ioaddr = get_reg_base_addr(dev);

	/*
	 * Initialization procedure as described in the dwc xgmac 10G Ethernet MAC data book.
	 */
	ret = dwxgmac_software_reset(ioaddr);
	if (ret) {
		LOG_ERR("MAC reset timeout");
		return ret;
	}

	dwxgmac_dma_init(ioaddr, &config->dma_cfg);

	dwxgmac_dma_desc_init(config, data);

	dwxgmac_dma_chnl_init(ioaddr, config, data);

	dwxgmac_dma_mtl_init(ioaddr, config);

	dwxgmac_mac_init(ioaddr, config, data);

	/* set MAC address */
	dwxgmac_set_mac_addr_by_idx(ioaddr, data->mac_addr, 0, false);

	dwxgmac_irq_init(dev);
	LOG_INF("XGMAC ethernet driver init done");
	return 0;
}

static void phy_link_state_change_callback(const struct device *phy_dev,
					   struct phy_link_state *state, void *user_data)
{
	ARG_UNUSED(phy_dev);
	const struct device *mac_dev = (const struct device *)user_data;
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)mac_dev->data;
	bool is_up = state->is_up;

	if (is_up) {
		LOG_INF("Link up");
		/* Announce link up status */
		switch (state->speed) {
		case LINK_HALF_1000BASE_T:
		case LINK_FULL_1000BASE_T:
			dev_data->link_speed = LINK_1GBIT;
			break;
		case LINK_HALF_100BASE_T:
		case LINK_FULL_100BASE_T:
			dev_data->link_speed = LINK_100MBIT;
			break;
		case LINK_HALF_10BASE_T:
		case LINK_FULL_10BASE_T:
		default:
			dev_data->link_speed = LINK_10MBIT;
		}
		/* Configure MAC link speed */
		eth_dwc_xgmac_update_link_speed(get_reg_base_addr(mac_dev), dev_data->link_speed);
		net_eth_carrier_on(dev_data->iface);
		/* Set up link */

	} else {
		LOG_INF("Link down");
		dev_data->link_speed = LINK_DOWN;
		/* Announce link down status */
		net_eth_carrier_off(dev_data->iface);
	}
}

/**
 * @brief XGMAC associated interface initialization function
 * Initializes the interface associated with a XGMAC device.
 *
 * @param iface Pointer to the associated interface data struct
 */
static void eth_dwc_xgmac_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_dwc_xgmac_config *const dev_conf =
		(struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *const dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;

	k_mutex_init(&dev_data->dev_cfg_lock);
#ifdef CONFIG_ETH_DWC_XGMAC_BOTTOM_HALF_WORK_QUEUE
	/*
	 * Initialize the (delayed) work item for RX pending, TX done
	 */
	k_work_init(&(dev_data->isr_work), eth_dwc_xgmac_irq_work);
#endif

#ifdef CONFIG_ETH_DWC_XGMAC_POLLING_MODE
	k_timer_init(&dev_data->isr_polling_timer, eth_dwc_xgmac_irq_poll, NULL);
#else
	(dev_conf->irq_config_fn)(dev);
#endif

	{
		/*
		 * Every RX descriptor in the descriptor ring needs to be prefilled with 2 RX buffer
		 * addresses by the MAC driver S/W and OWN bit will be set to 1. When new data is
		 * received the DMA will check the OWN bit and moves the data to corresponding
		 * recive buffers and clears the descripot OWN bit. If received data size is more
		 * than total of 2 buffer sizes  then DMA will use next descriptor in the ring.
		 */
		struct xgmac_dma_chnl_config *const dma_chnl_cfg =
			(struct xgmac_dma_chnl_config *)&dev_conf->dma_chnl_cfg;
		struct xgmac_dma_tx_desc_meta *tx_desc_meta;
		struct xgmac_dma_rx_desc_meta *rx_desc_meta;
		struct xgmac_dma_rx_desc *rx_desc = NULL;
		mem_addr_t reg_addr;
		uint32_t reg_val;
		mem_addr_t ioaddr;
		mem_addr_t *rx_frags;
		uint16_t desc_id = 0u;

		ioaddr = get_reg_base_addr(dev);
		/* Reserve the RX buffers and fill the RX descriptors with buffer addresses*/
		for (int dma_chnl = 0u; dma_chnl < dev_conf->num_dma_chnl; dma_chnl++) {
			tx_desc_meta =
				(struct xgmac_dma_tx_desc_meta *)&dev_data->tx_desc_meta[dma_chnl];
			rx_desc_meta =
				(struct xgmac_dma_rx_desc_meta *)&dev_data->rx_desc_meta[dma_chnl];
			/*
			 * Initialize semaphores and mutex for the RX/TX descriptor rings
			 */
			k_sem_init(&tx_desc_meta->free_tx_descs_sem, (dma_chnl_cfg->tdrl),
				   (dma_chnl_cfg->tdrl));
			k_mutex_init(&tx_desc_meta->ring_lock);

			for (; desc_id < dma_chnl_cfg->rdrl; desc_id++) {
				rx_desc = (struct xgmac_dma_rx_desc *)(dev_data->dma_rx_desc +
								       (dma_chnl *
									dma_chnl_cfg->rdrl) +
								       desc_id);
				rx_frags = (mem_addr_t *)(dev_data->rx_frags +
							  (((dma_chnl * dma_chnl_cfg->rdrl) +
							    desc_id) *
							   RX_FRAGS_PER_DESC));
				rx_frags[0u] = (mem_addr_t)net_pkt_get_reserve_rx_data(
					CONFIG_NET_BUF_DATA_SIZE, K_FOREVER);
				if (!rx_frags[0u]) {
					LOG_ERR("Failed to allocate a network buffer to fill the "
						"DMA descriptor");
					break;
				}
				rx_desc->rdes0.dword =
					POINTER_TO_UINT(((struct net_buf *)rx_frags[0u])->data);
				rx_desc->rdes1.dword =
					POINTER_TO_UINT(((struct net_buf *)rx_frags[0u])->data) >>
					32u;
				rx_frags[1u] = (mem_addr_t)net_pkt_get_reserve_rx_data(
					CONFIG_NET_BUF_DATA_SIZE, K_FOREVER);
				if (!rx_frags[1u]) {
					net_pkt_frag_unref((struct net_buf *)(rx_frags[0u]));
					LOG_ERR("Failed to allocate a network buffer to fill the "
						"DMA descriptor");
					break;
				}
				rx_desc->rdes2.dword =
					POINTER_TO_UINT(((struct net_buf *)rx_frags[1u])->data);
				rx_desc->rdes3.dword =
					XGMAC_RDES3_OWN | XGMAC_RDES3_IOC |
					(POINTER_TO_UINT(((struct net_buf *)rx_frags[1u])->data) >>
					 32u);
				rx_desc_meta->desc_tail_addr = (mem_addr_t)(rx_desc + 1);
			}

			reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
				    DMA_CHx_RXDESC_TAIL_LPOINTER_OFST);
			reg_val =
				DMA_CHx_RXDESC_TAIL_LPOINTER_RDT_SET(rx_desc_meta->desc_tail_addr);
			sys_write32(reg_val, reg_addr);
		}
	}

	/* Set the initial contents of the current instance's run-time data */
	dev_data->iface = iface;
	(void)net_if_set_link_addr(iface, dev_data->mac_addr, ETH_MAC_ADDRESS_SIZE,
				   NET_LINK_ETHERNET);
	net_if_carrier_off(iface);
	ethernet_init(iface);
	net_if_set_mtu(iface, dev_conf->mtu);
	if (device_is_ready(dev_conf->phy_dev)) {
		phy_link_callback_set(dev_conf->phy_dev, &phy_link_state_change_callback,
				      (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}
	LOG_INF("Ethernet iface init done");
}

/**
 * @brief XGMAC device start function
 * XGMAC device start function. Clears all status registers and any
 * pending interrupts, enables RX and TX, enables interrupts.
 *
 * @param dev Pointer to the ethernet device
 * @retval    0 upon successful completion
 */
static int eth_dwc_xgmac_start_device(const struct device *dev)
{
	const struct eth_dwc_xgmac_config *dev_conf = (struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	mem_addr_t ioaddr;
	mem_addr_t reg_addr;
	uint32_t reg_val;

	if (dev_data->dev_started) {
		LOG_INF("Eth device already started");
		return 0;
	}

	ioaddr = get_reg_base_addr(dev);

	for (int dma_chnl = 0u; dma_chnl < dev_conf->num_dma_chnl; dma_chnl++) {
		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TX_CONTROL_OFST);
		reg_val = sys_read32(reg_addr) | DMA_CHx_TX_CONTROL_ST_SET_MSK;
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RX_CONTROL_OFST);
		reg_val = sys_read32(reg_addr) | DMA_CHx_RX_CONTROL_SR_SET_MSK;
		sys_write32(reg_val, reg_addr);

		/* enable the channel dma channel interrupts*/
		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_INTERRUPT_ENABLE_OFST);
		reg_val = DMA_CHx_INTERRUPT_ENABLE_NIE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_AIE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_CDEE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_FBEE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_DDEE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_RSE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_RBUE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_RIE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_TBUE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_TXSE_SET(1u) |
			  DMA_CHx_INTERRUPT_ENABLE_TIE_SET(1u);
		sys_write32(reg_val, reg_addr);
	}

	reg_val = sys_read32(ioaddr + CORE_MAC_TX_CONFIGURATION_OFST);
	reg_val |= CORE_MAC_TX_CONFIGURATION_TE_SET(1u);
	sys_write32(reg_val, (ioaddr + CORE_MAC_TX_CONFIGURATION_OFST));

	reg_val = sys_read32(ioaddr + CORE_MAC_RX_CONFIGURATION_OFST);
	reg_val |= CORE_MAC_RX_CONFIGURATION_RE_SET(1u);
	sys_write32(reg_val, (ioaddr + CORE_MAC_RX_CONFIGURATION_OFST));

	reg_val = sys_read32(ioaddr + CORE_MAC_INTERRUPT_ENABLE_OFST);
	reg_val = CORE_MAC_INTERRUPT_ENABLE_LSIE_SET(1u);
	sys_write32(reg_val, (ioaddr + CORE_MAC_INTERRUPT_ENABLE_OFST));

#ifdef CONFIG_ETH_DWC_XGMAC_POLLING_MODE
	k_timer_start(&dev_data->isr_polling_timer,
		      K_USEC(CONFIG_ETH_DWC_XGMAC_INTERRUPT_POLLING_INTERVAL_US),
		      K_USEC(CONFIG_ETH_DWC_XGMAC_INTERRUPT_POLLING_INTERVAL_US));
#else
	(dev_conf->irq_enable_fn)(dev, true);
#endif

	dev_data->dev_started = true;
	return 0;
}

/**
 * @brief XGMAC device stop function
 * XGMAC device stop function. Disables all interrupts, disables
 * RX and TX, clears all status registers.
 *
 * @param dev Pointer to the ethernet device
 * @retval    0 upon successful completion
 */
static int eth_dwc_xgmac_stop_device(const struct device *dev)
{
	const struct eth_dwc_xgmac_config *dev_conf = (struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	mem_addr_t ioaddr;
	mem_addr_t reg_addr;
	uint32_t reg_val;

	if (!dev_data->dev_started) {
		LOG_INF("Eth device already stopped");
		return 0;
	}
	dev_data->dev_started = false;

	ioaddr = get_reg_base_addr(dev);

	for (int dma_chnl = 0; dma_chnl < dev_conf->num_dma_chnl; dma_chnl++) {
		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_TX_CONTROL_OFST);
		reg_val = sys_read32(reg_addr) & DMA_CHx_TX_CONTROL_ST_CLR_MSK;
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_RX_CONTROL_OFST);
		reg_val = sys_read32(reg_addr) & DMA_CHx_RX_CONTROL_SR_CLR_MSK;
		sys_write32(reg_val, reg_addr);

		reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
			    DMA_CHx_INTERRUPT_ENABLE_OFST);
		reg_val = 0u;
		sys_write32(reg_val, reg_addr);
	}

	reg_val = sys_read32(ioaddr + CORE_MAC_TX_CONFIGURATION_OFST);
	reg_val &= CORE_MAC_TX_CONFIGURATION_TE_CLR_MSK;
	sys_write32(reg_val, (ioaddr + CORE_MAC_TX_CONFIGURATION_OFST));

	reg_val = sys_read32(ioaddr + CORE_MAC_RX_CONFIGURATION_OFST);
	reg_val &= CORE_MAC_RX_CONFIGURATION_RE_CLR_MSK;
	sys_write32(reg_val, (ioaddr + CORE_MAC_RX_CONFIGURATION_OFST));

	reg_addr = (ioaddr + CORE_MAC_INTERRUPT_ENABLE_OFST);
	reg_val = 0u;
	sys_write32(reg_val, reg_addr);

#ifdef CONFIG_ETH_DWC_XGMAC_POLLING_MODE
	k_timer_stop(&dev_data->isr_polling_timer);
#else
	(dev_conf->irq_enable_fn)(dev, false);
#endif

	return 0;
}

static inline void update_desc_tail_ptr(const struct device *dev, uint8_t dma_chnl,
					uint32_t desc_tail_addr)
{
	mem_addr_t reg_addr, ioaddr = get_reg_base_addr(dev);
	uint32_t reg_val;

	reg_addr = (ioaddr + XGMAC_DMA_CHNLx_BASE_ADDR_OFFSET(dma_chnl) +
		    DMA_CHx_TXDESC_TAIL_LPOINTER_OFST);
	reg_val = DMA_CHx_TXDESC_TAIL_LPOINTER_TDT_SET(desc_tail_addr);
	sys_write32(reg_val, reg_addr);
}
/**
 * @brief XGMAC data send function
 * XGMAC data send function. Blocks until a TX complete notification has been
 * received & processed.
 *
 * @param dev Pointer to the ethernet device
 * @param packet Pointer to the data packet to be sent
 * @retval -EINVAL in case of invalid parameters, e.g. zero data length
 * @retval -EIO in case of:
 *         (1) the attempt to TX data while the device is stopped,
 *             the interface is down or the link is down,
 * @retval -ETIMEDOUT in case of:
 *         (1) the attempt to TX data while no free buffers are available
 *             in the DMA memory area,
 *         (2) the transmission completion notification timing out
 * @retval -EBUSY in case of:
 *         (1) the Tx desc ring lock is acquired within timeout
 * @retval 0 if the packet was transmitted successfully

 */
static int eth_dwc_xgmac_send(const struct device *dev, struct net_pkt *pkt)
{
	int ret;
	struct xgmac_tx_cntxt context;
	const struct eth_dwc_xgmac_config *dev_conf = (struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	struct xgmac_dma_chnl_config *dma_ch_cfg =
		(struct xgmac_dma_chnl_config *)&dev_conf->dma_chnl_cfg;
	uint32_t tdes2_flgs, tdes3_flgs, tdes3_fd_flg;

	if (!pkt || !pkt->frags) {
		LOG_ERR("cannot TX, invalid argument");
		return -EINVAL;
	}

	if (net_pkt_get_len(pkt) == 0) {
		LOG_ERR("%s cannot TX, zero packet length", dev->name);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.errors.tx++;
#endif
		return -EINVAL;
	}

	if (!dev_data->dev_started || dev_data->link_speed == LINK_DOWN ||
	    (!net_if_flag_is_set(dev_data->iface, NET_IF_UP))) {
		LOG_ERR("%s cannot TX, due to any of these reasons, device not started,link is "
			"down or network interface is not up",
			dev->name);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.tx_dropped++;
#endif
		return -EIO;
	}

	context.q_id = net_tx_priority2tc(net_pkt_priority(pkt));
	context.descmeta = (struct xgmac_dma_tx_desc_meta *)&dev_data->tx_desc_meta[context.q_id];
	context.pkt_desc_id = context.descmeta->next_to_use;
	/*lock the TX desc ring while acquiring the resources*/
	(void)k_mutex_lock(&(context.descmeta->ring_lock), K_FOREVER);
	(void)net_pkt_ref(pkt);
	LOG_DBG("dwc xgmac: %p packet referanced for tx", pkt);
	tdes3_fd_flg = XGMAC_TDES3_FD;
	for (struct net_buf *frag = pkt->frags; frag; frag = frag->frags) {
		ret = k_sem_take(&context.descmeta->free_tx_descs_sem, K_MSEC(1));
		if (ret != 0) {
			LOG_DBG("enough free tx descriptors are not available");
			goto abort_tx;
		}
		context.tx_desc = (struct xgmac_dma_tx_desc *)(dev_data->dma_tx_desc +
							       (context.q_id * dma_ch_cfg->tdrl) +
							       context.pkt_desc_id);

		context.tx_desc->tdes0.buf1ap = (uint32_t)POINTER_TO_UINT(frag->data);
		context.tx_desc->tdes1.buf2ap = (uint32_t)(POINTER_TO_UINT(frag->data) >> 32u);
		tdes2_flgs = frag->len;
		tdes3_flgs = XGMAC_TDES3_OWN | tdes3_fd_flg |
#ifdef CONFIG_ETH_DWC_XGMAC_TX_CS_OFFLOAD
			     XGMAC_TDES3_CS_EN_MSK |
#endif
			     net_pkt_get_len(pkt);
		tdes3_fd_flg = 0;

		if (!frag->frags) { /* check last fragment of the packet */
			/* Set interrupt on completion for last fragment descriptor */
			tdes3_flgs |= XGMAC_TDES3_LD;
			tdes2_flgs |= XGMAC_TDES2_IOC;
			/* pin the transmitted packet address to unpin after getting transmitted by
			 * HW
			 */
			*(dev_data->tx_pkts + ((context.q_id * dma_ch_cfg->tdrl) +
					       context.pkt_desc_id)) = (mem_addr_t)pkt;
			context.descmeta->desc_tail_addr =
				(mem_addr_t)POINTER_TO_UINT(context.tx_desc + 1);
		}

		context.tx_desc->tdes2.dword = tdes2_flgs;
		context.tx_desc->tdes3.dword = tdes3_flgs;
		context.pkt_desc_id = ((context.pkt_desc_id + 1) % dma_ch_cfg->tdrl);
	}
	context.descmeta->next_to_use = context.pkt_desc_id;

	if (context.descmeta->desc_tail_addr ==
	    (mem_addr_t)POINTER_TO_UINT(
		    (struct xgmac_dma_tx_desc *)(dev_data->dma_tx_desc +
						 (context.q_id * dma_ch_cfg->tdrl) +
						 dma_ch_cfg->tdrl))) {
		context.descmeta->desc_tail_addr = (mem_addr_t)POINTER_TO_UINT(
			(struct xgmac_dma_tx_desc *)(dev_data->dma_tx_desc +
						     (context.q_id * dma_ch_cfg->tdrl)));
	}

	/*Update the descriptor tail pointer to DMA channel*/
	update_desc_tail_ptr(dev, context.q_id, (uint32_t)context.descmeta->desc_tail_addr);
	/*unlock the TX desc ring*/
	(void)k_mutex_unlock(&(context.descmeta->ring_lock));

#ifdef CONFIG_NET_STATISTICS_ETHERNET
	dev_data->stats.bytes.sent += net_pkt_get_len(pkt);
	dev_data->stats.pkts.tx++;
#endif

	return 0;

abort_tx:
	/*Aborting the packet transmission and return error code*/
	for (uint16_t desc_id = context.descmeta->next_to_use; desc_id != context.pkt_desc_id;
	     desc_id = ((desc_id + 1) % dma_ch_cfg->tdrl)) {
		context.tx_desc =
			(struct xgmac_dma_tx_desc *)(dev_data->dma_tx_desc +
						     (context.q_id * dma_ch_cfg->tdrl) + desc_id);
		context.tx_desc->tdes0.dword = 0u;
		context.tx_desc->tdes1.dword = 0u;
		context.tx_desc->tdes2.dword = 0u;
		context.tx_desc->tdes3.dword = 0u;
		k_sem_give(&context.descmeta->free_tx_descs_sem);
	}
	(void)k_mutex_unlock(&(context.descmeta->ring_lock));
	LOG_DBG("dwc xgmac: %p packet unreferenced after dropping", pkt);
	net_pkt_unref(pkt);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	dev_data->stats.tx_dropped++;
#endif
	return -EIO;
}

static enum phy_link_speed get_phy_adv_speeds(bool auto_neg, bool duplex_mode,
					      enum eth_dwc_xgmac_link_speed link_speed)
{
	enum phy_link_speed adv_speeds = 0u;

	if (auto_neg) {
		adv_speeds = LINK_HALF_1000BASE_T | LINK_HALF_1000BASE_T | LINK_HALF_100BASE_T |
			     LINK_FULL_100BASE_T | LINK_HALF_10BASE_T | LINK_FULL_10BASE_T;

	} else {
		if (duplex_mode) {
			switch (link_speed) {
			case LINK_1GBIT:
				adv_speeds = LINK_FULL_1000BASE_T;
				break;
			case LINK_100MBIT:
				adv_speeds = LINK_FULL_100BASE_T;
				break;
			default:
				adv_speeds = LINK_FULL_10BASE_T;
			}
		} else {
			switch (link_speed) {
			case LINK_1GBIT:
				adv_speeds = LINK_HALF_1000BASE_T;
				break;
			case LINK_100MBIT:
				adv_speeds = LINK_HALF_100BASE_T;
				break;
			default:
				adv_speeds = LINK_HALF_10BASE_T;
			}
		}
	}
	return adv_speeds;
}
#ifdef CONFIG_ETH_DWC_XGMAC_HW_FILTERING
static inline int get_free_mac_addr_indx(mem_addr_t ioaddr)
{
	mem_addr_t reg_addr;
	uint32_t reg_val;

	for (int idx = 1u; idx < XGMAC_MAX_MAC_ADDR_COUNT; idx++) {
		reg_addr = (ioaddr + XGMAC_CORE_ADDRx_HIGH(idx));
		reg_val = sys_read32(reg_addr);
		if (!(reg_val & CORE_MAC_ADDRESS1_HIGH_AE_SET_MSK)) {
			return idx;
		}
	}
	LOG_ERR("MAC address filter failed. All MAC address slots are in use");
	return -EIO;
}

static inline void disable_filter_for_mac_addr(mem_addr_t ioaddr, uint8_t *addr)
{
	mem_addr_t reg_addr;

	for (int idx = 1u; idx < XGMAC_MAX_MAC_ADDR_COUNT; idx++) {
		reg_addr = (ioaddr + XGMAC_CORE_ADDRx_HIGH(idx) + 2u);
		if (!(memcmp((uint8_t *)reg_addr, addr, 6u))) {
			sys_write32(CORE_MAC_ADDRESS1_HIGH_AE_CLR_MSK, XGMAC_CORE_ADDRx_HIGH(idx));
			sys_write32(CORE_MAC_ADDRESS1_LOW_ADDRLO_SET_MSK,
				    XGMAC_CORE_ADDRx_LOW(idx));
		}
	}
}
#endif
/**
 * @brief XGMAC set config function
 * XGMAC set config function facilitates to update the existing MAC settings
 *
 * @param dev Pointer to the ethernet device
 * @param type Type of configuration
 * @param config Pointer to configuration data
 * @retval 0 configuration updated successfully
 * @retval -EALREADY in case of:
 *          (1) if existing configuration is equals to input configuration
 *         -ENOTSUP for invalid config type
 */
static int eth_dwc_xgmac_set_config(const struct device *dev, enum ethernet_config_type type,
				    const struct ethernet_config *config)
{
	const struct eth_dwc_xgmac_config *dev_conf = (struct eth_dwc_xgmac_config *)dev->config;
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;
	const struct device *phy = dev_conf->phy_dev;
	const struct ethphy_driver_api *phy_api = phy->api;
	enum phy_link_speed adv_speeds;

	mem_addr_t ioaddr = get_reg_base_addr(dev);
	int retval = 0;

	(void)k_mutex_lock(&dev_data->dev_cfg_lock, K_FOREVER);
	switch (type) {
	case ETHERNET_CONFIG_TYPE_AUTO_NEG:
		if (dev_data->auto_neg != config->auto_negotiation) {
			dev_data->auto_neg = config->auto_negotiation;
			adv_speeds =
				get_phy_adv_speeds(dev_data->auto_neg, dev_data->enable_full_duplex,
						   dev_data->link_speed);
			retval = phy_api->cfg_link(phy, adv_speeds);
		} else {
			retval = -EALREADY;
		}
		break;
	case ETHERNET_CONFIG_TYPE_LINK:
		if ((config->l.link_10bt && dev_data->link_speed == LINK_10MBIT) ||
		    (config->l.link_100bt && dev_data->link_speed == LINK_100MBIT) ||
		    (config->l.link_1000bt && dev_data->link_speed == LINK_1GBIT)) {
			retval = -EALREADY;
			break;
		}

		if (config->l.link_1000bt) {
			dev_data->link_speed = LINK_1GBIT;
		} else if (config->l.link_100bt) {
			dev_data->link_speed = LINK_100MBIT;
		} else if (config->l.link_10bt) {
			dev_data->link_speed = LINK_10MBIT;
		}
		adv_speeds = get_phy_adv_speeds(dev_data->auto_neg, dev_data->enable_full_duplex,
						dev_data->link_speed);
		retval = phy_api->cfg_link(phy, adv_speeds);
		break;
	case ETHERNET_CONFIG_TYPE_DUPLEX:
		if (config->full_duplex == dev_data->enable_full_duplex) {
			retval = -EALREADY;
			break;
		}
		dev_data->enable_full_duplex = config->full_duplex;

		adv_speeds = get_phy_adv_speeds(dev_data->auto_neg, dev_data->enable_full_duplex,
						dev_data->link_speed);
		retval = phy_api->cfg_link(phy, adv_speeds);
		break;
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, ETH_MAC_ADDRESS_SIZE);
		retval = net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
					      ETH_MAC_ADDRESS_SIZE, NET_LINK_ETHERNET);
		if (retval == 0) {
			dwxgmac_set_mac_addr_by_idx(ioaddr, dev_data->mac_addr, 0u, false);
		}
		break;
#ifdef CONFIG_ETH_DWC_XGMAC_PROMISCUOUS

	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode != dev_data->promisc_mode) {
			uint32_t reg_val = sys_read32(ioaddr + CORE_MAC_PACKET_FILTER_OFST);

			dev_data->promisc_mode = config->promisc_mode;
			reg_val &= CORE_MAC_PACKET_FILTER_PR_CLR_MSK;
			reg_val |= CORE_MAC_PACKET_FILTER_PR_SET(dev_data->promisc_mode);
			sys_write32(reg_val, ioaddr + CORE_MAC_PACKET_FILTER_OFST);
		} else {
			retval = -EALREADY;
		}
		break;

#endif

#ifdef CONFIG_ETH_DWC_XGMAC_HW_FILTERING

	case ETHERNET_CONFIG_TYPE_FILTER:
		if (!(config->filter.set)) {
			disable_filter_for_mac_addr(ioaddr,
						    (uint8_t *)config->filter.mac_address.addr);
		} else {
			int mac_idx = get_free_mac_addr_indx(ioaddr);

			if (mac_idx > 0u) {
				dwxgmac_set_mac_addr_by_idx(
					ioaddr, (uint8_t *)config->filter.mac_address.addr, mac_idx,
					config->filter.type);
			} else {
				retval = -EIO;
			}
		}
		break;

#endif

	default:
		retval = -ENOTSUP;
		break;
	}
	k_mutex_unlock(&dev_data->dev_cfg_lock);

	return retval;
}
/**
 * @brief XGMAC get config function
 * XGMAC get config function facilitates to read the existing MAC settings
 *
 * @param dev Pointer to the ethernet device
 * @param type Type of configuration
 * @param config Pointer to configuration data
 * @retval 0 get configuration successful
 *         -ENOTSUP for invalid config type
 */
static int eth_dwc_xgmac_get_config(const struct device *dev, enum ethernet_config_type type,
				    struct ethernet_config *config)
{
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_AUTO_NEG:
		config->auto_negotiation = dev_data->auto_neg;
		break;
	case ETHERNET_CONFIG_TYPE_LINK:
		if (dev_data->link_speed == LINK_1GBIT) {
			config->l.link_1000bt = true;
		} else if (dev_data->link_speed == LINK_100MBIT) {
			config->l.link_100bt = true;
		} else if (dev_data->link_speed == LINK_10MBIT) {
			config->l.link_10bt = true;
		}
		break;
	case ETHERNET_CONFIG_TYPE_DUPLEX:
		config->full_duplex = dev_data->enable_full_duplex;
		break;
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(config->mac_address.addr, dev_data->mac_addr, 6);
		break;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		config->promisc_mode = dev_data->promisc_mode;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief XGMAC capability request function
 * Returns the capabilities of the XGMAC controller as an enumeration.
 * All of the data returned is derived from the device configuration
 * of the current XGMAC device instance.
 *
 * @param dev Pointer to the ethernet device
 * @return Enumeration containing the current XGMAC device's capabilities
 */
static enum ethernet_hw_caps eth_dwc_xgmac_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	enum ethernet_hw_caps caps = (enum ethernet_hw_caps)0;

	caps = (ETHERNET_LINK_1000BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_LINK_10BASE_T |
		ETHERNET_AUTO_NEGOTIATION_SET | ETHERNET_DUPLEX_SET);

#ifdef CONFIG_ETH_DWC_XGMAC_RX_CS_OFFLOAD
	caps |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
#endif

#ifdef CONFIG_ETH_DWC_XGMAC_TX_CS_OFFLOAD
	caps |= ETHERNET_HW_TX_CHKSUM_OFFLOAD;
#endif

#ifdef CONFIG_ETH_DWC_XGMAC_PROMISCUOUS
	caps |= ETHERNET_PROMISC_MODE;
#endif

#ifdef CONFIG_ETH_DWC_XGMAC_HW_FILTERING
	caps |= ETHERNET_HW_FILTERING;
#endif

	return caps;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
/**
 * @brief XGMAC statistics data request function
 * Returns a pointer to the statistics data of the current XGMAC controller.
 *
 * @param dev Pointer to the ethernet device
 * @return Pointer to the current XGMAC device's statistics data
 */
static struct net_stats_eth *eth_dwc_xgmac_stats(const struct device *dev)
{
	struct eth_dwc_xgmac_dev_data *dev_data = (struct eth_dwc_xgmac_dev_data *)dev->data;

	return &dev_data->stats;
}
#endif

static const struct ethernet_api eth_dwc_xgmac_apis = {
	.iface_api.init = eth_dwc_xgmac_iface_init,
	.send = eth_dwc_xgmac_send,
	.start = eth_dwc_xgmac_start_device,
	.stop = eth_dwc_xgmac_stop_device,
	.get_capabilities = eth_dwc_xgmac_get_capabilities,
	.set_config = eth_dwc_xgmac_set_config,
	.get_config = eth_dwc_xgmac_get_config,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.get_stats = eth_dwc_xgmac_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

#define XGMAC_SNPS_DESIGNWARE_RESET_SPEC_INIT(port) .reset = RESET_DT_SPEC_INST_GET(port),

/* Interrupt configuration function macro */
#define ETH_DWC_XGMAC_CONFIG_IRQ_FUNC(port)                                                        \
	static void eth_dwc_xgmac##port##_irq_config(const struct device *dev)                     \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(port), DT_INST_IRQ(port, priority), eth_dwc_xgmac_isr,    \
			    DEVICE_DT_INST_GET(port), 0);                                          \
	}                                                                                          \
	static void eth_dwc_xgmac##port##_irq_enable(const struct device *dev, bool en)            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		en ? irq_enable(DT_INST_IRQN(port)) : irq_disable(DT_INST_IRQN(port));             \
	}                                                                                          \
	volatile uint32_t eth_dwc_xgmac##port##_dma_ch_int_status[DT_INST_PROP(port, num_dma_ch)];

#define ETH_DWC_XGMAC_ALLOC_DMA_DESC(port)                                                         \
	mem_addr_t eth_dwc_xgmac##port##_tx_pkts[DT_INST_PROP(port, num_dma_ch)]                   \
						[DT_INST_PROP(port, dma_ch_tdrl)];                 \
	mem_addr_t eth_dwc_xgmac##port##_rx_frags[DT_INST_PROP(port, num_dma_ch)][DT_INST_PROP(    \
		port, dma_ch_tdrl)][RX_FRAGS_PER_DESC];                                            \
	static struct xgmac_dma_rx_desc eth_dwc_xgmac##port##_rx_desc[DT_INST_PROP(                \
		port, num_dma_ch)][DT_INST_PROP(port, dma_ch_rdrl)] __aligned(32);                 \
	static struct xgmac_dma_tx_desc eth_dwc_xgmac##port##_tx_desc[DT_INST_PROP(                \
		port, num_dma_ch)][DT_INST_PROP(port, dma_ch_tdrl)] __aligned(32);                 \
	static struct xgmac_dma_rx_desc_meta                                                       \
		eth_dwc_xgmac##port##_rx_desc_meta[DT_INST_PROP(port, num_dma_ch)];                \
	static struct xgmac_dma_tx_desc_meta                                                       \
		eth_dwc_xgmac##port##_tx_desc_meta[DT_INST_PROP(port, num_dma_ch)];

#define DWC_XGMAC_NUM_QUEUES (DT_INST_PROP(port, num_queues) u)

#define ETH_DWC_XGMAC_DEV_CONFIG_TCQ(port)                                                         \
	static struct xgmac_tcq_config eth_dwc_xgmac##port##_tcq = {                               \
		.rx_q_ddma_en = DT_INST_PROP(port, rxq_dyn_dma_en),                                \
		.rx_q_dma_chnl_sel = DT_INST_PROP(port, rxq_dma_ch_sel),                           \
		.tx_q_size = DT_INST_PROP(port, txq_size),                                         \
		.q_to_tc_map = DT_INST_PROP(port, map_queue_tc),                                   \
		.ttc = DT_INST_PROP(port, tx_threshold_ctrl),                                      \
		.rx_q_size = DT_INST_PROP(port, rxq_size),                                         \
		.tsf_en = DT_INST_PROP(port, tx_store_fwrd_en),                                    \
		.hfc_en = DT_INST_PROP(port, hfc_en),                                              \
		.cs_err_pkt_drop_dis = DT_INST_PROP(port, cs_error_pkt_drop_dis),                  \
		.rsf_en = DT_INST_PROP(port, rx_store_fwrd_en),                                    \
		.fep_en = DT_INST_PROP(port, fep_en),                                              \
		.fup_en = DT_INST_PROP(port, fup_en),                                              \
		.rtc = DT_INST_PROP(port, rx_threshold_ctrl),                                      \
		.pstc = DT_INST_PROP(port, priorities_map_tc),                                     \
	};
/* Device run-time data declaration macro */
#define ETH_DWC_XGMAC_DEV_DATA(port)                                                               \
	static struct eth_dwc_xgmac_dev_data eth_dwc_xgmac##port##_dev_data = {                    \
		.mac_addr = DT_INST_PROP(port, local_mac_address),                                 \
		.link_speed = DT_INST_PROP(port, max_speed),                                       \
		.auto_neg = true,                                                                  \
		.enable_full_duplex = DT_INST_PROP(port, full_duplex_mode_en),                     \
		.dma_rx_desc = &eth_dwc_xgmac##port##_rx_desc[0u][0u],                             \
		.dma_tx_desc = &eth_dwc_xgmac##port##_tx_desc[0u][0u],                             \
		.tx_desc_meta = eth_dwc_xgmac##port##_tx_desc_meta,                                \
		.rx_desc_meta = eth_dwc_xgmac##port##_rx_desc_meta,                                \
		.tx_pkts = &eth_dwc_xgmac##port##_tx_pkts[0u][0u],                                 \
		.rx_frags = &eth_dwc_xgmac##port##_rx_frags[0u][0u][0u],                           \
		.irq_cntxt_data.dma_chnl_interrupt_sts = eth_dwc_xgmac##port##_dma_ch_int_status,  \
	};

/* Device configuration data declaration macro */

#define ETH_DWC_XGMAC_DEV_CONFIG(port)                                                             \
	static const struct eth_dwc_xgmac_config eth_dwc_xgmac##port##_dev_cfg = {                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(port)),                                           \
		.num_tx_Qs = DT_INST_PROP(port, num_tx_queues),                                    \
		.num_rx_Qs = DT_INST_PROP(port, num_rx_queues),                                    \
		.num_dma_chnl = DT_INST_PROP(port, num_dma_ch),                                    \
		.num_TCs = DT_INST_PROP(port, num_tc),                                             \
		.mtu = DT_INST_PROP(port, max_frame_size),                                         \
		.tx_fifo_size = DT_INST_PROP(port, tx_fifo_size),                                  \
		.rx_fifo_size = DT_INST_PROP(port, rx_fifo_size),                                  \
		.dma_cfg.wr_osr_lmt = DT_INST_PROP(port, wr_osr_lmt),                              \
		.dma_cfg.rd_osr_lmt = DT_INST_PROP(port, rd_osr_lmt),                              \
		.dma_cfg.edma_tdps = DT_INST_PROP(port, edma_tdps),                                \
		.dma_cfg.edma_rdps = DT_INST_PROP(port, edma_rdps),                                \
		.dma_cfg.ubl = DT_INST_PROP(port, ubl),                                            \
		.dma_cfg.blen4 = DT_INST_PROP(port, blen4),                                        \
		.dma_cfg.blen8 = DT_INST_PROP(port, blen8),                                        \
		.dma_cfg.blen16 = DT_INST_PROP(port, blen16),                                      \
		.dma_cfg.blen32 = DT_INST_PROP(port, blen32),                                      \
		.dma_cfg.blen64 = DT_INST_PROP(port, blen64),                                      \
		.dma_cfg.blen128 = DT_INST_PROP(port, blen128),                                    \
		.dma_cfg.blen256 = DT_INST_PROP(port, blen256),                                    \
		.dma_cfg.aal = DT_INST_PROP(port, aal),                                            \
		.dma_cfg.eame = DT_INST_PROP(port, eame),                                          \
		.dma_chnl_cfg.pblx8 = DT_INST_PROP(port, pblx8),                                   \
		.dma_chnl_cfg.mss = DT_INST_PROP(port, dma_ch_mss),                                \
		.dma_chnl_cfg.tdrl = DT_INST_PROP(port, dma_ch_tdrl),                              \
		.dma_chnl_cfg.rdrl = DT_INST_PROP(port, dma_ch_rdrl),                              \
		.dma_chnl_cfg.arbs = DT_INST_PROP(port, dma_ch_arbs),                              \
		.dma_chnl_cfg.rxpbl = DT_INST_PROP(port, dma_ch_rxpbl),                            \
		.dma_chnl_cfg.txpbl = DT_INST_PROP(port, dma_ch_txpbl),                            \
		.dma_chnl_cfg.sph = DT_INST_PROP(port, dma_ch_sph),                                \
		.dma_chnl_cfg.tse = DT_INST_PROP(port, dma_ch_tse),                                \
		.dma_chnl_cfg.osp = DT_INST_PROP(port, dma_ch_osp),                                \
		.mtl_cfg.raa = DT_INST_PROP(port, mtl_raa),                                        \
		.mtl_cfg.etsalg = DT_INST_PROP(port, mtl_etsalg),                                  \
		.mac_cfg.gpsl = DT_INST_PROP(port, gaint_pkt_size_limit),                          \
		.mac_cfg.arp_offload_en = ETH_DWC_XGMAC_ARP_OFFLOAD,                               \
		.mac_cfg.je = DT_INST_PROP(port, jumbo_pkt_en),                                    \
		.tcq_config = &eth_dwc_xgmac##port##_tcq,                                          \
		.phy_dev = (const struct device *)DEVICE_DT_GET(DT_CHILD(DT_DRV_INST(port), phy)), \
		.irq_config_fn = eth_dwc_xgmac##port##_irq_config,                                 \
		.irq_enable_fn = eth_dwc_xgmac##port##_irq_enable,                                 \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(port, resets),                                    \
			   (XGMAC_SNPS_DESIGNWARE_RESET_SPEC_INIT(port)))};

/* Device initialization macro */
#define ETH_DWC_XGMAC_NET_DEV_INIT(port)                                                           \
	ETH_NET_DEVICE_DT_INST_DEFINE(port, eth_dwc_xgmac_dev_init, NULL,                          \
				      &eth_dwc_xgmac##port##_dev_data,                             \
				      &eth_dwc_xgmac##port##_dev_cfg, CONFIG_ETH_INIT_PRIORITY,    \
				      &eth_dwc_xgmac_apis, DT_INST_PROP(port, max_frame_size));

/* Top-level device initialization macro - bundles all of the above */
#define ETH_DWC_XGMAC_INITIALIZE(port)                                                             \
	ETH_DWC_XGMAC_CONFIG_IRQ_FUNC(port)                                                        \
	ETH_DWC_XGMAC_ALLOC_DMA_DESC(port)                                                         \
	ETH_DWC_XGMAC_DEV_DATA(port)                                                               \
	ETH_DWC_XGMAC_DEV_CONFIG_TCQ(port)                                                         \
	ETH_DWC_XGMAC_DEV_CONFIG(port)                                                             \
	ETH_DWC_XGMAC_NET_DEV_INIT(port)

/*
 * Insert the configuration & run-time data for all XGMAC instances which
 * are enabled in the device tree of the current target board.
 */
DT_INST_FOREACH_STATUS_OKAY(ETH_DWC_XGMAC_INITIALIZE)
