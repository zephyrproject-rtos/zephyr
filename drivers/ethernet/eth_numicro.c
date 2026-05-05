/*
 * Copyright (c) 2026 Fiona Behrens (me@kloenk.dev)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numicro_ethernet

#include "ethernet/eth_stats.h"
#include "zephyr/drivers/pinctrl.h"
#include "zephyr/net/phy.h"
#include "zephyr/pm/policy.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(eth_numicro, CONFIG_ETHERNET_LOG_LEVEL);

#include <NuMicro.h>
#include "eth_numicro_priv.h"

struct numicro_eth_config {
	EMAC_T *regs;
	uint32_t rmii: 1;
	const struct device *phy_dev;

	struct net_eth_mac_config mac_cfg;
	volatile struct numicro_eth_dma_desc * dma_rx_descs;
	volatile struct numicro_eth_dma_desc *dma_tx_descs;
	volatile uint8_t * dma_rx_buf;
	volatile uint8_t * dma_tx_buf;

	const struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
};

struct numicro_eth_data {
	struct net_if *iface;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];

	volatile struct numicro_eth_dma_desc *dma_rx_desc_current;
	volatile struct numicro_eth_dma_desc *dma_tx_desc_current;
	volatile struct numicro_eth_dma_desc *dma_tx_desc_next;

	struct k_sem rx_int_sem;
	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_NUMICRO_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;

#ifdef CONFIG_NET_STATISTICS_ETHERNET
	struct net_stats_eth stats;
#endif
};

static void numicro_eth_enable_cam_entry(const struct device *dev, uint8_t entry,
					 uint8_t mac_addr[NET_ETH_ADDR_LEN])
{
	const struct numicro_eth_config *config = dev->config;
	uint32_t cam_low, cam_high;
	uint32_t *reg;

	__ASSERT_NO_MSG(entry < 16);

	cam_low = ((uint32_t)mac_addr[4] << 24) | ((uint32_t)mac_addr[5] << 16);
	cam_high = ((uint32_t)mac_addr[0] << 24) | ((uint32_t)mac_addr[1] << 16) |
		   ((uint32_t)mac_addr[2] << 8) | ((uint32_t)mac_addr[3] << 0);

	reg = (uint32_t *)&config->regs->CAM0M + entry * 2;
	*reg = cam_high;
	reg = (uint32_t *)&config->regs->CAM0L + entry * 2;
	*reg = cam_low;

	config->regs->CAMEN |= BIT(entry);
}

static void numicro_eth_disable_cam_entry(const struct device *dev, uint8_t entry)
{
	const struct numicro_eth_config *config = dev->config;

	__ASSERT_NO_MSG(entry < 16);

	config->regs->CAMEN &= ~BIT(entry);
}

static void numicro_eth_set_mac_addr(const struct device *dev, uint8_t mac_addr[NET_ETH_ADDR_LEN],
				     struct net_if *iface)
{
	numicro_eth_enable_cam_entry(dev, 0, mac_addr);
	net_if_set_link_addr(iface, mac_addr, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
}

static const struct device *numicro_eth_get_phy(const struct device *dev, struct net_if *iface __unused)
{
	const struct numicro_eth_config *config = dev->config;

	return config->phy_dev;
}

static void numicro_eth_set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	const struct numicro_eth_config *config = dev->config;
	uint32_t tmpreg = config->regs->CTL;

	if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
		tmpreg |= EMAC_CTL_FUDUP_Msk;
	} else {
		tmpreg &= ~EMAC_CTL_FUDUP_Msk;
	}

	if (PHY_LINK_IS_SPEED_100M(state->speed)) {
		tmpreg |= EMAC_CTL_OPMODE_Msk;
	} else if (PHY_LINK_IS_SPEED_10M(state->speed)) {
		tmpreg &= ~EMAC_CTL_OPMODE_Msk;
	}

	config->regs->CTL = tmpreg;
}

static int numicro_eth_start(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;

	LOG_DBG("Starting numicro EMAC");

	config->regs->CTL |= EMAC_CTL_TXON_Msk | EMAC_CTL_RXON_Msk;

	return 0;
}

static int numicro_eth_stop(const struct device *dev, struct net_if *iface __unused)
{
	const struct numicro_eth_config *config = dev->config;

	LOG_DBG("Stopping numicro EMAC");

	config->regs->CTL &= ~(EMAC_CTL_TXON_Msk | EMAC_CTL_RXON_Msk);

	return 0;
}

static void numicro_eth_phy_link_changed(const struct device *phy_dev, struct phy_link_state *state,
					 void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct numicro_eth_data *data = dev->data;

	ARG_UNUSED(phy_dev);

	numicro_eth_stop(dev, data->iface);
	if (state->is_up) {
		numicro_eth_set_mac_config(dev, state);
		numicro_eth_start(dev);
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}
}

static void numicro_eth_init_dma_descs(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;
	volatile struct numicro_eth_dma_desc *dma_desc;
	int i;

	for (i = 0; i < ETH_RXBUF_NB; i++) {
		dma_desc = &config->dma_rx_descs[i];
		memset((void*)dma_desc, 0, sizeof(struct numicro_eth_dma_desc));
		dma_desc->buffer_addr = (uint32_t)&config->dma_rx_buf[i * ETH_RXBUF_SIZE];
		dma_desc->buffer_addr_back = dma_desc->buffer_addr;
		dma_desc->next_desc =
			(uint32_t)(i < (ETH_RXBUF_NB - 1) ? &config->dma_rx_descs[i + 1]
							  : &config->dma_rx_descs[0]);
		dma_desc->next_desc_back = dma_desc->next_desc;
		dma_desc->status |= BIT(NUMICRO_ETH_DMA_DESC_OWN_Pos);
	}
	data->dma_rx_desc_current = config->dma_rx_descs;
	config->regs->RXDSA = (uint32_t)data->dma_rx_desc_current;

	for (i = 0; i < ETH_TXBUF_NB; i++) {
		dma_desc = &config->dma_tx_descs[i];
		memset((void*)dma_desc, 0, sizeof(struct numicro_eth_dma_desc));
		dma_desc->buffer_addr = (uint32_t)&config->dma_tx_buf[i * ETH_RXBUF_SIZE];
		dma_desc->buffer_addr_back = dma_desc->buffer_addr;
		dma_desc->next_desc =
			(uint32_t)(i < (ETH_TXBUF_NB - 1) ? &config->dma_tx_descs[i + 1]
							  : &config->dma_tx_descs[0]);
		dma_desc->next_desc_back = dma_desc->next_desc;
	}
	data->dma_tx_desc_current = config->dma_tx_descs;
	config->regs->TXDSA = (uint32_t)data->dma_tx_desc_current;
	data->dma_tx_desc_next = config->dma_tx_descs;
}

static int numicro_eth_mac_init(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;

	config->regs->INTEN = EMAC_INTEN_RXIEN_Msk | EMAC_INTEN_TXIEN_Msk | EMAC_INTEN_RXGDIEN_Msk |
			      EMAC_INTEN_TXCPIEN_Msk | EMAC_INTEN_RXBEIEN_Msk |
			      EMAC_INTEN_TXBEIEN_Msk | EMAC_INTEN_RDUIEN_Msk |
			      EMAC_INTEN_TSALMIEN_Msk | EMAC_INTEN_WOLIEN_Msk;

	config->regs->CTL = EMAC_CTL_STRIPCRC_Msk | (config->rmii << EMAC_CTL_RMIIEN_Pos);

	config->regs->CAMCTL = EMAC_CAMCTL_CMPEN_Msk | EMAC_CAMCTL_AMP_Msk | EMAC_CAMCTL_ABP_Msk;

	config->regs->MRFL = NET_ETH_MAX_FRAME_SIZE;

	numicro_eth_init_dma_descs(dev);
	return 0;
}

static void numicro_eth_rx_thread(void *arg1, void *unused1, void *unused2);

static void numicro_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;

	if (data->iface == NULL) {
		data->iface = iface;

		k_thread_create(&data->rx_thread, data->rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), numicro_eth_rx_thread,
				(void *)dev, NULL, NULL,
				K_PRIO_COOP(CONFIG_ETH_NUMICRO_RX_THREAD_PRIO), 0, K_NO_WAIT);

		k_thread_name_set(&data->rx_thread, dev->name);
	}

	numicro_eth_set_mac_addr(dev, data->mac_addr, iface);
	numicro_eth_mac_init(dev);
	ethernet_init(iface);

	net_if_carrier_off(iface);
	net_lldp_set_lldpdu(iface);

	if (device_is_ready(config->phy_dev)) {
		phy_link_callback_set(config->phy_dev, numicro_eth_phy_link_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}
}

static enum ethernet_hw_caps numicro_eth_get_capabilities(const struct device *dev, struct net_if *iface __unused)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
	/* TODO: other caps? */
}

