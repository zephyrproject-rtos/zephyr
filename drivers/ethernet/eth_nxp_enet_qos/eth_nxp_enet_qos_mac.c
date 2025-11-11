/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_qos_mac

#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_nxp_enet_qos_mac, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/phy.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys_clock.h>

#if defined(CONFIG_ETH_NXP_ENET_QOS_MAC_UNIQUE_MAC_ADDRESS)
#include <zephyr/sys/crc.h>
#include <zephyr/drivers/hwinfo.h>
#endif
#include <ethernet/eth_stats.h>
#include "../eth.h"
#include "nxp_enet_qos_priv.h"

/* Verify configuration */
BUILD_ASSERT((ENET_QOS_RX_BUFFER_SIZE * NUM_RX_BUFDESC) >= ENET_QOS_MAX_NORMAL_FRAME_LEN,
	"ENET_QOS_RX_BUFFER_SIZE * NUM_RX_BUFDESC is not large enough to receive a full frame");

static const uint32_t rx_desc_refresh_flags =
	OWN_FLAG | RX_INTERRUPT_ON_COMPLETE_FLAG | BUF1_ADDR_VALID_FLAG;

K_THREAD_STACK_DEFINE(enet_qos_rx_stack, CONFIG_ETH_NXP_ENET_QOS_RX_THREAD_STACK_SIZE);
static struct k_work_q rx_work_queue;

static int rx_queue_init(void)
{
	struct k_work_queue_config cfg = {.name = "ENETQOS_RX"};

	k_work_queue_init(&rx_work_queue);
	k_work_queue_start(&rx_work_queue, enet_qos_rx_stack,
			   K_THREAD_STACK_SIZEOF(enet_qos_rx_stack),
			   K_PRIO_COOP(CONFIG_ETH_NXP_ENET_QOS_RX_THREAD_PRIORITY),
			   &cfg);

	return 0;
}

SYS_INIT(rx_queue_init, POST_KERNEL, 0);

static void eth_nxp_enet_qos_phy_cb(const struct device *phy,
		struct phy_link_state *state, void *eth_dev)
{
	const struct device *dev = eth_dev;
	struct nxp_enet_qos_mac_data *data = dev->data;

	if (!data->iface) {
		return;
	}

	if (state->is_up) {
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}

	LOG_INF("Link is %s", state->is_up ? "up" : "down");

	/* handle link speed in MAC configuration register */
	if (state->is_up) {
		const struct nxp_enet_qos_mac_config *config = dev->config;
		struct nxp_enet_qos_config *module_cfg = ENET_QOS_MODULE_CFG(config->enet_dev);
		enet_qos_t *base = module_cfg->base;

		if (PHY_LINK_IS_SPEED_10M(state->speed)) {
			LOG_DBG("Link Speed reduced to 10MBit");
			base->MAC_CONFIGURATION &= ~ENET_QOS_REG_PREP(MAC_CONFIGURATION, FES, 0b1);
		} else {
			LOG_DBG("Link Speed 100MBit or higher");
			base->MAC_CONFIGURATION |= ENET_QOS_REG_PREP(MAC_CONFIGURATION, FES, 0b1);
		}
	}
}

static void eth_nxp_enet_qos_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_enet_qos_mac_data *data = dev->data;
	const struct nxp_enet_qos_mac_config *config = dev->config;

	net_if_set_link_addr(iface, data->mac_addr.addr,
			     sizeof(((struct net_eth_addr *)NULL)->addr), NET_LINK_ETHERNET);

	if (data->iface == NULL) {
		data->iface = iface;
	}

	ethernet_init(iface);

	if (device_is_ready(config->phy_dev)) {
		/* Do not start the interface until PHY link is up */
		net_if_carrier_off(iface);

		phy_link_callback_set(config->phy_dev, eth_nxp_enet_qos_phy_cb, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}
}

