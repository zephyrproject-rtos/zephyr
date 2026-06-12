/*
 * Copyright 2024-2026 NXP
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
#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
#include <zephyr/drivers/ptp_clock.h>
#endif

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

	/* handle link speed and duplex in MAC configuration register */
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

		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			LOG_DBG("Link Full Duplex");
			base->MAC_CONFIGURATION |= ENET_QOS_REG_PREP(MAC_CONFIGURATION, DM, 0b1);
		} else {
			LOG_DBG("Link Half Duplex");
			base->MAC_CONFIGURATION &= ~ENET_QOS_REG_PREP(MAC_CONFIGURATION, DM, 0b1);
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

	data->iface = iface;

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
	int frags_count = 0, total_bytes = 0, frags_idx = 0;
	int ret;
#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
	bool pkt_is_ptp;
#endif

	/* Report ENETDOWN immediately when the controller is stopped (e.g.
	 * link is down) instead of letting the caller waste work and then
	 * fail later at the tx_sem with EAGAIN.
	 */
	if (!atomic_get(&data->running)) {
		return -ENETDOWN;
	}

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

	/* Re-check the gate now that we hold the sem: .stop may have flipped
	 * running to 0 between the early-return above and our sem_take, and
	 * a stale acquire-read of running could still let us through. .stop
	 * blocks on this sem before touching tx.pkt, so giving the sem back
	 * here both bails this submission cleanly and lets .stop's bounded
	 * sem_take return immediately instead of waiting out its timeout.
	 */
	if (!atomic_get(&data->running)) {
		k_sem_give(&data->tx.tx_sem);
		return -ENETDOWN;
	}

	net_pkt_ref(pkt);
	data->tx.pkt = pkt;

	LOG_DBG("Setting up TX descriptors for packet %p", pkt);

	/* Reset the descriptors */
	memset((void *)data->tx.descriptors, 0, sizeof(union nxp_enet_qos_tx_desc) * frags_count);

	/* Setting up the descriptors  */
	fragment = pkt->frags;
	tx_desc_ptr->read.control2 = FIRST_DESCRIPTOR_FLAG;
	while (frags_idx < frags_count) {
		net_pkt_frag_ref(fragment);

		tx_desc_ptr->read.buf1_addr = (uint32_t)fragment->data;
		tx_desc_ptr->read.control1 = FIELD_PREP(0x3FFF, fragment->len);
		tx_desc_ptr->read.control2 |= FIELD_PREP(0x7FFF, total_bytes);

		/* if there are more fragments use buffer2 - ringbuffer mode */
		if (frags_idx + 1 < frags_count) {
			fragment = fragment->frags;
			net_pkt_frag_ref(fragment);

			tx_desc_ptr->read.buf2_addr = (uint32_t)fragment->data;
			tx_desc_ptr->read.control1 |= FIELD_PREP(0x3FFF0000, fragment->len);
			frags_idx++;
		}

		fragment = fragment->frags;
		tx_desc_ptr++;
		frags_idx++;
	}
	last_desc_ptr = tx_desc_ptr - 1;
	last_desc_ptr->read.control2 |= LAST_DESCRIPTOR_FLAG;
	last_desc_ptr->read.control1 |= TX_INTERRUPT_ON_COMPLETE_FLAG;

#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
	pkt_is_ptp = net_ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP;
	if (net_pkt_is_tx_timestamping(pkt) || pkt_is_ptp) {
		LOG_DBG("SET TX TIMESTAMP %p control %x", pkt, base->MAC_TIMESTAMP_CONTROL);
		last_desc_ptr->read.control1 |= TX_TIMESTAMP_ENABLE_FLAG;
	}
#endif

	LOG_DBG("Starting TX DMA on packet %p", pkt);
	data->tx.num_descs = (frags_count + 1) / 2;

	/* Set the DMA ownership of all the used descriptors */
	__DMB();
	for (int i = 0; i < data->tx.num_descs; i++) {
		data->tx.descriptors[i].read.control2 |= OWN_FLAG;
	}
	__DSB();

	/* This implementation is clearly naive and basic, it just changes the
	 * ring length for every TX send, there is room for optimization
	 */
	base->DMA_CH[0].DMA_CHX_TXDESC_RING_LENGTH = data->tx.num_descs - 1;
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
	struct net_buf *fragment;

	if (pkt == NULL) {
		/* Two legitimate ways to reach here:
		 *   - .stop already snapshot-and-nulled tx.pkt under irq_lock
		 *     and a late TI ISR (e.g. one latched in the NVIC just
		 *     before .stop's mask write) is now dispatching.
		 *   - FTQ self-completion latched a TI for the abandoned frame
		 *     that the NVIC then redelivers after .start re-unmasks.
		 * Both cases are expected with the current stop sequence, so
		 * only warn when we are still nominally running.
		 */
		if (atomic_get(&data->running)) {
			LOG_WRN("%s TX DMA done on nonexistent packet?", dev->name);
		} else {
			LOG_DBG("%s TX DMA done on nonexistent packet (post-stop race)",
				dev->name);
		}
		goto skip;
	}

	fragment = pkt->frags;
	LOG_DBG("TX DMA completed on packet %p", pkt);

#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
	volatile union nxp_enet_qos_tx_desc *last_desc =
		&tx_data->descriptors[tx_data->num_descs - 1];

	if (last_desc->write.status & LAST_DESCRIPTOR_FLAG) {
		LOG_DBG("LAST DESCRIPTOR : status: %X", last_desc->write.status);
	}
	if (last_desc->write.status & TX_TIMESTAMP_STATUS_FLAG) {
		pkt->timestamp.nanosecond = last_desc->write.timestamp_low;
		pkt->timestamp.second = last_desc->write.timestamp_high;
		LOG_DBG("TX HARD TIMESTAMP %llu.%09u", pkt->timestamp.second,
			pkt->timestamp.nanosecond);
		net_if_add_tx_timestamp(pkt);
	}
#endif

	/* Returning the buffers and packet to the pool */
	while (fragment != NULL) {
		net_pkt_frag_unref(fragment);
		fragment = fragment->frags;
	}
	net_pkt_unref(pkt);

	/* Mark the slot empty so .stop can tell a true in-flight TX from
	 * one that already completed.
	 */
	tx_data->pkt = NULL;

	eth_stats_update_pkts_tx(data->iface);

skip:
	/* Allows another send */
	k_sem_give(&data->tx.tx_sem);
	LOG_DBG("Gave driver TX sem %p by thread %s", &data->tx.tx_sem,
						      k_thread_name_get(k_current_get()));
}