static int numicro_eth_set_config(const struct device *dev, struct net_if *iface __unused, enum ethernet_config_type type,
				  const struct ethernet_config *cfg)
{
	struct numicro_eth_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, cfg->mac_address.addr, NET_ETH_ADDR_LEN);
		numicro_eth_set_mac_addr(dev, data->mac_addr, data->iface);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static int numicro_eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);

	/* Get Full length of packet */
	size_t total_len = net_pkt_get_len(pkt);

	LOG_DBG("Sending Packet: %p of length: %u", pkt, total_len);
	if (total_len > ETH_TXBUF_SIZE) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("Packet bigger then MTU allows");
		return -ENOBUFS;
	}

	if (!NUMICRO_ETH_DMA_DESC_IS_CPU_OWNED(data->dma_tx_desc_next)) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("No Descriptors Available");
		return -EBUSY;
	}

	/* Copy packet to TX Buf */
	if (net_pkt_read(pkt, (void *)(data->dma_tx_desc_next->buffer_addr), total_len) != 0) {
		eth_stats_update_errors_tx(data->iface);
		return -ENOBUFS;
	}

	/* Set descriptor bits and hand to DMA engine */
	data->dma_tx_desc_next->tx_status = total_len & 0xFFFF;
	data->dma_tx_desc_next->status |=
		BIT(NUMICRO_ETH_DMA_DESC_OWN_Pos) | BIT(NUMICRO_ETH_DMA_DESC_TX_INTEN_Pos) |
		BIT(NUMICRO_ETH_DMA_DESC_TX_CRCAPP_Pos) | BIT(NUMICRO_ETH_DMA_DESC_TX_PADEN_Pos);

	data->dma_tx_desc_next = (struct numicro_eth_dma_desc *)data->dma_tx_desc_next->next_desc;

	config->regs->TXST = 0;

	return 0;
}

