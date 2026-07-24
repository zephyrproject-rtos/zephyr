/* TI AM62L CPSW3G Ethernet driver
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Author: Siddharth Vadapalli <s-vadapalli@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am62l_cpsw

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/nvmem.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(eth_ti_am62l_cpsw, CONFIG_ETHERNET_LOG_LEVEL);

#include "eth_ti_am62l_cpsw.h"

/* Register offset in bytes between subsequent GMII registers */
#define CPSW_GMII_SEL_REG_OFFSET 4U

/* Register offset in bytes between MAC Low and MAC High registers */
#define CPSW_MAC_LO_MAC_HI_OFFSET 4U

static const uint32_t cpsw_port_bases[CPSW_NUM_MAC_PORTS] = {
	CPSW_PORT1_BASE,
	CPSW_PORT2_BASE,
};

static const uint32_t cpsw_ale_portctls[CPSW_NUM_MAC_PORTS] = {
	ALE_PORTCTL1,
	ALE_PORTCTL2,
};

/**
 * @brief Per-MAC-port configuration resolved from Device Tree at build time
 *
 * @param phy_dev       PHY device for this MAC port
 * @param phy_conn_type PHY connection type resolved at compile time from DT
 * @param port_num	MAC Port number (1-based)
 * @param mac_only      MAC Only mode, port does not participate in L2 switching
 * @param mac_cfg       MAC address configuration (static, random, or eFuse)
 */
struct cpsw_port_dt_config {
	const struct device *phy_dev;
	enum cpsw_phy_conn_type phy_conn_type;
	uint8_t port_num;
	bool mac_only;
	struct net_eth_mac_config mac_cfg;
};

/**
 * @brief CPSW NUSS Device configuration from Device Tree
 *
 * @param DEVICE_MMIO_ROM physical address and size of the CPSW NU Subsystem
 *                        register region.  Must be the first member.
 * @param gmii_sel  DEVICE_MMIO_NAMED_ROM for "gmii_sel": physical address and
 *                  size of the GMII configuration CTRL MMR registers.
 * @param mac_efuse DEVICE_MMIO_NAMED_ROM for "mac_efuse": physical address and
 *                  size of the eFuse register holding MAC Address for MAC Port 1.
 * @param pktdma_dev Pointer to the PKTDMA device used by CPSW
 * @param tx_chan_id Transmit DMA Channel ID (transmit packets to CPSW)
 * @param rx_chan_id Receive DMA Channel ID (receive packets from CPSW)
 * @param tx_asel    ASEL value associated with the transmit DMA Channel
 * @param rx_asel    ASEL value associated with the receive DMA Channel
 * @param port       Per-MAC-port configuration, indexed by (port_num - 1)
 */
struct cpsw_dt_config {
	DEVICE_MMIO_ROM;
	DEVICE_MMIO_NAMED_ROM(gmii_sel);
	DEVICE_MMIO_NAMED_ROM(mac_efuse);
	const struct device *pktdma_dev;
	uint32_t tx_chan_id;
	uint32_t rx_chan_id;
	uint8_t tx_asel;
	uint8_t rx_asel;
	struct cpsw_port_dt_config port[CPSW_NUM_MAC_PORTS];
};

/**
 * @brief Get 0-based port index for the given network interface
 *
 * @param priv Driver private data
 * @param iface Network interface to look up
 * @retval 0-based port index (0 for MAC Port 1, 1 for MAC Port 2)
 */
static inline int cpsw_port_idx(const struct cpsw_priv *priv,
				const struct net_if *iface)
{
	if (priv->iface[0] == iface) {
		return 0;
	}

	/*
	 * Since there are only two MAC Ports, the interface must be
	 * that of the second MAC Port.
	 */
	return 1;
}


/**
 * @brief Prepare a TX CPPI5 host descriptor
 *
 * @param tx_desc  Pointer to the descriptor to fill
 * @param buf_phys Physical address of the packet data buffer
 * @param pkt_len  Length of the packet in bytes
 * @param port_id  CPSW MAC port index to transmit the packet on
 * @param asel     Address Select value for IO Coherency
 * @param tx_chan_id Transmit DMA Channel ID
 */
static void cppi5_fill_tx_desc(struct cppi5_host_desc *tx_desc,
			       uintptr_t buf_phys, uint32_t pkt_len,
			       uint8_t port_id, uint8_t asel,
			       uint32_t tx_chan_id)
{
	memset(tx_desc, 0, CPPI5_DESC_SIZE);

	/* Word 0: descriptor type, EPIB present, PSD=16 bytes, packet length */
	tx_desc->pkt_info0 = CPPI5_INFO0_HDESC_TYPE_HOST |
			     CPPI5_INFO0_HDESC_EPIB_PRESENT |
			     FIELD_PREP(CPPI5_INFO0_HDESC_PSFLGS_MASK,
					CPPI5_PSD_SIZE_CPSW >> 2) |
			     (pkt_len & CPPI5_INFO0_HDESC_PKTLEN_MASK);

	/* Word 1: transmit flow index */
	tx_desc->pkt_info1 = CPPI5_INFO1_HDESC_FLOW_ID;

	/* Word 2: packet type, return queue */
	tx_desc->pkt_info2 = FIELD_PREP(CPPI5_INFO2_HDESC_PKTTYPE_MASK,
					CPPI5_INFO2_HDESC_PKTTYPE_CPSW) |
			     (tx_chan_id & CPPI5_INFO2_HDESC_RETQ_MASK);

	/* Word 3: destination tag = MAC port index. */
	tx_desc->src_dst_tag = (port_id & CPPI5_TAG_DSTTAG_MASK);

	/* Words 6-7: buffer pointer with ASEL added to bits 48-51 */
	tx_desc->buf_ptr = (uint64_t)buf_phys |
			   FIELD_PREP(AM62L_ADDRESS_ASEL_MASK, (uint64_t)asel);

	/* Word 8: buffer length = packet length. */
	tx_desc->buf_len = pkt_len;

	/* sw_data[0-7]: original buffer pointer (set lower 32 bits, higher bits = 0) */
	tx_desc->sw_data[0] = FIELD_GET(GENMASK(7, 0), buf_phys);
	tx_desc->sw_data[1] = FIELD_GET(GENMASK(15, 8), buf_phys);
	tx_desc->sw_data[2] = FIELD_GET(GENMASK(23, 16), buf_phys);
	tx_desc->sw_data[3] = FIELD_GET(GENMASK(31, 24), buf_phys);
}

/**
 * @brief Prepare a CPPI5 buffer descriptor for TX Scatter Gather
 *
 * @param desc          Pointer to the descriptor to fill
 * @param buf_phys      Physical address of the fragment's data buffer
 * @param buf_len       Length of the fragment in bytes
 * @param next_desc_phys Physical address of the next descriptor in the chain
 *                      (0 for the final fragment)
 * @param asel          Address Select value for IO Coherency
 */
