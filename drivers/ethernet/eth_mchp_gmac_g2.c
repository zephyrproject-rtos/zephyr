/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Ethernet driver for the Microchip G2 GMAC, the "ETH" peripheral of
 * PIC32CZ CA: a Cadence GEM core at +0x1000 behind a Microchip wrapper at
 * +0x000. Both halves are reached through eth_registers_t.
 *
 * Not the peripheral eth_mchp_gmac_g1.c drives. That one is the 10/100 GMAC
 * of SAM E5x and PIC32CX SG, written against gmac_registers_t; this core adds
 * gigabit over GMII, a 64-bit data bus, six DMA queues with one interrupt
 * each and the wrapper, and it sits on a Cortex-M7 with the data cache on.
 *
 * Data path. Every frame goes through queue 0 and is copied: received frames
 * from a ring of driver-owned buffers into a net_pkt, transmitted ones from
 * the net_pkt into a ring of driver-owned buffers. Each buffer holds one
 * whole frame, so every descriptor carries a complete frame and none of the
 * fragment bookkeeping of a zero-copy ring is needed. Descriptors live in
 * non-cacheable memory; buffers are cacheable, cache-line aligned and sized,
 * flushed before the DMA reads them and invalidated before the CPU reads
 * what the DMA wrote. Queues 1..n are parked on an inert descriptor, because
 * the core requires every queue to point at a valid one even when unused;
 * their interrupts are disabled at the MAC and their lines are left
 * unconnected.
 */

#define DT_DRV_COMPAT microchip_gmac_g2_eth

#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>

#include <soc.h>

LOG_MODULE_REGISTER(eth_mchp_gmac_g2, CONFIG_ETHERNET_LOG_LEVEL);

/*
 * One buffer holds one frame: 1536 bytes covers the largest frame the MAC
 * accepts with NCFGR.MAXFS set (the FCS is stripped before the DMA). It is
 * also a multiple of 64, the unit of DCFGR.DRBS, and of the cache line.
 */
#define ETH_G2_BUF_SIZE  1536U
#define ETH_G2_BUF_ALIGN 64U
#define ETH_G2_DESC_ALIGN 8U

BUILD_ASSERT((ETH_G2_BUF_SIZE % 64U) == 0U, "DRBS counts in units of 64 bytes");
#if defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 0)
BUILD_ASSERT((ETH_G2_BUF_ALIGN % CONFIG_DCACHE_LINE_SIZE) == 0U,
	     "DMA buffers must not share a cache line with anything else");
#endif
#if defined(CONFIG_DCACHE)
BUILD_ASSERT(IS_ENABLED(CONFIG_NOCACHE_MEMORY),
	     "__nocache is a no-op without CONFIG_NOCACHE_MEMORY, and the "
	     "descriptors would then be cached behind the DMA's back");
#endif

#define ETH_G2_RX_COUNT CONFIG_ETH_MCHP_GMAC_G2_RX_BUF_COUNT
#define ETH_G2_TX_COUNT CONFIG_ETH_MCHP_GMAC_G2_TX_BUF_COUNT

/* How long send() waits for a free descriptor before restarting the queue. */
#define ETH_G2_TX_TIMEOUT K_MSEC(100)

/* CTRLA.SWRST and CTRLA.ENABLE both synchronise; neither should take long. */
#define ETH_G2_SYNC_TIMEOUT_US 1000U

/* Receive descriptor, word 0. */
#define ETH_G2_RXD_OWN  BIT(0)
#define ETH_G2_RXD_WRAP BIT(1)
/* Receive descriptor, word 1. */
#define ETH_G2_RXD_LEN_MASK GENMASK(12, 0)
#define ETH_G2_RXD_SOF      BIT(14)
#define ETH_G2_RXD_EOF      BIT(15)

/* Transmit descriptor, word 1. */
#define ETH_G2_TXD_LEN_MASK GENMASK(13, 0)
#define ETH_G2_TXD_LAST     BIT(15)
#define ETH_G2_TXD_ERRORS   (BIT(20) | BIT(21) | BIT(22) | BIT(26) | BIT(27) | BIT(29))
#define ETH_G2_TXD_WRAP     BIT(30)
#define ETH_G2_TXD_USED     BIT(31)

