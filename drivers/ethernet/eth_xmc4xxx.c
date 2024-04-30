/* XMC4XXX Ethernet controller
 *
 * Copyright (c) 2023 SLB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_ethernet

#include "eth.h"

#include <stdint.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/gptp.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <ethernet/eth_stats.h>

#include <xmc_eth_mac.h>
#include <xmc_scu.h>

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_xmc4xxx);

#define NUM_TX_DMA_DESCRIPTORS CONFIG_ETH_XMC4XXX_NUM_TX_DMA_DESCRIPTORS
#define NUM_RX_DMA_DESCRIPTORS CONFIG_ETH_XMC4XXX_NUM_RX_DMA_DESCRIPTORS

#define ETH_NODE DT_NODELABEL(eth)
#define PHY_NODE DT_PHANDLE_BY_IDX(ETH_NODE, phy, 0)

#define INFINEON_OUI_B0 0x00
#define INFINEON_OUI_B1 0x03
#define INFINEON_OUI_B2 0x19

#define MODULO_INC_TX(val) {(val) = (++(val) < NUM_TX_DMA_DESCRIPTORS) ? (val) : 0; }
#define MODULO_INC_RX(val) {(val) = (++(val) < NUM_RX_DMA_DESCRIPTORS) ? (val) : 0; }

#define IS_OWNED_BY_DMA_TX(desc) (((desc)->status & ETH_MAC_DMA_TDES0_OWN) != 0)
#define IS_OWNED_BY_DMA_RX(desc) (((desc)->status & ETH_MAC_DMA_RDES0_OWN) != 0)

#define IS_START_OF_FRAME_RX(desc) (((desc)->status & ETH_MAC_DMA_RDES0_FS) != 0)
#define IS_END_OF_FRAME_RX(desc) (((desc)->status & ETH_MAC_DMA_RDES0_LS) != 0)

#define IS_TIMESTAMP_AVAILABLE_RX(desc) (((desc)->status & ETH_MAC_DMA_RDES0_TSA) != 0)
#define IS_TIMESTAMP_AVAILABLE_TX(desc) (((desc)->status & ETH_MAC_DMA_TDES0_TTSS) != 0)

#define TOTAL_FRAME_LENGTH(desc) (FIELD_GET(ETH_MAC_DMA_RDES0_FL, (desc)->status) - 4)

#define ETH_STATUS_ERROR_TRANSMIT_EVENTS                                                           \
	(XMC_ETH_MAC_EVENT_BUS_ERROR | XMC_ETH_MAC_EVENT_TRANSMIT_JABBER_TIMEOUT |                 \
	 XMC_ETH_MAC_EVENT_TRANSMIT_UNDERFLOW | XMC_ETH_MAC_EVENT_TRANSMIT_PROCESS_STOPPED)

#define ETH_STATUS_ERROR_RECEIVE_EVENTS                                                            \
	(XMC_ETH_MAC_EVENT_BUS_ERROR | XMC_ETH_MAC_EVENT_RECEIVE_OVERFLOW)

#define ETH_STATUS_ALL_EVENTS                                                                      \
	(ETH_STATUS_ERROR_TRANSMIT_EVENTS | ETH_STATUS_ERROR_RECEIVE_EVENTS |                      \
	 XMC_ETH_MAC_EVENT_RECEIVE | XMC_ETH_MAC_EVENT_TRANSMIT | ETH_INTERRUPT_ENABLE_NIE_Msk |   \
	 ETH_INTERRUPT_ENABLE_AIE_Msk)

#define ETH_MAC_DISABLE_MMC_INTERRUPT_MSK              0x03ffffffu
#define ETH_MAC_DISABLE_MMC_IPC_RECEIVE_INTERRUPT_MSK  0x3fff3fffu

#define ETH_STATUS_CLEARABLE_BITS 0x1e7ffu

#define ETH_RX_DMA_DESC_SECOND_ADDR_CHAINED_MASK BIT(14)

#define ETH_RESET_TIMEOUT_USEC 200000u
#define ETH_TIMESTAMP_CONTROL_REG_TIMEOUT_USEC 100000u

#define ETH_LINK_SPEED_10M 0
#define ETH_LINK_SPEED_100M 1

#define ETH_LINK_DUPLEX_HALF 0
#define ETH_LINK_DUPLEX_FULL 1

#define ETH_PTP_CLOCK_FREQUENCY 50000000
#define ETH_PTP_RATE_ADJUST_RATIO_MIN 0.9
#define ETH_PTP_RATE_ADJUST_RATIO_MAX 1.1

struct eth_xmc4xxx_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_sem tx_desc_sem;
	bool link_up;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
	bool tx_frames_flushed;
	uint16_t dma_desc_tx_head;
	uint16_t dma_desc_rx_tail;
	sys_slist_t tx_frame_list;
	struct net_buf *rx_frag_list[NUM_RX_DMA_DESCRIPTORS];
#if defined(CONFIG_PTP_CLOCK_XMC4XXX)
	const struct device *ptp_clock;
#endif
};

struct eth_xmc4xxx_config {
	ETH_GLOBAL_TypeDef *regs;
	const struct device *phy_dev;
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
	const uint8_t phy_connection_type;
	XMC_ETH_MAC_PORT_CTRL_t port_ctrl;
};

struct eth_xmc4xxx_tx_frame {
	sys_snode_t node;
	struct net_pkt *pkt;
	uint16_t tail_index;
	uint16_t head_index;
};

K_MEM_SLAB_DEFINE_STATIC(tx_frame_slab, sizeof(struct eth_xmc4xxx_tx_frame),
			 CONFIG_ETH_XMC4XXX_TX_FRAME_POOL_SIZE, 4);

static XMC_ETH_MAC_DMA_DESC_t __aligned(4) tx_dma_desc[NUM_TX_DMA_DESCRIPTORS];
static XMC_ETH_MAC_DMA_DESC_t __aligned(4) rx_dma_desc[NUM_RX_DMA_DESCRIPTORS];

static inline struct net_if *get_iface(struct eth_xmc4xxx_data *ctx)
{
	return ctx->iface;
}

static void eth_xmc4xxx_tx_dma_descriptors_init(const struct device *dev)
{
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	memset(tx_dma_desc, 0, sizeof(tx_dma_desc));

	dev_cfg->regs->TRANSMIT_DESCRIPTOR_LIST_ADDRESS = (uint32_t)&tx_dma_desc[0];

	/* chain the descriptors */
	for (int i = 0; i < NUM_TX_DMA_DESCRIPTORS - 1; i++) {
		XMC_ETH_MAC_DMA_DESC_t *dma_desc = &tx_dma_desc[i];

		dma_desc->buffer2 = (volatile uint32_t)&tx_dma_desc[i + 1];
	}

	/* TER: transmit end of ring - it is the last descriptor in ring */
	tx_dma_desc[NUM_TX_DMA_DESCRIPTORS - 1].status |= ETH_MAC_DMA_TDES0_TER;
	tx_dma_desc[NUM_TX_DMA_DESCRIPTORS - 1].buffer2 = (volatile uint32_t)&tx_dma_desc[0];
}