#ifdef CONFIG_NET_STATISTICS_ETHERNET
static struct net_stats_eth *numicro_eth_get_stats(const struct device *dev)
{
	struct numicro_eth_data *data = dev->data;

	return &data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static const struct ethernet_api numicro_eth_api = {
	.iface_api.init = numicro_eth_iface_init,
	.stop = numicro_eth_stop,
	.get_capabilities = numicro_eth_get_capabilities,
	.set_config = numicro_eth_set_config,
	.get_phy = numicro_eth_get_phy,
	.send = numicro_eth_tx,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.get_stats = numicro_eth_get_stats,
#endif
};

static struct net_pkt *numicro_eth_rx(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;

	uint32_t status;
	struct net_pkt *pkt = NULL;
	volatile struct numicro_eth_dma_desc *desc = data->dma_rx_desc_current;

	if (!NUMICRO_ETH_DMA_DESC_IS_CPU_OWNED(desc)) {
		return NULL;
	}
	status = desc->status >> 16;

	if (!(status & EMAC_RXFD_RXGD)) {
		eth_stats_update_errors_rx(data->iface);
		goto release_desc;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, desc->status & 0xFFFF, AF_UNSPEC, 0,
					   K_MSEC(100));
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain RX Buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, (void *)(desc->buffer_addr_back), desc->status & 0xFFFF) != 0) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto release_desc;
	}

	LOG_DBG("Receiving Packet: %p", pkt);

release_desc:
	desc->next_desc = desc->next_desc_back;
	desc->buffer_addr = desc->buffer_addr_back;

	desc->status |= BIT(NUMICRO_ETH_DMA_DESC_OWN_Pos);

	data->dma_rx_desc_current = (struct numicro_eth_dma_desc *)desc->next_desc;

	config->regs->RXST = 0;

	return pkt;
}

static void numicro_eth_rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct numicro_eth_data *data = dev->data;
	struct net_pkt *pkt;
	int ret;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		ret = k_sem_take(&data->rx_int_sem, K_FOREVER);
		if (ret == 0) {
			while ((pkt = numicro_eth_rx(dev)) != NULL) {
				ret = net_recv_data(data->iface, pkt);
				if (ret < 0) {
					eth_stats_update_errors_rx(data->iface);
					LOG_ERR("Failed to enqueue frame into RX queue: %d", ret);
					net_pkt_unref(pkt);
				}
			}
		}
	}
}

static void numicro_eth_rx_isr(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;

	uint32_t reg = config->regs->INTSTS;
	/* Clear all RX related interrupt statuses */
	config->regs->INTSTS = reg & 0xFFFF;

	if (reg & EMAC_INTSTS_RXBEIF_Msk) {
		LOG_ERR("Bus Error");
	}

	k_sem_give(&data->rx_int_sem);
}

