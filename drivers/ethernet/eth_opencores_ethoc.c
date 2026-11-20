/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT opencores_ethoc

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <ethernet/eth_stats.h>

LOG_MODULE_REGISTER(eth_opencores_ethoc, CONFIG_ETHERNET_LOG_LEVEL);

/*
 * The MAC has no scatter-gather support, one buffer descriptor always holds a
 * whole frame. Transmitted frames are sent from the network buffer they are
 * stored in if the packet is, or can be compacted into, a single fragment.
 *
 * With CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY, received frames are written
 * straight into network buffers, and other packets are copied into a new
 * network buffer before they are sent. Otherwise each descriptor has an
 * internal frame buffer that received frames are copied out of and other
 * packets are copied into.
 */

#define ETHOC_MODER      0x00
#define ETHOC_INT_SOURCE 0x04
#define ETHOC_INT_MASK   0x08
#define ETHOC_IPGT       0x0c
#define ETHOC_PACKETLEN  0x18
#define ETHOC_TX_BD_NUM  0x20
#define ETHOC_MAC_ADDR0  0x40
#define ETHOC_MAC_ADDR1  0x44
#define ETHOC_ETH_HASH0  0x48
#define ETHOC_ETH_HASH1  0x4c
#define ETHOC_BD_BASE    0x400

#define ETHOC_BD_STAT(i) (ETHOC_BD_BASE + ((i) * 8U))
#define ETHOC_BD_ADDR(i) (ETHOC_BD_BASE + ((i) * 8U) + 4U)

#define ETHOC_MODER_RXEN  BIT(0)
#define ETHOC_MODER_TXEN  BIT(1)
#define ETHOC_MODER_PRO   BIT(5)
#define ETHOC_MODER_FULLD BIT(10)
#define ETHOC_MODER_CRCEN BIT(13)
#define ETHOC_MODER_PAD   BIT(15)

#define ETHOC_INT_TXB  BIT(0)
#define ETHOC_INT_TXE  BIT(1)
#define ETHOC_INT_RXB  BIT(2)
#define ETHOC_INT_RXE  BIT(3)
#define ETHOC_INT_BUSY BIT(4)
#define ETHOC_INT_ALL  GENMASK(6, 0)

#define ETHOC_INT_TX (ETHOC_INT_TXB | ETHOC_INT_TXE)
#define ETHOC_INT_RX (ETHOC_INT_RXB | ETHOC_INT_RXE)

#define ETHOC_PACKETLEN_MINFL GENMASK(31, 16)
#define ETHOC_PACKETLEN_MAXFL GENMASK(15, 0)

/* recommended inter packet gap for full and half duplex */
#define ETHOC_IPGT_FULLD 0x15U
#define ETHOC_IPGT_HALFD 0x12U

#define ETHOC_BD_LEN GENMASK(31, 16)

#define ETHOC_TX_BD_RD  BIT(15)
#define ETHOC_TX_BD_IRQ BIT(14)
#define ETHOC_TX_BD_WR  BIT(13)
#define ETHOC_TX_BD_PAD BIT(12)
#define ETHOC_TX_BD_CRC BIT(11)
#define ETHOC_TX_BD_UR  BIT(8)
#define ETHOC_TX_BD_RL  BIT(3)
#define ETHOC_TX_BD_LC  BIT(2)
#define ETHOC_TX_BD_CS  BIT(0)

#define ETHOC_TX_BD_ERRORS (ETHOC_TX_BD_UR | ETHOC_TX_BD_RL | ETHOC_TX_BD_LC | ETHOC_TX_BD_CS)

#define ETHOC_RX_BD_E    BIT(15)
#define ETHOC_RX_BD_IRQ  BIT(14)
#define ETHOC_RX_BD_WRAP BIT(13)
#define ETHOC_RX_BD_OR   BIT(6)
#define ETHOC_RX_BD_IS   BIT(5)
#define ETHOC_RX_BD_TL   BIT(3)
#define ETHOC_RX_BD_SF   BIT(2)
#define ETHOC_RX_BD_CRC  BIT(1)
#define ETHOC_RX_BD_LC   BIT(0)