static void cppi5_fill_buf_desc(struct cppi5_host_desc *desc,
				uintptr_t buf_phys, uint32_t buf_len,
				uint64_t next_desc_phys, uint8_t asel)
{
	memset(desc, 0, CPPI5_DESC_SIZE);

	/* Word 0: descriptor type + buffer length */
	desc->pkt_info0  = CPPI5_INFO0_HDESC_TYPE_HOST |
			   (buf_len & CPPI5_INFO0_HDESC_PKTLEN_MASK);

	/* Words 4-5: link to the next descriptor in the chain (or null) */
	desc->next_desc  = next_desc_phys;

	/* Words 6-7: buffer pointer with ASEL added to bits 48-51 */
	desc->buf_ptr    = (uint64_t)buf_phys |
			   FIELD_PREP(AM62L_ADDRESS_ASEL_MASK, (uint64_t)asel);

	/* Word 8: buffer length */
	desc->buf_len    = buf_len;
}

/**
 * @brief Prepare an RX CPPI5 host descriptor for the free-descriptor ring
 *
 * @param rx_desc  Pointer to the descriptor to fill
 * @param buf_phys Physical address of the receive buffer
 * @param buf_size Size of the receive buffer in bytes
 * @param asel     Address Select value for IO Coherency
 */
static void cppi5_fill_rx_desc(struct cppi5_host_desc *rx_desc,
				uintptr_t buf_phys, uint32_t buf_size,
				uint8_t asel)
{
	memset(rx_desc, 0, CPPI5_DESC_SIZE);

	/* Word 0: descriptor type, buffer size (overwritten by DMA on completion) */
	rx_desc->pkt_info0 = CPPI5_INFO0_HDESC_TYPE_HOST |
			     (buf_size & CPPI5_INFO0_HDESC_PKTLEN_MASK);

	/* Words 6-7: receive buffer pointer with ASEL in bits 48-51 */
	rx_desc->buf_ptr = (uint64_t)buf_phys |
			   FIELD_PREP(AM62L_ADDRESS_ASEL_MASK, (uint64_t)asel);

	/* Word 8: buffer length */
	rx_desc->buf_len = buf_size;
}

/**
 * @brief Reset a CPSW MAC port
 *
 * @param priv         Driver private data
 * @param port_base_ofs Offset of the MAC Port base register region
 * @retval 0 on success, -ETIMEDOUT on reset timeout
 */
static int cpsw_macsl_reset(struct cpsw_priv *priv, uint32_t port_base_ofs)
{
	uint32_t macsl_base = port_base_ofs + CPSW_MACSL_OFS;

	cpsw_write(priv, macsl_base + CPSW_MACSL_RESET_REG, MACSL_RESET_BIT);

	for (int i = 0; i < CPSW_MAC_RESET_MAX_RETRIES; i++) {
		if (!(cpsw_read(priv, macsl_base + CPSW_MACSL_RESET_REG) &
		      MACSL_RESET_BIT)) {
			return 0;
		}
		k_busy_wait(CPSW_POLL_INTERVAL_US);
	}

	LOG_ERR("MAC Port reset timeout");
	return -ETIMEDOUT;
}

/**
 * @brief TX completion DMA callback
 *
 * @param dma_dev  PKTDMA device (unused)
 * @param user_data CPSW device pointer
 * @param channel  PKTDMA channel number (unused)
 * @param status   DMA completion status (unused)
 */
static void cpsw_tx_dma_cb(const struct device *dma_dev, void *user_data,
			   uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct cpsw_priv *priv = dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	k_sem_give(&priv->dma.tx_compl_sem);
}

/**
 * @brief RX completion DMA callback
 *
 * @param dma_dev  PKTDMA device (unused)
 * @param user_data CPSW device pointer
 * @param channel  PKTDMA channel number (unused)
 * @param status   DMA completion status (unused)
 */
static void cpsw_rx_dma_cb(const struct device *dma_dev, void *user_data,
			   uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct cpsw_priv *priv = dev->data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	k_sem_give(&priv->dma.rx_avail_sem);
}

/**
 * @brief Configure CPSW's PKTDMA channels for TX and RX
 *
 * @param priv Driver private data
 * @retval 0 on success, negative errno on failure
 */
static int cpsw_pktdma_setup(struct cpsw_priv *priv)
{
	const struct cpsw_dt_config *config = priv->dev->config;
	struct cpsw_pktdma *dma = &priv->dma;
	struct net_buf *frag;
	uintptr_t buf_phys;
	int ret;

	/*
	 * Initialize interrupt-driven completion semaphores:
	 * For TX, all TX Descriptors are initially unused and therefore free.
	 * For RX, all RX Descriptors have been submitted to DMA and we do not
	 * have any of them available (0).
	 */
	k_sem_init(&dma->tx_free_sem, CPSW_TX_DESC_NUM, CPSW_TX_DESC_NUM);
	k_sem_init(&dma->tx_compl_sem, 0, CPSW_TX_DESC_NUM);
	k_sem_init(&dma->rx_avail_sem, 0, CPSW_RX_DESC_NUM);

	struct ti_pktdma_chan_cfg tx_chan_cfg = {
		.fwd_ring_mem = (uintptr_t)dma->tx_ring_mem,
		/* Completions are written back to the same ring */
		.rev_ring_mem = (uintptr_t)dma->tx_ring_mem,
		.ring_cnt     = CPSW_TX_DESC_NUM,
		.asel         = config->tx_asel,
		.cb_user_data = priv->dev,
	};
	struct dma_config tx_dma_cfg = {
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.user_data         = &tx_chan_cfg,
		.dma_callback      = cpsw_tx_dma_cb,
	};

	struct ti_pktdma_chan_cfg rx_chan_cfg = {
		.fwd_ring_mem = (uintptr_t)dma->rx_ring_mem,
		.rev_ring_mem = (uintptr_t)dma->rx_ring_mem,
		.ring_cnt     = CPSW_RX_DESC_NUM,
		.asel         = config->rx_asel,
		.cb_user_data = priv->dev,
	};
	struct dma_config rx_dma_cfg = {
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.user_data         = &rx_chan_cfg,
		.dma_callback      = cpsw_rx_dma_cb,
	};

	ret = dma_config(config->pktdma_dev, config->tx_chan_id, &tx_dma_cfg);
	if (ret < 0) {
		LOG_ERR("TX dma_config failed: %d", ret);
		return ret;
	}

	ret = dma_start(config->pktdma_dev, config->tx_chan_id);
	if (ret < 0) {
		LOG_ERR("TX dma_start failed: %d", ret);
		return ret;
	}

	ret = dma_config(config->pktdma_dev, config->rx_chan_id, &rx_dma_cfg);
	if (ret < 0) {
		LOG_ERR("RX dma_config failed: %d", ret);
		return ret;
	}

	ret = dma_start(config->pktdma_dev, config->rx_chan_id);
	if (ret < 0) {
		LOG_ERR("RX dma_start failed: %d", ret);
		return ret;
	}

	/*
	 * Push all RX descriptors to the RX Free Descriptor ring to allow
	 * PKTDMA to send the packets received from CPSW to Software
	 */
	for (int i = 0; i < CPSW_RX_DESC_NUM; i++) {
		frag = net_pkt_get_reserve_rx_data(CPSW_RX_BUF_SIZE, K_NO_WAIT);

		if (frag == NULL) {
			LOG_ERR("RX: failed to alloc frag[%d]", i);
			for (int j = 0; j < i; j++) {
				net_pkt_frag_unref(dma->rx_frags[j]);
				dma->rx_frags[j] = NULL;
			}
			return -ENOMEM;
		}

		dma->rx_frags[i] = frag;
		buf_phys = (uintptr_t)frag->data;

		if (!AM62L_PKTDMA_IS_COHERENT(config->rx_asel)) {
			sys_cache_data_invd_range(frag->data, CPSW_RX_BUF_SIZE);
		}
		cppi5_fill_rx_desc(&dma->rx_descs[i], buf_phys,
				   CPSW_RX_BUF_SIZE, config->rx_asel);
		if (!AM62L_PKTDMA_IS_COHERENT(config->rx_asel)) {
			sys_cache_data_flush_range(&dma->rx_descs[i], CPPI5_DESC_SIZE);
		}

		ret = dma_reload(config->pktdma_dev, config->rx_chan_id,
				 0U, (uintptr_t)&dma->rx_descs[i], 0U);
		if (ret < 0) {
			LOG_ERR("RX dma_reload[%d] failed: %d", i, ret);
			for (int j = 0; j <= i; j++) {
				net_pkt_frag_unref(dma->rx_frags[j]);
				dma->rx_frags[j] = NULL;
			}
			return ret;
		}
	}

	/*
	 * The Pad Buffer is used whenever a transmitted packet's size is less than
	 * 64 bytes which is the minimum supported by CPSW.
	 */
	if (!AM62L_PKTDMA_IS_COHERENT(config->tx_asel)) {
		sys_cache_data_flush_range(dma->tx_pad_buf, sizeof(dma->tx_pad_buf));
	}

	dma->initialized = true;
	LOG_DBG("PKTDMA: TX chan %u and RX chan %u configured",
		config->tx_chan_id, config->rx_chan_id);

	return 0;
}