static int eth_nxp_enet_qos_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	enet_qos_t *base = config->base;

	volatile union nxp_enet_qos_tx_desc *tx_desc_ptr = data->tx.descriptors;
	volatile union nxp_enet_qos_tx_desc *last_desc_ptr;

	struct net_buf *fragment = pkt->frags;
	int frags_count = 0, total_bytes = 0;
	int ret;

	/* Only allow send of the maximum normal packet size */
	while (fragment != NULL) {
		frags_count++;
		total_bytes += fragment->len;
		fragment = fragment->frags;

		if (total_bytes > config->hw_info.max_frame_len ||
			frags_count > NUM_TX_BUFDESC) {
			LOG_ERR("TX packet too large");
			return -E2BIG;
		}
	}


	/* One TX at a time in the current implementation */
	ret = k_sem_take(&data->tx.tx_sem, K_NO_WAIT);
	if (ret) {
		LOG_DBG("%s TX busy, rejected thread %s", dev->name,
							  k_thread_name_get(k_current_get()));
		return ret;
	}
	LOG_DBG("Took driver TX sem %p by thread %s", &data->tx.tx_sem,
						      k_thread_name_get(k_current_get()));

	net_pkt_ref(pkt);
	data->tx.pkt = pkt;

	LOG_DBG("Setting up TX descriptors for packet %p", pkt);

	/* Reset the descriptors */
	memset((void *)data->tx.descriptors, 0, sizeof(union nxp_enet_qos_tx_desc) * frags_count);

	/* Setting up the descriptors  */
	fragment = pkt->frags;
	tx_desc_ptr->read.control2 |= FIRST_DESCRIPTOR_FLAG;
	for (int i = 0; i < frags_count; i++) {
		net_pkt_frag_ref(fragment);

		tx_desc_ptr->read.buf1_addr = (uint32_t)fragment->data;
		tx_desc_ptr->read.control1 = FIELD_PREP(0x3FFF, fragment->len);
		tx_desc_ptr->read.control2 |= FIELD_PREP(0x7FFF, total_bytes);

		fragment = fragment->frags;
		tx_desc_ptr++;
	}
	last_desc_ptr = tx_desc_ptr - 1;
	last_desc_ptr->read.control2 |= LAST_DESCRIPTOR_FLAG;
	last_desc_ptr->read.control1 |= TX_INTERRUPT_ON_COMPLETE_FLAG;

	LOG_DBG("Starting TX DMA on packet %p", pkt);

	/* Set the DMA ownership of all the used descriptors */
	for (int i = 0; i < frags_count; i++) {
		data->tx.descriptors[i].read.control2 |= OWN_FLAG;
	}

	/* This implementation is clearly naive and basic, it just changes the
	 * ring length for every TX send, there is room for optimization
	 */
	base->DMA_CH[0].DMA_CHX_TXDESC_RING_LENGTH = frags_count - 1;
	base->DMA_CH[0].DMA_CHX_TXDESC_TAIL_PTR =
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TXDESC_TAIL_PTR, TDTP,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t) tx_desc_ptr));

	return 0;
}

static void tx_dma_done(const struct device *dev)
{
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_tx_data *tx_data = &data->tx;
	struct net_pkt *pkt = tx_data->pkt;
	struct net_buf *fragment = pkt->frags;

	if (pkt == NULL) {
		LOG_WRN("%s TX DMA done on nonexistent packet?", dev->name);
		goto skip;
	} else {
		LOG_DBG("TX DMA completed on packet %p", pkt);
	}

	/* Returning the buffers and packet to the pool */
	while (fragment != NULL) {
		net_pkt_frag_unref(fragment);
		fragment = fragment->frags;
	}
	net_pkt_unref(pkt);

	eth_stats_update_pkts_tx(data->iface);

skip:
	/* Allows another send */
	k_sem_give(&data->tx.tx_sem);
	LOG_DBG("Gave driver TX sem %p by thread %s", &data->tx.tx_sem,
						      k_thread_name_get(k_current_get()));
}

static enum ethernet_hw_caps eth_nxp_enet_qos_get_capabilities(const struct device *dev)
{
	return ETHERNET_LINK_100BASE | ETHERNET_LINK_10BASE | ENET_MAC_PACKET_FILTER_PM_MASK;
}

static bool software_owns_descriptor(volatile union nxp_enet_qos_rx_desc *desc)
{
	return (desc->write.control3 & OWN_FLAG) != OWN_FLAG;
}

/* this function resumes the rx dma in case of underrun */
static void enet_qos_dma_rx_resume(const struct device *dev)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	enet_qos_t *base = config->base;
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_rx_data *rx_data = &data->rx;

	if (!atomic_cas(&rx_data->rbu_flag, 1, 0)) {
		/* no RBU flag means no underrun happened */
		return;
	}

	LOG_DBG("Handle RX underrun");

	/* We need to just touch the tail ptr to make sure it resumes if underrun happened.
	 * Then it will resume if there are DMA owned descriptors queued up in the ring properly.
	 */
	base->DMA_CH[0].DMA_CHX_RXDESC_TAIL_PTR = ENET_QOS_REG_PREP(
		DMA_CH_DMA_CHX_RXDESC_TAIL_PTR, RDTP,
		ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)&rx_data->descriptors[NUM_RX_BUFDESC]));
}


