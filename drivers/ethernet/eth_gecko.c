/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_ethernet

/* Silicon Labs EFM32 Giant Gecko 11 Ethernet driver.
 * Limitations:
 * - no link monitoring through PHY interrupt
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(eth_gecko, CONFIG_ETHERNET_LOG_LEVEL);

#include <soc.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <em_cmu.h>

#include "phy_gecko.h"
#include "eth_gecko_priv.h"

#include "eth.h"

static uint8_t dma_tx_buffer[ETH_TX_BUF_COUNT][ETH_TX_BUF_SIZE]
__aligned(ETH_BUF_ALIGNMENT);
static uint8_t dma_rx_buffer[ETH_RX_BUF_COUNT][ETH_RX_BUF_SIZE]
__aligned(ETH_BUF_ALIGNMENT);
static struct eth_buf_desc dma_tx_desc_tab[ETH_TX_BUF_COUNT]
__aligned(ETH_DESC_ALIGNMENT);
static struct eth_buf_desc dma_rx_desc_tab[ETH_RX_BUF_COUNT]
__aligned(ETH_DESC_ALIGNMENT);
static uint32_t tx_buf_idx;
static uint32_t rx_buf_idx;


static void link_configure(ETH_TypeDef *eth, uint32_t flags)
{
	uint32_t val;

	__ASSERT_NO_MSG(eth != NULL);

	/* Disable receiver & transmitter */
	eth->NETWORKCTRL &= ~(ETH_NETWORKCTRL_ENBTX | ETH_NETWORKCTRL_ENBRX);

	/* Set duplex mode and speed */
	val = eth->NETWORKCFG;
	val &= ~(_ETH_NETWORKCFG_FULLDUPLEX_MASK | _ETH_NETWORKCFG_SPEED_MASK);
	val |= flags &
	       (_ETH_NETWORKCFG_FULLDUPLEX_MASK | _ETH_NETWORKCFG_SPEED_MASK);
	eth->NETWORKCFG = val;

	/* Enable transmitter and receiver */
	eth->NETWORKCTRL |= (ETH_NETWORKCTRL_ENBTX | ETH_NETWORKCTRL_ENBRX);
}

static void eth_gecko_setup_mac(const struct device *dev)
{
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;
	uint32_t link_status;
	int result;

	/* PHY auto-negotiate link parameters */
	result = phy_gecko_auto_negotiate(&cfg->phy, &link_status);
	if (result < 0) {
		LOG_ERR("ETH PHY auto-negotiate sequence failed");
		return;
	}

	LOG_INF("Speed %s Mb",
		link_status & ETH_NETWORKCFG_SPEED ? "100" : "10");
	LOG_INF("%s duplex",
		link_status & ETH_NETWORKCFG_FULLDUPLEX ? "Full" : "Half");

	/* Set up link parameters and enable receiver/transmitter */
	link_configure(eth, link_status);
}

static void eth_init_tx_buf_desc(void)
{
	uint32_t address;
	int i;

	/* Initialize TX buffer descriptors */
	for (i = 0; i < ETH_TX_BUF_COUNT; i++) {
		address = (uint32_t) dma_tx_buffer[i];
		dma_tx_desc_tab[i].address = address;
		dma_tx_desc_tab[i].status = ETH_TX_USED;
	}

	/* Mark last descriptor entry with wrap flag */
	dma_tx_desc_tab[i - 1].status |= ETH_TX_WRAP;
	tx_buf_idx = 0;
}

static void eth_init_rx_buf_desc(void)
{
	uint32_t address;
	int i;

	for (i = 0; i < ETH_RX_BUF_COUNT; i++) {
		address = (uint32_t) dma_rx_buffer[i];
		dma_rx_desc_tab[i].address = address & ETH_RX_ADDRESS;
		dma_rx_desc_tab[i].status = 0;
	}

	/* Mark last descriptor entry with wrap flag */
	dma_rx_desc_tab[i - 1].address |= ETH_RX_WRAP;
	rx_buf_idx = 0;
}

static void rx_error_handler(ETH_TypeDef *eth)
{
	__ASSERT_NO_MSG(eth != NULL);

	/* Stop reception */
	ETH_RX_DISABLE(eth);

	/* Reset RX buffer descriptor list */
	eth_init_rx_buf_desc();
	eth->RXQPTR = (uint32_t)dma_rx_desc_tab;

	/* Restart reception */
	ETH_RX_ENABLE(eth);
}