/**
 * @brief PHY link state-change callback, shared by all MAC ports
 *
 * @param phy_dev   PHY device that generated the callback (unused)
 * @param state     Current link state (speed, duplex, is_up)
 * @param user_data Pointer to network interface for the corresponding MAC port
 */
static void cpsw_phy_link_cb(const struct device *phy_dev,
			      struct phy_link_state *state,
			      void *user_data)
{
	struct net_if *iface = user_data;
	const struct device *dev = net_if_get_device(iface);
	struct cpsw_priv *priv = dev->data;
	int ret, idx = cpsw_port_idx(priv, iface);
	uint32_t port_base, macsl_ctl;

	ARG_UNUSED(phy_dev);

	if (idx < 0) {
		LOG_WRN("PHY callback for unknown iface, ignoring");
		return;
	}

	port_base = cpsw_port_bases[idx];

	if (!state->is_up) {
		net_eth_carrier_off(iface);
		return;
	}

	ret = cpsw_macsl_reset(priv, CPSW_NU_BASE + port_base);
	if (ret < 0) {
		LOG_ERR("MAC port %u reset failed: %d", (uint32_t)(idx + 1), ret);
	}

	macsl_ctl = MACSL_CTL_GMII_EN;

	if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
		macsl_ctl |= MACSL_CTL_FULL_DUPLEX;
	}

	if (PHY_LINK_IS_SPEED_1000M(state->speed)) {
		macsl_ctl |= MACSL_CTL_GIG;
	}

	cpsw_write(priv,
		   CPSW_NU_BASE + port_base + CPSW_MACSL_OFS +
		   CPSW_MACSL_CTL_REG,
		   macsl_ctl);

	net_eth_carrier_on(iface);
}

/**
 * @brief Initialize CPSW
 *
 * @param priv Driver private data
 * @retval 0 on success, negative errno on failure
 */
static int cpsw_hw_init(struct cpsw_priv *priv)
{
	const struct cpsw_dt_config *config = priv->dev->config;
	uint32_t stat_en = CPSW_NUSS_STAT_P0_EN;
	int ret, i;

	/* Initialize PKTDMA channels */
	ret = cpsw_pktdma_setup(priv);
	if (ret < 0) {
		LOG_ERR("PKTDMA setup failed: %d", ret);
		return ret;
	}

	/* Enable host port */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_NUSS_CONTROL,
		   CPSW_NUSS_CTL_P0_ENABLE |
		   CPSW_NUSS_CTL_P0_TX_CRC_REMOVE |
		   CPSW_NUSS_CTL_P0_RX_PAD);

	/* Disable priority elevation. */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_NUSS_PTYPE, 0U);

	/* Enable statistics */
	for (i = 0; i < CPSW_NUM_MAC_PORTS; i++) {
		stat_en |= BIT(i+1);
	}
	cpsw_write(priv, CPSW_NU_BASE + CPSW_NUSS_STAT_PORT_EN, stat_en);

	/* Set maximum receive frame length for the host port */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_PORT0_BASE + CPSW_PN_RX_MAXLEN,
		   CPSW_MAX_PACKET_SIZE);

	/* Set host port RX flow ID base */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_PORT0_BASE + CPSW_P0_FLOW_ID_REG,
		   config->rx_chan_id);

	/*
	 * Set max receive frame length, source MAC address, ALE port state
	 * for the MAC Ports.
	 */
	for (i = 0; i < CPSW_NUM_MAC_PORTS; i++) {
		uint32_t port_base = cpsw_port_bases[i];
		uint32_t sa_l, sa_h;
		uint8_t *mac_addr = priv->mac_addr[i];

		sa_l = ((uint32_t)mac_addr[5] <<  0) | ((uint32_t)mac_addr[4] <<  8) |
			((uint32_t)mac_addr[3] << 16) | ((uint32_t)mac_addr[2] << 24);
		sa_h = ((uint32_t)mac_addr[1] <<  0) | ((uint32_t)mac_addr[0] <<  8);

		/* RMW isn't required since other fields aren't present in registers */
		cpsw_write(priv,
			CPSW_NU_BASE + port_base + CPSW_PN_RX_MAXLEN,
			CPSW_MAX_PACKET_SIZE);
		cpsw_write(priv, CPSW_NU_BASE + port_base + CPSW_PN_SA_L,
			sa_l);
		cpsw_write(priv, CPSW_NU_BASE + port_base + CPSW_PN_SA_H,
			sa_h);
	}

	/* Clear ALE table and enable ALE in bypass mode */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_ALE_BASE + ALE_CONTROL,
		   ALE_CONTROL_ENABLE | ALE_CONTROL_CLEAR_TBL |
		   ALE_CONTROL_BYPASS);

	/* Set Host Port ALE state to Forwarding */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_ALE_BASE + ALE_PORTCTL0,
		   ALE_PORTCTL_FORWARD);

	/* Enable the default ALE thread mapping */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_ALE_BASE + ALE_THREADMAPDEF,
		   ALE_DEFTHREAD_EN);

	return 0;
}

/**
 * @brief Process a single TX completion
 *
 * @param dma Pointer to the PKTDMA state
 */