static enum ethernet_hw_caps eth_nxp_enet_qos_get_capabilities(const struct device *dev __unused,
							      struct net_if *iface __unused)
{
	enum ethernet_hw_caps caps = ETHERNET_LINK_100BASE | ETHERNET_LINK_10BASE;

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	caps |= ETHERNET_PROMISC_MODE;
#endif
#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
	caps |= ETHERNET_PTP;
#endif
	return caps;
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
	const struct device *dev = net_if_get_device(data->iface);
	volatile union nxp_enet_qos_rx_desc *desc_arr = data->rx.descriptors;
	uint32_t desc_idx = rx_data->next_desc_idx;
	volatile union nxp_enet_qos_rx_desc *desc = &desc_arr[desc_idx];
	struct net_pkt *pkt = NULL;
	struct net_buf *new_buf;
	struct net_buf *buf;
	size_t pkt_len;
	size_t processed_len;
	int ret;

	/* The work item may have been submitted by the IRQ just before .stop
	 * masked the DMA/MAC interrupts and called k_work_cancel_sync(). Skip
	 * the descriptor walk in that narrow race window.
	 */
	if (!atomic_get(&data->running)) {
		return;
	}

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
		 * The complete length is in a descriptor with the last descriptor flag set.
		 * The MAC strips CRC from Type packets, so the packet passed to upper layers
		 * does not include the FCS.
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

#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
			/*
			 * When PTP timestamping is enabled the hardware appends a
			 * context descriptor (RDES3 bit 30 = CTXT) immediately after
			 * the last regular descriptor.  Peek at that slot; if it is
			 * software-owned and carries the CTXT flag, extract the RX
			 * timestamp (RDES0 = nanoseconds, RDES1 = seconds) and
			 * recycle the slot as a regular RX descriptor.
			 */
			{
				uint32_t ctx_idx = rx_data->next_desc_idx;
				volatile union nxp_enet_qos_rx_desc *ctx_desc = &desc_arr[ctx_idx];

				if ((desc->write.control3 & RX_STATUS1_VALID_FLAG) &&
				    (desc->write.control1 & RX_TIMESTAMP_AVAILABLE_FLAG) &&
				    !(ctx_desc->write.control3 & OWN_FLAG) &&
				    (ctx_desc->write.control3 & RECEIVE_CONTEXT_DESCRIPTOR_FLAG)) {
					pkt->timestamp.nanosecond = ctx_desc->write.vlan_tag;
					pkt->timestamp.second = ctx_desc->write.control1;
					net_pkt_set_rx_timestamping(pkt, true);

					/* Recycle the context descriptor slot as a
					 * regular RX descriptor - the reserved_buf at
					 * this index was untouched by the context write.
					 */
					ctx_desc->read.buf1_addr =
						(uint32_t)data->rx.reserved_bufs[ctx_idx]->data;
					ctx_desc->read.control = rx_desc_refresh_flags;
					rx_data->next_desc_idx = (ctx_idx + 1U) % NUM_RX_BUFDESC;
				}
			}
#endif /* CONFIG_PTP_CLOCK_NXP_ENET_QOS */