#define ETHOC_RX_BD_ERRORS                                                                         \
	(ETHOC_RX_BD_OR | ETHOC_RX_BD_IS | ETHOC_RX_BD_TL | ETHOC_RX_BD_SF | ETHOC_RX_BD_CRC |     \
	 ETHOC_RX_BD_LC)

#define ETHOC_NUM_BDS 128U
#define ETHOC_NB_TX   CONFIG_ETH_OPENCORES_ETHOC_NB_TX_DESCS
#define ETHOC_NB_RX   CONFIG_ETH_OPENCORES_ETHOC_NB_RX_DESCS

BUILD_ASSERT((ETHOC_NB_TX + ETHOC_NB_RX) <= ETHOC_NUM_BDS,
	     "The MAC has only 128 buffer descriptors for TX and RX");

#define ETHOC_FCS_LEN 4U
#define ETHOC_MINFL   64U
#define ETHOC_MAXFL   (NET_ETH_MAX_FRAME_SIZE + ETHOC_FCS_LEN)

/* all instances are assumed to be either DMA coherent or not */
#define ETHOC_DMA_COHERENT DT_ALL_INST_HAS_BOOL_STATUS_OKAY(dma_coherent)

/* cache maintenance of a buffer must not affect neighbouring data */
#if !ETHOC_DMA_COHERENT && defined(CONFIG_DCACHE)
#define ETHOC_BUF_ALIGN MAX(4U, CONFIG_DCACHE_LINE_SIZE)
#else
#define ETHOC_BUF_ALIGN 4U
#endif

/* QEMU appends the 4 FCS bytes even to frames of MAXFL - 1 bytes */
#define ETHOC_RX_BUF_SIZE ROUND_UP(ETHOC_MAXFL + ETHOC_FCS_LEN, ETHOC_BUF_ALIGN)
#define ETHOC_TX_BUF_SIZE ROUND_UP(NET_ETH_MAX_FRAME_SIZE, ETHOC_BUF_ALIGN)

#if defined(CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY) && defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
BUILD_ASSERT(CONFIG_NET_BUF_DATA_SIZE >= ETHOC_RX_BUF_SIZE,
	     "A whole frame must fit into one network buffer");
#endif

#define ETHOC_TX_TIMEOUT K_MSEC(100)

struct ethoc_config {
	DEVICE_MMIO_ROM;
	const struct device *phy_dev;
	void (*irq_config)(void);
	struct net_eth_mac_config mac_cfg;
};

struct ethoc_data {
	DEVICE_MMIO_RAM;
	struct net_if *iface;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	struct k_spinlock tx_lock;
	struct k_sem free_tx_descs;
	struct net_pkt *tx_pkts[ETHOC_NB_TX];
	uint8_t tx_head;
	uint8_t tx_tail;
	uint8_t rx_tail;
#ifdef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
	/* linearized copy of a packet, NULL if it is sent from its own buffer */
	struct net_buf *tx_bufs[ETHOC_NB_TX];
	struct net_buf *rx_bufs[ETHOC_NB_RX];
#else
	uint8_t tx_frames[ETHOC_NB_TX][ETHOC_TX_BUF_SIZE] __aligned(ETHOC_BUF_ALIGN);
	uint8_t rx_frames[ETHOC_NB_RX][ETHOC_RX_BUF_SIZE] __aligned(ETHOC_BUF_ALIGN);
#endif
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	struct net_stats_eth stats;
#endif
};

static inline uint32_t ethoc_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void ethoc_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

static inline void ethoc_cache_flush(void *addr, size_t size)
{
	if (ETHOC_DMA_COHERENT == 0) {
		sys_cache_data_flush_range(addr, size);
	}
}

static inline void ethoc_cache_invd(void *addr, size_t size)
{
	if (ETHOC_DMA_COHERENT == 0) {
		sys_cache_data_invd_range(addr, size);
	}
}