static inline void cpsw_tx_process_one_compl(struct cpsw_pktdma *dma)
{
	uint32_t head_idx = dma->tx_cmpl_head_idxs[dma->tx_cmpl_rptr %
						    CPSW_TX_DESC_NUM];
	uint32_t slot;
	uint8_t num_descs, i;

	dma->tx_cmpl_rptr++;

	if (head_idx >= CPSW_TX_DESC_NUM) {
		return;
	}

	num_descs = dma->tx_n_descs[head_idx];

	/* Free all descriptors linked to the head descriptor */
	for (i = 0; i < num_descs; i++) {
		slot = (head_idx + i) % CPSW_TX_DESC_NUM;
		if (dma->tx_frags[slot] != NULL) {
			net_pkt_frag_unref(dma->tx_frags[slot]);
			dma->tx_frags[slot] = NULL;
		}
		k_sem_give(&dma->tx_free_sem);
	}
}

/**
 * @brief Process a batch of TX completions
 *
 * @param priv Driver private data
 */
static inline void cpsw_tx_handle_cmpl_batch(struct cpsw_priv *priv)
{
	struct cpsw_pktdma *dma = &priv->dma;
	int count = 0;

	/* Process the completion that woke the thread */
	cpsw_tx_process_one_compl(dma);
	count++;

	/* Drain additional completions belonging to the same burst */
	while (count < CPSW_TX_BUDGET &&
	       k_sem_take(&dma->tx_compl_sem, K_NO_WAIT) == 0) {
		cpsw_tx_process_one_compl(dma);
		count++;
		if (count == CPSW_TX_BUDGET) {
			k_yield();
			count = 0;
		}
	}
}

/**
 * @brief TX completion thread
 *
 * @param arg1 Driver private data
 * @param arg2 Unused
 * @param arg3 Unused
 */
static void cpsw_tx_cmpl_thread(void *arg1, void *arg2, void *arg3)
{
	struct cpsw_priv *priv = (struct cpsw_priv *)arg1;
	const struct cpsw_dt_config *config = priv->dev->config;
	struct cpsw_pktdma *dma = &priv->dma;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("TX completion thread started");

	while (priv->ports_started != 0U) {
		k_sem_take(&dma->tx_compl_sem, K_FOREVER);

		/* Handle completions in a batch until budget limit */
		cpsw_tx_handle_cmpl_batch(priv);

		if (priv->ports_started != 0U) {
			dma_resume(config->pktdma_dev, config->tx_chan_id);
		}
	}

	LOG_DBG("TX completion thread stopped");
}

/**
 * @brief Build a net_pkt from a completed RX fragment
 *
 * @param rx_iface Network interface the packet is meant for
 * @param frag Packet fragment
 * @param pkt_len  Received packet length reported by the descriptor
 * @param rx_asel  ASEL value associated with the Receive DMA Channel
 * @retval Non-NULL net_pkt with frag attached on success and NULL on error
 */
static inline struct net_pkt *cpsw_rx_build_pkt(struct net_if *rx_iface,
						 struct net_buf *frag,
						 uint32_t pkt_len,
						 uint8_t rx_asel)
{
	struct net_pkt *pkt;

	if (pkt_len == 0U || pkt_len > CPSW_MAX_PACKET_SIZE) {
		LOG_WRN("RX: invalid pkt_len=%u, dropping", pkt_len);
		return NULL;
	}
	if (frag == NULL) {
		return NULL;
	}
	if (!AM62L_PKTDMA_IS_COHERENT(rx_asel)) {
		sys_cache_data_invd_range(frag->data, pkt_len);
	}
	pkt = net_pkt_rx_alloc_on_iface(rx_iface, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("RX: failed to alloc net_pkt");
		return NULL;
	}
	frag->len = pkt_len;
	net_pkt_frag_add(pkt, frag);
	return pkt;
}

/**
 * @brief Refill an RX descriptor slot with a new fragment and resubmit it
 *
 * @param priv     Driver private data
 * @param rx_desc  Descriptor to refill
 * @param idx      Index of the descriptor slot in rx_frags[] and rx_descs[]
 * @param old_frag Unconsumed fragment to reuse if allocating a new one fails,
 *                 or NULL if the fragment was handed to the network stack
 */
static inline void cpsw_rx_refill_desc(struct cpsw_priv *priv,
					struct cppi5_host_desc *rx_desc,
					uint32_t idx,
					struct net_buf *old_frag)
{
	const struct cpsw_dt_config *config = priv->dev->config;
	struct cpsw_pktdma *dma = &priv->dma;
	struct net_buf *new_frag;

	new_frag = net_pkt_get_reserve_rx_data(CPSW_RX_BUF_SIZE, K_NO_WAIT);
	if (new_frag == NULL) {
		new_frag = old_frag;
		old_frag = NULL;
	}

	if (new_frag != NULL) {
		if (!AM62L_PKTDMA_IS_COHERENT(config->rx_asel)) {
			sys_cache_data_invd_range(new_frag->data, CPSW_RX_BUF_SIZE);
		}
		dma->rx_frags[idx] = new_frag;
		cppi5_fill_rx_desc(rx_desc, (uintptr_t)new_frag->data,
				   CPSW_RX_BUF_SIZE, config->rx_asel);
		if (!AM62L_PKTDMA_IS_COHERENT(config->rx_asel)) {
			sys_cache_data_flush_range(rx_desc, CPPI5_DESC_SIZE);
		}
		dma_reload(config->pktdma_dev, config->rx_chan_id,
			   0U, (uintptr_t)rx_desc, 0U);
	} else {
		LOG_ERR("RX: out of buffers, descriptor slot lost");
	}

	if (old_frag != NULL) {
		net_pkt_frag_unref(old_frag);
	}
}

/**
 * @brief Process a single RX packet completion
 *
 * @param priv Driver private data
 */
static inline void cpsw_rx_process_one_pkt(struct cpsw_priv *priv)
{
	const struct cpsw_dt_config *config = priv->dev->config;
	struct cpsw_pktdma *dma = &priv->dma;
	struct cppi5_host_desc *rx_desc;
	struct net_if *rx_iface;
	struct net_pkt *pkt;
	struct net_buf *frag;
	uint32_t src_port, pkt_len, idx;

	idx = dma->rx_drain_idx % CPSW_RX_DESC_NUM;
	rx_desc = &dma->rx_descs[idx];
	dma->rx_drain_idx++;

	if (!AM62L_PKTDMA_IS_COHERENT(config->rx_asel)) {
		sys_cache_data_invd_range(rx_desc, CPPI5_DESC_SIZE);
	}

	pkt_len  = rx_desc->pkt_info0 & CPPI5_INFO0_HDESC_PKTLEN_MASK;
	src_port = FIELD_GET(CPPI5_TAG_SRCTAG_PORT_MASK, rx_desc->src_dst_tag);

	if (src_port >= 1U && src_port <= CPSW_NUM_MAC_PORTS &&
	    priv->iface[src_port - 1] != NULL) {
		rx_iface = priv->iface[src_port - 1];
	} else {
		rx_iface = priv->iface[0];
		LOG_WRN("RX: unexpected src_port=%u", src_port);
	}

	frag = dma->rx_frags[idx];
	dma->rx_frags[idx] = NULL;

	pkt = cpsw_rx_build_pkt(rx_iface, frag, pkt_len, config->rx_asel);
	cpsw_rx_refill_desc(priv, rx_desc, idx,
			    (pkt != NULL) ? NULL : frag);

	if (pkt != NULL) {
		LOG_DBG("RX: %u bytes (port %u)", pkt_len, src_port);
		if (net_recv_data(rx_iface, pkt) < 0) {
			net_pkt_unref(pkt);
		}
	}
}