#define ETH_G2_TX_ERR_INT (ETH_ISR_TUR_Msk | ETH_ISR_RLEX_Msk | ETH_ISR_TFC_Msk)
#define ETH_G2_RX_INT     (ETH_ISR_RCOMP_Msk | ETH_ISR_RXUBR_Msk | ETH_ISR_ROVR_Msk)
#define ETH_G2_INT_MASK                                                                            \
	(ETH_G2_RX_INT | ETH_ISR_TCOMP_Msk | ETH_G2_TX_ERR_INT | ETH_ISR_HRESP_Msk)

/* Frames the receive thread takes before it yields the processor. */
#define ETH_G2_RX_BUDGET 8U

/* Bits of eth_g2_data.flags: recovery the ISR asks a thread to carry out. */
#define ETH_G2_RX_RESTART 0
#define ETH_G2_TX_RESTART 1

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
#define ETH_G2_STAT_ADD(data, field, n) ((data)->stats.field += (n))
#else
#define ETH_G2_STAT_ADD(data, field, n) ARG_UNUSED(data)
#endif

enum eth_g2_conn {
	ETH_G2_CONN_MII,
	ETH_G2_CONN_RMII,
	ETH_G2_CONN_GMII,
};

struct eth_g2_desc {
	uint32_t addr;
	uint32_t status;
};

struct eth_g2_config {
	eth_registers_t *regs;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_ahb;
	clock_control_subsys_t mclk_apb;
	clock_control_subsys_t gclk_tx;
	clock_control_subsys_t gclk_tsu;
	const struct device *phy_dev;
	struct net_eth_mac_config mcfg;
	void (*irq_config)(void);
	enum eth_g2_conn conn;
	uint8_t prio_queues;
	struct eth_g2_desc *rx_desc;
	struct eth_g2_desc *tx_desc;
	/* [0] parks the receive side of queues 1..n, [1] the transmit side. */
	struct eth_g2_desc *dummy_desc;
	uint8_t (*rx_buf)[ETH_G2_BUF_SIZE];
	uint8_t (*tx_buf)[ETH_G2_BUF_SIZE];
};

struct eth_g2_data {
	struct net_if *iface;
	/* Free transmit descriptors. Taken by send(), given back by the ISR. */
	struct k_sem tx_sem;
	/* Serialises send() and every reset of the transmit ring. */
	struct k_mutex tx_lock;
	/* Serialises the receive thread and every reset of the receive ring. */
	struct k_mutex rx_lock;
	struct k_sem rx_sem;
	atomic_t flags;
	bool link_up;
	uint16_t tx_head;
	uint16_t tx_tail;
	uint16_t rx_tail;
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_ETH_MCHP_GMAC_G2_RX_THREAD_STACK_SIZE);
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

/*
 * NCR is written from the transmit path, the receive thread and the link
 * callback. A read-modify-write interleaved with another one would bring back
 * a bit the other just cleared - RXEN in the middle of a ring reset, say.
 */
static void eth_g2_ncr_update(eth_registers_t *regs, uint32_t clear, uint32_t set)
{
	unsigned int key = irq_lock();

	regs->ETH_NCR = (regs->ETH_NCR & ~clear) | set;
	irq_unlock(key);
}

/*
 * Reset and enable the wrapper, unless it already is: the MDIO driver shares
 * this block and may have been here first, and resetting it then would throw
 * away its MDC divider and management port enable.
 */