static void eth_xmc4xxx_flush_rx(const struct device *dev)
{
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;
	struct eth_xmc4xxx_data *dev_data = dev->data;

	dev_cfg->regs->OPERATION_MODE &= ~ETH_OPERATION_MODE_SR_Msk;

	for (int i = 0; i < NUM_RX_DMA_DESCRIPTORS; i++) {
		rx_dma_desc[i].status = ETH_MAC_DMA_RDES0_OWN;
	}

	dev_cfg->regs->OPERATION_MODE |= ETH_OPERATION_MODE_SR_Msk;
	dev_data->dma_desc_rx_tail = 0;
}

static void eth_xmc4xxx_flush_tx(const struct device *dev)
{
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;
	struct eth_xmc4xxx_data *dev_data = dev->data;
	sys_snode_t *node;

	LOG_DBG("Flushing tx frames");

	if (dev_data->tx_frames_flushed) {
		return;
	}

	dev_cfg->regs->OPERATION_MODE &= ~ETH_OPERATION_MODE_ST_Msk;

	node = sys_slist_get(&dev_data->tx_frame_list);
	while (node) {
		struct eth_xmc4xxx_tx_frame *tx_frame = SYS_SLIST_CONTAINER(node, tx_frame, node);

		net_pkt_unref(tx_frame->pkt);
		k_mem_slab_free(&tx_frame_slab, (void *)tx_frame);

		node = sys_slist_get(&dev_data->tx_frame_list);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.errors.tx++;
		dev_data->stats.error_details.tx_aborted_errors++;
#endif
	}

	k_sem_reset(&dev_data->tx_desc_sem);

	eth_xmc4xxx_tx_dma_descriptors_init(dev);
	dev_cfg->regs->OPERATION_MODE |= ETH_OPERATION_MODE_ST_Msk;
	dev_data->dma_desc_tx_head = 0;
	dev_data->tx_frames_flushed = true;

	for (int i = 0; i < NUM_TX_DMA_DESCRIPTORS; i++) {
		k_sem_give(&dev_data->tx_desc_sem);
	}
}

static inline void eth_xmc4xxx_trigger_dma_tx(ETH_GLOBAL_TypeDef *regs)
{
	regs->STATUS = ETH_STATUS_TPS_Msk;
	regs->TRANSMIT_POLL_DEMAND = 0;
}

static inline void eth_xmc4xxx_trigger_dma_rx(ETH_GLOBAL_TypeDef *regs)
{
	regs->STATUS = ETH_STATUS_RU_Msk;
	regs->RECEIVE_POLL_DEMAND = 0U;
}