			ret = net_recv_data(data->iface, pkt);
			if (ret < 0) {
				LOG_WRN("RECV failed on pkt %p (err=%d)", pkt, ret);
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

#if defined(ENET_MTL_QUEUE_MTL_TXQX_OP_MODE_TQS) && defined(ENET_MTL_QUEUE_MTL_TXQX_OP_MODE_TXQEN)
	/* Enable only Transmit Queue 0 (optimization/configuration pending) with maximum size */
	base->MTL_QUEUE[0].MTL_TXQX_OP_MODE =
		/* Store and forward is required for reliable PTP timestamps. */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, TSF, 0b1) |
		/* Sets the size */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, TQS, 0b111) |
		/* Sets it to on */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, TXQEN, 0b10);
#endif

	/* Enable only Receive Queue 0 (optimization/configuration pending) with maximum size */
	base->MTL_QUEUE[0].MTL_RXQX_OP_MODE |=
#ifdef ENET_MTL_QUEUE_MTL_RXQX_OP_MODE_RQS
		/* Sets the size */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_RXQX_OP_MODE, RQS, 0b111) |
#endif
		/* Store and forward is required for reliable PTP timestamps. */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_RXQX_OP_MODE, RSF, 0b1) |
		/* Keep small packets */
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_RXQX_OP_MODE, FUP, 0b1);
}

static inline void enet_qos_mac_config_init(enet_qos_t *base, struct nxp_enet_qos_mac_data *data,
					    uint32_t clk_rate)
{
	uint32_t mac_configuration;

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

#ifdef ENET_MAC_ONEUS_TIC_COUNTER_TIC_1US_CNTR
	/* Set the reference for 1 microsecond of ENET QOS CSR clock cycles */
	base->MAC_ONEUS_TIC_COUNTER =
		ENET_QOS_REG_PREP(MAC_ONEUS_TIC_COUNTER, TIC_1US_CNTR,
					(clk_rate / USEC_PER_SEC) - 1);
#endif

	mac_configuration = base->MAC_CONFIGURATION;
	mac_configuration |=
		/* For 10/100 Mbps operation */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, PS, 0b1) |
		/* Strip CRC from Type packets before handing frames to the stack */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, CST, 0b1) |
		/* Don't talk unless no one else is talking */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, ECRSFD, 0b1);
	mac_configuration &= ~(
		/* Half duplex mode, adjust duplex in phy callback if needed */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, DM, 0b1) |
		/* 10 Mbps mode, adjust link speed in phy callback if needed */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, FES, 0b1)
	);
	base->MAC_CONFIGURATION = mac_configuration;