static struct net_pkt *frame_get(const struct device *dev)
{
	struct eth_gecko_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;
	struct net_pkt *rx_frame = NULL;
	uint16_t frag_len, total_len;
	uint32_t sofIdx, eofIdx;
	uint32_t i, j;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	/* Preset indeces and total frame length */
	sofIdx = UINT32_MAX;
	eofIdx = UINT32_MAX;
	total_len = 0;

	/* Check if a full frame is received (SOF/EOF present)
	 *  and determine total length of frame
	 */
	for (i = 0; i < ETH_RX_BUF_COUNT; i++) {
		j = (i + rx_buf_idx);
		if (j >= ETH_RX_BUF_COUNT) {
			j -= ETH_RX_BUF_COUNT;
		}

		/* Verify it is an ETH owned buffer */
		if (!(dma_rx_desc_tab[j].address & ETH_RX_OWNERSHIP)) {
			/* No more ETH owned buffers to process */
			break;
		}

		/* Check for SOF */
		if (dma_rx_desc_tab[j].status & ETH_RX_SOF) {
			sofIdx = j;
		}

		if (sofIdx != UINT32_MAX) {
			total_len += (dma_rx_desc_tab[j].status &
				ETH_RX_LENGTH);

			/* Check for EOF */
			if (dma_rx_desc_tab[j].status & ETH_RX_EOF) {
				eofIdx = j;
				break;
			}
		}
	}

	LOG_DBG("sof/eof: %u/%u, rx_buf_idx: %u, len: %u", sofIdx, eofIdx,
		rx_buf_idx, total_len);

	/* Verify we found a full frame */
	if (eofIdx != UINT32_MAX) {
		/* Allocate room for full frame */
		rx_frame = net_pkt_rx_alloc_with_buffer(dev_data->iface,
					total_len, AF_UNSPEC, 0, K_NO_WAIT);
		if (!rx_frame) {
			LOG_ERR("Failed to obtain RX buffer");
			ETH_RX_DISABLE(eth);
			eth_init_rx_buf_desc();
			eth->RXQPTR = (uint32_t)dma_rx_desc_tab;
			ETH_RX_ENABLE(eth);
			return rx_frame;
		}

		/* Copy frame (fragments)*/
		j = sofIdx;
		while (total_len) {
			frag_len = MIN(total_len, ETH_RX_BUF_SIZE);
			LOG_DBG("frag: %u, fraglen: %u, rx_buf_idx: %u", j,
				frag_len, rx_buf_idx);
			if (net_pkt_write(rx_frame, &dma_rx_buffer[j],
					  frag_len) < 0) {
				LOG_ERR("Failed to append RX buffer");
				dma_rx_desc_tab[j].address &=
					~ETH_RX_OWNERSHIP;
				net_pkt_unref(rx_frame);
				rx_frame = NULL;
				break;
			}

			dma_rx_desc_tab[j].address &= ~ETH_RX_OWNERSHIP;

			total_len -= frag_len;
			if (++j >= ETH_RX_BUF_COUNT) {
				j -= ETH_RX_BUF_COUNT;
			}

			if (++rx_buf_idx >= ETH_RX_BUF_COUNT) {
				rx_buf_idx -= ETH_RX_BUF_COUNT;
			}
		}
	}

	return rx_frame;
}