static int eth_xmc4xxx_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;
	struct net_buf *frag;
	uint8_t *frag_data;
	uint16_t frag_len;
	int ret = 0;
	XMC_ETH_MAC_DMA_DESC_t *dma_desc = NULL;
	struct eth_xmc4xxx_tx_frame *tx_frame;
	int num_frags = 0;
	bool first_descriptor = false;

	frag = pkt->frags;
	while (frag) {
		num_frags++;
		frag = frag->frags;
	}

	if (num_frags > NUM_TX_DMA_DESCRIPTORS) {
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.error_details.tx_dma_failed++;
#endif
		LOG_DBG("Number of fragments exceeds total descriptors. Dropping packet");
		return -ENOMEM;
	}

	/* All available frames buffered inside the driver. Apply back pressure in the driver. */
	while (tx_frame_slab.info.num_used == CONFIG_ETH_XMC4XXX_TX_FRAME_POOL_SIZE) {
		eth_xmc4xxx_trigger_dma_tx(dev_cfg->regs);
		k_yield();
	}

	ret = k_mem_slab_alloc(&tx_frame_slab, (void **)&tx_frame, K_NO_WAIT);
	__ASSERT_NO_MSG(ret == 0);

	net_pkt_ref(pkt);

	dev_data->tx_frames_flushed = false;

	first_descriptor = true;
	tx_frame->pkt = pkt;
	tx_frame->tail_index = dev_data->dma_desc_tx_head;

	frag = pkt->frags;
	while (frag) {
		ret = k_sem_take(&dev_data->tx_desc_sem, K_FOREVER);
		/* isr may call k_sem_reset() */
		if (ret < 0 || dev_data->tx_frames_flushed) {
			k_mem_slab_free(&tx_frame_slab, (void **)&tx_frame);
			net_pkt_unref(pkt);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
			dev_data->stats.error_details.tx_aborted_errors++;
#endif
			LOG_DBG("Dropping frame. Buffered Tx frames were flushed in ISR.");
			return -EIO;
		}

		unsigned int key = irq_lock();
		/* Critical section for dma_desc_tx_head and tx_dma_desc. Isr may */
		/* reinitialize the descriptors and set dma_desc_tx_head to 0 */

		dma_desc = &tx_dma_desc[dev_data->dma_desc_tx_head];

		frag_data = frag->data;
		frag_len = frag->len;

		dma_desc->buffer1 = (volatile uint32_t)frag_data;
		dma_desc->length = frag_len;

		/* give ownership of descriptor back to dma and set checksum offload */
		/* TCH we are using a circular list */
		dma_desc->status = ETH_MAC_DMA_TDES0_CIC | ETH_MAC_DMA_TDES0_TCH;

		if (!first_descriptor) {
			/* Delay giving ownership of first frag to DMA. Prevents race condition */
			/* where second other frags are not ready */
			dma_desc->status |= ETH_MAC_DMA_TDES0_OWN;
		} else {
			dma_desc->status |= ETH_MAC_DMA_TDES0_FS;

#if defined(CONFIG_NET_GPTP)
			struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

			if (ntohs(hdr->type) == NET_ETH_PTYPE_PTP) {
				dma_desc->status |= ETH_MAC_DMA_TDES0_TTSE;
			}
#endif
		}
		first_descriptor = false;

		tx_frame->head_index = dev_data->dma_desc_tx_head;

		MODULO_INC_TX(dev_data->dma_desc_tx_head);

		irq_unlock(key);

		frag = frag->frags;
	}

	if (dev_data->tx_frames_flushed) {
		k_mem_slab_free(&tx_frame_slab, (void **)&tx_frame);
		net_pkt_unref(pkt);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.error_details.tx_aborted_errors++;
#endif
		LOG_DBG("Dropping frame. Buffered Tx frames were flushed in ISR.");
		return -EIO;
	}

	unsigned int key = irq_lock();

	/* label last dma descriptor as last segment and trigger interrupt on last segment */
	dma_desc->status |= ETH_MAC_DMA_TDES0_IC | ETH_MAC_DMA_TDES0_LS;

	/* Finally give ownership of first frag to DMA. After this point the DMA engine */
	/* may transfer the whole frame from RAM to Ethernet */
	tx_dma_desc[tx_frame->tail_index].status |= ETH_MAC_DMA_TDES0_OWN;

	sys_slist_append(&dev_data->tx_frame_list, &tx_frame->node);

	eth_xmc4xxx_trigger_dma_tx(dev_cfg->regs);

	irq_unlock(key);

	return 0;
}