#ifdef ENET_MAC_RXQ_CTRL_RXQ0EN
	/* Enable the MAC RX channel 0 */
	base->MAC_RXQ_CTRL[0] |=
		ENET_QOS_REG_PREP(MAC_RXQ_CTRL, RXQ0EN, 0b1);
#endif
}

static inline void enet_qos_start(enet_qos_t *base)
{
	/* Set start bits of the RX and TX DMAs */
	base->DMA_CH[0].DMA_CHX_RX_CTRL |=
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, SR, 0b1);
	base->DMA_CH[0].DMA_CHX_TX_CTRL |=
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TX_CTRL, ST, 0b1);

	/* Enable interrupts. Use |= so bits owned by sibling blocks
	 * (PTP, LPI, PMT, ...) in MAC_INTERRUPT_ENABLE are preserved; this
	 * mirrors the &= ~(...) mask in eth_nxp_enet_qos_stop().
	 */
	base->DMA_CH[0].DMA_CHX_INT_EN |=
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
	base->MAC_INTERRUPT_ENABLE |=
		/* Receive and Transmit IRQs */
		ENET_QOS_REG_PREP(MAC_INTERRUPT_ENABLE, TXSTSIE, 0b1) |
		ENET_QOS_REG_PREP(MAC_INTERRUPT_ENABLE, RXSTSIE, 0b1);

	/* Start the TX and RX on the MAC */
	base->MAC_CONFIGURATION |=
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, TE, 0b1) |
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, RE, 0b1);
}

/*
 * Poll the MTL Tx Queue 0 debug register until the queue is empty and the
 * read-controller is idle. Bounded so a stuck queue (e.g. transmit clock
 * gated by a link drop) cannot hang the system. Returns -EBUSY on timeout.
 *
 * k_busy_wait() is intentional: empirically the loop exits on the first
 * iteration in every observed scenario, so common-case latency is ~10 us.
 * k_usleep(10) would require a tick/timer round-trip whose own overhead
 * exceeds that. Yielding from inside .stop is also moot because .stop is
 * itself the system-workqueue work item.
 */
static inline int enet_qos_wait_tx_idle(enet_qos_t *base)
{
	/* Bounded busy-wait, ~10 ms total at 10 us granularity. */
	const int max_iter = 1000;
	uint32_t dbg;

	for (int i = 0; i < max_iter; i++) {
		dbg = base->MTL_QUEUE[0].MTL_TXQX_DBG;
		/* Idle when neither TRCSTS reports an active read (==0b01) nor
		 * TXQSTS reports a non-empty queue.
		 */
		if ((ENET_QOS_REG_GET(MTL_QUEUE_MTL_TXQX_DBG, TRCSTS, dbg) != 0x1U) &&
		    (ENET_QOS_REG_GET(MTL_QUEUE_MTL_TXQX_DBG, TXQSTS, dbg) == 0U)) {
			return 0;
		}
		k_busy_wait(10);
	}

	return -EBUSY;
}

/* Same shape as enet_qos_wait_tx_idle(), but for the MTL Rx Queue 0.
 * Call with SR=1 so the RX DMA can still drain the MTL RX FIFO into
 * descriptors; once both PRXQ and RXQSTS read zero the queue is empty
 * and SR can safely be cleared.
 */
static inline int enet_qos_wait_rx_idle(enet_qos_t *base)
{
	const int max_iter = 1000;
	uint32_t dbg;

	for (int i = 0; i < max_iter; i++) {
		dbg = base->MTL_QUEUE[0].MTL_RXQX_DBG;
		if ((ENET_QOS_REG_GET(MTL_QUEUE_MTL_RXQX_DBG, PRXQ, dbg) == 0U) &&
		    (ENET_QOS_REG_GET(MTL_QUEUE_MTL_RXQX_DBG, RXQSTS, dbg) == 0U)) {
			return 0;
		}
		k_busy_wait(10);
	}

	return -EBUSY;
}

