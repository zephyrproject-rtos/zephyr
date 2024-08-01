/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_qos_mac

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_nxp_enet_qos_mac, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/phy.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys_clock.h>
#include <ethernet/eth_stats.h>
#include "../eth.h"
#include "nxp_enet_qos_priv.h"

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

static void eth_nxp_enet_qos_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_enet_qos_mac_data *data = dev->data;

	net_if_set_link_addr(iface, data->mac_addr.addr,
			     sizeof(((struct net_eth_addr *)NULL)->addr), NET_LINK_ETHERNET);

	if (data->iface == NULL) {
		data->iface = iface;
	}

	ethernet_init(iface);
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

	/* Only allow send of the maximum normal packet size */
	while (fragment != NULL) {
		frags_count++;
		total_bytes += fragment->len;
		fragment = fragment->frags;
	}

	if (total_bytes > config->hw_info.max_frame_len ||
	    frags_count > NUM_TX_BUFDESC) {
		LOG_ERR("TX packet too large");
		return -E2BIG;
	}

	/* One TX at a time in the current implementation */
	k_sem_take(&data->tx.tx_sem, K_FOREVER);

	net_pkt_ref(pkt);

	data->tx.pkt = pkt;
	/* Need to save the header because the ethernet stack
	 * otherwise discards it from the packet after this call
	 */
	data->tx.tx_header = pkt->frags;

	LOG_DBG("Setting up TX descriptors for packet %p", pkt);

	/* Reset the descriptors */
	memset((void *)data->tx.descriptors, 0, sizeof(union nxp_enet_qos_tx_desc) * frags_count);

	/* Setting up the descriptors  */
	fragment = pkt->frags;
	tx_desc_ptr->read.control2 |= FIRST_TX_DESCRIPTOR_FLAG;
	for (int i = 0; i < frags_count; i++) {
		net_pkt_frag_ref(fragment);

		tx_desc_ptr->read.buf1_addr = (uint32_t)fragment->data;
		tx_desc_ptr->read.control1 = FIELD_PREP(0x3FFF, fragment->len);
		tx_desc_ptr->read.control2 |= FIELD_PREP(0x7FFF, total_bytes);

		fragment = fragment->frags;
		tx_desc_ptr++;
	}
	last_desc_ptr = tx_desc_ptr - 1;
	last_desc_ptr->read.control2 |= LAST_TX_DESCRIPTOR_FLAG;
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

static void tx_dma_done(struct k_work *work)
{
	struct nxp_enet_qos_tx_data *tx_data =
		CONTAINER_OF(work, struct nxp_enet_qos_tx_data, tx_done_work);
	struct nxp_enet_qos_mac_data *data =
		CONTAINER_OF(tx_data, struct nxp_enet_qos_mac_data, tx);
	struct net_pkt *pkt = tx_data->pkt;
	struct net_buf *fragment = pkt->frags;

	LOG_DBG("TX DMA completed on packet %p", pkt);

	/* Returning the buffers and packet to the pool */
	while (fragment != NULL) {
		net_pkt_frag_unref(fragment);
		fragment = fragment->frags;
	}

	net_pkt_frag_unref(data->tx.tx_header);
	net_pkt_unref(pkt);

	eth_stats_update_pkts_tx(data->iface);

	/* Allows another send */
	k_sem_give(&data->tx.tx_sem);
}

static enum ethernet_hw_caps eth_nxp_enet_qos_get_capabilities(const struct device *dev)
{
	return ETHERNET_LINK_100BASE_T | ETHERNET_LINK_10BASE_T;
}