static struct net_pkt *eth_xmc4xxx_rx_pkt(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;
	struct net_pkt *pkt = NULL;
	struct net_buf *new_frag;

	bool eof_found = false;
	uint16_t tail;
	XMC_ETH_MAC_DMA_DESC_t *dma_desc;
	int num_frags = 0;
	uint16_t frame_end_index;
	struct net_buf *frag, *last_frag = NULL;

	tail = dev_data->dma_desc_rx_tail;
	dma_desc = &rx_dma_desc[tail];

	if (IS_OWNED_BY_DMA_RX(dma_desc)) {
		return NULL;
	}

	if (!IS_START_OF_FRAME_RX(dma_desc)) {
		/* handle this error - missing SOF packet? */
		eth_xmc4xxx_flush_rx(dev);
		return NULL;
	}

	while (!IS_OWNED_BY_DMA_RX(dma_desc)) {
		eof_found = IS_END_OF_FRAME_RX(dma_desc);
		num_frags++;
		if (eof_found) {
			break;
		}

		MODULO_INC_RX(tail);

		if (tail == dev_data->dma_desc_rx_tail) {
			/* wrapped */
			break;
		}

		dma_desc = &rx_dma_desc[tail];
	}

	if (!eof_found) {
		return NULL;
	}

	frame_end_index = tail;

	pkt = net_pkt_rx_alloc(K_NO_WAIT);
	if (pkt == NULL) {
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.errors.rx++;
		dev_data->stats.error_details.rx_no_buffer_count++;
#endif
		LOG_DBG("Net packet allocation error");
		/* continue because we still need to read out the packet */
	}

	tail = dev_data->dma_desc_rx_tail;
	dma_desc = &rx_dma_desc[tail];
	for (;;) {
		if (pkt != NULL) {
			uint16_t frag_len = CONFIG_NET_BUF_DATA_SIZE;

			frag = dev_data->rx_frag_list[tail];
			if (tail == frame_end_index) {
				frag_len = TOTAL_FRAME_LENGTH(dma_desc) -
					   CONFIG_NET_BUF_DATA_SIZE * (num_frags - 1);

				if (IS_TIMESTAMP_AVAILABLE_RX(dma_desc)) {
					struct net_ptp_time timestamp = {
						.second = dma_desc->time_stamp_seconds,
						.nanosecond = dma_desc->time_stamp_nanoseconds};

					net_pkt_set_timestamp(pkt, &timestamp);
					net_pkt_set_priority(pkt, NET_PRIORITY_CA);
				}
			}

			new_frag = net_pkt_get_frag(pkt, CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
			if (new_frag == NULL) {
#ifdef CONFIG_NET_STATISTICS_ETHERNET
				dev_data->stats.errors.rx++;
				dev_data->stats.error_details.rx_buf_alloc_failed++;
#endif
				LOG_DBG("Frag allocation error. Increase CONFIG_NET_BUF_RX_COUNT.");
				net_pkt_unref(pkt);
				pkt = NULL;
			} else {
				net_buf_add(frag, frag_len);
				if (!last_frag) {
					net_pkt_frag_insert(pkt, frag);
				} else {
					net_buf_frag_insert(last_frag, frag);
				}

				last_frag = frag;
				frag = new_frag;
				dev_data->rx_frag_list[tail] = frag;
			}
		}

		dma_desc->buffer1 = (uint32_t)dev_data->rx_frag_list[tail]->data;
		dma_desc->length = dev_data->rx_frag_list[tail]->size |
				   ETH_RX_DMA_DESC_SECOND_ADDR_CHAINED_MASK;
		dma_desc->status = ETH_MAC_DMA_RDES0_OWN;

		if (tail == frame_end_index) {
			break;
		}

		MODULO_INC_RX(tail);
		dma_desc = &rx_dma_desc[tail];
	}


	MODULO_INC_RX(tail);
	dev_data->dma_desc_rx_tail = tail;

	eth_xmc4xxx_trigger_dma_rx(dev_cfg->regs);

	return pkt;
}

static void eth_xmc4xxx_handle_rx(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	struct net_pkt *pkt = NULL;

	for (;;) {
		pkt = eth_xmc4xxx_rx_pkt(dev);
		if (!pkt) {
			return;
		}

		if (net_recv_data(get_iface(dev_data), pkt) < 0) {
			eth_stats_update_errors_rx(get_iface(dev_data));
			net_pkt_unref(pkt);
		}
	}
}

static void eth_xmc4xxx_handle_tx(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	sys_snode_t *node = sys_slist_peek_head(&dev_data->tx_frame_list);

	while (node) {
		struct eth_xmc4xxx_tx_frame *tx_frame = SYS_SLIST_CONTAINER(node, tx_frame, node);
		bool owned_by_mcu = true;
		uint8_t index;
		int num_descriptors;

		if (tx_frame->head_index >= tx_frame->tail_index) {
			num_descriptors = tx_frame->head_index - tx_frame->tail_index + 1;
		} else {
			num_descriptors = tx_frame->head_index + NUM_TX_DMA_DESCRIPTORS -
					  tx_frame->tail_index + 1;
		}

		index = tx_frame->tail_index;
		for (int i = 0; i < num_descriptors; i++) {
			if (IS_OWNED_BY_DMA_TX(&tx_dma_desc[index])) {
				owned_by_mcu = false;
				break;
			}

			MODULO_INC_TX(index);
		}

		if (owned_by_mcu) {
#if defined(CONFIG_NET_GPTP)
			XMC_ETH_MAC_DMA_DESC_t *dma_desc = &tx_dma_desc[tx_frame->head_index];

			if (IS_TIMESTAMP_AVAILABLE_TX(dma_desc)) {
				struct net_pkt *pkt = tx_frame->pkt;

				if (atomic_get(&pkt->atomic_ref) > 1) {
					struct net_ptp_time timestamp = {
						.second = dma_desc->time_stamp_seconds,
						.nanosecond = dma_desc->time_stamp_nanoseconds};

					net_pkt_set_timestamp(pkt, &timestamp);
					net_if_add_tx_timestamp(pkt);
				}
			}
#endif

			for (int i = 0; i < num_descriptors; i++) {
				k_sem_give(&dev_data->tx_desc_sem);
			}

			sys_slist_get(&dev_data->tx_frame_list);
			net_pkt_unref(tx_frame->pkt);
			k_mem_slab_free(&tx_frame_slab, (void *)tx_frame);
			node = sys_slist_peek_head(&dev_data->tx_frame_list);
		} else {
			node = NULL;
		}
	}
}

static void eth_xmc4xxx_isr(const struct device *dev)
{
	uint32_t lock;
	uint32_t status;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	lock = irq_lock();
	status = dev_cfg->regs->STATUS;

	if ((status & XMC_ETH_MAC_EVENT_RECEIVE) != 0) {
		eth_xmc4xxx_handle_rx(dev);
	}

	if ((status & XMC_ETH_MAC_EVENT_TRANSMIT) != 0) {
		eth_xmc4xxx_handle_tx(dev);
	}

	if ((status & ETH_STATUS_ERROR_TRANSMIT_EVENTS) != 0) {
		LOG_ERR("Transmit error event [0x%x]", status);
		eth_xmc4xxx_flush_tx(dev);
	}

	if ((status & ETH_STATUS_ERROR_RECEIVE_EVENTS) != 0) {
		LOG_ERR("Receive error event [0x%x]", status);
		eth_xmc4xxx_flush_rx(dev);
	}

	dev_cfg->regs->STATUS = status & ETH_STATUS_CLEARABLE_BITS;

	irq_unlock(lock);
}

static inline void eth_xmc4xxx_enable_tx(ETH_GLOBAL_TypeDef *regs)
{
	regs->OPERATION_MODE |= ETH_OPERATION_MODE_ST_Msk;
	regs->MAC_CONFIGURATION |= ETH_MAC_CONFIGURATION_TE_Msk;
}

static inline void eth_xmc4xxx_enable_rx(ETH_GLOBAL_TypeDef *regs)
{
	regs->OPERATION_MODE |= ETH_OPERATION_MODE_SR_Msk;
	regs->MAC_CONFIGURATION |= ETH_MAC_CONFIGURATION_RE_Msk;
}

static inline void eth_xmc4xxx_set_link(ETH_GLOBAL_TypeDef *regs, struct phy_link_state *state)
{
	uint32_t reg = regs->MAC_CONFIGURATION;
	uint32_t val;

	reg &= ~(ETH_MAC_CONFIGURATION_DM_Msk | ETH_MAC_CONFIGURATION_FES_Msk);

	val = PHY_LINK_IS_FULL_DUPLEX(state->speed) ? ETH_LINK_DUPLEX_FULL :
						      ETH_LINK_DUPLEX_HALF;
	reg |= FIELD_PREP(ETH_MAC_CONFIGURATION_DM_Msk, val);

	val = PHY_LINK_IS_SPEED_100M(state->speed) ? ETH_LINK_SPEED_100M :
						     ETH_LINK_SPEED_10M;
	reg |= FIELD_PREP(ETH_MAC_CONFIGURATION_FES_Msk, val);

	regs->MAC_CONFIGURATION = reg;
}

static void phy_link_state_changed(const struct device *phy_dev, struct phy_link_state *state,
				   void *user_data)
{
	struct device *dev = user_data;
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;
	bool is_up = state->is_up;

	if (is_up && !dev_data->link_up) {
		LOG_INF("Link up");
		dev_data->link_up = true;
		net_eth_carrier_on(dev_data->iface);
		eth_xmc4xxx_set_link(dev_cfg->regs, state);
	} else if (!is_up && dev_data->link_up) {
		LOG_INF("Link down");
		dev_data->link_up = false;
		net_eth_carrier_off(dev_data->iface);
	}
}

static void eth_xmc4xxx_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	dev_data->iface = iface;

	net_if_set_link_addr(iface, dev_data->mac_addr, sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	dev_cfg->irq_config_func();

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	phy_link_callback_set(dev_cfg->phy_dev, &phy_link_state_changed, (void *)dev);

	dev_cfg->regs->INTERRUPT_ENABLE |= ETH_STATUS_ALL_EVENTS;

	eth_xmc4xxx_enable_tx(dev_cfg->regs);
	eth_xmc4xxx_enable_rx(dev_cfg->regs);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_xmc4xxx_stats(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;

	return &dev_data->stats;
}
#endif

static inline void eth_xmc4xxx_free_rx_bufs(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;

	for (int i = 0; i < NUM_RX_DMA_DESCRIPTORS; i++) {
		if (dev_data->rx_frag_list[i]) {
			net_buf_unref(dev_data->rx_frag_list[i]);
			dev_data->rx_frag_list[i] = NULL;
		}
	}
}

static int eth_xmc4xxx_rx_dma_descriptors_init(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	dev_cfg->regs->RECEIVE_DESCRIPTOR_LIST_ADDRESS = (uint32_t)&rx_dma_desc[0];

	for (int i = 0; i < NUM_RX_DMA_DESCRIPTORS - 1; i++) {
		XMC_ETH_MAC_DMA_DESC_t *dma_desc = &rx_dma_desc[i];

		dma_desc->buffer2 = (volatile uint32_t)&rx_dma_desc[i + 1];
	}

	rx_dma_desc[NUM_RX_DMA_DESCRIPTORS - 1].status |= ETH_MAC_DMA_TDES0_TER;
	rx_dma_desc[NUM_RX_DMA_DESCRIPTORS - 1].buffer2 = (volatile uint32_t)&rx_dma_desc[0];

	for (int i = 0; i < NUM_RX_DMA_DESCRIPTORS; i++) {
		XMC_ETH_MAC_DMA_DESC_t *dma_desc = &rx_dma_desc[i];
		struct net_buf *rx_buf = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE,
								     K_NO_WAIT);

		if (rx_buf == NULL) {
			eth_xmc4xxx_free_rx_bufs(dev);
			LOG_ERR("Failed to reserve data net buffers");
			return -ENOBUFS;
		}

		dev_data->rx_frag_list[i] = rx_buf;
		dma_desc->buffer1 = (uint32_t)rx_buf->data;
		dma_desc->length = rx_buf->size | ETH_RX_DMA_DESC_SECOND_ADDR_CHAINED_MASK;
		dma_desc->status = ETH_MAC_DMA_RDES0_OWN;
	}

	return 0;
}

static inline int eth_xmc4xxx_reset(const struct device *dev)
{
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	dev_cfg->regs->BUS_MODE |= ETH_BUS_MODE_SWR_Msk;

	/* reset may fail if the clocks are not properly setup */
	if (!WAIT_FOR((dev_cfg->regs->BUS_MODE & ETH_BUS_MODE_SWR_Msk) == 0,
		      ETH_RESET_TIMEOUT_USEC,)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static inline void eth_xmc4xxx_set_mac_address(ETH_GLOBAL_TypeDef *regs, uint8_t *const addr)
{
	regs->MAC_ADDRESS0_HIGH = addr[4] | (addr[5] << 8);
	regs->MAC_ADDRESS0_LOW = addr[0] | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24);
}

static inline void eth_xmc4xxx_mask_unused_interrupts(ETH_GLOBAL_TypeDef *regs)
{
	/* Disable Mac Management Counter (MMC) interrupt events */
	regs->MMC_TRANSMIT_INTERRUPT_MASK = ETH_MAC_DISABLE_MMC_INTERRUPT_MSK;
	regs->MMC_RECEIVE_INTERRUPT_MASK = ETH_MAC_DISABLE_MMC_INTERRUPT_MSK;

	/* IPC - Receive IP checksum checker */
	regs->MMC_IPC_RECEIVE_INTERRUPT_MASK = ETH_MAC_DISABLE_MMC_IPC_RECEIVE_INTERRUPT_MSK;

	/* Disable PMT and timestamp interrupt events */
	regs->INTERRUPT_MASK = ETH_INTERRUPT_MASK_PMTIM_Msk | ETH_INTERRUPT_MASK_TSIM_Msk;
}

static inline int eth_xmc4xxx_init_timestamp_control_reg(ETH_GLOBAL_TypeDef *regs)
{
#if defined(CONFIG_NET_GPTP)
	regs->TIMESTAMP_CONTROL = ETH_TIMESTAMP_CONTROL_TSENA_Msk |
				  ETH_TIMESTAMP_CONTROL_TSENALL_Msk;
#endif

#if defined(CONFIG_PTP_CLOCK_XMC4XXX)
	/* use fine control */
	regs->TIMESTAMP_CONTROL |= ETH_TIMESTAMP_CONTROL_TSCFUPDT_Msk |
				  ETH_TIMESTAMP_CONTROL_TSCTRLSSR_Msk;

	/* make ptp run at 50MHz - implies 20ns increment for each increment of the */
	/* sub_second_register */
	regs->SUB_SECOND_INCREMENT = 20;

	/* f_out = f_cpu * K / 2^32, where K = TIMESTAMP_ADDEND. Target F_out = 50MHz  */
	/* Therefore, K = ceil(f_out * 2^32 / f_cpu) */

	uint32_t f_cpu = XMC_SCU_CLOCK_GetSystemClockFrequency();
	uint32_t K = (BIT64(32) * ETH_PTP_CLOCK_FREQUENCY  + f_cpu / 2) / f_cpu;

	regs->TIMESTAMP_ADDEND = K;

	/* Addend register update */
	regs->TIMESTAMP_CONTROL |= ETH_TIMESTAMP_CONTROL_TSADDREG_Msk;
	if (!WAIT_FOR((regs->TIMESTAMP_CONTROL & ETH_TIMESTAMP_CONTROL_TSADDREG_Msk) == 0,
		      ETH_TIMESTAMP_CONTROL_REG_TIMEOUT_USEC,)) {
		return -ETIMEDOUT;
	}

	regs->TIMESTAMP_CONTROL |= ETH_TIMESTAMP_CONTROL_TSINIT_Msk;
	if (!WAIT_FOR((regs->TIMESTAMP_CONTROL & ETH_TIMESTAMP_CONTROL_TSINIT_Msk) == 0,
		      ETH_TIMESTAMP_CONTROL_REG_TIMEOUT_USEC,)) {
		return -ETIMEDOUT;
	}
#endif
	return 0;
}

static int eth_xmc4xxx_init(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;
	XMC_ETH_MAC_PORT_CTRL_t port_ctrl;
	int ret;

	sys_slist_init(&dev_data->tx_frame_list);
	k_sem_init(&dev_data->tx_desc_sem, NUM_TX_DMA_DESCRIPTORS,
					   NUM_TX_DMA_DESCRIPTORS);

	if (!device_is_ready(dev_cfg->phy_dev)) {
		LOG_ERR("Phy device not ready");
		return -ENODEV;
	}

	/* get the port control initialized by MDIO driver */
	port_ctrl.raw = ETH0_CON->CON;
	port_ctrl.raw |= dev_cfg->port_ctrl.raw;

	XMC_ETH_MAC_Disable(NULL);
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	XMC_ETH_MAC_SetPortControl(NULL, port_ctrl);
	XMC_ETH_MAC_Enable(NULL);

	ret = eth_xmc4xxx_reset(dev);
	if (ret != 0) {
		LOG_ERR("Error resetting ethernet [%d]", ret);
		return ret;
	}

	/* Initialize MAC configuration */
	/* enable checksum offload */
	dev_cfg->regs->MAC_CONFIGURATION = ETH_MAC_CONFIGURATION_IPC_Msk;

	/* disable jumbo frames */
	dev_cfg->regs->MAC_CONFIGURATION &= ~ETH_MAC_CONFIGURATION_JE_Msk;


	/* Initialize Filter registers - disable zero quanta pause*/
	dev_cfg->regs->FLOW_CONTROL = ETH_FLOW_CONTROL_DZPQ_Msk;

	/* rsf - receive store and forward */
	/* tsf - transmit store and forward */
	dev_cfg->regs->OPERATION_MODE = ETH_OPERATION_MODE_RSF_Msk | ETH_OPERATION_MODE_TSF_Msk |
					ETH_OPERATION_MODE_OSF_Msk;

	/* Increase enhanced descriptor to 8 WORDS, required when the Advanced */
	/* Time-Stamp feature or Full IPC Offload Engine is enabled */
	dev_cfg->regs->BUS_MODE = ETH_BUS_MODE_ATDS_Msk | ETH_BUS_MODE_AAL_Msk |
				  ETH_BUS_MODE_FB_Msk | (0x20 << ETH_BUS_MODE_PBL_Pos);

	eth_xmc4xxx_tx_dma_descriptors_init(dev);
	ret = eth_xmc4xxx_rx_dma_descriptors_init(dev);
	if (ret != 0) {
		return ret;
	}

	/* Clear interrupts */
	dev_cfg->regs->STATUS = ETH_STATUS_CLEARABLE_BITS;

	eth_xmc4xxx_mask_unused_interrupts(dev_cfg->regs);

#if !DT_INST_NODE_HAS_PROP(0, local_mac_address)
	gen_random_mac(dev_data->mac_addr, INFINEON_OUI_B0, INFINEON_OUI_B1, INFINEON_OUI_B2);
#endif
	eth_xmc4xxx_set_mac_address(dev_cfg->regs, dev_data->mac_addr);

	uint32_t reg = dev_cfg->regs->MAC_FRAME_FILTER;
	/* enable reception of broadcast frames */
	reg &= ~ETH_MAC_FRAME_FILTER_DBF_Msk;
	/* pass all multicast frames */
	reg |= ETH_MAC_FRAME_FILTER_PM_Msk;
	dev_cfg->regs->MAC_FRAME_FILTER = reg;

	return eth_xmc4xxx_init_timestamp_control_reg(dev_cfg->regs);
}

static enum ethernet_hw_caps eth_xmc4xxx_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	enum ethernet_hw_caps caps =  ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T |
	       ETHERNET_HW_TX_CHKSUM_OFFLOAD | ETHERNET_HW_RX_CHKSUM_OFFLOAD;

#if defined(CONFIG_PTP_CLOCK_XMC4XXX)
	caps |= ETHERNET_PTP;
#endif

#if defined(CONFIG_NET_VLAN)
	caps |= ETHERNET_HW_VLAN;
#endif

	return caps;
}

static int eth_xmc4xxx_set_config(const struct device *dev, enum ethernet_config_type type,
				  const struct ethernet_config *config)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, sizeof(dev_data->mac_addr));
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name,
			dev_data->mac_addr[0], dev_data->mac_addr[1], dev_data->mac_addr[2],
			dev_data->mac_addr[3], dev_data->mac_addr[4], dev_data->mac_addr[5]);

		eth_xmc4xxx_set_mac_address(dev_cfg->regs, dev_data->mac_addr);
		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr), NET_LINK_ETHERNET);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static void eth_xmc4xxx_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_xmc4xxx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