static void eth_nxp_enet_qos_rx(struct k_work *work)
{
	struct nxp_enet_qos_rx_data *rx_data =
		CONTAINER_OF(work, struct nxp_enet_qos_rx_data, rx_work);
	struct nxp_enet_qos_mac_data *data =
		CONTAINER_OF(rx_data, struct nxp_enet_qos_mac_data, rx);
	const struct device *dev = data->iface->if_dev->dev;
	volatile union nxp_enet_qos_rx_desc *desc_arr = data->rx.descriptors;
	uint32_t desc_idx = rx_data->next_desc_idx;
	volatile union nxp_enet_qos_rx_desc *desc = &desc_arr[desc_idx];
	struct net_pkt *pkt = NULL;
	struct net_buf *new_buf;
	struct net_buf *buf;
	size_t pkt_len;
	size_t processed_len;

	LOG_DBG("RX work start: %p", work);

	/* Walk through the descriptor ring and refresh the descriptors we own so that the
	 * DMA can use them for receiving again. We stop when we reach DMA owned part of ring.
	 */
	while (software_owns_descriptor(desc)) {
		rx_data->next_desc_idx = (desc_idx + 1U) % NUM_RX_BUFDESC;

		if (pkt == NULL) {
			if ((desc->write.control3 & FIRST_DESCRIPTOR_FLAG) !=
				FIRST_DESCRIPTOR_FLAG) {
				LOG_DBG("receive packet mask %X ",
					(desc->write.control3 >> 28) & 0x0f);
				/* Error statistics for this packet already updated earlier */
				LOG_DBG("dropping frame from errored packet");
				desc->read.control = rx_desc_refresh_flags;
				goto next;
			}

			/* Otherwise, we found a packet that we need to process */
			pkt = net_pkt_rx_alloc(K_NO_WAIT);

			if (!pkt) {
				LOG_WRN("Could not alloc new RX pkt");
				/* error: no new buffer, reuse previous immediately */
				desc->read.control = rx_desc_refresh_flags;
				eth_stats_update_errors_rx(data->iface);
				goto next;
			}

			processed_len = 0U;

			LOG_DBG("Created new RX pkt %u of %d: %p",
				desc_idx + 1U, NUM_RX_BUFDESC, pkt);
		}

		/* Read the cumulative length of data in this buffer and previous buffers (if any).
		 * The complete length is in a descriptor with the last descriptor flag set
		 * (note that it includes four byte FCS as well). This length will be validated
		 * against processed_len to ensure it's within expected bounds.
		 */
		pkt_len = desc->write.control3 & DESC_RX_PKT_LEN;
		if ((pkt_len < processed_len) ||
			((pkt_len - processed_len) > ENET_QOS_RX_BUFFER_SIZE)) {
			LOG_WRN("Invalid packet length in descriptor: pkt_len=%u, processed_len=%u",
				pkt_len, processed_len);
			net_pkt_unref(pkt);
			pkt = NULL;
			desc->read.control = rx_desc_refresh_flags;
			eth_stats_update_errors_rx(data->iface);
			goto next;
		}

		/* We need to know if we can replace the reserved fragment in advance.
		 * At no point can we allow the driver to have less the amount of reserved
		 * buffers it needs to function, so we will not give up our previous buffer
		 * unless we know we can get a new one.
		 */
		new_buf = net_pkt_get_frag(pkt, CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		if (new_buf == NULL) {
			/* We have no choice but to lose the previous packet,
			 * as the buffer is more important. If we recv this packet,
			 * we don't know what the upper layer will do to our poor buffer.
			 * drop this buffer, reuse allocated DMA buffer
			 */
			LOG_WRN("No new RX buf available");
			net_pkt_unref(pkt);
			pkt = NULL;
			desc->read.control = rx_desc_refresh_flags;
			eth_stats_update_errors_rx(data->iface);
			goto next;
		}

		/* Append buffer to a packet */
		buf = data->rx.reserved_bufs[desc_idx];
		net_buf_add(buf, pkt_len - processed_len);
		net_pkt_frag_add(pkt, buf);
		processed_len = pkt_len;

		if ((desc->write.control3 & LAST_DESCRIPTOR_FLAG) == LAST_DESCRIPTOR_FLAG) {
			/* Propagate completed packet to network stack */
			LOG_DBG("Receiving RX packet");
			if (net_recv_data(data->iface, pkt)) {
				LOG_WRN("RECV failed on pkt %p", pkt);
				/* Error during processing, we continue with new buffer */
				net_pkt_unref(pkt);
				eth_stats_update_errors_rx(data->iface);
			} else {
				/* Record successfully received packet */
				eth_stats_update_pkts_rx(data->iface);
			}
			pkt = NULL;
		}

		LOG_DBG("Swap RX buf %p for %p", data->rx.reserved_bufs[desc_idx], new_buf);
		/* Allow receive into a new buffer */
		data->rx.reserved_bufs[desc_idx] = new_buf;
		desc->read.buf1_addr = (uint32_t)new_buf->data;
		desc->read.control = rx_desc_refresh_flags;

next:
		desc_idx = rx_data->next_desc_idx;
		desc = &desc_arr[desc_idx];
	}

	if (pkt != NULL) {
		/* Looped through descriptors without reaching the final
		 * fragment of the packet, deallocate the incomplete one
		 */
		LOG_DBG("Incomplete packet received, cleaning up");
		net_pkt_unref(pkt);
		pkt = NULL;
		eth_stats_update_errors_rx(data->iface);
	}

	/* now that we updated the descriptors, resume in case we are suspended */
	enet_qos_dma_rx_resume(dev);

	LOG_DBG("End RX work normally");
	return;
}

static void eth_nxp_enet_qos_mac_isr(const struct device *dev)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_rx_data *rx_data = &data->rx;
	enet_qos_t *base = config->base;

	/* cleared on read */
	(void)base->MAC_INTERRUPT_STATUS;
	(void)base->MAC_RX_TX_STATUS;
	uint32_t dma_interrupts = base->DMA_INTERRUPT_STATUS;
	uint32_t dma_ch0_interrupts = base->DMA_CH[0].DMA_CHX_STAT;

	/* clear pending bits except RBU
	 * handle the receive underrun in the worker
	 */
	base->DMA_CH[0].DMA_CHX_STAT = dma_ch0_interrupts;

	if (dma_ch0_interrupts & ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_STAT, RBU, 0b1)) {
		atomic_set(&data->rx.rbu_flag, 1);
	}

	if (ENET_QOS_REG_GET(DMA_INTERRUPT_STATUS, DC0IS, dma_interrupts)) {
		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, TI, dma_ch0_interrupts)) {
			/* add pending tx to queue */
			tx_dma_done(dev);
		}

		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, RI, dma_ch0_interrupts)) {
			/* add pending rx to queue */
			k_work_submit_to_queue(&rx_work_queue, &rx_data->rx_work);
		}
		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, FBE, dma_ch0_interrupts)) {
			LOG_ERR("Fatal bus error: RX:%x, TX:%x", (dma_ch0_interrupts >> 19) & 0x07,
				(dma_ch0_interrupts >> 16) & 0x07);
		}
		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, RBU, dma_ch0_interrupts)) {
			LOG_WRN("RX buffer underrun");
			if (!ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, RI, dma_ch0_interrupts)) {
				/* RBU might happen without RI, schedule worker */
				k_work_submit_to_queue(&rx_work_queue, &rx_data->rx_work);
			}
		}
		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, TBU, dma_ch0_interrupts)) {
			/* by design for now */
		}
	}
}