static void eth_rx(const struct device *dev)
{
	struct eth_gecko_dev_data *const dev_data = DEV_DATA(dev);
	struct net_pkt *rx_frame;
	int res = 0;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);

	/* Iterate across (possibly multiple) frames */
	rx_frame = frame_get(dev);
	while (rx_frame) {
		/* All data for this frame received */
		res = net_recv_data(dev_data->iface, rx_frame);
		if (res < 0) {
			LOG_ERR("Failed to enqueue frame into RX queue: %d",
				res);
			eth_stats_update_errors_rx(dev_data->iface);
			net_pkt_unref(rx_frame);
		}

		/* Check if more frames are received */
		rx_frame = frame_get(dev);
	}
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_gecko_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;
	uint16_t total_len;
	uint8_t *dma_buffer;
	int res = 0;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	__ASSERT(pkt, "Buf pointer is NULL");
	__ASSERT(pkt->frags, "Frame data missing");

	/* Determine length of frame */
	total_len = net_pkt_get_len(pkt);
	if (total_len > ETH_TX_BUF_SIZE) {
		LOG_ERR("PKT to big");
		res = -EIO;
		goto error;
	}

	if (k_sem_take(&dev_data->tx_sem, K_MSEC(100)) != 0) {
		LOG_ERR("TX process did not complete within 100ms");
		res = -EIO;
		goto error;
	}

	/* Make sure current buffer is available for writing */
	if (!(dma_tx_desc_tab[tx_buf_idx].status & ETH_TX_USED)) {
		LOG_ERR("Buffer already in use");
		res = -EIO;
		goto error;
	}

	dma_buffer = (uint8_t *)dma_tx_desc_tab[tx_buf_idx].address;
	if (net_pkt_read(pkt, dma_buffer, total_len)) {
		LOG_ERR("Failed to read packet into buffer");
		res = -EIO;
		goto error;
	}

	if (tx_buf_idx < (ETH_TX_BUF_COUNT - 1)) {
		dma_tx_desc_tab[tx_buf_idx].status =
			(total_len & ETH_TX_LENGTH) | ETH_TX_LAST;
		tx_buf_idx++;
	} else {
		dma_tx_desc_tab[tx_buf_idx].status =
			(total_len & ETH_TX_LENGTH) | (ETH_TX_LAST |
				ETH_TX_WRAP);
		tx_buf_idx = 0;
	}

	/* Kick off transmission */
	eth->NETWORKCTRL |= ETH_NETWORKCTRL_TXSTRT;

error:
	return res;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct eth_gecko_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	int res;

	__ASSERT_NO_MSG(arg1 != NULL);
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	while (1) {
		res = k_sem_take(&dev_data->rx_sem, K_MSEC(
			 CONFIG_ETH_GECKO_CARRIER_CHECK_RX_IDLE_TIMEOUT_MS));
		if (res == 0) {
			if (dev_data->link_up != true) {
				dev_data->link_up = true;
				LOG_INF("Link up");
				eth_gecko_setup_mac(dev);
				net_eth_carrier_on(dev_data->iface);
			}

			/* Process received data */
			eth_rx(dev);
		} else if (res == -EAGAIN) {
			if (phy_gecko_is_linked(&cfg->phy)) {
				if (dev_data->link_up != true) {
					dev_data->link_up = true;
					LOG_INF("Link up");
					eth_gecko_setup_mac(dev);
					net_eth_carrier_on(dev_data->iface);
				}
			} else   {
				if (dev_data->link_up != false) {
					dev_data->link_up = false;
					LOG_INF("Link down");
					net_eth_carrier_off(dev_data->iface);
				}
			}
		}
	}
}

static void eth_isr(const struct device *dev)
{
	struct eth_gecko_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;
	uint32_t int_clr = 0;
	uint32_t int_stat = eth->IFCR;
	uint32_t tx_irq_mask = (ETH_IENS_TXCMPLT | ETH_IENS_TXUNDERRUN |
				ETH_IENS_RTRYLMTORLATECOL |
				ETH_IENS_TXUSEDBITREAD |
				ETH_IENS_AMBAERR);
	uint32_t rx_irq_mask = (ETH_IENS_RXCMPLT | ETH_IENS_RXUSEDBITREAD);

	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	/* Receive handling */
	if (int_stat & rx_irq_mask) {
		if (int_stat & ETH_IENS_RXCMPLT) {
			/* Receive complete */
			k_sem_give(&dev_data->rx_sem);
		} else   {
			/* Receive error */
			LOG_DBG("RX Error");
			rx_error_handler(eth);
		}

		int_clr |= rx_irq_mask;
	}

	/* Transmit handling */
	if (int_stat & tx_irq_mask) {
		if (int_stat & ETH_IENS_TXCMPLT) {
			/* Transmit complete */
		} else   {
			/* Transmit error: no actual handling, the current
			 * buffer is no longer used and we release the
			 * semaphore which signals the user thread to
			 * start TX of a new packet
			 */
		}

		int_clr |= tx_irq_mask;

		/* Signal TX thread we're ready to start transmission */
		k_sem_give(&dev_data->tx_sem);
	}

	/* Clear interrupts */
	eth->IFCR = int_clr;
}