#if defined(CONFIG_PTP_CLOCK_XMC4XXX)
static const struct device *eth_xmc4xxx_get_ptp_clock(const struct device *dev)
{
	struct eth_xmc4xxx_data *dev_data = dev->data;

	return dev_data->ptp_clock;
}
#endif


#if defined(CONFIG_ETH_XMC4XXX_VLAN_HW_FILTER)
int eth_xmc4xxx_vlan_setup(const struct device *dev, struct net_if *iface, uint16_t tag,
			   bool enable)
{
	ARG_UNUSED(iface);
	const struct eth_xmc4xxx_config *dev_cfg = dev->config;

	LOG_INF("Configuring vlan %d", tag);

	if (enable) {
		dev_cfg->regs->VLAN_TAG = FIELD_PREP(ETH_VLAN_TAG_VL_Msk, tag) |
					  ETH_VLAN_TAG_ETV_Msk |
					  ETH_VLAN_TAG_ESVL_Msk;
		dev_cfg->regs->MAC_FRAME_FILTER |= ETH_MAC_FRAME_FILTER_VTFE_Msk;
	} else {
		dev_cfg->regs->VLAN_TAG = 0;
		dev_cfg->regs->MAC_FRAME_FILTER &= ~ETH_MAC_FRAME_FILTER_VTFE_Msk;
	}

	return 0;
}
#endif