static inline int enet_qos_dma_reset(enet_qos_t *base)
{
	/* Save off ENET->MAC_MDIO_ADDRESS: CR Field Prior to Reset */
	int cr = 0;

	cr = ENET_QOS_REG_GET(MAC_MDIO_ADDRESS, CR, base->MAC_MDIO_ADDRESS);
	/* Set the software reset of the DMA */
	base->DMA_MODE |= ENET_QOS_REG_PREP(DMA_MODE, SWR, 0b1);

	if (CONFIG_ETH_NXP_ENET_QOS_DMA_RESET_WAIT_TIME == 0) {
		/* spin and wait forever for the reset flag to clear */
		while (ENET_QOS_REG_GET(DMA_MODE, SWR, base->DMA_MODE)) {
			;
		}
		goto done;
	}

	int wait_chunk = DIV_ROUND_UP(CONFIG_ETH_NXP_ENET_QOS_DMA_RESET_WAIT_TIME,
				      NUM_SWR_WAIT_CHUNKS);

	for (int time_elapsed = 0;
		time_elapsed < CONFIG_ETH_NXP_ENET_QOS_DMA_RESET_WAIT_TIME;
		time_elapsed += wait_chunk) {

		k_busy_wait(wait_chunk);

		if (!ENET_QOS_REG_GET(DMA_MODE, SWR, base->DMA_MODE)) {
			/* DMA cleared the bit */
			goto done;
		}
	}

	/* all ENET QOS domain clocks must resolve to clear software reset,
	 * if getting this error, try checking phy clock connection
	 */
	LOG_ERR("Can't clear SWR");
	return -EIO;

done:
	base->MAC_MDIO_ADDRESS = ENET_QOS_REG_PREP(MAC_MDIO_ADDRESS, CR, cr);
	return 0;
}