static int eth_g2_wrapper_enable(eth_registers_t *regs)
{
	if ((regs->ETH_CTRLA & ETH_CTRLA_ENABLE_Msk) != 0U) {
		return 0;
	}

	regs->ETH_CTRLA = ETH_CTRLA_SWRST_Msk;
	if (!WAIT_FOR((regs->ETH_SYNCB & ETH_SYNCB_SWRST_Msk) == 0U, ETH_G2_SYNC_TIMEOUT_US,
		      k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	regs->ETH_CTRLA = ETH_CTRLA_ENABLE_Msk;
	if (!WAIT_FOR((regs->ETH_SYNCB & ETH_SYNCB_ENABLE_Msk) == 0U, ETH_G2_SYNC_TIMEOUT_US,
		      k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

/* Receive ring back to its initial state. RXEN must be clear. */
static void eth_g2_rx_ring_init(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;

	for (int i = 0; i < ETH_G2_RX_COUNT; i++) {
		cfg->rx_desc[i].status = 0U;
		cfg->rx_desc[i].addr = (uint32_t)cfg->rx_buf[i] |
				       ((i == ETH_G2_RX_COUNT - 1) ? ETH_G2_RXD_WRAP : 0U);
	}

	/*
	 * Nothing may be dirty in the cache over a receive buffer: a line
	 * evicted after the DMA wrote the frame would overwrite it.
	 */
	sys_cache_data_invd_range((void *)cfg->rx_buf, ETH_G2_RX_COUNT * ETH_G2_BUF_SIZE);

	data->rx_tail = 0U;
	barrier_dmem_fence_full();
	cfg->regs->ETH_RBQB = (uint32_t)cfg->rx_desc;
	cfg->regs->ETH_RSR = cfg->regs->ETH_RSR;
}

/*
 * Transmit ring back to its initial state, dropping whatever was queued.
 * TXEN must be clear and tx_lock held.
 */
static void eth_g2_tx_ring_init(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	unsigned int key;

	key = irq_lock();

	for (int i = 0; i < ETH_G2_TX_COUNT; i++) {
		cfg->tx_desc[i].addr = (uint32_t)cfg->tx_buf[i];
		cfg->tx_desc[i].status = ETH_G2_TXD_USED |
					 ((i == ETH_G2_TX_COUNT - 1) ? ETH_G2_TXD_WRAP : 0U);
	}

	data->tx_head = 0U;
	data->tx_tail = 0U;

	/* One descriptor always stays empty, so head == tail means idle. */
	k_sem_reset(&data->tx_sem);
	for (int i = 0; i < ETH_G2_TX_COUNT - 1; i++) {
		k_sem_give(&data->tx_sem);
	}

	barrier_dmem_fence_full();
	cfg->regs->ETH_TBQB = (uint32_t)cfg->tx_desc;
	cfg->regs->ETH_TSR = cfg->regs->ETH_TSR;

	irq_unlock(key);
}

/* Called with tx_lock held. */
static void eth_g2_tx_restart(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;

	eth_g2_ncr_update(cfg->regs, ETH_NCR_TXEN_Msk, 0U);
	eth_g2_tx_ring_init(dev);
	if (data->link_up) {
		eth_g2_ncr_update(cfg->regs, 0U, ETH_NCR_TXEN_Msk);
	}

	ETH_G2_STAT_ADD(data, tx_restart_queue, 1);
}

/* Called with rx_lock held. */
static void eth_g2_rx_restart(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;

	eth_g2_ncr_update(cfg->regs, ETH_NCR_RXEN_Msk, 0U);
	eth_g2_rx_ring_init(dev);
	if (data->link_up) {
		eth_g2_ncr_update(cfg->regs, 0U, ETH_NCR_RXEN_Msk);
	}
}

static void eth_g2_rx_frame(const struct device *dev, const uint8_t *buf, uint32_t status)
{
	struct eth_g2_data *data = dev->data;
	uint32_t len = status & ETH_G2_RXD_LEN_MASK;
	struct net_pkt *pkt;

	/*
	 * A frame larger than one buffer would span descriptors. MAXFS caps
	 * frames at the buffer size, so this is a malformed descriptor rather
	 * than a frame to reassemble.
	 */
	if ((status & (ETH_G2_RXD_SOF | ETH_G2_RXD_EOF)) !=
	    (ETH_G2_RXD_SOF | ETH_G2_RXD_EOF) || len > ETH_G2_BUF_SIZE) {
		ETH_G2_STAT_ADD(data, error_details.rx_frame_errors, 1);
		ETH_G2_STAT_ADD(data, errors.rx, 1);
		return;
	}

	sys_cache_data_invd_range((void *)buf, ROUND_UP(len, ETH_G2_BUF_ALIGN));

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		ETH_G2_STAT_ADD(data, error_details.rx_no_buffer_count, 1);
		ETH_G2_STAT_ADD(data, errors.rx, 1);
		return;
	}

	if (net_pkt_write(pkt, buf, len) != 0 || net_recv_data(data->iface, pkt) != 0) {
		net_pkt_unref(pkt);
	}
}

/*
 * Hand at most ETH_G2_RX_BUDGET completed descriptors to the stack, and say
 * whether the ring ran empty. Called with rx_lock held.
 *
 * The budget is what keeps the receive path from starving the rest of the
 * system: at line rate the ring refills while it is being emptied, so an
 * unbounded loop here holds the processor for as long as the sender keeps
 * sending.
 */
static bool eth_g2_rx_drain(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	struct eth_g2_desc *desc;
	bool empty = false;

	for (unsigned int n = 0U; n < ETH_G2_RX_BUDGET; n++) {
		desc = &cfg->rx_desc[data->rx_tail];
		if ((desc->addr & ETH_G2_RXD_OWN) == 0U) {
			empty = true;
			break;
		}

		barrier_dmem_fence_full();
		eth_g2_rx_frame(dev, cfg->rx_buf[data->rx_tail], desc->status);

		desc->status = 0U;
		barrier_dmem_fence_full();
		desc->addr &= ~ETH_G2_RXD_OWN;

		data->rx_tail = (data->rx_tail + 1U) % ETH_G2_RX_COUNT;
	}

	/* BNA and REC are sticky; clear them now the ring has room again. */
	cfg->regs->ETH_RSR = cfg->regs->ETH_RSR;

	return empty;
}

static void eth_g2_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	bool empty;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->rx_sem, K_FOREVER);

		/*
		 * The interrupt is masked from the moment the handler asks for
		 * a drain until the ring is empty again. Leaving it unmasked
		 * re-enters the handler for as long as the ring is full, which
		 * is a livelock: this thread is the only thing that frees a
		 * descriptor, and it never gets the processor. Between batches
		 * the thread yields, so a full ring slows the system down
		 * instead of stopping it.
		 */
		do {
			k_mutex_lock(&data->rx_lock, K_FOREVER);
			if (atomic_test_and_clear_bit(&data->flags, ETH_G2_RX_RESTART)) {
				eth_g2_rx_restart(dev);
				empty = true;
			} else {
				empty = eth_g2_rx_drain(dev);
			}

			if (empty) {
				cfg->regs->ETH_IER = ETH_G2_RX_INT;
			}
			k_mutex_unlock(&data->rx_lock);

			if (!empty) {
				k_yield();
			}
		} while (!empty);
	}
}

/* Reclaim every descriptor the MAC has finished with. ISR context. */
static void eth_g2_tx_complete(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	uint32_t status;

	while (data->tx_tail != data->tx_head) {
		status = cfg->tx_desc[data->tx_tail].status;
		if ((status & ETH_G2_TXD_USED) == 0U) {
			break;
		}

		if ((status & ETH_G2_TXD_ERRORS) != 0U) {
			ETH_G2_STAT_ADD(data, errors.tx, 1);
		}

		data->tx_tail = (data->tx_tail + 1U) % ETH_G2_TX_COUNT;
		k_sem_give(&data->tx_sem);
	}
}

static void eth_g2_isr(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	uint32_t isr = cfg->regs->ETH_ISR;

	/* Clear-on-read or write-one-to-clear depending on the core; do both. */
	cfg->regs->ETH_ISR = isr;

	if ((isr & ETH_ISR_TCOMP_Msk) != 0U) {
		eth_g2_tx_complete(dev);
	}

	/*
	 * RXUBR (ring full) and ROVR (FIFO overrun) each drop a frame, which the
	 * RRE and ROE statistics registers count; the ring only needs draining.
	 */
	if ((isr & ETH_G2_RX_INT) != 0U) {
		cfg->regs->ETH_IDR = ETH_G2_RX_INT;
		k_sem_give(&data->rx_sem);
	}

	/* The transmitter has stopped; the next send() restarts it. */
	if ((isr & ETH_G2_TX_ERR_INT) != 0U) {
		atomic_set_bit(&data->flags, ETH_G2_TX_RESTART);
	}

	/* A bus error aborts both DMA directions. */
	if ((isr & ETH_ISR_HRESP_Msk) != 0U) {
		ETH_G2_STAT_ADD(data, error_details.rx_dma_failed, 1);
		ETH_G2_STAT_ADD(data, error_details.tx_dma_failed, 1);
		atomic_set_bit(&data->flags, ETH_G2_TX_RESTART);
		atomic_set_bit(&data->flags, ETH_G2_RX_RESTART);
		cfg->regs->ETH_IDR = ETH_G2_RX_INT;
		k_sem_give(&data->rx_sem);
	}
}

static int eth_g2_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	size_t len = net_pkt_get_len(pkt);
	uint16_t idx;
	uint8_t *buf;
	unsigned int key;

	if (len > ETH_G2_BUF_SIZE) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&data->tx_lock, K_FOREVER);

	if (atomic_test_and_clear_bit(&data->flags, ETH_G2_TX_RESTART)) {
		eth_g2_tx_restart(dev);
	}

	if (k_sem_take(&data->tx_sem, ETH_G2_TX_TIMEOUT) != 0) {
		LOG_WRN("TX timeout, restarting the transmit queue");
		ETH_G2_STAT_ADD(data, tx_timeout_count, 1);
		eth_g2_tx_restart(dev);
		(void)k_sem_take(&data->tx_sem, K_NO_WAIT);
	}

	idx = data->tx_head;
	buf = cfg->tx_buf[idx];

	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, buf, len) != 0) {
		k_sem_give(&data->tx_sem);
		k_mutex_unlock(&data->tx_lock);
		return -EIO;
	}

	sys_cache_data_flush_range(buf, len);

	/*
	 * Handing the descriptor over and advancing head is one step as far as
	 * the ISR is concerned: a transmitter that is still running can send the
	 * frame and complete it before TSTART below.
	 */
	key = irq_lock();
	cfg->tx_desc[idx].status = (len & ETH_G2_TXD_LEN_MASK) | ETH_G2_TXD_LAST |
				   ((idx == ETH_G2_TX_COUNT - 1) ? ETH_G2_TXD_WRAP : 0U);
	data->tx_head = (idx + 1U) % ETH_G2_TX_COUNT;
	irq_unlock(key);

	barrier_dmem_fence_full();
	eth_g2_ncr_update(cfg->regs, 0U, ETH_NCR_TSTART_Msk);

	k_mutex_unlock(&data->tx_lock);

	return 0;
}