static void eth_init_clocks(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_ETH, true);
}

static void eth_init_pins(const struct device *dev)
{
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;
	uint32_t idx;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	eth->ROUTELOC1 = 0;
	eth->ROUTEPEN = 0;

#if DT_INST_NODE_HAS_PROP(0, location_rmii)
	for (idx = 0; idx < ARRAY_SIZE(cfg->pin_list->rmii); idx++)
		soc_gpio_configure(&cfg->pin_list->rmii[idx]);

	eth->ROUTELOC1 |= (DT_INST_PROP(0, location_rmii) <<
			   _ETH_ROUTELOC1_RMIILOC_SHIFT);
	eth->ROUTEPEN |= ETH_ROUTEPEN_RMIIPEN;
#endif

#if DT_INST_NODE_HAS_PROP(0, location_mdio)
	for (idx = 0; idx < ARRAY_SIZE(cfg->pin_list->mdio); idx++)
		soc_gpio_configure(&cfg->pin_list->mdio[idx]);

	eth->ROUTELOC1 |= (DT_INST_PROP(0, location_mdio) <<
			   _ETH_ROUTELOC1_MDIOLOC_SHIFT);
	eth->ROUTEPEN |= ETH_ROUTEPEN_MDIOPEN;
#endif

}

static int eth_init(const struct device *dev)
{
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	/* Enable clocks */
	eth_init_clocks(dev);

	/* Connect pins to peripheral */
	eth_init_pins(dev);

#if DT_INST_NODE_HAS_PROP(0, location_rmii)
	/* Enable global clock and RMII operation */
	eth->CTRL = ETH_CTRL_GBLCLKEN | ETH_CTRL_MIISEL_RMII;
#endif

	/* Connect and enable IRQ */
	cfg->config_func();

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static void generate_mac(uint8_t mac_addr[6])
{
#if DT_INST_PROP(0, zephyr_random_mac_address)
	gen_random_mac(mac_addr, SILABS_OUI_B0, SILABS_OUI_B1, SILABS_OUI_B2);
#endif
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_gecko_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_gecko_dev_cfg *const cfg = DEV_CFG(dev);
	ETH_TypeDef *eth = cfg->regs;
	int result;

	__ASSERT_NO_MSG(iface != NULL);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	LOG_DBG("eth_initialize");

	dev_data->iface = iface;
	dev_data->link_up = false;
	ethernet_init(iface);

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	/* Generate MAC address, possibly used for filtering */
	generate_mac(dev_data->mac_addr);

	/* Set link address */
	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr), NET_LINK_ETHERNET);

	/* Disable transmit and receive circuits */
	eth->NETWORKCTRL = 0;
	eth->NETWORKCFG = 0;

	/* Filtering MAC addresses */
	eth->SPECADDR1BOTTOM =
		(dev_data->mac_addr[0] << 0)  |
		(dev_data->mac_addr[1] << 8)  |
		(dev_data->mac_addr[2] << 16) |
		(dev_data->mac_addr[3] << 24);
	eth->SPECADDR1TOP =
		(dev_data->mac_addr[4] << 0)  |
		(dev_data->mac_addr[5] << 8);

	eth->SPECADDR2BOTTOM = 0;
	eth->SPECADDR3BOTTOM = 0;
	eth->SPECADDR4BOTTOM = 0;

	/* Initialise hash table */
	eth->HASHBOTTOM = 0;
	eth->HASHTOP = 0;

	/* Initialise DMA buffers */
	eth_init_tx_buf_desc();
	eth_init_rx_buf_desc();

	/* Point to locations of TX/RX DMA descriptor lists */
	eth->TXQPTR = (uint32_t)dma_tx_desc_tab;
	eth->RXQPTR = (uint32_t)dma_rx_desc_tab;

	/* DMA RX size configuration */
	eth->DMACFG = (eth->DMACFG & ~_ETH_DMACFG_RXBUFSIZE_MASK) |
		      ((ETH_RX_BUF_SIZE / 64) << _ETH_DMACFG_RXBUFSIZE_SHIFT);

	/* Clear status/interrupt registers */
	eth->IFCR |= _ETH_IFCR_MASK;
	eth->TXSTATUS = ETH_TXSTATUS_TXUNDERRUN | ETH_TXSTATUS_TXCMPLT |
			ETH_TXSTATUS_AMBAERR | ETH_TXSTATUS_TXGO |
			ETH_TXSTATUS_RETRYLMTEXCD | ETH_TXSTATUS_COLOCCRD |
			ETH_TXSTATUS_USEDBITREAD;
	eth->RXSTATUS = ETH_RXSTATUS_RESPNOTOK | ETH_RXSTATUS_RXOVERRUN |
			ETH_RXSTATUS_FRMRX | ETH_RXSTATUS_BUFFNOTAVAIL;

	/* Enable interrupts */
	eth->IENS = ETH_IENS_RXCMPLT |
		    ETH_IENS_RXUSEDBITREAD |
		    ETH_IENS_TXCMPLT |
		    ETH_IENS_TXUNDERRUN |
		    ETH_IENS_RTRYLMTORLATECOL |
		    ETH_IENS_TXUSEDBITREAD |
		    ETH_IENS_AMBAERR;

	/* Additional DMA configuration */
	eth->DMACFG |= _ETH_DMACFG_AMBABRSTLEN_MASK |
		       ETH_DMACFG_FRCDISCARDONERR |
		       ETH_DMACFG_TXPBUFTCPEN;
	eth->DMACFG &= ~ETH_DMACFG_HDRDATASPLITEN;

	/* Set network configuration */
	eth->NETWORKCFG |= ETH_NETWORKCFG_FCSREMOVE |
			   ETH_NETWORKCFG_UNICASTHASHEN |
			   ETH_NETWORKCFG_MULTICASTHASHEN |
			   ETH_NETWORKCFG_RX1536BYTEFRAMES |
			   ETH_NETWORKCFG_RXCHKSUMOFFLOADEN;

	/* Setup PHY management port */
	eth->NETWORKCFG |= (4 << _ETH_NETWORKCFG_MDCCLKDIV_SHIFT) &
			   _ETH_NETWORKCFG_MDCCLKDIV_MASK;
	eth->NETWORKCTRL |= ETH_NETWORKCTRL_MANPORTEN;

	/* Initialise PHY */
	result = phy_gecko_init(&cfg->phy);
	if (result < 0) {
		LOG_ERR("ETH PHY Initialization Error");
		return;
	}

	/* Initialise TX/RX semaphores */
	k_sem_init(&dev_data->tx_sem, 1, ETH_TX_BUF_COUNT);
	k_sem_init(&dev_data->rx_sem, 0, UINT_MAX);

	/* Start interruption-poll thread */
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack),
			rx_thread, (void *) dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_GECKO_RX_THREAD_PRIO),
			0, K_NO_WAIT);
}