static inline void enet_qos_dma_config_init(enet_qos_t *base)
{
	base->DMA_CH[0].DMA_CHX_TX_CTRL |=
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TX_CTRL, TxPBL, 0b1);
	base->DMA_CH[0].DMA_CHX_RX_CTRL |=
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, RxPBL, 0b1);
}

static inline void enet_qos_mtl_config_init(enet_qos_t *base)
{
	base->MTL_QUEUE[0].MTL_TXQX_OP_MODE |=
		/* Flush the queue */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, FTQ, 0b1);

	/* Wait for flush to finish */
	while (ENET_QOS_REG_GET(MTL_QUEUE_MTL_TXQX_OP_MODE, FTQ,
				base->MTL_QUEUE[0].MTL_TXQX_OP_MODE)) {
		;
	}

	/* Enable only Transmit Queue 0 (optimization/configuration pending) with maximum size */
	base->MTL_QUEUE[0].MTL_TXQX_OP_MODE =
		/* Sets the size */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, TQS, 0b111) |
		/* Sets it to on */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, TXQEN, 0b10);

	/* Enable only Receive Queue 0 (optimization/configuration pending) with maximum size */
	base->MTL_QUEUE[0].MTL_RXQX_OP_MODE |=
		/* Sets the size */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_RXQX_OP_MODE, RQS, 0b111) |
		/* Keep small packets */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_RXQX_OP_MODE, FUP, 0b1);
}

static inline void enet_qos_mac_config_init(enet_qos_t *base, struct nxp_enet_qos_mac_data *data,
					    uint32_t clk_rate)
{
	/* Set MAC address */
	base->MAC_ADDRESS0_HIGH =
		ENET_QOS_REG_PREP(MAC_ADDRESS0_HIGH, ADDRHI,
					data->mac_addr.addr[5] << 8 |
					data->mac_addr.addr[4]);
	base->MAC_ADDRESS0_LOW =
		ENET_QOS_REG_PREP(MAC_ADDRESS0_LOW, ADDRLO,
					data->mac_addr.addr[3] << 24 |
					data->mac_addr.addr[2] << 16 |
					data->mac_addr.addr[1] << 8  |
					data->mac_addr.addr[0]);

	/* permit multicast packets if there is no space in hash table for mac addresses */
	if ((base->MAC_HW_FEAT[1] & ENET_MAC_HW_FEAT_HASHTBLSZ_MASK) == 0) {
		base->MAC_PACKET_FILTER |= ENET_MAC_PACKET_FILTER_PM_MASK;
	}

	/* Set the reference for 1 microsecond of ENET QOS CSR clock cycles */
	base->MAC_ONEUS_TIC_COUNTER =
		ENET_QOS_REG_PREP(MAC_ONEUS_TIC_COUNTER, TIC_1US_CNTR,
					(clk_rate / USEC_PER_SEC) - 1);

	base->MAC_CONFIGURATION |=
		/* For 10/100 Mbps operation */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, PS, 0b1) |
		/* Full duplex mode */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, DM, 0b1) |
		/* 100 Mbps mode, adjust link speed in phy callback if needed */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, FES, 0b1) |
		/* Don't talk unless no one else is talking */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, ECRSFD, 0b1);

	/* Enable the MAC RX channel 0 */
	base->MAC_RXQ_CTRL[0] |=
		ENET_QOS_REG_PREP(MAC_RXQ_CTRL, RXQ0EN, 0b1);
}