static void ethoc_set_mac_addr(const struct device *dev, const uint8_t *addr)
{
	ethoc_write(dev, ETHOC_MAC_ADDR0, sys_get_be32(&addr[2]));
	ethoc_write(dev, ETHOC_MAC_ADDR1, sys_get_be16(&addr[0]));
}

static void ethoc_rx_arm(const struct device *dev, uint8_t idx, uint8_t *buf)
{
	uint32_t stat = ETHOC_RX_BD_E | ETHOC_RX_BD_IRQ;

	__ASSERT((POINTER_TO_UINT(buf) & 3U) == 0U, "RX buffer not word aligned");

	if (idx == (ETHOC_NB_RX - 1U)) {
		stat |= ETHOC_RX_BD_WRAP;
	}

	/* no dirty cache line may be written back over the received data */
	ethoc_cache_invd(buf, ETHOC_RX_BUF_SIZE);

	ethoc_write(dev, ETHOC_BD_ADDR(ETHOC_NB_TX + idx), POINTER_TO_UINT(buf));
	ethoc_write(dev, ETHOC_BD_STAT(ETHOC_NB_TX + idx), stat);
}

#ifdef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
static uint8_t *ethoc_rx_buf(struct ethoc_data *data, uint8_t idx)
{
	return data->rx_bufs[idx]->data;
}

static struct net_pkt *ethoc_rx_pkt(const struct device *dev, uint8_t idx, size_t len)
{
	struct ethoc_data *data = dev->data;
	struct net_buf *buf = data->rx_bufs[idx];
	struct net_buf *new_buf;
	struct net_pkt *pkt;

	/* the frame is dropped and its buffer reused if there is no replacement */
	new_buf = net_pkt_get_reserve_rx_data(ETHOC_RX_BUF_SIZE, K_NO_WAIT);
	if (new_buf == NULL) {
		return NULL;
	}

	pkt = net_pkt_rx_alloc_on_iface(data->iface, K_NO_WAIT);
	if (pkt == NULL) {
		net_buf_unref(new_buf);
		return NULL;
	}

	ethoc_cache_invd(buf->data, len);
	net_buf_add(buf, len - ETHOC_FCS_LEN);
	net_pkt_frag_add(pkt, buf);

	data->rx_bufs[idx] = new_buf;
	ethoc_rx_arm(dev, idx, new_buf->data);

	return pkt;
}
#else
static uint8_t *ethoc_rx_buf(struct ethoc_data *data, uint8_t idx)
{
	return data->rx_frames[idx];
}

static struct net_pkt *ethoc_rx_pkt(const struct device *dev, uint8_t idx, size_t len)
{
	struct ethoc_data *data = dev->data;
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len - ETHOC_FCS_LEN, NET_AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		return NULL;
	}

	ethoc_cache_invd(data->rx_frames[idx], len);
	if (net_pkt_write(pkt, data->rx_frames[idx], len - ETHOC_FCS_LEN) < 0) {
		net_pkt_unref(pkt);
		return NULL;
	}

	ethoc_rx_arm(dev, idx, data->rx_frames[idx]);

	return pkt;
}
#endif /* CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY */

static struct net_pkt *ethoc_rx_frame(const struct device *dev, uint8_t idx, uint32_t stat)
{
	size_t len = FIELD_GET(ETHOC_BD_LEN, stat);

	if (((stat & ETHOC_RX_BD_ERRORS) != 0U) || (len <= ETHOC_FCS_LEN)) {
		LOG_DBG("RX error, status 0x%04x", stat & 0xffffU);
		return NULL;
	}

	return ethoc_rx_pkt(dev, idx, len);
}