static const struct ethernet_api eth_xmc4xxx_api = {
	.iface_api.init = eth_xmc4xxx_iface_init,
	.send = eth_xmc4xxx_send,
	.set_config = eth_xmc4xxx_set_config,
	.get_capabilities = eth_xmc4xxx_capabilities,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_xmc4xxx_stats,
#endif
#if defined(CONFIG_PTP_CLOCK_XMC4XXX)
	.get_ptp_clock = eth_xmc4xxx_get_ptp_clock,
#endif
#if defined(CONFIG_ETH_XMC4XXX_VLAN_HW_FILTER)
	.vlan_setup = eth_xmc4xxx_vlan_setup,
#endif
};

PINCTRL_DT_INST_DEFINE(0);

static struct eth_xmc4xxx_config eth_xmc4xxx_config = {
	.regs = (ETH_GLOBAL_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(0)),
	.irq_config_func = eth_xmc4xxx_irq_config,
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.port_ctrl = {
	    .rxd0 = DT_INST_ENUM_IDX(0, rxd0_port_ctrl),
	    .rxd1 = DT_INST_ENUM_IDX(0, rxd1_port_ctrl),
	    .rxd2 = DT_INST_ENUM_IDX_OR(0, rxd2_port_ctrl, 0),
	    .rxd3 = DT_INST_ENUM_IDX_OR(0, rxd3_port_ctrl, 0),
	    .clk_rmii = DT_INST_ENUM_IDX(0, rmii_rx_clk_port_ctrl),
	    .crs_dv = DT_INST_ENUM_IDX(0, crs_rx_dv_port_ctrl),
	    .crs = DT_INST_ENUM_IDX_OR(0, crs_port_ctrl, 0),
	    .rxer = DT_INST_ENUM_IDX(0, rxer_port_ctrl),
	    .col = DT_INST_ENUM_IDX_OR(0, col_port_ctrl, 0),
	    .clk_tx = DT_INST_ENUM_IDX_OR(0, tx_clk_port_ctrl, 0),
	    .mode = DT_INST_ENUM_IDX_OR(0, phy_connection_type, 0),
	}
};