/**
 * @brief Process a batch of RX completions
 *
 * @param priv Driver private data
 */
static inline void cpsw_rx_handle_batch(struct cpsw_priv *priv)
{
	struct cpsw_pktdma *dma = &priv->dma;
	int count = 0;

	/* Process the completion that woke the thread */
	cpsw_rx_process_one_pkt(priv);
	count++;

	/* Drain any additional completions belonging to the same burst */
	while (count < CPSW_RX_BUDGET &&
	       k_sem_take(&dma->rx_avail_sem, K_NO_WAIT) == 0) {
		cpsw_rx_process_one_pkt(priv);
		count++;
		if (count == CPSW_RX_BUDGET) {
			k_yield();
			count = 0;
		}
	}
}

/**
 * @brief RX completion thread
 *
 * @param arg1 Driver private data
 * @param arg2 Unused
 * @param arg3 Unused
 */
static void cpsw_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct cpsw_priv *priv = (struct cpsw_priv *)arg1;
	const struct cpsw_dt_config *config = priv->dev->config;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("RX thread started");

	while (priv->ports_started != 0U) {
		k_sem_take(&priv->dma.rx_avail_sem, K_FOREVER);

		cpsw_rx_handle_batch(priv);

		if (priv->ports_started != 0U) {
			dma_resume(config->pktdma_dev, config->rx_chan_id);
		}
	}

	LOG_DBG("RX thread stopped");
}

/**
 * @brief Count the non-empty fragments in a TX packet
 *
 * @param pkt Packet to be transmitted
 * @retval Number of fragments in the packet
 */
static uint8_t cpsw_tx_count_frags(const struct net_pkt *pkt)
{
	uint8_t n = 0;

	for (const struct net_buf *frag = pkt->frags; frag != NULL; frag = frag->frags) {
		if (frag->len > 0) {
			n++;
		}
	}
	return n;
}

/**
 * @brief Acquire TX descriptor slots
 *
 * @param dma Pointer to the PKTDMA state
 * @param priv Driver private data
 * @param n_descs Number of descriptors to acquire
 * @retval 0 on success, -ENOBUFS on timeout
 */
static int cpsw_tx_acquire_descs(struct cpsw_pktdma *dma, struct cpsw_priv *priv,
				 uint8_t n_descs)
{
	uint8_t i;

	for (i = 0; i < n_descs; i++) {
		if (k_sem_take(&dma->tx_free_sem,
			       K_MSEC(CPSW_TX_DESC_ACQUIRE_TIMEOUT_MS)) != 0) {
			for (uint8_t j = 0; j < i; j++) {
				k_sem_give(&dma->tx_free_sem);
			}
			LOG_ERR("TX: no free slots (need %u)", n_descs);
			return -ENOBUFS;
		}
	}

	if (k_mutex_lock(&priv->tx_lock,
			 K_MSEC(CPSW_TX_DESC_ACQUIRE_TIMEOUT_MS)) != 0) {
		for (i = 0; i < n_descs; i++) {
			k_sem_give(&dma->tx_free_sem);
		}
		LOG_ERR("TX: tx_lock timeout");
		return -ENOBUFS;
	}

	return 0;
}

/**
 * @brief Build the CPPI5 descriptor chain for a TX packet
 *
 * @param dev CPSW device pointer
 * @param head_idx Index of the Head Descriptor pointing to start of packet
 * @param pkt Pointer to the packet to be transmitted
 * @param pkt_len Length of the packet to be transmitted
 * @param port_num 1-based MAC Port number
 * @param n_descs Number of descriptors required by the packet
 * @param needs_pad Boolean indicating whether zero padding is required or not
 */
static void cpsw_tx_fill_chain(const struct device *dev, uint32_t head_idx,
			       struct net_pkt *pkt, uint16_t pkt_len,
			       uint8_t port_num, uint8_t n_descs, bool needs_pad)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	struct cpsw_pktdma *dma = &priv->dma;
	struct cppi5_host_desc *desc;
	struct net_buf *frag;
	uint64_t next_phys;
	uint32_t desc_idx, nxt;
	uint8_t i = 0;

	desc_idx = head_idx;

	for (frag = pkt->frags; frag != NULL; frag = frag->frags) {
		if (frag->len == 0) {
			continue;
		}

		next_phys = 0;
		if (i < n_descs - 1U) {
			nxt = (desc_idx + 1U) % CPSW_TX_DESC_NUM;
			next_phys = (uint64_t)(uintptr_t)&dma->tx_descs[nxt] |
				    FIELD_PREP(AM62L_ADDRESS_ASEL_MASK,
					       (uint64_t)config->tx_asel);
		}

		desc = &dma->tx_descs[desc_idx];
		if (i == 0) {
			cppi5_fill_tx_desc(desc, (uintptr_t)frag->data,
					   frag->len, port_num,
					   config->tx_asel, config->tx_chan_id);
			desc->pkt_info0 =
				(desc->pkt_info0 & ~CPPI5_INFO0_HDESC_PKTLEN_MASK) |
				((needs_pad ? CPSW_MIN_PACKET_SIZE : pkt_len)
				 & CPPI5_INFO0_HDESC_PKTLEN_MASK);
			desc->next_desc = next_phys;
		} else {
			cppi5_fill_buf_desc(desc, (uintptr_t)frag->data,
					    frag->len, next_phys,
					    config->tx_asel);
		}

		if (!AM62L_PKTDMA_IS_COHERENT(config->tx_asel)) {
			sys_cache_data_flush_range(frag->data, frag->len);
			sys_cache_data_flush_range(desc, CPPI5_DESC_SIZE);
		}

		net_pkt_frag_ref(frag);
		dma->tx_frags[desc_idx] = frag;

		desc_idx = (desc_idx + 1U) % CPSW_TX_DESC_NUM;
		i++;
	}

	if (needs_pad) {
		uint16_t pad_len = CPSW_MIN_PACKET_SIZE - pkt_len;

		desc = &dma->tx_descs[desc_idx];
		cppi5_fill_buf_desc(desc, (uintptr_t)dma->tx_pad_buf,
				    pad_len, 0U, config->tx_asel);
		if (!AM62L_PKTDMA_IS_COHERENT(config->tx_asel)) {
			sys_cache_data_flush_range(desc, CPPI5_DESC_SIZE);
		}
		dma->tx_frags[desc_idx] = NULL;
	}
}