/*
 * Re-arm the RX descriptor ring so the DMA can use every slot again.
 *
 * Called on every .start (including the one issued from init via
 * enet_qos_rx_desc_init). Does not touch reserved_bufs[] -- those buffers
 * are reserved once at init and reused for the lifetime of the device so
 * that .start does not have to take from the RX buf pool and cannot fail.
 *
 * Only DMA_CHX_RXDESC_TAIL_PTR is written here. RX_CONTROL2 (ring length)
 * and RX_CTRL.RBSZ are programmed once in enet_qos_rx_desc_init() and
 * survive across SR=0->1 transitions. RXDESC_LIST_ADDR is also persistent,
 * but eth_nxp_enet_qos_start() re-writes it to force the DMA's current-
 * descriptor pointer back to descriptor 0 in sync with next_desc_idx=0;
 * see the comment there.
 */
static inline void enet_qos_rx_desc_rearm(enet_qos_t *base, struct nxp_enet_qos_rx_data *rx)
{
	/* Discard the write-back state from the previous run and hand every
	 * descriptor back to the DMA, pointing at its reserved buffer.
	 */
	memset((void *)rx->descriptors, 0, sizeof(union nxp_enet_qos_rx_desc) * NUM_RX_BUFDESC);
	for (int i = 0; i < NUM_RX_BUFDESC; i++) {
		rx->descriptors[i].read.buf1_addr = (uint32_t)rx->reserved_bufs[i]->data;
		rx->descriptors[i].read.control = rx_desc_refresh_flags;
	}

	/* Drain any stale RBU latched from the previous run so the next RX
	 * work invocation does not see a phantom underrun.
	 */
	atomic_set(&rx->rbu_flag, 0);

	/* Order the descriptor stores (Normal memory, write buffer) before
	 * the Device-memory MMIO write that re-arms the DMA tail pointer.
	 * Without this barrier the DMA could fetch a descriptor before the
	 * CPU has drained the buf1_addr / control stores to memory.
	 */
	__DSB();

	/* DMA restarts from RXDESC_LIST_ADDR (descriptor 0) on SR=0->1, so
	 * match it in software and re-point the tail past the end so the DMA
	 * does not suspend immediately.
	 */
	rx->next_desc_idx = 0U;
	base->DMA_CH[0].DMA_CHX_RXDESC_TAIL_PTR = ENET_QOS_REG_PREP(
		DMA_CH_DMA_CHX_RXDESC_TAIL_PTR, RDTP,
		ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)&rx->descriptors[NUM_RX_BUFDESC]));
}

/*
 * Bring the ENET QoS MAC out of stopped state: re-arm the RX descriptor
 * ring, enable the DMA and MAC TX/RX paths and unmask interrupts.
 * No-op if already running, so it is safe to invoke on every link-up
 * notification from the PHY.
 */
static int eth_nxp_enet_qos_start(const struct device *dev, struct net_if *iface)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_config *module_cfg = ENET_QOS_MODULE_CFG(config->enet_dev);
	enet_qos_t *base = module_cfg->base;

	ARG_UNUSED(iface);

	if (atomic_get(&data->running)) {
		return 0;
	}

	/* Re-arm the RX descriptor ring (no allocation: the buffers reserved
	 * at init are still owned by the driver). The static MAC RX registers
	 * programmed in enet_qos_rx_desc_init() survive across SR=0->1.
	 */
	enet_qos_rx_desc_rearm(base, &data->rx);

	/* Re-program RXDESC_LIST_ADDR so the DMA's current-descriptor
	 * pointer snaps back to descriptor 0 on SR=0->1, matching
	 * next_desc_idx=0 in software. Without this, after a stop/start
	 * cycle the DMA can resume from wherever it was pre-stop while the
	 * software walk restarts from 0, leading to periodic RX stalls
	 * (visible as RBU bursts and ping latency spikes) until the two
	 * cursors meet again. Safe to write here because SR=0 at this point
	 * (.stop cleared it, and .start has not set it yet).
	 * Skipped at the init-path call to enet_qos_rx_desc_rearm() because
	 * enet_qos_rx_desc_init() has just written this register.
	 */
	base->DMA_CH[0].DMA_CHX_RXDESC_LIST_ADDR =
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RXDESC_LIST_ADDR, RDESLA,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)&data->rx.descriptors[0]));

	enet_qos_start(base);

	/* Restore the TX semaphore. .stop reset it; .start re-establishes
	 * the "one TX slot available" initial state. No thread is actually
	 * blocked on the sem because the TX path uses K_NO_WAIT.
	 */
	k_sem_reset(&data->tx.tx_sem);
	k_sem_give(&data->tx.tx_sem);

	atomic_set(&data->running, 1);

	return 0;
}