static void eth_nxp_enet_qos_rx(struct k_work *work)
{
	struct nxp_enet_qos_rx_data *rx_data =
		CONTAINER_OF(work, struct nxp_enet_qos_rx_data, rx_work);
	struct nxp_enet_qos_mac_data *data =
		CONTAINER_OF(rx_data, struct nxp_enet_qos_mac_data, rx);
	volatile union nxp_enet_qos_rx_desc *desc_arr = data->rx.descriptors;
	volatile union nxp_enet_qos_rx_desc *desc;
	struct net_pkt *pkt;
	struct net_buf *new_buf;
	struct net_buf *buf;
	size_t pkt_len;

	/* We are going to find all of the descriptors we own and update them */
	for (int i = 0; i < NUM_RX_BUFDESC; i++) {
		desc = &desc_arr[i];

		if (desc->write.control3 & OWN_FLAG) {
			/* The DMA owns the descriptor, we cannot touch it */
			continue;
		}

		/* Otherwise, we found a packet that we need to process */
		pkt = net_pkt_rx_alloc(K_NO_WAIT);

		if (!pkt) {
			LOG_ERR("Could not alloc RX pkt");
			goto error;
		}

		LOG_DBG("Created RX pkt %p", pkt);

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
			 */
			LOG_ERR("No RX buf available");
			goto error;
		}

		buf = data->rx.reserved_bufs[i];
		pkt_len = desc->write.control3 & DESC_RX_PKT_LEN;

		LOG_DBG("Receiving RX packet");

		/* Finally, we have decided that it is time to wrap the buffer nicely
		 * up within a packet, and try to send it. It's only one buffer,
		 * thanks to ENET QOS hardware handing the fragmentation,
		 * so the construction of the packet is very simple.
		 */
		net_buf_add(buf, pkt_len);
		net_pkt_frag_insert(pkt, buf);
		if (net_recv_data(data->iface, pkt)) {
			LOG_ERR("RECV failed");
			/* Quite a shame. */
			goto error;
		}

		LOG_DBG("Recycling RX buf");

		/* Fresh meat */
		data->rx.reserved_bufs[i] = new_buf;
		desc->read.buf1_addr = (uint32_t)new_buf->data;
		desc->read.control |= rx_desc_refresh_flags;

		/* Record our glorious victory */
		eth_stats_update_pkts_rx(data->iface);
	}

	return;

error:
	net_pkt_unref(pkt);
	eth_stats_update_errors_rx(data->iface);
}

static void eth_nxp_enet_qos_mac_isr(const struct device *dev)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;
	struct nxp_enet_qos_mac_data *data = dev->data;
	enet_qos_t *base = config->base;

	/* cleared on read */
	volatile uint32_t mac_interrupts = base->MAC_INTERRUPT_STATUS;
	volatile uint32_t mac_rx_tx_status = base->MAC_RX_TX_STATUS;
	volatile uint32_t dma_interrupts = base->DMA_INTERRUPT_STATUS;
	volatile uint32_t dma_ch0_interrupts = base->DMA_CH[0].DMA_CHX_STAT;

	mac_interrupts; mac_rx_tx_status;

	base->DMA_CH[0].DMA_CHX_STAT = 0xFFFFFFFF;

	if (ENET_QOS_REG_GET(DMA_INTERRUPT_STATUS, DC0IS, dma_interrupts)) {
		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, TI, dma_ch0_interrupts)) {
			k_work_submit(&data->tx.tx_done_work);
		}
		if (ENET_QOS_REG_GET(DMA_CH_DMA_CHX_STAT, RI, dma_ch0_interrupts)) {
			k_work_submit_to_queue(&rx_work_queue, &data->rx.rx_work);
		}
	}
}

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
}

static inline int enet_qos_dma_reset(enet_qos_t *base)
{
	/* Set the software reset of the DMA */
	base->DMA_MODE |= ENET_QOS_REG_PREP(DMA_MODE, SWR, 0b1);

	if (CONFIG_ETH_NXP_ENET_QOS_DMA_RESET_WAIT_TIME == 0) {
		/* spin and wait forever for the reset flag to clear */
		while (ENET_QOS_REG_GET(DMA_MODE, SWR, base->DMA_MODE))
			;
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
				base->MTL_QUEUE[0].MTL_TXQX_OP_MODE))
		;

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