static inline void enet_qos_start(enet_qos_t *base)
{
	/* Set start bits of the RX and TX DMAs */
	base->DMA_CH[0].DMA_CHX_RX_CTRL |=
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, SR, 0b1);
	base->DMA_CH[0].DMA_CHX_TX_CTRL |=
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TX_CTRL, ST, 0b1);

	/* Enable interrupts */
	base->DMA_CH[0].DMA_CHX_INT_EN =
		/* Normal interrupts (includes tx, rx) */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, NIE, 0b1) |
		/* Transmit interrupt */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, TIE, 0b1) |
		/* Receive interrupt */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RIE, 0b1) |
		/* Abnormal interrupts (includes rbu, rs) */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, AIE, 0b1) |
		/* Receive buffer unavailable */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RBUE, 0b1) |
		/* Receive stopped */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RSE, 0b1);
	base->MAC_INTERRUPT_ENABLE =
		/* Receive and Transmit IRQs */
		ENET_QOS_REG_PREP(MAC_INTERRUPT_ENABLE, TXSTSIE, 0b1) |
		ENET_QOS_REG_PREP(MAC_INTERRUPT_ENABLE, RXSTSIE, 0b1);

	/* Start the TX and RX on the MAC */
	base->MAC_CONFIGURATION |=
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, TE, 0b1) |
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, RE, 0b1);
}

static inline void enet_qos_tx_desc_init(enet_qos_t *base, struct nxp_enet_qos_tx_data *tx)
{
	memset((void *)tx->descriptors, 0, sizeof(union nxp_enet_qos_tx_desc) * NUM_TX_BUFDESC);

	base->DMA_CH[0].DMA_CHX_TXDESC_LIST_ADDR =
		/* Start of tx descriptors buffer */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TXDESC_LIST_ADDR, TDESLA,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)tx->descriptors));
	base->DMA_CH[0].DMA_CHX_TXDESC_TAIL_PTR =
		/* Do not move the tail pointer past the start until send is requested */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TXDESC_TAIL_PTR, TDTP,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)tx->descriptors));
	base->DMA_CH[0].DMA_CHX_TXDESC_RING_LENGTH =
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TXDESC_RING_LENGTH, TDRL, NUM_TX_BUFDESC);
}

static inline int enet_qos_rx_desc_init(enet_qos_t *base, struct nxp_enet_qos_rx_data *rx)
{
	struct net_buf *buf;

	memset((void *)rx->descriptors, 0, sizeof(union nxp_enet_qos_rx_desc) * NUM_RX_BUFDESC);

	/* Here we reserve an RX buffer for each of the DMA descriptors. */
	for (int i = 0; i < NUM_RX_BUFDESC; i++) {
		buf = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Missing a buf");
			return -ENOMEM;
		}
		rx->reserved_bufs[i] = buf;
		rx->descriptors[i].read.buf1_addr = (uint32_t)buf->data;
		rx->descriptors[i].read.control |= rx_desc_refresh_flags;
	}

	/* Set next descriptor where data will be received */
	rx->next_desc_idx = 0U;

	/* Set up RX descriptors on channel 0 */
	base->DMA_CH[0].DMA_CHX_RXDESC_LIST_ADDR =
		/* Start of tx descriptors buffer */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RXDESC_LIST_ADDR, RDESLA,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)&rx->descriptors[0]));
	base->DMA_CH[0].DMA_CHX_RXDESC_TAIL_PTR =
		/* When the DMA reaches the tail pointer, it suspends. Set to last descriptor */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RXDESC_TAIL_PTR, RDTP,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)&rx->descriptors[NUM_RX_BUFDESC]));
	base->DMA_CH[0].DMA_CHX_RX_CONTROL2 =
		/* Ring length == Buffer size. Register is this value minus one. */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CONTROL2, RDRL, NUM_RX_BUFDESC - 1);
	base->DMA_CH[0].DMA_CHX_RX_CTRL |=
		/* Set DMA receive buffer size. The low 2 bits are not entered to this field. */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, RBSZ_13_Y, ENET_QOS_RX_BUFFER_SIZE >> 2);

	return 0;
}