static void eth_g2_mac_addr_set(eth_registers_t *regs, const uint8_t *mac)
{
	/* Writing SAB disables the filter until SAT is written. */
	regs->SA[0].ETH_SAB = sys_get_le32(mac);
	regs->SA[0].ETH_SAT = ETH_SAT_ADDR(sys_get_le16(&mac[4]));
}

static void eth_g2_link_changed(const struct device *phy_dev, struct phy_link_state *state,
				void *user_data)
{
	const struct device *dev = user_data;
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	bool up = state->is_up;
	uint32_t ncfgr;

	ARG_UNUSED(phy_dev);

	if (up && PHY_LINK_IS_SPEED_1000M(state->speed) && cfg->conn != ETH_G2_CONN_GMII) {
		LOG_ERR("1000 Mbit/s link needs GMII; keeping the carrier off");
		up = false;
	}

	/*
	 * Clearing RXEN and TXEN rewinds both queue pointers to their base, so
	 * both rings are rebuilt before the MAC runs again at the new speed.
	 */
	k_mutex_lock(&data->tx_lock, K_FOREVER);
	k_mutex_lock(&data->rx_lock, K_FOREVER);

	eth_g2_ncr_update(cfg->regs, ETH_NCR_RXEN_Msk | ETH_NCR_TXEN_Msk, 0U);
	data->link_up = up;

	if (up) {
		ncfgr = cfg->regs->ETH_NCFGR &
			~(ETH_NCFGR_SPD_Msk | ETH_NCFGR_FD_Msk | ETH_NCFGR_GIGE_Msk);
		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			ncfgr |= ETH_NCFGR_FD_Msk;
		}
		if (PHY_LINK_IS_SPEED_1000M(state->speed)) {
			ncfgr |= ETH_NCFGR_GIGE_Msk;
		} else if (PHY_LINK_IS_SPEED_100M(state->speed)) {
			ncfgr |= ETH_NCFGR_SPD_Msk;
		}
		cfg->regs->ETH_NCFGR = ncfgr;

		eth_g2_rx_ring_init(dev);
		eth_g2_tx_ring_init(dev);
		atomic_clear(&data->flags);

		eth_g2_ncr_update(cfg->regs, 0U, ETH_NCR_RXEN_Msk | ETH_NCR_TXEN_Msk);
	}

	k_mutex_unlock(&data->rx_lock);
	k_mutex_unlock(&data->tx_lock);

	net_eth_carrier_set(data->iface, up);
}