static enum ethernet_hw_caps eth_gecko_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ETHERNET_AUTO_NEGOTIATION_SET | ETHERNET_LINK_10BASE_T |
			ETHERNET_LINK_100BASE_T | ETHERNET_DUPLEX_SET);
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_gecko_get_capabilities,
	.send = eth_tx,
};

DEVICE_DECLARE(eth_gecko);

static void eth0_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), eth_isr,
		    DEVICE_GET(eth_gecko), 0);
	irq_enable(DT_INST_IRQN(0));
}

static const struct eth_gecko_pin_list pins_eth0 = {
	.mdio = PIN_LIST_PHY,
	.rmii = PIN_LIST_RMII
};

static const struct eth_gecko_dev_cfg eth0_config = {
	.regs = (ETH_TypeDef *)
		DT_INST_REG_ADDR(0),
	.pin_list = &pins_eth0,
	.pin_list_size = ARRAY_SIZE(pins_eth0.mdio) +
			 ARRAY_SIZE(pins_eth0.rmii),
	.config_func = eth0_irq_config,
	.phy = { (ETH_TypeDef *)
		 DT_INST_REG_ADDR(0),
		 DT_INST_PROP(0, phy_address) },
};

static struct eth_gecko_dev_data eth0_data = {
#if NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	.mac_addr = DT_INST_PROP(0, local_mac_address),
#endif
};

ETH_NET_DEVICE_INIT(eth_gecko, CONFIG_ETH_GECKO_NAME, eth_init,
		    device_pm_control_nop, &eth0_data, &eth0_config,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_GECKO_MTU);