/**
 * @brief Free descriptors in a descriptor chain
 *
 * @param dma Pointer to the PKTDMA state
 * @param head_idx Index of the Head descriptor pointing to the start of the packet
 * @param n_descs Number of descriptors in the descriptor chain
 */
static void cpsw_tx_release_chain(struct cpsw_pktdma *dma, uint32_t head_idx,
				  uint8_t n_descs)
{
	for (uint8_t i = 0; i < n_descs; i++) {
		uint32_t slot = (head_idx + i) % CPSW_TX_DESC_NUM;

		if (dma->tx_frags[slot] != NULL) {
			net_pkt_frag_unref(dma->tx_frags[slot]);
			dma->tx_frags[slot] = NULL;
		}
		k_sem_give(&dma->tx_free_sem);
	}
}

/**
 * @brief Transmit a packet via CPSW
 *
 * @param dev CPSW device pointer
 * @param pkt Pointer to the packet to transmit
 * @retval 0 on success, negative errno on failure
 */
static int cpsw_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	struct cpsw_pktdma *dma = &priv->dma;
	uint32_t head_idx;
	uint16_t pkt_len;
	uint8_t port_num, n_frags, n_descs;
	bool needs_pad;
	int i, ret;

	i = cpsw_port_idx(priv, net_pkt_iface(pkt));
	if (i < 0 || !(priv->ports_started & BIT(i)) || !dma->initialized) {
		return -ENETDOWN;
	}

	pkt_len = net_pkt_get_len(pkt);
	if (pkt_len > CPSW_MAX_PACKET_SIZE) {
		LOG_ERR("TX: packet too large (%u)", pkt_len);
		return -EMSGSIZE;
	}

	port_num = (uint8_t)(i + 1);

	n_frags = cpsw_tx_count_frags(pkt);
	if (n_frags == 0) {
		LOG_ERR("TX: packet has no data");
		return -EINVAL;
	}
	if (n_frags > CPSW_TX_MAX_SG_FRAGS) {
		LOG_ERR("TX: too many fragments (%u > %u)", n_frags,
			CPSW_TX_MAX_SG_FRAGS);
		return -EMSGSIZE;
	}

	needs_pad = (pkt_len < CPSW_MIN_PACKET_SIZE);
	n_descs = n_frags + (needs_pad ? 1U : 0U);

	ret = cpsw_tx_acquire_descs(dma, priv, n_descs);
	if (ret < 0) {
		return ret;
	}

	head_idx = dma->tx_head % CPSW_TX_DESC_NUM;
	cpsw_tx_fill_chain(dev, head_idx, pkt, pkt_len, port_num, n_descs, needs_pad);
	dma->tx_n_descs[head_idx] = n_descs;

	ret = dma_reload(config->pktdma_dev, config->tx_chan_id,
			 (uintptr_t)&dma->tx_descs[head_idx], 0U, 0U);
	if (ret < 0) {
		LOG_ERR("TX: dma_reload failed: %d", ret);
		cpsw_tx_release_chain(dma, head_idx, n_descs);
	} else {
		dma->tx_cmpl_head_idxs[dma->tx_cmpl_wptr % CPSW_TX_DESC_NUM] = head_idx;
		dma->tx_cmpl_wptr++;
		LOG_DBG("TX: %u bytes queued",
			needs_pad ? CPSW_MIN_PACKET_SIZE : pkt_len);
		dma->tx_head += n_descs;
	}

	k_mutex_unlock(&priv->tx_lock);
	return ret;
}

/**
 * @brief Program the GMII_SEL MMR register for a MAC Port based on requested
 *        PHY Connection Type
 *
 * @param port_cfg      Per-port configuration containing phy_conn_type
 * @param gmii_sel_virt Virtual address of this port's GMII_SEL register
 */
static void cpsw_gmii_sel_set(const struct cpsw_port_dt_config *port_cfg,
			      mm_reg_t gmii_sel_virt)
{
	uint32_t reg, mode_val, rgmii_id = 0U;

	switch (port_cfg->phy_conn_type) {
	case CPSW_PHY_CONN_RMII:
		mode_val = GMII_SEL_MODE_RMII;
		break;
	case CPSW_PHY_CONN_RGMII_ID:
	case CPSW_PHY_CONN_RGMII_TXID:
		/*
		 * MAC Port has to add a delay for RGMII_ID and RGMII_TX_ID
		 * modes. A delay is added when 'RGMII_ID' field of the
		 * register is 'cleared', we have to set it to '0'.
		 */
		mode_val = GMII_SEL_MODE_RGMII;
		break;
	case CPSW_PHY_CONN_RGMII:
	case CPSW_PHY_CONN_RGMII_RXID:
		mode_val = GMII_SEL_MODE_RGMII;
		/* Do not add a delay */
		rgmii_id = GMII_SEL_RGMII_ID_BIT;
		break;
	default:
		LOG_WRN("Unsupported phy_conn_type: %d", port_cfg->phy_conn_type);
		return;
	}

	reg = sys_read32(gmii_sel_virt);
	reg &= ~(GMII_SEL_MODE_MASK | GMII_SEL_RGMII_ID_BIT);
	reg |= (mode_val & GMII_SEL_MODE_MASK) | rgmii_id;
	sys_write32(reg, gmii_sel_virt);

	LOG_DBG("MAC port %u: GMII_SEL @0x%08lx = 0x%08x",
		port_cfg->port_num, (unsigned long)gmii_sel_virt, reg);
}

/**
 * @brief Start the CPSW network interface for the MAC Port
 *
 * @param dev CPSW device pointer
 * @param iface Network interface pointer
 * @retval 0 on success, negative errno on failure
 */
static int cpsw_start(const struct device *dev, struct net_if *iface)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	int ret, i = cpsw_port_idx(priv, iface);
	uint8_t port_num;

	if (i < 0) {
		return -EINVAL;
	}

	port_num = (uint8_t)(i + 1);

	k_mutex_lock(&priv->start_lock, K_FOREVER);

	/*
	 * First port to start initialises the shared hardware i.e.
	 * setup DMA Channels, configure ALE and Host Port, and create
	 * completion threads. Subsequent ports will skip this setup.
	 */
	if (priv->ports_started == 0U) {
		LOG_DBG("Starting CPSW shared hardware");

		ret = cpsw_hw_init(priv);
		if (ret < 0) {
			k_mutex_unlock(&priv->start_lock);
			LOG_ERR("HW init failed: %d", ret);
			return ret;
		}

		priv->ports_started |= BIT(i);

		priv->tx_tid = k_thread_create(&priv->tx_thread,
					       priv->tx_stack,
					       CPSW_TX_THREAD_STACK_SIZE,
					       cpsw_tx_cmpl_thread,
					       priv, NULL, NULL,
					       CPSW_TX_THREAD_PRIORITY,
					       0, K_NO_WAIT);
		k_thread_name_set(priv->tx_tid, "cpsw_tx");

		priv->rx_tid = k_thread_create(&priv->rx_thread,
					       priv->rx_stack,
					       CPSW_RX_THREAD_STACK_SIZE,
					       cpsw_rx_thread,
					       priv, NULL, NULL,
					       CPSW_RX_THREAD_PRIORITY,
					       0, K_NO_WAIT);
		k_thread_name_set(priv->rx_tid, "cpsw_rx");

		dma_resume(config->pktdma_dev, config->tx_chan_id);
		dma_resume(config->pktdma_dev, config->rx_chan_id);
	} else {
		priv->ports_started |= BIT(i);
	}

	k_mutex_unlock(&priv->start_lock);

	/* Program the GMII_SEL register with the phy connection type from DT */
	cpsw_gmii_sel_set(&config->port[i],
			  priv->gmii_sel_base + ((uint8_t)i * CPSW_GMII_SEL_REG_OFFSET));

	/* Set MAC Port's ALE state to forwarding and set MAC Only mode based on DT */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_ALE_BASE + cpsw_ale_portctls[i],
		   ALE_PORTCTL_FORWARD |
		   (config->port[i].mac_only ? ALE_PORTCTL_MAC_ONLY : 0U));

	LOG_DBG("Started MAC port %u", port_num);

	return 0;
}