static void ethoc_rx(const struct device *dev)
{
	struct ethoc_data *data = dev->data;
	struct net_pkt *pkt;
	uint32_t stat;
	uint8_t idx;

	while (true) {
		idx = data->rx_tail;
		stat = ethoc_read(dev, ETHOC_BD_STAT(ETHOC_NB_TX + idx));
		if ((stat & ETHOC_RX_BD_E) != 0U) {
			break;
		}

		pkt = ethoc_rx_frame(dev, idx, stat);
		if (pkt == NULL) {
			eth_stats_update_errors_rx(data->iface);
			/* hand the buffer back to the MAC */
			ethoc_rx_arm(dev, idx, ethoc_rx_buf(data, idx));
		} else if (net_recv_data(data->iface, pkt) < 0) {
			eth_stats_update_errors_rx(data->iface);
			net_pkt_unref(pkt);
		}

		data->rx_tail = (idx + 1U) % ETHOC_NB_RX;
	}
}

static void ethoc_tx_done(const struct device *dev)
{
	struct ethoc_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);
	uint32_t stat;
	uint8_t idx;

	while (data->tx_pkts[data->tx_tail] != NULL) {
		idx = data->tx_tail;
		stat = ethoc_read(dev, ETHOC_BD_STAT(idx));
		if ((stat & ETHOC_TX_BD_RD) != 0U) {
			break;
		}

		if ((stat & ETHOC_TX_BD_ERRORS) != 0U) {
			LOG_DBG("TX error, status 0x%04x", stat & 0xffffU);
			eth_stats_update_errors_tx(data->iface);
		}

#ifdef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
		if (data->tx_bufs[idx] != NULL) {
			net_buf_unref(data->tx_bufs[idx]);
			data->tx_bufs[idx] = NULL;
		}
#endif

		net_pkt_unref(data->tx_pkts[idx]);
		data->tx_pkts[idx] = NULL;

		data->tx_tail = (idx + 1U) % ETHOC_NB_TX;
		k_sem_give(&data->free_tx_descs);
	}

	k_spin_unlock(&data->tx_lock, key);
}

static void ethoc_isr(const struct device *dev)
{
	struct ethoc_data *data __maybe_unused = dev->data;
	uint32_t pending;

	pending = ethoc_read(dev, ETHOC_INT_SOURCE) & ethoc_read(dev, ETHOC_INT_MASK);
	ethoc_write(dev, ETHOC_INT_SOURCE, pending);

	if ((pending & ETHOC_INT_TX) != 0U) {
		ethoc_tx_done(dev);
	}

	if ((pending & ETHOC_INT_RX) != 0U) {
		ethoc_rx(dev);
	}

	if ((pending & ETHOC_INT_BUSY) != 0U) {
		/* a frame was dropped for lack of an empty RX descriptor */
		eth_stats_update_errors_rx(data->iface);
	}
}

/* move the frame into the first fragment if it fits and no fragment is shared */
static void ethoc_tx_compact(struct net_pkt *pkt, size_t len)
{
	struct net_buf *first = pkt->frags;

	if (len > (first->len + net_buf_tailroom(first))) {
		return;
	}

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		if (frag->ref != 1U) {
			return;
		}
	}

	net_pkt_compact(pkt);
}

#ifdef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
static struct net_buf *ethoc_tx_linearize(struct net_pkt *pkt, size_t len)
{
	struct net_buf *buf;

	buf = net_pkt_get_reserve_tx_data(len, ETHOC_TX_TIMEOUT);
	if (buf == NULL) {
		return NULL;
	}

	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, net_buf_add(buf, len), len) < 0) {
		net_buf_unref(buf);
		return NULL;
	}

	return buf;
}
#endif /* CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY */

static int ethoc_send(const struct device *dev, struct net_pkt *pkt)
{
	struct ethoc_data *data = dev->data;
	size_t len = net_pkt_get_len(pkt);
	struct net_buf *linear __maybe_unused = NULL;
	uint8_t *frame = NULL;
	k_spinlock_key_t key;
	uint32_t stat;
	uint8_t idx;

	if ((len == 0U) || (len > NET_ETH_MAX_FRAME_SIZE)) {
		return -EINVAL;
	}

	if (k_sem_take(&data->free_tx_descs, ETHOC_TX_TIMEOUT) != 0) {
		return -ENOBUFS;
	}

	if (pkt->frags->frags != NULL) {
		ethoc_tx_compact(pkt, len);
	}

	if (pkt->frags->frags == NULL) {
		frame = pkt->frags->data;
	} else {
#ifdef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
		linear = ethoc_tx_linearize(pkt, len);
		if (linear == NULL) {
			k_sem_give(&data->free_tx_descs);
			return -ENOBUFS;
		}

		frame = linear->data;
#endif
	}

	stat = FIELD_PREP(ETHOC_BD_LEN, len) | ETHOC_TX_BD_RD | ETHOC_TX_BD_IRQ | ETHOC_TX_BD_PAD |
	       ETHOC_TX_BD_CRC;

	key = k_spin_lock(&data->tx_lock);

	idx = data->tx_head;
	if (idx == (ETHOC_NB_TX - 1U)) {
		stat |= ETHOC_TX_BD_WR;
	}

#ifndef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
	if (frame == NULL) {
		/* copy into the internal buffer of the descriptor, it is free */
		net_pkt_cursor_init(pkt);
		if (net_pkt_read(pkt, data->tx_frames[idx], len) < 0) {
			k_spin_unlock(&data->tx_lock, key);
			k_sem_give(&data->free_tx_descs);
			return -EIO;
		}

		frame = data->tx_frames[idx];
	}
#else
	data->tx_bufs[idx] = linear;
#endif

	ethoc_cache_flush(frame, len);

	data->tx_pkts[idx] = net_pkt_ref(pkt);
	data->tx_head = (idx + 1U) % ETHOC_NB_TX;

	/* the MAC may start as soon as the ready bit is set */
	ethoc_write(dev, ETHOC_BD_ADDR(idx), POINTER_TO_UINT(frame));
	ethoc_write(dev, ETHOC_BD_STAT(idx), stat);

	k_spin_unlock(&data->tx_lock, key);

	return 0;
}

static void ethoc_phy_link_state_changed(const struct device *phy_dev __unused,
					 struct phy_link_state *state, void *user_data)
{
	const struct device *dev = user_data;
	struct ethoc_data *data = dev->data;
	uint32_t moder;

	if (state->is_up) {
		moder = ethoc_read(dev, ETHOC_MODER);

		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			moder |= ETHOC_MODER_FULLD;
			ethoc_write(dev, ETHOC_IPGT, ETHOC_IPGT_FULLD);
		} else {
			moder &= ~ETHOC_MODER_FULLD;
			ethoc_write(dev, ETHOC_IPGT, ETHOC_IPGT_HALFD);
		}

		ethoc_write(dev, ETHOC_MODER, moder);
	}

	net_eth_carrier_set(data->iface, state->is_up);
}

static void ethoc_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct ethoc_config *cfg = dev->config;
	struct ethoc_data *data = dev->data;

	data->iface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	/* do not start the interface until the PHY link is up */
	net_if_carrier_off(iface);
	phy_link_callback_set(cfg->phy_dev, ethoc_phy_link_state_changed, (void *)dev);

	ethoc_write(dev, ETHOC_INT_SOURCE, ETHOC_INT_ALL);
	ethoc_write(dev, ETHOC_INT_MASK, ETHOC_INT_TX | ETHOC_INT_RX | ETHOC_INT_BUSY);
	ethoc_write(dev, ETHOC_MODER,
		    ethoc_read(dev, ETHOC_MODER) | ETHOC_MODER_RXEN | ETHOC_MODER_TXEN);
}

static enum ethernet_hw_caps ethoc_get_capabilities(const struct device *dev __unused,
						    struct net_if *iface __unused)
{
	enum ethernet_hw_caps caps = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;

	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
		caps |= ETHERNET_PROMISC_MODE;
	}

	return caps;
}

static int ethoc_set_config(const struct device *dev, struct net_if *iface __unused,
			    enum ethernet_config_type type, const struct ethernet_config *config)
{
	struct ethoc_data *data = dev->data;
	uint32_t moder;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		ethoc_set_mac_addr(dev, data->mac_addr);
		net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			return -ENOTSUP;
		}

		moder = ethoc_read(dev, ETHOC_MODER);
		if (config->promisc_mode == ((moder & ETHOC_MODER_PRO) != 0U)) {
			return -EALREADY;
		}

		ethoc_write(dev, ETHOC_MODER, moder ^ ETHOC_MODER_PRO);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static const struct device *ethoc_get_phy(const struct device *dev, struct net_if *iface __unused)
{
	const struct ethoc_config *cfg = dev->config;

	return cfg->phy_dev;
}