static void eth_g2_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	uint8_t mac[NET_ETH_ADDR_LEN];
	int ret;

	data->iface = iface;
	ethernet_init(iface);

	ret = net_eth_mac_load(&cfg->mcfg, mac);
	if (ret == 0) {
		eth_g2_mac_addr_set(cfg->regs, mac);
		net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);
	} else if (ret != -ENODATA) {
		LOG_ERR("Failed to load the MAC address: %d", ret);
	}

	net_if_carrier_off(iface);

	k_thread_create(&data->rx_thread, data->rx_stack, K_KERNEL_STACK_SIZEOF(data->rx_stack),
			eth_g2_rx_thread, (void *)dev, NULL, NULL,
			IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)
				? K_PRIO_PREEMPT(CONFIG_ETH_MCHP_GMAC_G2_RX_THREAD_PRIORITY)
				: K_PRIO_COOP(CONFIG_ETH_MCHP_GMAC_G2_RX_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_mchp_g2_rx");

	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY %s is not ready", cfg->phy_dev->name);
		return;
	}

	phy_link_callback_set(cfg->phy_dev, eth_g2_link_changed, (void *)dev);
}

static enum ethernet_hw_caps eth_g2_get_capabilities(const struct device *dev,
						      struct net_if *iface)
{
	const struct eth_g2_config *cfg = dev->config;
	enum ethernet_hw_caps caps = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE |
				     ETHERNET_HW_TX_CHKSUM_OFFLOAD | ETHERNET_HW_RX_CHKSUM_OFFLOAD;