static inline void enet_qos_mac_config_init(enet_qos_t *base,
				struct nxp_enet_qos_mac_data *data, uint32_t clk_rate)
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

	/* Set the reference for 1 microsecond of ENET QOS CSR clock cycles */
	base->MAC_ONEUS_TIC_COUNTER =
		ENET_QOS_REG_PREP(MAC_ONEUS_TIC_COUNTER, TIC_1US_CNTR,
					(clk_rate / USEC_PER_SEC) - 1);

	base->MAC_CONFIGURATION |=
		/* For 10/100 Mbps operation */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, PS, 0b1) |
		/* Full duplex mode */
		ENET_QOS_REG_PREP(MAC_CONFIGURATION, DM, 0b1) |
		/* 100 Mbps mode */
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
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_INT_EN, RIE, 0b1);
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
		ENET_QOS_REG_PREP(DMA_CH_DMA_CHX_RX_CTRL, RBSZ_13_Y, NET_ETH_MAX_FRAME_SIZE >> 2);

	return 0;
}

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

	/* For reporting the status of the link connection */
	ret = phy_link_callback_set(config->phy_dev, eth_nxp_enet_qos_phy_cb, (void *)dev);
	if (ret) {
		return ret;
	}

	/* Random mac therefore overrides local mac that may have been initialized */
	if (config->random_mac) {
		gen_random_mac(data->mac_addr.addr,
			       NXP_OUI_BYTE_0, NXP_OUI_BYTE_1, NXP_OUI_BYTE_2);
	}

	/* This driver cannot work without interrupts. */
	if (config->irq_config_func) {
		config->irq_config_func();
	} else {
		return -ENOSYS;
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

	/* Work upon a complete transmission by a channel's TX DMA */
	k_work_init(&data->tx.tx_done_work, tx_dma_done);

	return ret;
}

static const struct device *eth_nxp_enet_qos_get_phy(const struct device *dev)
{
	const struct nxp_enet_qos_mac_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init = eth_nxp_enet_qos_iface_init,
	.send = eth_nxp_enet_qos_tx,
	.get_capabilities = eth_nxp_enet_qos_get_capabilities,
	.get_phy = eth_nxp_enet_qos_get_phy,
};

#define NXP_ENET_QOS_NODE_HAS_MAC_ADDR_CHECK(n)						\
	BUILD_ASSERT(NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)) ||				\
			DT_INST_PROP(n, zephyr_random_mac_address),			\
			"MAC address not specified on ENET QOS DT node");

#define NXP_ENET_QOS_CONNECT_IRQS(node_id, prop, idx)					\
	do {										\
		IRQ_CONNECT(DT_IRQN_BY_IDX(node_id, idx),				\
				DT_IRQ_BY_IDX(node_id, idx, priority),			\
				eth_nxp_enet_qos_mac_isr,				\
				DEVICE_DT_GET(node_id),					\
				0);							\
		irq_enable(DT_IRQN_BY_IDX(node_id, idx));				\
	} while (false);

#define NXP_ENET_QOS_IRQ_CONFIG_FUNC(n)							\
	static void nxp_enet_qos_##n##_irq_config_func(void)				\
	{										\
		DT_FOREACH_PROP_ELEM(DT_DRV_INST(n),					\
				interrupt_names,					\
				NXP_ENET_QOS_CONNECT_IRQS)				\
	}

#define NXP_ENET_QOS_DRIVER_STRUCTS_INIT(n)						\
	static const struct nxp_enet_qos_mac_config enet_qos_##n##_mac_config = {	\
		.enet_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),				\
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),		\
		.base = (enet_qos_t *)DT_REG_ADDR(DT_INST_PARENT(n)),			\
		.hw_info = {								\
			.max_frame_len = ENET_QOS_MAX_NORMAL_FRAME_LEN,			\
		},									\
		.irq_config_func = nxp_enet_qos_##n##_irq_config_func,			\
		.random_mac = DT_INST_PROP(n, zephyr_random_mac_address),		\
	};										\
											\
	static struct nxp_enet_qos_mac_data enet_qos_##n##_mac_data =			\
	{										\
		.mac_addr.addr = DT_INST_PROP_OR(n, local_mac_address, {0}),		\
	};

#define NXP_ENET_QOS_DRIVER_INIT(n)							\
	NXP_ENET_QOS_NODE_HAS_MAC_ADDR_CHECK(n)						\
	NXP_ENET_QOS_IRQ_CONFIG_FUNC(n)							\
	NXP_ENET_QOS_DRIVER_STRUCTS_INIT(n)

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_QOS_DRIVER_INIT)

#define NXP_ENET_QOS_MAC_DEVICE_DEFINE(n)						\
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_nxp_enet_qos_mac_init, NULL,		\
				&enet_qos_##n##_mac_data, &enet_qos_##n##_mac_config,	\
				CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_QOS_MAC_DEVICE_DEFINE)