/**
 * @brief Stop the CPSW network interface
 *
 * @param dev CPSW device pointer
 * @param iface Network interface pointer
 * @retval 0 always
 */
static int cpsw_stop(const struct device *dev, struct net_if *iface)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	int i = cpsw_port_idx(priv, iface);
	uint8_t port_num;

	/* Exit if port does not exist */
	if (i < 0) {
		return 0;
	}

	port_num = (uint8_t)(i + 1);
	priv->ports_started &= ~BIT(i);

	LOG_DBG("Stopped MAC port %u", port_num);

	/* If other ports are active, shared hardware should remain functional */
	if (priv->ports_started != 0U) {
		return 0;
	}

	/*
	 * Stop DMA channels to disable the channel IRQs and prevent
	 * new callbacks from firing.
	 */
	dma_stop(config->pktdma_dev, config->tx_chan_id);
	dma_stop(config->pktdma_dev, config->rx_chan_id);

	/* Wake the TX and RX completion threads so they can exit their loops */
	k_sem_give(&priv->dma.tx_compl_sem);
	k_sem_give(&priv->dma.rx_avail_sem);

	/* Stop receiving wire-side traffic by disabling ALE */
	cpsw_write(priv, CPSW_NU_BASE + CPSW_ALE_BASE + ALE_CONTROL, 0U);
	cpsw_write(priv, CPSW_NUSS_CONTROL, 0U);

	/* Allow the RX thread to finish draining existing packets and exit */
	k_sleep(K_MSEC(CPSW_RX_DRAIN_PERIOD_MS));

	/* Release all RX fragments remaining that aren't drained by the RX thread */
	for (int j = 0; j < CPSW_RX_DESC_NUM; j++) {
		if (priv->dma.rx_frags[j] != NULL) {
			net_pkt_frag_unref(priv->dma.rx_frags[j]);
			priv->dma.rx_frags[j] = NULL;
		}
	}

	/* Release any TX fragment references not freed via the completion ring */
	for (int j = 0; j < CPSW_TX_DESC_NUM; j++) {
		if (priv->dma.tx_frags[j] != NULL) {
			net_pkt_frag_unref(priv->dma.tx_frags[j]);
			priv->dma.tx_frags[j] = NULL;
		}
	}

	priv->dma.initialized = false;

	return 0;
}

/**
 * @brief Return the link speeds supported by this CPSW MAC port
 *
 * @param dev CPSW device pointer
 * @param iface Network interface pointer
 * @return Bitmask of supported ETHERNET_LINK capabilities
 */
static enum ethernet_hw_caps cpsw_get_capabilities(const struct device *dev,
						   struct net_if *iface)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	int i = cpsw_port_idx(priv, iface);

	if (i >= 0 && config->port[i].phy_conn_type == CPSW_PHY_CONN_RMII) {
		return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
	}

	/* Default for RGMII */
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE |
	       ETHERNET_LINK_1000BASE;
}

/**
 * @brief Handle CPSW network interface configuration commands
 *
 * @param dev    CPSW device pointer
 * @param iface Network interface pointer
 * @param type   Configuration type
 * @param config Configuration value
 * @retval 0 on success, -ENOTSUP for unsupported types
 */
static int cpsw_set_config(const struct device *dev,
			   struct net_if *iface,
			   enum ethernet_config_type type,
			   const struct ethernet_config *config)
{
	struct cpsw_priv *priv = dev->data;
	int i = cpsw_port_idx(priv, iface);
	uint32_t sa_l, sa_h;
	uint8_t *mac;

	if (i < 0) {
		return -EINVAL;
	}

	mac = priv->mac_addr[i];

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(mac, config->mac_address.addr, NET_ETH_ADDR_LEN);

		sa_l = ((uint32_t)mac[5] <<  0) | ((uint32_t)mac[4] <<  8) |
		       ((uint32_t)mac[3] << 16) | ((uint32_t)mac[2] << 24);
		sa_h = ((uint32_t)mac[1] <<  0) | ((uint32_t)mac[0] <<  8);

		cpsw_write(priv,
			   CPSW_NU_BASE + cpsw_port_bases[i] + CPSW_PN_SA_L,
			   sa_l);
		cpsw_write(priv,
			   CPSW_NU_BASE + cpsw_port_bases[i] + CPSW_PN_SA_H,
			   sa_h);

		LOG_DBG("MAC port %u address updated", i + 1);
		return 0;
	}

	return -ENOTSUP;
}

/**
 * @brief Initialize the network interface for CPSW
 *
 * @param iface The network interface to initialize
 */
static void cpsw_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct cpsw_dt_config *config = dev->config;
	const struct cpsw_port_dt_config *port_cfg;
	struct cpsw_priv *priv = dev->data;
	int i;

	/* Determine which port slot this interface belongs to */
	for (i = 0; i < CPSW_NUM_MAC_PORTS; i++) {
		if (priv->iface[i] == NULL) {
			break;
		}
	}

	if (i >= CPSW_NUM_MAC_PORTS) {
		LOG_ERR("CPSW: interface does not have an associated MAC port");
		return;
	}

	port_cfg = &config->port[i];
	priv->iface[i] = iface;
	net_if_set_link_addr(iface, priv->mac_addr[i],
			     sizeof(priv->mac_addr[i]), NET_LINK_ETHERNET);
	ethernet_init(iface);

	/* Register PHY callback for the MAC Port */
	if (port_cfg->phy_dev != NULL) {
		/* Carrier should be enabled by PHY */
		net_if_carrier_off(iface);

		if (device_is_ready(port_cfg->phy_dev)) {
			phy_link_callback_set(port_cfg->phy_dev,
					      cpsw_phy_link_cb,
					      iface);
		} else {
			LOG_ERR("PHY Device for MAC port %d is not ready", i + 1);
		}
	}
}

/**
 * @brief Get the Ethernet PHY device for CPSW's MAC Port
 *
 * @param dev CPSW device pointer
 * @param iface Network interface pointer
 * returns PHY Device pointer (NULL if PHY doesn't exist)
 */