	ARG_UNUSED(iface);

	if (cfg->conn == ETH_G2_CONN_GMII) {
		caps |= ETHERNET_LINK_1000BASE;
	}

	return caps;
}

static int eth_g2_set_config(const struct device *dev, struct net_if *iface,
			     enum ethernet_config_type type, const struct ethernet_config *config)
{
	const struct eth_g2_config *cfg = dev->config;

	ARG_UNUSED(iface);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		eth_g2_mac_addr_set(cfg->regs, config->mac_address.addr);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int eth_g2_get_config(const struct device *dev, struct net_if *iface,
			     enum ethernet_config_type type, struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT:
	case ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT:
		config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
					 ETHERNET_CHECKSUM_SUPPORT_TCP |
					 ETHERNET_CHECKSUM_SUPPORT_UDP;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static const struct device *eth_g2_get_phy(const struct device *dev, struct net_if *iface)
{
	const struct eth_g2_config *cfg = dev->config;

	ARG_UNUSED(iface);

	return cfg->phy_dev;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
/* The core's statistics registers clear on read; each is read exactly once. */
static struct net_stats_eth *eth_g2_get_stats(const struct device *dev, struct net_if *iface)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	eth_registers_t *regs = cfg->regs;
	struct net_stats_eth *stats = &data->stats;
	uint32_t fcse = regs->ETH_FCSE;
	uint32_t ae = regs->ETH_AE;
	uint32_t ofr = regs->ETH_OFR;
	uint32_t jr = regs->ETH_JR;
	uint32_t ufr = regs->ETH_UFR;
	uint32_t lffe = regs->ETH_LFFE;
	uint32_t rse = regs->ETH_RSE;
	uint32_t rre = regs->ETH_RRE;
	uint32_t roe = regs->ETH_ROE;
	uint32_t csum = regs->ETH_IHCE + regs->ETH_TCE + regs->ETH_UCE;
	uint32_t tur = regs->ETH_TUR;
	uint32_t ec = regs->ETH_EC;
	uint32_t lc = regs->ETH_LC;
	uint32_t cse = regs->ETH_CSE;

	ARG_UNUSED(iface);

	stats->error_details.rx_crc_errors += fcse;
	stats->error_details.rx_align_errors += ae;
	stats->error_details.rx_long_length_errors += ofr + jr;
	stats->error_details.rx_short_length_errors += ufr;
	stats->error_details.rx_length_errors += lffe;
	stats->error_details.rx_frame_errors += rse;
	stats->error_details.rx_missed_errors += rre;
	stats->error_details.rx_over_errors += roe;
	stats->csum.rx_csum_offload_errors += csum;
	stats->errors.rx += fcse + ae + ofr + jr + ufr + lffe + rse + rre + roe + csum;

	stats->error_details.tx_fifo_errors += tur;
	stats->error_details.tx_aborted_errors += ec;
	stats->error_details.tx_window_errors += lc;
	stats->error_details.tx_carrier_errors += cse;
	stats->collisions += regs->ETH_SCF + regs->ETH_MCF + ec + lc;
	stats->errors.tx += tur + ec + lc + cse;

	return stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static const struct ethernet_api eth_g2_api = {
	.iface_api.init = eth_g2_iface_init,
	.get_capabilities = eth_g2_get_capabilities,
	.set_config = eth_g2_set_config,
	.get_config = eth_g2_get_config,
	.get_phy = eth_g2_get_phy,
	.send = eth_g2_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_g2_get_stats,
#endif
};

static int eth_g2_clock_on(const struct eth_g2_config *cfg, clock_control_subsys_t sys)
{
	int ret = clock_control_on(cfg->clock_dev, sys);

	/* -EALREADY: the board devicetree turned it on at boot. */
	return (ret == -EALREADY) ? 0 : ret;
}

static int eth_g2_init(const struct device *dev)
{
	const struct eth_g2_config *cfg = dev->config;
	struct eth_g2_data *data = dev->data;
	eth_registers_t *regs = cfg->regs;
	uint32_t val;
	int ret;

	ret = eth_g2_clock_on(cfg, cfg->mclk_ahb);
	if (ret == 0) {
		ret = eth_g2_clock_on(cfg, cfg->mclk_apb);
	}
	if (ret == 0) {
		ret = eth_g2_clock_on(cfg, cfg->gclk_tx);
	}
	if (ret == 0) {
		/* Without it the wrapper's SWRST never completes. */
		ret = eth_g2_clock_on(cfg, cfg->gclk_tsu);
	}
	if (ret != 0) {
		LOG_ERR("Failed to enable clocks: %d", ret);
		return ret;
	}

	/*
	 * Before the PHY driver, which is initialized after this device
	 * because its node descends from this one: a PHY latches its
	 * strapping from the receive pins when it leaves reset, and the pulls
	 * that set the strapping are part of this state.
	 */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = eth_g2_wrapper_enable(regs);
	if (ret != 0) {
		LOG_ERR("Wrapper did not come out of reset");
		return ret;
	}

	/* Quiesce: receiver and transmitter off, the management port as found. */
	regs->ETH_NCR = (regs->ETH_NCR & ETH_NCR_MPE_Msk) | ETH_NCR_CLRSTAT_Msk;
	regs->ETH_IDR = UINT32_MAX;
	regs->ETH_ISR = regs->ETH_ISR;

	/*
	 * Only queue 0 has a handler, so every source of the other queues is
	 * disabled here. IMRQ comes out of reset with the transmit used bit
	 * read and the receive overrun left unmasked, so this is what keeps
	 * their lines from ever asserting, not the reset value.
	 */
	for (int q = 0; q < cfg->prio_queues; q++) {
		regs->ETH_IDRQ[q] = UINT32_MAX;
	}

	val = regs->ETH_CTRLB & ~(ETH_CTRLB_GMIIEN_Msk | ETH_CTRLB_GBITCLKREQ_Msk);
	if (cfg->conn == ETH_G2_CONN_GMII) {
		val |= ETH_CTRLB_GMIIEN_Msk | ETH_CTRLB_GBITCLKREQ_Msk;
	}
	regs->ETH_CTRLB = val;
	regs->ETH_UR = (cfg->conn == ETH_G2_CONN_MII) ? ETH_UR_MII_Msk : 0U;

	/* The MDC divider belongs to the MDIO driver and is kept as it is. */
	regs->ETH_NCFGR = (regs->ETH_NCFGR & ETH_NCFGR_CLK_Msk) | ETH_NCFGR_DBW(1) |
			  ETH_NCFGR_MTIHEN_Msk | ETH_NCFGR_MAXFS_Msk | ETH_NCFGR_RFCS_Msk |
			  ETH_NCFGR_RXCOEN_Msk;
	regs->ETH_DCFGR = ETH_DCFGR_FBLDO(4) | ETH_DCFGR_RXBMS(3) | ETH_DCFGR_TXPBMS_Msk |
			  ETH_DCFGR_TXCOEN_Msk | ETH_DCFGR_DRBS(ETH_G2_BUF_SIZE / 64U);

	/* Every multicast frame passes the hash filter; the stack sorts them out. */
	regs->ETH_HRB = UINT32_MAX;
	regs->ETH_HRT = UINT32_MAX;

	cfg->dummy_desc[0].addr = ETH_G2_RXD_OWN | ETH_G2_RXD_WRAP;
	cfg->dummy_desc[0].status = 0U;
	cfg->dummy_desc[1].addr = 0U;
	cfg->dummy_desc[1].status = ETH_G2_TXD_USED | ETH_G2_TXD_WRAP;
	barrier_dmem_fence_full();
	for (int q = 0; q < cfg->prio_queues; q++) {
		regs->ETH_RBPQB[q] = (uint32_t)&cfg->dummy_desc[0];
		regs->ETH_TBPQB[q] = (uint32_t)&cfg->dummy_desc[1];
	}

	k_sem_init(&data->tx_sem, 0, ETH_G2_TX_COUNT - 1);
	k_sem_init(&data->rx_sem, 0, 1);
	k_mutex_init(&data->tx_lock);
	k_mutex_init(&data->rx_lock);

	eth_g2_rx_ring_init(dev);
	eth_g2_tx_ring_init(dev);

	cfg->irq_config();
	regs->ETH_IER = ETH_G2_INT_MASK;

	return 0;
}

#define ETH_G2_CONN(n)                                                                             \
	(DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii)   ? ETH_G2_CONN_GMII                 \
	 : DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii) ? ETH_G2_CONN_RMII                 \
								 : ETH_G2_CONN_MII)

#define ETH_G2_DEFINE(n)                                                                           \
	BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii) ||                       \
			     DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, mii) ||                \
			     DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii),                 \
		     "phy-connection-type must be gmii, mii or rmii");                             \
	BUILD_ASSERT(DT_INST_PROP(n, num_queues) >= 1 &&                                           \
			     DT_INST_PROP(n, num_queues) - 1 <=                                    \
				     ARRAY_SIZE(((eth_registers_t *)0)->ETH_RBPQB),                \
		     "num-queues is outside what the register map can address");                   \
                                                                                                   \
	static struct eth_g2_desc eth_g2_rx_desc_##n[ETH_G2_RX_COUNT] __nocache                    \
		__aligned(ETH_G2_DESC_ALIGN);                                                      \
	static struct eth_g2_desc eth_g2_tx_desc_##n[ETH_G2_TX_COUNT] __nocache                    \
		__aligned(ETH_G2_DESC_ALIGN);                                                      \
	static struct eth_g2_desc eth_g2_dummy_desc_##n[2] __nocache __aligned(ETH_G2_DESC_ALIGN); \
	static uint8_t eth_g2_rx_buf_##n[ETH_G2_RX_COUNT][ETH_G2_BUF_SIZE]                         \
		__aligned(ETH_G2_BUF_ALIGN);                                                       \
	static uint8_t eth_g2_tx_buf_##n[ETH_G2_TX_COUNT][ETH_G2_BUF_SIZE]                         \
		__aligned(ETH_G2_BUF_ALIGN);                                                       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void eth_g2_irq_config_##n(void)                                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    eth_g2_isr, DEVICE_DT_INST_GET(n), 0);                                 \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
	}                                                                                          \
                                                                                                   \
	static const struct eth_g2_config eth_g2_config_##n = {                                    \
		.regs = (eth_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_ahb = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_ahb, subsystem),           \
		.mclk_apb = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_apb, subsystem),           \
		.gclk_tx = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_tx, subsystem),             \
		.gclk_tsu = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_tsu, subsystem),           \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                          \
		.mcfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                                        \
		.irq_config = eth_g2_irq_config_##n,                                               \
		.conn = ETH_G2_CONN(n),                                                            \
		.prio_queues = DT_INST_PROP(n, num_queues) - 1,                                    \
		.rx_desc = eth_g2_rx_desc_##n,                                                     \
		.tx_desc = eth_g2_tx_desc_##n,                                                     \
		.dummy_desc = eth_g2_dummy_desc_##n,                                               \
		.rx_buf = eth_g2_rx_buf_##n,                                                       \
		.tx_buf = eth_g2_tx_buf_##n,                                                       \
	};                                                                                         \
                                                                                                   \
	static struct eth_g2_data eth_g2_data_##n;                                                 \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_g2_init, NULL, &eth_g2_data_##n, &eth_g2_config_##n,  \
				      CONFIG_ETH_INIT_PRIORITY, &eth_g2_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_G2_DEFINE)