#ifdef CONFIG_NET_STATISTICS_ETHERNET
static struct net_stats_eth *ethoc_get_stats(const struct device *dev,
					     struct net_if *iface __unused)
{
	struct ethoc_data *data = dev->data;

	return &data->stats;
}
#endif

static int ethoc_init(const struct device *dev)
{
	const struct ethoc_config *cfg = dev->config;
	struct ethoc_data *data = dev->data;
	struct net_buf *buf __maybe_unused;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ethoc_write(dev, ETHOC_MODER, ETHOC_MODER_PAD | ETHOC_MODER_CRCEN | ETHOC_MODER_FULLD);
	ethoc_write(dev, ETHOC_INT_MASK, 0U);
	ethoc_write(dev, ETHOC_IPGT, ETHOC_IPGT_FULLD);
	ethoc_write(dev, ETHOC_PACKETLEN,
		    FIELD_PREP(ETHOC_PACKETLEN_MINFL, ETHOC_MINFL) |
			    FIELD_PREP(ETHOC_PACKETLEN_MAXFL, ETHOC_MAXFL));
	ethoc_write(dev, ETHOC_TX_BD_NUM, ETHOC_NB_TX);

	for (uint8_t i = 0U; i < ETHOC_NB_TX; i++) {
		ethoc_write(dev, ETHOC_BD_STAT(i), 0U);
		ethoc_write(dev, ETHOC_BD_ADDR(i), 0U);
	}

	/* accept all multicast frames */
	ethoc_write(dev, ETHOC_ETH_HASH0, UINT32_MAX);
	ethoc_write(dev, ETHOC_ETH_HASH1, UINT32_MAX);

	ret = net_eth_mac_load(&cfg->mac_cfg, data->mac_addr);
	if ((ret < 0) && (ret != -ENODATA)) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
		return ret;
	}

	ethoc_set_mac_addr(dev, data->mac_addr);

	for (uint8_t i = 0U; i < ETHOC_NB_RX; i++) {
#ifdef CONFIG_ETH_OPENCORES_ETHOC_ZERO_COPY
		buf = net_pkt_get_reserve_rx_data(ETHOC_RX_BUF_SIZE, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Not enough RX buffers for %u descriptors", ETHOC_NB_RX);
			return -ENOMEM;
		}

		data->rx_bufs[i] = buf;
#endif
		ethoc_rx_arm(dev, i, ethoc_rx_buf(data, i));
	}

	k_sem_init(&data->free_tx_descs, ETHOC_NB_TX, ETHOC_NB_TX);

	cfg->irq_config();

	return 0;
}

static const struct ethernet_api ethoc_api = {
	.iface_api.init = ethoc_iface_init,
	.get_capabilities = ethoc_get_capabilities,
	.set_config = ethoc_set_config,
	.get_phy = ethoc_get_phy,
	.send = ethoc_send,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.get_stats = ethoc_get_stats,
#endif
};

#define ETHOC_IRQ_PRIORITY(n)                                                                      \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, priority), (DT_INST_IRQ(n, priority)), (0))

#define ETHOC_INIT(n)                                                                              \
	static void ethoc_irq_config_##n(void)                                                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), ETHOC_IRQ_PRIORITY(n), ethoc_isr,                     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ethoc_config ethoc_config_##n = {                                      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                          \
		.irq_config = ethoc_irq_config_##n,                                                \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                                     \
	};                                                                                         \
                                                                                                   \
	static struct ethoc_data ethoc_data_##n;                                                   \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, ethoc_init, NULL, &ethoc_data_##n, &ethoc_config_##n,     \
				      CONFIG_ETH_INIT_PRIORITY, &ethoc_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETHOC_INIT)