/*
 * Bring the ENET QoS MAC into stopped state.
 *
 * Sequence (numbered comments inline below):
 *   1. Close the submission gate (running = 0) so new TXes are rejected.
 *   2. Take tx_sem (bounded wait). Serializes against any in-flight
 *      eth_nxp_enet_qos_tx() that may have observed a stale running=1
 *      after step 1: either it bails at its post-take recheck (and gives
 *      the sem back so our take returns immediately), or its frame
 *      completes naturally (tx_dma_done gives the sem), or we time out
 *      and step 10 cleans up. Holding the sem afterwards excludes any
 *      new submitter from racing with the rest of the sequence.
 *   3. Stop the TX DMA (clear ST). RX DMA stays running so MTL RX can
 *      drain in step 5.
 *   4. Disable MAC RX+TX paths in a single RMW on MAC_CONFIGURATION.
 *   5. Wait, with bounded timeout, for the MTL RX queue to drain into
 *      descriptors now that no new frames can enter it (RE=0).
 *   6. Stop the RX DMA (clear SR).
 *   7. Force-flush any frame still queued in the MTL TX FIFO (FTQ),
 *      then bounded-wait for MTL TX idle. We rely on FTQ here as the
 *      single TX-quiesce mechanism rather than the RM-suggested
 *      "TE=0 -> wait TX idle -> ST=0" pattern, because FTQ also
 *      recovers from a stuck MAC (no TX clock on link-down) where the
 *      RM sequence would hang.
 *   8. Mask only the DMA/MAC interrupt bits .start enables, then DSB.
 *   9. Cancel pending RX work (with self-deadlock guard for the case
 *      where .stop is ever called from the RX work thread itself).
 *  10. Snapshot-and-null data->tx.pkt under irq_lock, then release the
 *      packet outside the lock. irq_lock is required because step 8
 *      only stops *new* peripheral IRQ assertions: a TI latched in the
 *      NVIC just before the mask write, or one produced by step 7's
 *      FTQ self-completion, can still dispatch tx_dma_done() between
 *      step 8 and here. Without the snapshot, that ISR and step 10
 *      would race on tx.pkt and double-unref it.
 *
 * No-op if already stopped.
 *
 * Latency note: this function may block for up to ~110 ms in the worst
 * case (100 ms sem wait + 10 ms MTL idle waits). L2 invokes .stop on
 * the system workqueue, so the system WQ is stalled for that long when
 * stopping a stuck link.
 */