#if defined(CONFIG_ETH_NXP_ENET_QOS_MAC_UNIQUE_MAC_ADDRESS)
/* Note this is not universally unique, it just is probably unique on a network */
static inline void nxp_enet_unique_mac(uint8_t *mac_addr)
{
	uint8_t unique_device_ID_16_bytes[16] = {0};
	ssize_t uuid_length =
		hwinfo_get_device_id(unique_device_ID_16_bytes, sizeof(unique_device_ID_16_bytes));
	uint32_t hash = 0;

	if (uuid_length > 0) {
		hash = crc24_pgp((uint8_t *)unique_device_ID_16_bytes, uuid_length);
	} else {
		LOG_ERR("No unique MAC can be provided in this platform");
	}

	/* Setting LAA bit because it is not guaranteed universally unique */
	mac_addr[0] = NXP_OUI_BYTE_0 | 0x02;
	mac_addr[1] = NXP_OUI_BYTE_1;
	mac_addr[2] = NXP_OUI_BYTE_2;
	mac_addr[3] = FIELD_GET(0xFF0000, hash);
	mac_addr[4] = FIELD_GET(0x00FF00, hash);
	mac_addr[5] = FIELD_GET(0x0000FF, hash);
}
#else
#define nxp_enet_unique_mac(arg)
#endif

static int eth_nxp_enet_qos_mac_init(const struct device *dev)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_config *module_cfg = ENET_QOS_MODULE_CFG(config->enet_dev);
	enet_qos_t *base = module_cfg->base;
	uint32_t clk_rate;
	int ret;

	/* Used to configure timings of the MAC */
	ret = clock_control_get_rate(module_cfg->clock_dev, module_cfg->clock_subsys, &clk_rate);
	if (ret) {
		return ret;
	}

	switch (config->mac_addr_source) {
	case NXP_ENET_QOS_MAC_ADDR_SOURCE_RANDOM:
		gen_random_mac(data->mac_addr.addr, NXP_OUI_BYTE_0, NXP_OUI_BYTE_1, NXP_OUI_BYTE_2);
		break;

	case NXP_ENET_QOS_MAC_ADDR_SOURCE_UNIQUE:
		nxp_enet_unique_mac(data->mac_addr.addr);
		break;

	default:
		break;
	}

	/* Effectively reset of the peripheral */
	ret = enet_qos_dma_reset(base);
	if (ret) {
		return ret;
	}

	/* DMA is the interface presented to software for interaction by the ENET module */
	enet_qos_dma_config_init(base);

	/*
	 * MTL = MAC Translation Layer.
	 * MTL is an asynchronous circuit needed because the MAC transmitter/receiver
	 * and the DMA interface are on different clock domains, MTL compromises the two.
	 */
	enet_qos_mtl_config_init(base);

	/* Configuration of the actual MAC hardware */
	enet_qos_mac_config_init(base, data, clk_rate);

	/* Current use of TX descriptor in the driver is such that
	 * one packet is sent at a time, and each descriptor is used
	 * to collect the fragments of it from the networking stack,
	 * and send them with a zero copy implementation.
	 */
	enet_qos_tx_desc_init(base, &data->tx);

	/* Current use of RX descriptor in the driver is such that
	 * each RX descriptor corresponds to a reserved fragment, that will
	 * hold the entirety of the contents of a packet. And these fragments
	 * are recycled in and out of the RX pkt buf pool to achieve a zero copy implementation.
	 */
	ret = enet_qos_rx_desc_init(base, &data->rx);
	if (ret) {
		return ret;
	}

	/* Clearly, start the cogs to motion. */
	enet_qos_start(base);

	/* The tx sem is taken during ethernet send function,
	 * and given when DMA transmission is finished. Ie, send calls will be blocked
	 * until the DMA is available again. This is therefore a simple but naive implementation.
	 */
	k_sem_init(&data->tx.tx_sem, 1, 1);

	/* Work upon a reception of a packet to a buffer */
	k_work_init(&data->rx.rx_work, eth_nxp_enet_qos_rx);

	/* This driver cannot work without interrupts. */
	if (config->irq_config_func) {
		config->irq_config_func();
	} else {
		return -ENOSYS;
	}

	return ret;
}

static const struct device *eth_nxp_enet_qos_get_phy(const struct device *dev)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;

	return config->phy_dev;
}