static struct eth_xmc4xxx_data eth_xmc4xxx_data = {
	.mac_addr = DT_INST_PROP_OR(0, local_mac_address, {0}),
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_xmc4xxx_init, NULL, &eth_xmc4xxx_data, &eth_xmc4xxx_config,
			      CONFIG_ETH_INIT_PRIORITY, &eth_xmc4xxx_api, NET_ETH_MTU);

#if defined(CONFIG_PTP_CLOCK_XMC4XXX)

struct ptp_context {
	const struct device *eth_dev;
};

static struct ptp_context ptp_xmc4xxx_context_0;

static int eth_xmc4xxx_ptp_clock_set(const struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = ptp_context->eth_dev->config;

	dev_cfg->regs->SYSTEM_TIME_NANOSECONDS_UPDATE = tm->nanosecond;
	dev_cfg->regs->SYSTEM_TIME_SECONDS_UPDATE = tm->second;

	dev_cfg->regs->TIMESTAMP_CONTROL |= ETH_TIMESTAMP_CONTROL_TSINIT_Msk;
	if (!WAIT_FOR((dev_cfg->regs->TIMESTAMP_CONTROL & ETH_TIMESTAMP_CONTROL_TSINIT_Msk) == 0,
		      ETH_TIMESTAMP_CONTROL_REG_TIMEOUT_USEC,)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int eth_xmc4xxx_ptp_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = ptp_context->eth_dev->config;

	uint32_t nanosecond_0 = dev_cfg->regs->SYSTEM_TIME_NANOSECONDS;
	uint32_t second_0 = dev_cfg->regs->SYSTEM_TIME_SECONDS;

	uint32_t nanosecond_1 = dev_cfg->regs->SYSTEM_TIME_NANOSECONDS;
	uint32_t second_1 = dev_cfg->regs->SYSTEM_TIME_SECONDS;

	/* check that there is no roll over while we read the timestamp. If roll over happens */
	/* just choose the later value */
	if (second_0 == second_1) {
		tm->second = second_0;
		tm->nanosecond = nanosecond_0;
	} else {
		tm->second = second_1;
		tm->nanosecond = nanosecond_1;
	}

	return 0;
}

static int eth_xmc4xxx_ptp_clock_adjust(const struct device *dev, int increment)
{
	struct ptp_context *ptp_context = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = ptp_context->eth_dev->config;
	uint32_t increment_tmp;

	if ((increment <= -(int)NSEC_PER_SEC) || (increment >= (int)NSEC_PER_SEC)) {
		return -EINVAL;
	}

	if (increment < 0) {
		increment_tmp = -increment;
		increment_tmp |= ETH_SYSTEM_TIME_NANOSECONDS_UPDATE_ADDSUB_Msk;
	} else {
		increment_tmp = increment;
	}

	dev_cfg->regs->SYSTEM_TIME_NANOSECONDS_UPDATE = increment_tmp;
	dev_cfg->regs->SYSTEM_TIME_SECONDS_UPDATE = 0;

	dev_cfg->regs->TIMESTAMP_CONTROL |= ETH_TIMESTAMP_CONTROL_TSUPDT_Msk;
	if (!WAIT_FOR((dev_cfg->regs->TIMESTAMP_CONTROL & ETH_TIMESTAMP_CONTROL_TSUPDT_Msk) == 0,
		      ETH_TIMESTAMP_CONTROL_REG_TIMEOUT_USEC,)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int eth_xmc4xxx_ptp_clock_rate_adjust(const struct device *dev, double ratio)
{
	struct ptp_context *ptp_context = dev->data;
	const struct eth_xmc4xxx_config *dev_cfg = ptp_context->eth_dev->config;
	uint64_t K = dev_cfg->regs->TIMESTAMP_ADDEND;

	if (ratio < ETH_PTP_RATE_ADJUST_RATIO_MIN || ratio > ETH_PTP_RATE_ADJUST_RATIO_MAX) {
		return -EINVAL;
	}

	/* f_out = f_cpu * K / 2^32, where K = TIMESTAMP_ADDEND. Target F_out = 50MHz  */
	K = K * ratio + 0.5;
	if (K > UINT32_MAX) {
		return -EINVAL;
	}
	dev_cfg->regs->TIMESTAMP_ADDEND = K;

	/* Addend register update */
	dev_cfg->regs->TIMESTAMP_CONTROL |= ETH_TIMESTAMP_CONTROL_TSADDREG_Msk;
	if (!WAIT_FOR((dev_cfg->regs->TIMESTAMP_CONTROL & ETH_TIMESTAMP_CONTROL_TSADDREG_Msk) == 0,
		      ETH_TIMESTAMP_CONTROL_REG_TIMEOUT_USEC,)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static const struct ptp_clock_driver_api ptp_api_xmc4xxx = {
	.set = eth_xmc4xxx_ptp_clock_set,
	.get = eth_xmc4xxx_ptp_clock_get,
	.adjust = eth_xmc4xxx_ptp_clock_adjust,
	.rate_adjust = eth_xmc4xxx_ptp_clock_rate_adjust,
};

static int ptp_clock_xmc4xxx_init(const struct device *port)
{
	const struct device *const eth_dev = DEVICE_DT_INST_GET(0);
	struct eth_xmc4xxx_data *dev_data = eth_dev->data;
	struct ptp_context *ptp_context = port->data;

	dev_data->ptp_clock = port;
	ptp_context->eth_dev = eth_dev;

	return 0;
}

DEVICE_DEFINE(xmc4xxx_ptp_clock_0, PTP_CLOCK_NAME, ptp_clock_xmc4xxx_init, NULL,
	      &ptp_xmc4xxx_context_0, NULL, POST_KERNEL, CONFIG_PTP_CLOCK_INIT_PRIORITY,
	      &ptp_api_xmc4xxx);

#endif /* CONFIG_PTP_CLOCK_XMC4XXX */