static int eth_nxp_enet_qos_stop(const struct device *dev, struct net_if *iface)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	struct nxp_enet_qos_config *module_cfg = ENET_QOS_MODULE_CFG(config->enet_dev);
	enet_qos_t *base = module_cfg->base;

	ARG_UNUSED(iface);

	if (!atomic_get(&data->running)) {
		return 0;
	}

	/* Step 1: close the submission gate. */
	atomic_set(&data->running, 0);

	/* Step 2: serialize against any in-flight submitter. Return value
	 * is intentionally ignored: on success we hold the sem and have
	 * exclusive ownership; on timeout we proceed and step 10 cleans up.
	 * Either way .start will reset+give the sem.
	 */
	(void)k_sem_take(&data->tx.tx_sem, K_MSEC(100));

	/* Step 3: stop the TX DMA. */
	base->DMA_CH[0].DMA_CHX_TX_CTRL &=
		~ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_TX_CTRL, ST, 0b1);

	/* Step 4: disable the MAC TX+RX paths in a single RMW.
	 * MAC_CONFIGURATION lives behind a MAC-clock-domain synchronizer:
	 * a write is captured into the MAC clock domain, and on some EQOS
	 * revisions a second back-to-back write can be dropped or coalesced
	 * until the previous one has finished crossing. Fusing RE and TE
	 * into one write sidesteps that hazard. (Pure bus ordering between
	 * Device-memory accesses to the same address is already guaranteed
	 * by the architecture; this is specifically about the clock-domain
	 * synchronizer.)
	 */
	base->MAC_CONFIGURATION &=
		~(ENET_QOS_REG_PREP(MAC_CONFIGURATION, RE, 0b1) |
		  ENET_QOS_REG_PREP(MAC_CONFIGURATION, TE, 0b1));

	/* Step 5: wait for the MTL RX queue to drain into descriptors.
	 * RE=0 (step 4) blocks new frames from entering MTL RX; SR=1 still
	 * lets the RX DMA consume what is already there. Bounded so a
	 * misbehaving block cannot hang us.
	 */
	if (enet_qos_wait_rx_idle(base) != 0) {
		LOG_WRN("%s .stop: MTL RX did not drain within ~10 ms", dev->name);
	}

	/* Step 6: stop the RX DMA. */
	base->DMA_CH[0].DMA_CHX_RX_CTRL &=
		~ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, SR, 0b1);

	/* Step 7: force-flush the MTL TX queue. FTQ self-clears. This is
	 * unconditional rather than fallback-only because it is the cheapest
	 * way to quiesce a TX path that may be stuck waiting for a TX clock
	 * that will never come (link down, half-duplex collision storm, ...).
	 */
	base->MTL_QUEUE[0].MTL_TXQX_OP_MODE |=
		ENET_QOS_REG_PREP(MTL_QUEUE_MTL_TXQX_OP_MODE, FTQ, 0b1);
	if (enet_qos_wait_tx_idle(base) != 0) {
		LOG_WRN("%s .stop: MTL TX did not drain within ~10 ms after FTQ",
			dev->name);
	}

	/* Step 8: mask only the bits enet_qos_start() sets, so unrelated
	 * bits owned by sibling blocks (PTP, LPI, PMT, ...) are left alone.
	 */
	base->DMA_CH[0].DMA_CHX_INT_EN &= ~(
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, NIE, 0b1) |
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, TIE, 0b1) |
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RIE, 0b1) |
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, AIE, 0b1) |
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RBUE, 0b1) |
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RSE, 0b1));
	base->MAC_INTERRUPT_ENABLE &= ~(
		ENET_QOS_REG_PREP(MAC_INTERRUPT_ENABLE, TXSTSIE, 0b1) |
		ENET_QOS_REG_PREP(MAC_INTERRUPT_ENABLE, RXSTSIE, 0b1));

	/* Ensure the MMIO writes that masked the interrupts have committed
	 * before we touch shared state the ISR would otherwise see.
	 */
	__DSB();

	/* Step 9: cancel pending RX work. Waiting would self-deadlock if
	 * .stop were ever called from the RX work thread itself, so fall
	 * back to a non-blocking cancel in that case (today the only caller
	 * is L2 on the system workqueue).
	 */
	if (k_current_get() != k_work_queue_thread_get(&rx_work_queue)) {
		struct k_work_sync sync;

		(void)k_work_cancel_sync(&data->rx.rx_work, &sync);
	} else {
		(void)k_work_cancel(&data->rx.rx_work);
	}

	/* Step 10: release the in-flight TX packet, if any. Step 8 only
	 * stops *new* peripheral IRQ assertions: a TI bit latched in the
	 * NVIC just before the mask write, or one produced by step 7's FTQ
	 * self-completion, can still dispatch tx_dma_done() after step 8.
	 * Snapshot-and-null tx.pkt under irq_lock so the late ISR and this
	 * cleanup cannot both unref the same packet.
	 */
	{
		struct net_pkt *pkt;
		unsigned int key = irq_lock();

		pkt = data->tx.pkt;
		data->tx.pkt = NULL;
		irq_unlock(key);

		if (pkt != NULL) {
			struct net_buf *fragment = pkt->frags;

			while (fragment != NULL) {
				net_pkt_frag_unref(fragment);
				fragment = fragment->frags;
			}
			net_pkt_unref(pkt);
			eth_stats_update_errors_tx(data->iface);
		}
	}

	return 0;
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

	/* One-time RX buffer reservation: one net_buf per descriptor, held
	 * for the lifetime of the device. Subsequent .start cycles reuse
	 * these via enet_qos_rx_desc_rearm(); the RX work handler keeps the
	 * pool topped up by swapping consumed buffers for fresh ones.
	 */
	for (int i = 0; i < NUM_RX_BUFDESC; i++) {
		buf = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Missing a buf");
			return -ENOMEM;
		}
		rx->reserved_bufs[i] = buf;
	}

	/* One-time MAC RX programming. RXDESC_LIST_ADDR, ring length and
	 * buffer size are preserved across stop/start cycles.
	 */
	base->DMA_CH[0].DMA_CHX_RXDESC_LIST_ADDR =
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RXDESC_LIST_ADDR, RDESLA,
			ENET_QOS_ALIGN_ADDR_SHIFT((uint32_t)&rx->descriptors[0]));
	base->DMA_CH[0].DMA_CHX_RX_CONTROL2 =
		/* Ring length == Buffer size. Register is this value minus one. */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CONTROL2, RDRL, NUM_RX_BUFDESC - 1);
	base->DMA_CH[0].DMA_CHX_RX_CTRL |=
		/* Set DMA receive buffer size. The low 2 bits are not entered to this field. */
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, RBSZ_13_Y, ENET_QOS_RX_BUFFER_SIZE >> 2);

	enet_qos_rx_desc_rearm(base, rx);

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

	/* tx_sem is taken during the ethernet send function and given when DMA
	 * transmission is finished. Send calls are therefore blocked until the
	 * DMA is available again -- a simple but naive single-outstanding-TX
	 * implementation. Initialised to 0 because we leave the controller
	 * stopped at init; .start raises it to 1 once the data path is up.
	 */
	k_sem_init(&data->tx.tx_sem, 0, 1);

	/* Work upon a reception of a packet to a buffer */
	k_work_init(&data->rx.rx_work, eth_nxp_enet_qos_rx);

	/* Leave the controller stopped at init to match iface_init's
	 * net_if_carrier_off(): the data path is only enabled when L2 calls
	 * .start on the link-up edge from the PHY callback. RX buf reservation
	 * has already happened in enet_qos_rx_desc_init() above so it does not
	 * depend on link state.
	 */
	atomic_set(&data->running, 0);

	/* This driver cannot work without interrupts. */
	if (config->irq_config_func) {
		config->irq_config_func();
	} else {
		return -ENOSYS;
	}

	return ret;
}