static void numicro_eth_tx_isr(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;
	volatile struct numicro_eth_dma_desc *desc = data->dma_tx_desc_current;

	uint32_t reg = config->regs->INTSTS;
	uint32_t status;
	config->regs->INTSTS = reg & ((0xFFFF << 16) & ~EMAC_INTSTS_TSALMIF_Msk);

	if (reg & EMAC_INTSTS_TXBEIF_Msk) {
		LOG_ERR("Bus error");
		eth_stats_update_errors_tx(data->iface);
		return;
	}

	do {
		if (!NUMICRO_ETH_DMA_DESC_IS_CPU_OWNED(desc)) {
			break;
		}

		status = desc->tx_status >> 16;

		if (status & EMAC_TXFD_TXCP) {
			/* success? */
		}

		/* TODO test errors */

		desc->next_desc = desc->next_desc_back;
		desc->buffer_addr = desc->buffer_addr_back;
		/* go to next desc in link */
		desc = (struct numicro_eth_dma_desc *)desc->next_desc;
	} while (config->regs->CTXDSA != (uint32_t)desc);

	data->dma_tx_desc_current = desc;
}

static int numicro_eth_init(const struct device *dev)
{
	const struct numicro_eth_config *config = dev->config;
	struct numicro_eth_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

	reset_line_toggle_dt(&config->reset);
	/* Also toggle EMAC specific RST line */
	config->regs->CTL |= EMAC_CTL_RST_Msk;
	while ((config->regs->CTL & EMAC_CTL_RST_Msk) != 0) {
	}

	/* Configure MAC Address */
	ret = net_eth_mac_load(&config->mac_cfg, data->mac_addr);
	if (ret == -ENODATA) {
		// TODO: get hw mac address
		ret = 0;
		data->mac_addr[0] = 0x78;
		data->mac_addr[1] = 0x2c;
		data->mac_addr[2] = 0xcc;
		data->mac_addr[3] = 0x39;
		data->mac_addr[4] = 0xee;
		data->mac_addr[5] = 0xe5;
	}
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x", data->mac_addr[0], data->mac_addr[1],
		data->mac_addr[2], data->mac_addr[3], data->mac_addr[4], data->mac_addr[5]);

	k_sem_init(&data->rx_int_sem, 0, K_SEM_MAX_LIMIT);

	config->irq_config_func(dev);

	return 0;
}

#define NUMICRO_ETH_DEFINE_IRQ_HANDLER(inst)                                                       \
	static void numicro_eth_irq_config_func_##inst(const struct device *dev)                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, emac_tx, irq),                               \
			    DT_INST_IRQ_BY_NAME(inst, emac_tx, priority), numicro_eth_tx_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, emac_tx, irq));                               \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, emac_rx, irq),                               \
			    DT_INST_IRQ_BY_NAME(inst, emac_rx, priority), numicro_eth_rx_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, emac_rx, irq));                               \
	}

#define NUMICRO_ETH_DEVICE(inst)                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	NUMICRO_ETH_DEFINE_IRQ_HANDLER(inst);                                                      \
                                                                                                   \
	static volatile struct numicro_eth_dma_desc                                                \
		__aligned(4) numicro_eth_dma_rx_desc_##inst[ETH_RXBUF_NB];                         \
	static volatile struct numicro_eth_dma_desc                                                \
		__aligned(4) numicro_eth_dma_tx_desc_##inst[ETH_TXBUF_NB];                         \
	static volatile uint8_t                                                                    \
		__aligned(4) numicro_eth_dma_rx_buf_##inst[ETH_RXBUF_NB * ETH_RXBUF_SIZE];         \
	static volatile uint8_t                                                                    \
		__aligned(4) numicro_eth_dma_tx_buf_##inst[ETH_TXBUF_NB * ETH_TXBUF_SIZE];         \
                                                                                                   \
	const static struct numicro_eth_config numicro_eth_config_##inst = {                       \
		.regs = (EMAC_T *)DT_REG_ADDR(DT_INST_PARENT(inst)),                               \
		.rmii = DT_ENUM_IDX_OR(inst, phy_connection_type, 0),                              \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phy_handle)),                       \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                  \
		.dma_rx_descs = numicro_eth_dma_rx_desc_##inst,                                    \
		.dma_tx_descs = numicro_eth_dma_tx_desc_##inst,                                    \
		.dma_tx_buf = numicro_eth_dma_tx_buf_##inst,                                       \
		.dma_rx_buf = numicro_eth_dma_rx_buf_##inst,                                       \
		.reset = RESET_DT_SPEC_GET(DT_INST_PARENT(inst)),                                  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.irq_config_func = numicro_eth_irq_config_func_##inst,                             \
	};                                                                                         \
                                                                                                   \
	static struct numicro_eth_data numicro_eth_data_##inst = {};                               \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, &numicro_eth_init, NULL, &numicro_eth_data_##inst,     \
				      &numicro_eth_config_##inst, CONFIG_ETH_INIT_PRIORITY,        \
				      &numicro_eth_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_ETH_DEVICE);