static int eth_nxp_enet_qos_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *cfg)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_config *module_cfg = ENET_QOS_MODULE_CFG(config->enet_dev);
	enet_qos_t *base = module_cfg->base;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr.addr,
		       cfg->mac_address.addr,
		       sizeof(data->mac_addr.addr));
		/* Set MAC address */
		base->MAC_ADDRESS0_HIGH =
			ENET_QOS_REG_PREP(MAC_ADDRESS0_HIGH, ADDRHI,
						data->mac_addr.addr[5] << 8 |
						data->mac_addr.addr[4]);
		base->MAC_ADDRESS0_LOW =
			ENET_QOS_REG_PREP(MAC_ADDRESS0_LOW, ADDRLO,
						data->mac_addr.addr[3] << 24 |
						data->mac_addr.addr[2] << 16 |
						data->mac_addr.addr[1] << 8  |
						data->mac_addr.addr[0]);
		net_if_set_link_addr(data->iface, data->mac_addr.addr,
				     sizeof(data->mac_addr.addr),
				     NET_LINK_ETHERNET);
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			data->mac_addr.addr[0], data->mac_addr.addr[1],
			data->mac_addr.addr[2], data->mac_addr.addr[3],
			data->mac_addr.addr[4], data->mac_addr.addr[5]);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init = eth_nxp_enet_qos_iface_init,
	.send = eth_nxp_enet_qos_tx,
	.get_capabilities = eth_nxp_enet_qos_get_capabilities,
	.get_phy = eth_nxp_enet_qos_get_phy,
	.set_config	= eth_nxp_enet_qos_set_config,
};

#define NXP_ENET_QOS_NODE_HAS_MAC_ADDR_CHECK(n)                                                    \
	BUILD_ASSERT(NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)) ||                                    \
			     DT_INST_PROP(n, zephyr_random_mac_address) ||                         \
			     DT_INST_PROP(n, nxp_unique_mac),                                      \
		     "MAC address not specified on ENET QOS DT node");

#define NXP_ENET_QOS_MAC_ADDR_SOURCE(n)                                                            \
	COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address),					   \
			(NXP_ENET_QOS_MAC_ADDR_SOURCE_RANDOM),					   \
	(COND_CODE_1(DT_INST_PROP(n, nxp_unique_mac),						   \
			(NXP_ENET_QOS_MAC_ADDR_SOURCE_UNIQUE),					   \
	(COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), local_mac_address),			   \
			(NXP_ENET_QOS_MAC_ADDR_SOURCE_LOCAL),					   \
	(NXP_ENET_QOS_MAC_ADDR_SOURCE_INVALID))))))

#define NXP_ENET_QOS_CONNECT_IRQS(node_id, prop, idx)                                              \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQN_BY_IDX(node_id, idx), DT_IRQ_BY_IDX(node_id, idx, priority),   \
			    eth_nxp_enet_qos_mac_isr, DEVICE_DT_GET(node_id), 0);                  \
		irq_enable(DT_IRQN_BY_IDX(node_id, idx));                                          \
	} while (false);

#define NXP_ENET_QOS_IRQ_CONFIG_FUNC(n)                                                            \
	static void nxp_enet_qos_##n##_irq_config_func(void)                                       \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), interrupt_names, NXP_ENET_QOS_CONNECT_IRQS)   \
	}
#define NXP_ENET_QOS_DRIVER_STRUCTS_INIT(n)                                                        \
	static const struct nxp_enet_qos_mac_config enet_qos_##n##_mac_config = {                  \
		.enet_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                          \
		.base = (enet_qos_t *)DT_REG_ADDR(DT_INST_PARENT(n)),                              \
		.hw_info =                                                                         \
			{                                                                          \
				.max_frame_len = ENET_QOS_MAX_NORMAL_FRAME_LEN,                    \
			},                                                                         \
		.irq_config_func = nxp_enet_qos_##n##_irq_config_func,                             \
		.mac_addr_source = NXP_ENET_QOS_MAC_ADDR_SOURCE(n),                                \
	};                                                                                         \
	static struct nxp_enet_qos_mac_data enet_qos_##n##_mac_data = {                            \
		.mac_addr.addr = DT_INST_PROP_OR(n, local_mac_address, {0}),                       \
	};

#define NXP_ENET_QOS_DRIVER_INIT(n)                                                                \
	NXP_ENET_QOS_NODE_HAS_MAC_ADDR_CHECK(n)                                                    \
	NXP_ENET_QOS_IRQ_CONFIG_FUNC(n)                                                            \
	NXP_ENET_QOS_DRIVER_STRUCTS_INIT(n)

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_QOS_DRIVER_INIT)

#define NXP_ENET_QOS_MAC_DEVICE_DEFINE(n)                                                          \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_nxp_enet_qos_mac_init, NULL,                          \
				      &enet_qos_##n##_mac_data, &enet_qos_##n##_mac_config,        \
				      CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_QOS_MAC_DEVICE_DEFINE)