static const struct device *eth_nxp_enet_qos_get_phy(const struct device *dev,
						     struct net_if *iface __unused)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;

	return config->phy_dev;
}

#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
static const struct device *eth_nxp_enet_qos_get_ptp_clock(const struct device *dev,
							   struct net_if *iface __unused)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;

	return config->ptp_clock;
}
#endif

static int eth_nxp_enet_qos_set_config(const struct device *dev,
			       struct net_if *iface __unused,
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
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			data->mac_addr.addr[0], data->mac_addr.addr[1],
			data->mac_addr.addr[2], data->mac_addr.addr[3],
			data->mac_addr.addr[4], data->mac_addr.addr[5]);
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (cfg->promisc_mode) {
			base->MAC_PACKET_FILTER |= ENET_MAC_PACKET_FILTER_PR_MASK;
		} else {
			base->MAC_PACKET_FILTER &= ~ENET_MAC_PACKET_FILTER_PR_MASK;
		}
		return 0;
#endif
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init = eth_nxp_enet_qos_iface_init,
	.start = eth_nxp_enet_qos_start,
	.stop = eth_nxp_enet_qos_stop,
	.send = eth_nxp_enet_qos_tx,
	.get_capabilities = eth_nxp_enet_qos_get_capabilities,
	.get_phy = eth_nxp_enet_qos_get_phy,
	.set_config = eth_nxp_enet_qos_set_config,
#if defined(CONFIG_PTP_CLOCK_NXP_ENET_QOS)
	.get_ptp_clock = eth_nxp_enet_qos_get_ptp_clock,
#endif
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
		IF_ENABLED(CONFIG_PTP_CLOCK_NXP_ENET_QOS,                                          \
			(.ptp_clock = DEVICE_DT_GET(DT_CHILD(DT_DRV_INST(n), ptp_clock)),)) };     \
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