static const struct device *cpsw_get_phy(const struct device *dev, struct net_if *iface)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	int i = cpsw_port_idx(priv, iface);

	if (i < 0) {
		return NULL;
	}

	return config->port[i].phy_dev;
}

static const struct ethernet_api cpsw_api = {
	.iface_api.init   = cpsw_iface_init,
	.get_phy          = cpsw_get_phy,
	.start            = cpsw_start,
	.stop             = cpsw_stop,
	.get_capabilities = cpsw_get_capabilities,
	.set_config       = cpsw_set_config,
	.send             = cpsw_tx,
};

/**
 * @brief CPSW driver initialization callback
 *
 * @param dev CPSW device pointer
 * @retval 0 on success, negative errno on failure
 */
static int cpsw_init(const struct device *dev)
{
	const struct cpsw_dt_config *config = dev->config;
	struct cpsw_priv *priv = dev->data;
	int ret;

	/* Map the CPSW SS register region */
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	priv->base = (uint32_t *)DEVICE_MMIO_GET(dev);

	/*
	 * Map the GMII_SEL CTRL MMR region. Use device_map since
	 * we don't have a device for the GMII SEL block which falls
	 * in a different register space compared to the CPSW SS register
	 * region.
	 */
	device_map(&priv->gmii_sel_base,
		   config->gmii_sel.phys_addr,
		   config->gmii_sel.size,
		   K_MEM_CACHE_NONE);

	/*
	 * Map the MAC eFuse region to fetch MAC Port 1's MAC Address.
	 * Use device_map since we don't have a device for the eFuse
	 * registers storing the MAC Address and the registers belong
	 * to a different register space compared to the CPSW SS register
	 * region.
	 */
	device_map(&priv->mac_efuse_base,
		   config->mac_efuse.phys_addr,
		   config->mac_efuse.size,
		   K_MEM_CACHE_NONE);

	/*
	 * Attempt to use the Device-tree configured source for MAC Address and
	 * fallback to the MAC eFuse registers only for the first MAC Port.
	 */
	for (int i = 0; i < CPSW_NUM_MAC_PORTS; i++) {
		ret = net_eth_mac_load(&config->port[i].mac_cfg, priv->mac_addr[i]);
		if (ret == 0) {
			continue;
		}

		if (ret == -ENODATA && i == 0) {
			/* Fall back to hardware eFuse for MAC Port 1's MAC Address*/
			uint32_t mac_lo = sys_read32(priv->mac_efuse_base);
			uint32_t mac_hi = sys_read32(priv->mac_efuse_base +
						     CPSW_MAC_LO_MAC_HI_OFFSET);
			uint8_t *mac = priv->mac_addr[i];

			mac[0] = FIELD_GET(GENMASK(15, 8), mac_hi);
			mac[1] = FIELD_GET(GENMASK(7, 0), mac_hi);
			mac[2] = FIELD_GET(GENMASK(31, 24), mac_lo);
			mac[3] = FIELD_GET(GENMASK(23, 16), mac_lo);
			mac[4] = FIELD_GET(GENMASK(15, 8), mac_lo);
			mac[5] = FIELD_GET(GENMASK(7, 0), mac_lo);
		} else {
			LOG_WRN("MAC address not found for MAC port %d: %d",
				i + 1, ret);
		}
	}

	priv->dev = dev;
	k_mutex_init(&priv->tx_lock);
	k_mutex_init(&priv->start_lock);

	if (!device_is_ready(config->pktdma_dev)) {
		LOG_ERR("PKTDMA device not ready");
		return -ENODEV;
	}

	LOG_INF("CPSW version: 0x%08x", cpsw_read(priv, CPSW_NUSS_VER));

	return 0;
}

/* MAC port nodes fetched from device-tree */
#define CPSW_ETH_PORTS_NODE(n)	DT_INST_CHILD(n, ethernet_ports)
#define CPSW_PORT_NODE(n, idx)	DT_CHILD(CPSW_ETH_PORTS_NODE(n), port_##idx)

/* MAC Port device-tree configuration */
#define CPSW_PORT_DT_CONFIG_INIT(n, port_idx)						\
{											\
	.phy_dev = COND_CODE_1(DT_NODE_HAS_PROP(CPSW_PORT_NODE(n, port_idx), phy_handle),\
			       (DEVICE_DT_GET(DT_PHANDLE(CPSW_PORT_NODE(n, port_idx),	\
							 phy_handle))),			\
			       (NULL)),							\
	.phy_conn_type = CPSW_PHY_CONN_TYPE(CPSW_PORT_NODE(n, port_idx)),		\
	.mac_only      = DT_PROP(CPSW_PORT_NODE(n, port_idx), ti_mac_only),		\
	.port_num      = (port_idx),							\
	.mac_cfg       = NET_ETH_MAC_DT_CONFIG_INIT(CPSW_PORT_NODE(n, port_idx)),	\
}

#define CPSW_DEVICE_INIT(n)						\
	static struct cpsw_priv cpsw_priv_##n;				\
									\
	static const struct cpsw_dt_config cpsw_cfg_##n = {		\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),			\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(gmii_sel,		\
						   DT_DRV_INST(n)),	\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(mac_efuse,		\
						   DT_DRV_INST(n)),	\
		.pktdma_dev = DEVICE_DT_GET(				\
				DT_INST_DMAS_CTLR_BY_NAME(n, tx0)),	\
		.tx_chan_id  = DT_INST_DMAS_CELL_BY_NAME(n, tx0,	\
							 channel_id),	\
		.rx_chan_id  = DT_INST_DMAS_CELL_BY_NAME(n, rx,		\
							 channel_id),	\
		.tx_asel     = DT_INST_DMAS_CELL_BY_NAME(n, tx0, asel), \
		.rx_asel     = DT_INST_DMAS_CELL_BY_NAME(n, rx, asel),	\
		.port = {						\
			[0] = CPSW_PORT_DT_CONFIG_INIT(n, 1),		\
			[1] = CPSW_PORT_DT_CONFIG_INIT(n, 2),		\
		},							\
	};								\
									\
	BUILD_ASSERT(CPSW_PHY_CONN_TYPE(CPSW_PORT_NODE(n, 1)) !=	\
		     CPSW_PHY_CONN_INVALID,				\
		     "Invalid phy-connection-type for MAC port 1");	\
	BUILD_ASSERT(CPSW_PHY_CONN_TYPE(CPSW_PORT_NODE(n, 2)) !=	\
		     CPSW_PHY_CONN_INVALID,				\
		     "Invalid phy-connection-type for MAC port 2");	\
									\
	ETH_NET_DEVICE_DT_INST_DEFINE(n, cpsw_init, NULL,		\
				      &cpsw_priv_##n, &cpsw_cfg_##n,	\
				      CONFIG_ETH_INIT_PRIORITY,		\
				      &cpsw_api, NET_ETH_MTU);		\
									\
	NET_DEVICE_DT_INST_ADD_IFACE(n, ETHERNET_L2,			\
				     NET_L2_GET_CTX_TYPE(ETHERNET_L2),	\
				     NET_ETH_MTU, 1);

DT_INST_FOREACH_STATUS_OKAY(CPSW_DEVICE_INIT)
