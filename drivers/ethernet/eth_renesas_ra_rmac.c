/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <zephyr/drivers/ethernet/eth_renesas_ra_rmac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "r_rmac.h"
#include "r_layer3_switch.h"
#include "eth.h"

LOG_MODULE_REGISTER(eth_renesas_ra, CONFIG_ETHERNET_LOG_LEVEL);

/* At this time , HAL only support single descriptor, set fixed buffer length */
#define ETH_BUF_SIZE (1536)

#if defined(CONFIG_NOCACHE_MEMORY)
#define __eth_renesas_desc __nocache __aligned(32)
#define __eth_renesas_buf  __nocache __aligned(32)
#else /* CONFIG_NOCACHE_MEMORY */
#define __eth_renesas_desc
#define __eth_renesas_buf
#endif /* CONFIG_NOCACHE_MEMORY */

#define ETHPHYCLK_25MHZ MHZ(25)
#define ETHPHYCLK_50MHZ MHZ(50)

enum phy_clock_type {
	ETH_PHY_REF_CLK_XTAL,
	ETH_PHY_REF_CLK_INTERNAL
};

struct eswm_renesas_ra_data {
	layer3_switch_instance_ctrl_t *fsp_ctrl;
	ether_switch_cfg_t *fsp_cfg;
};

struct eswm_renesas_ra_config {
	const struct device *gwcaclk_dev;
	const struct device *pclk_dev;
	const struct device *eswclk_dev;
	const struct device *eswphyclk_dev;
	const struct device *ethphyclk_dev;
	struct clock_control_ra_subsys_cfg pclk_subsys;
	struct clock_control_ra_subsys_cfg ethphyclk_subsys;
	const uint8_t ethphyclk_enable;
	const struct pinctrl_dev_config *pin_cfg;
	void (*en_irq)(void);
};

#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
struct eth_renesas_ra_buf_header {
	uint8_t *buf;
};
#endif

struct eth_renesas_ra_data {
	struct net_if *iface;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];

#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
	/* DMA buffer header */
	struct eth_renesas_ra_buf_header *tx_buf_header;
	uint8_t tx_buf_idx;
	uint8_t tx_buf_num;
#else
	/* staging buffer */
	uint8_t *rx_frame;
	uint8_t *tx_frame;
#endif
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_RENESAS_RA_RX_THREAD_STACK_SIZE);
	struct k_sem rx_sem;
#if defined(CONFIG_ETH_RENESAS_RA_USE_HW_WRITEBACK)
	struct k_sem tx_sem;
#endif
	uint8_t phy_link_up;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
	rmac_instance_ctrl_t *fsp_ctrl;
	ether_cfg_t *fsp_cfg;
	ether_callback_args_t fsp_cb;
};

struct eth_renesas_ra_config {
	uint8_t random_mac; /* 0 if not using random mac */
	uint8_t valid_mac;  /* 1 if mac is valid */
	ether_phy_mii_type_t mii_type;
	const struct pinctrl_dev_config *pin_cfg;
	const struct device *phy_dev;
	const struct device *phy_clock;
	enum phy_clock_type phy_clock_type;
};

extern void layer3_switch_gwdi_isr(void);
extern void rmac_init_buffers(rmac_instance_ctrl_t *const p_instance_ctrl);
extern void rmac_init_descriptors(rmac_instance_ctrl_t *const p_instance_ctrl);
extern void rmac_configure_reception_filter(rmac_instance_ctrl_t const *const p_instance_ctrl);
extern void r_rmac_disable_reception(rmac_instance_ctrl_t *p_instance_ctrl);
extern fsp_err_t rmac_do_link(rmac_instance_ctrl_t *const p_instance_ctrl,
			      const layer3_switch_magic_packet_detection_t mode);

static void phy_link_cb(const struct device *phy_dev, struct phy_link_state *state, void *eth_dev)
{
	const struct device *dev = (struct device *)eth_dev;
	struct eth_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;

	if (state->is_up) {
		data->fsp_ctrl->link_establish_status = ETHER_LINK_ESTABLISH_STATUS_UP;
		data->phy_link_up = 1;

		/* Chage ETHA to config mode */
		r_rmac_phy_set_operation_mode(data->fsp_cfg->channel, RENESAS_RA_ETHA_DISABLE_MODE);
		r_rmac_phy_set_operation_mode(data->fsp_cfg->channel, RENESAS_RA_ETHA_CONFIG_MODE);

		switch (state->speed) {
		case LINK_HALF_10BASE:
			__fallthrough;
		case LINK_FULL_10BASE:
			data->fsp_ctrl->p_reg_rmac->MPIC_b.LSC = RENESAS_RA_MPIC_LSC_10;
			break;
		case LINK_HALF_100BASE:
			__fallthrough;
		case LINK_FULL_100BASE:
			data->fsp_ctrl->p_reg_rmac->MPIC_b.LSC = RENESAS_RA_MPIC_LSC_100;
			break;
		case LINK_HALF_1000BASE:
			__fallthrough;
		case LINK_FULL_1000BASE:
			data->fsp_ctrl->p_reg_rmac->MPIC_b.LSC = RENESAS_RA_MPIC_LSC_1000;
			break;
		default:
			LOG_DBG("phy link speed is not supported");
			break;
		}

		/* Chage ETHA to operate mode */
		r_rmac_phy_set_operation_mode(data->fsp_cfg->channel, RENESAS_RA_ETHA_DISABLE_MODE);
		r_rmac_phy_set_operation_mode(data->fsp_cfg->channel,
					      RENESAS_RA_ETHA_OPERATION_MODE);

		rmac_init_buffers(data->fsp_ctrl);
		rmac_init_descriptors(data->fsp_ctrl);
		rmac_configure_reception_filter(data->fsp_ctrl);

		fsp_err =
			rmac_do_link(data->fsp_ctrl, LAYER3_SWITCH_MAGIC_PACKET_DETECTION_DISABLE);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("link MAC failed, err=%d", fsp_err);
			return;
		}

		LOG_DBG("link up");

		net_eth_carrier_on(data->iface);
	} else {
		if (data->phy_link_up == 1) {
			/* phy state change from up to down */
			r_rmac_disable_reception(data->fsp_ctrl);

			data->fsp_ctrl->link_establish_status = ETHER_LINK_ESTABLISH_STATUS_DOWN;
			data->phy_link_up = 0;

			LOG_DBG("link down");
			net_eth_carrier_off(data->iface);
		}
	}
}

static void eth_rmac_cb(ether_callback_args_t *args)
{
	struct device *eth_dev = (struct device *)args->p_context;
	struct eth_renesas_ra_data *data = eth_dev->data;

	switch (args->event) {
#if defined(CONFIG_ETH_RENESAS_RA_USE_HW_WRITEBACK)
	case ETHER_EVENT_TX_COMPLETE:
		/* tx frame writen */
		k_sem_give(&data->tx_sem);
		break;
#endif
	case ETHER_EVENT_RX_MESSAGE_LOST:
		/* rx queue is full to append new frame */
		__fallthrough;
	case ETHER_EVENT_RX_COMPLETE:
		/* new rx frame is ready to read */
		k_sem_give(&data->rx_sem);
		break;
	default:
		break;
	}
}

static void eth_switch_cb(ether_switch_callback_args_t *args)
{
	/* Do nothing */
	ARG_UNUSED(args);
}

static enum ethernet_hw_caps eth_renesas_ra_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE;
}

static void eth_renesas_ra_init_iface(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_renesas_ra_data *data = dev->data;
	const struct eth_renesas_ra_config *config = dev->config;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	if (data->iface == NULL) {
		data->iface = iface;
	}

	if (!device_is_ready(config->phy_dev)) {
		LOG_DBG("phy is not ready");
		return;
	}

	ethernet_init(iface);

	net_if_carrier_off(data->iface);

	data->phy_link_up = 0;
	phy_link_callback_set(config->phy_dev, &phy_link_cb, (void *)dev);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_renesas_ra_get_stats(const struct device *dev)
{
	struct eth_renesas_ra_data *data = dev->data;

	return &data->stats;
}

#endif /* CONFIG_NET_STATISTICS_ETHERNET */

const struct device *eth_renesas_ra_get_phy(const struct device *dev)
{
	const struct eth_renesas_ra_config *config = dev->config;

	return config->phy_dev;
}

static int eth_renesas_ra_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_renesas_ra_data *data = dev->data;
	size_t len = net_pkt_get_len(pkt);
	fsp_err_t fsp_err = FSP_ERR_NOT_INITIALIZED;
	int ret = 0;
	uint8_t *tx_buf;

#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
	tx_buf = data->tx_buf_header[data->tx_buf_idx].buf;
	data->tx_buf_idx = (data->tx_buf_idx + 1) % (data->tx_buf_num);
#else
	tx_buf = data->tx_frame;
#endif

#if defined(CONFIG_ETH_RENESAS_RA_USE_HW_WRITEBACK)
	k_sem_reset(&data->tx_sem);
#endif

	ret = net_pkt_read(pkt, tx_buf, len);
	if (ret < 0) {
		LOG_DBG("failed to read TX packet");
		goto tx_end;
	}

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		ret = -EINVAL;
		LOG_DBG("TX packet too large");
		goto tx_end;
	}

	fsp_err = R_RMAC_Write(data->fsp_ctrl, tx_buf, (uint32_t)len);
	if (fsp_err != FSP_SUCCESS) {
		ret = (fsp_err == FSP_ERR_ETHER_ERROR_TRANSMIT_BUFFER_FULL) ? -ENOBUFS : -EIO;
		LOG_DBG("write to FIFO failed, err=%d", fsp_err);
	}

tx_end:

#if defined(CONFIG_ETH_RENESAS_RA_USE_HW_WRITEBACK)
	if (fsp_err == FSP_SUCCESS) {
		/* wait descriptor write back complete */
		ret = k_sem_take(&data->tx_sem, K_MSEC(1));

#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
		fsp_err = R_RMAC_TxStatusGet(data->fsp_ctrl, tx_buf);
		if (fsp_err != FSP_SUCCESS) {
			ret = -EIO;
		}
#endif
	}
#endif

	if (ret) {
		eth_stats_update_errors_tx(data->iface);
	}

	return ret;
}

static void renesas_ra_eth_rx(const struct device *dev)
{
	struct eth_renesas_ra_data *data = dev->data;
	struct net_pkt *pkt = NULL;
	size_t len = 0;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;
	uint8_t *rx_buf;

#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
	fsp_err = R_RMAC_Read(data->fsp_ctrl, &rx_buf, (uint32_t *)&len);
#else
	rx_buf = data->rx_frame;
	fsp_err = R_RMAC_Read(data->fsp_ctrl, rx_buf, (uint32_t *)&len);
#endif

	if (fsp_err == FSP_ERR_ETHER_ERROR_NO_DATA) {
		/* Nothing to receive, all desc in queue was read */
		k_sem_reset(&data->rx_sem);
		return;
	} else if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("failed to read from FIFO");
		ret = -EIO;
		goto rx_end;
	}

	/* read again for remaining data */
	k_sem_give(&data->rx_sem);

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, (size_t)len, AF_UNSPEC, 0, K_MSEC(100));
	if (pkt == NULL) {
		LOG_DBG("failed to obtain RX buffer");
		goto rx_end;
	}

	ret = net_pkt_write(pkt, rx_buf, len);
	if (ret < 0) {
		LOG_DBG("failed to append RX buffer to packet");
		net_pkt_unref(pkt);
		goto rx_end;
	}

#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
	fsp_err = R_RMAC_RxBufferUpdate(data->fsp_ctrl, (void *)rx_buf);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("failed to release RX buffer");
	}
#endif

	ret = net_recv_data(net_pkt_iface(pkt), pkt);
	if (ret < 0) {
		LOG_DBG("failed to push pkt to network stack");
		net_pkt_unref(pkt);
	}

rx_end:
	if (ret) {
		eth_stats_update_errors_rx(data->iface);
	}
}

static void eth_rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct eth_renesas_ra_data *data = dev->data;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		if (k_sem_take(&data->rx_sem, K_MSEC(100))) {
			continue;
		}

		renesas_ra_eth_rx(dev);
	}
}

static int renesas_ra_eswm_init(const struct device *dev)
{
	struct eswm_renesas_ra_data *data = dev->data;
	const struct eswm_renesas_ra_config *config = dev->config;
	uint32_t gwcaclk, pclk, eswclk, eswphyclk, ethphyclk;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	clock_control_get_rate(config->gwcaclk_dev, NULL, &gwcaclk);
	clock_control_get_rate(config->pclk_dev, NULL, &pclk);
	clock_control_get_rate(config->eswclk_dev, NULL, &eswclk);
	clock_control_get_rate(config->eswphyclk_dev, NULL, &eswphyclk);
	clock_control_get_rate(config->ethphyclk_dev, NULL, &ethphyclk);

	/* Clock restrictions for eswm on HM */
	if ((gwcaclk * 1.5 < eswclk) || (eswclk <= pclk) || (gwcaclk <= pclk)) {
		LOG_ERR("ESWM clock invalid");
		return -EIO;
	}

	fsp_err = R_LAYER3_SWITCH_Open(data->fsp_ctrl, data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("ESWM open failed, err=%d", fsp_err);
		return -EIO;
	}

	if (config->ethphyclk_enable) {
		ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
		if (ret) {
			return ret;
		}

		ret = clock_control_on(config->ethphyclk_dev,
				       (clock_control_subsys_t)&config->ethphyclk_subsys);
		if (ret) {
			LOG_DBG("failed to start eth phy clock, err=%d", ret);
		}
	} else {
		(void)clock_control_off(config->ethphyclk_dev,
					(clock_control_subsys_t)&config->ethphyclk_subsys);
	}

	config->en_irq();

	return 0;
}

static int renesas_ra_eth_init(const struct device *dev)
{
	struct eth_renesas_ra_data *data = dev->data;
	const struct eth_renesas_ra_config *config = dev->config;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;
	uint32_t phy_ref_rate;

	clock_control_get_rate(config->phy_clock, NULL, &phy_ref_rate);

	if (config->phy_clock_type == ETH_PHY_REF_CLK_INTERNAL) {
		/* internal phy clock should be 25/50 MHz */
		if (phy_ref_rate != ETHPHYCLK_25MHZ && phy_ref_rate != ETHPHYCLK_50MHZ) {
			LOG_DBG("internal PHY clock %d differ from 25/50 MHz", phy_ref_rate);
		}
	} else if (config->phy_clock_type != ETH_PHY_REF_CLK_XTAL) {
		LOG_DBG("invalid phy clock type");
	}

	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (config->random_mac || !config->valid_mac) {
		gen_random_mac(&data->mac_addr[0], RENESAS_OUI_B0, RENESAS_OUI_B1, RENESAS_OUI_B2);
	}

	data->fsp_cfg->p_mac_address = &data->mac_addr[0];

	if (data->fsp_ctrl->open != 0) {
		R_RMAC_Close(data->fsp_ctrl);
	}

	fsp_err = R_RMAC_Open(data->fsp_ctrl, data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("RMAC open failed, err=%d", fsp_err);
		return -EIO;
	}

	fsp_err = R_RMAC_CallbackSet(data->fsp_ctrl, (void *)&eth_rmac_cb, (void *)dev,
				     &data->fsp_cb);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("RMAC set cb failed, err=%d", fsp_err);
		return -EIO;
	}

	k_sem_init(&data->rx_sem, 0, K_SEM_MAX_LIMIT);
#if defined(CONFIG_ETH_RENESAS_RA_USE_HW_WRITEBACK)
	k_sem_init(&data->tx_sem, 0, 1);
#endif

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), eth_rx_thread, (void *)dev,
			NULL, NULL, K_PRIO_COOP(CONFIG_ETH_RENESAS_RA_RX_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_renesas_ra_rx");

	return 0;
}

static const struct ethernet_api eth_renesas_ra_api = {
	.iface_api.init = eth_renesas_ra_init_iface,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_renesas_ra_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	.get_capabilities = eth_renesas_ra_get_capabilities,
	.get_phy = eth_renesas_ra_get_phy,
	.send = eth_renesas_ra_tx,
};

#define DT_DRV_COMPAT renesas_ra_eswm

#define ETH_USE_INTERNAL_PHY_CLK(id)                                                               \
	COND_CODE_1(UTIL_AND(DT_NODE_HAS_COMPAT(id, renesas_ra_ethernet_rmac),                     \
		 DT_ENUM_HAS_VALUE(id, phy_clock_type, internal)), (1), (0))

PINCTRL_DT_INST_DEFINE(0);

static void renesas_ra_eswm_init_irq(void)
{
	R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(0, gwdi, irq)].IELS =
		BSP_PRV_IELS_ENUM(EVENT_ETHER_GWDI0);
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, gwdi, irq), DT_INST_IRQ_BY_NAME(0, gwdi, priority),
		    layer3_switch_gwdi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, gwdi, irq));
}

static layer3_switch_extended_cfg_t eswm_ext_cfg = {
	.p_ether_phy_instances = {NULL, NULL},
	.fowarding_target_port_masks = {LAYER3_SWITCH_PORT_BITMASK_PORT2,
					LAYER3_SWITCH_PORT_BITMASK_PORT2},
};
static layer3_switch_instance_ctrl_t eswm_ctrl = {0};
static ether_switch_cfg_t eswm_cfg = {
	.channel = 0,
	.p_extend = &eswm_ext_cfg,
	.p_callback = &eth_switch_cb,
	.p_context = (void *)DEVICE_DT_INST_GET(0),
	.irq = DT_INST_IRQ_BY_NAME(0, gwdi, irq),
	.ipl = DT_INST_IRQ_BY_NAME(0, gwdi, priority),
};
static ether_switch_instance_t eswm_inst = {
	.p_ctrl = (ether_switch_ctrl_t *)&eswm_ctrl,
	.p_cfg = (ether_switch_cfg_t *)&eswm_cfg,
	.p_api = (ether_switch_api_t *)&g_ether_switch_on_layer3_switch,
};
static struct eswm_renesas_ra_data eswm_data = {
	.fsp_ctrl = &eswm_ctrl,
	.fsp_cfg = &eswm_cfg,
};
static struct eswm_renesas_ra_config eswm_config = {
	.gwcaclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, gwcaclk)),
	.pclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, pclk)),
	.eswclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, eswclk)),
	.eswphyclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, eswphyclk)),
	.ethphyclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, ethphyclk)),
	.pclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(0, pclk, mstp),
	.pclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(0, pclk, stop_bit),
	.ethphyclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(0, ethphyclk, mstp),
	.ethphyclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(0, ethphyclk, stop_bit),
	.ethphyclk_enable = DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, ETH_USE_INTERNAL_PHY_CLK, (+)),
	.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.en_irq = &renesas_ra_eswm_init_irq,
};
DEVICE_DT_INST_DEFINE(0, renesas_ra_eswm_init, NULL, &eswm_data, &eswm_config, POST_KERNEL,
		      CONFIG_ESWM_RENESAS_RA_INIT_PRIORITY, NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_ra_ethernet_rmac

#define ETH_TX_QUEUE_NUM(n) DT_INST_PROP(n, txq_num)
#define ETH_RX_QUEUE_NUM(n) DT_INST_PROP(n, rxq_num)
#define ETH_TX_QUEUE_LEN(n) DT_INST_PROP(n, txq_len)
#define ETH_RX_QUEUE_LEN(n) DT_INST_PROP(n, rxq_len)
#define ETH_TX_BUF_NUM(n)   DT_INST_PROP(n, txb_num)
#define ETH_RX_BUF_NUM(n)   DT_INST_PROP(n, rxb_num)
#define ETH_BUF_NUM(n)      (ETH_TX_BUF_NUM(n) + ETH_RX_BUF_NUM(n))
#define ETH_DESC_NUM(n)                                                                            \
	(ETH_TX_QUEUE_NUM(n) * (ETH_TX_QUEUE_LEN(n) - 1) +                                         \
	 (ETH_RX_QUEUE_NUM(n) * (ETH_RX_QUEUE_LEN(n) - 1)))

#define ETH_PHY_CONN_TYPE(n)                                                                       \
	((DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii)                                    \
		  ? ETHER_PHY_MII_TYPE_RGMII                                                       \
		  : (DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii)                          \
			     ? ETHER_PHY_MII_TYPE_GMII                                             \
			     : (DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii)               \
					? ETHER_PHY_MII_TYPE_RMII                                  \
					: ETHER_PHY_MII_TYPE_MII))))
#define ETH_PHY_CLOCK_TYPE(n)                                                                      \
	DT_INST_ENUM_HAS_VALUE(n, phy_clock_type, xtal) ? ETH_PHY_REF_CLK_XTAL                     \
							: ETH_PHY_REF_CLK_INTERNAL

/* Buffers declare */
#define ETH_TX_BUF_DECLARE(idx, n)                                                                 \
	static uint8_t eth##n##_tx_buf##idx[ETH_BUF_SIZE] __eth_renesas_buf;
#define ETH_RX_BUF_DECLARE(idx, n)                                                                 \
	static uint8_t eth##n##_rx_buf##idx[ETH_BUF_SIZE] __eth_renesas_buf;
#define ETH_TX_BUF_PTR_DECLARE(idx, n) (uint8_t *)&eth##n##_tx_buf##idx[0]
#define ETH_RX_BUF_PTR_DECLARE(idx, n) (uint8_t *)&eth##n##_rx_buf##idx[0]

/* Descriptors declare */
#define ETH_TX_DESC_DECLARE(idx, n)                                                                \
	static layer3_switch_descriptor_t                                                          \
		eth##n##_tx_desc_array##idx[ETH_TX_BUF_NUM(n)] __eth_renesas_desc;
#define ETH_RX_DESC_DECLARE(idx, n)                                                                \
	static layer3_switch_descriptor_t                                                          \
		eth##n##_rx_desc_array##idx[ETH_RX_BUF_NUM(n)] __eth_renesas_desc;

/* Queues declare */
#define ETH_TX_QUEUE_DECLARE(idx, n)                                                               \
	{                                                                                          \
		.queue_cfg =                                                                       \
			{                                                                          \
				.array_length = ETH_TX_QUEUE_LEN(n),                               \
				.p_descriptor_array = eth##n##_tx_desc_array##idx,                 \
				.p_ts_descriptor_array = NULL,                                     \
				.ports = 1 << DT_INST_PROP(n, channel),                            \
				.type = LAYER3_SWITCH_QUEUE_TYPE_TX,                               \
				.write_back_mode = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,             \
				.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,    \
				.rx_timestamp_storage =                                            \
					LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,                \
			},                                                                         \
	}
#define ETH_RX_QUEUE_DECLARE(idx, n)                                                               \
	{                                                                                          \
		.queue_cfg =                                                                       \
			{                                                                          \
				.array_length = ETH_RX_QUEUE_LEN(n),                               \
				.p_descriptor_array = eth##n##_rx_desc_array##idx,                 \
				.p_ts_descriptor_array = NULL,                                     \
				.ports = 1 << DT_INST_PROP(n, channel),                            \
				.type = LAYER3_SWITCH_QUEUE_TYPE_RX,                               \
				.write_back_mode = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,             \
				.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,    \
				.rx_timestamp_storage =                                            \
					LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,                \
			},                                                                         \
	}

/* Buffer configuration for device data */
#if defined(CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY)
#define ETH_RENESAS_RA_DATA_BUF_MODE ETHER_ZEROCOPY_ENABLE
#define ETH_TX_BUF_HEADER_DECLARE(idx, n)                                                          \
	{                                                                                          \
		.buf = ETH_TX_BUF_PTR_DECLARE(idx, n),                                             \
	}
#define ETH_RENESAS_RA_DATA_BUF_DECLARE(n)                                                         \
	struct eth_renesas_ra_buf_header eth##n##_tx_buf_header[] = {                              \
		LISTIFY(ETH_TX_BUF_NUM(n), ETH_TX_BUF_HEADER_DECLARE, (,), n),			   \
	}
#define ETH_RENESAS_RA_DATA_BUF_PROP_DECLARE(n)                                                    \
	.tx_buf_header = eth##n##_tx_buf_header, .tx_buf_idx = 0, .tx_buf_num = ETH_TX_BUF_NUM(n)
#else /* CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY */
#define ETH_RENESAS_RA_DATA_BUF_MODE ETHER_ZEROCOPY_DISABLE
#define ETH_RENESAS_RA_DATA_BUF_DECLARE(n)                                                         \
	static uint8_t eth##n##_rx_frame[ETH_BUF_SIZE] __eth_renesas_buf;			   \
	static uint8_t eth##n##_tx_frame[ETH_BUF_SIZE] __eth_renesas_buf;
#define ETH_RENESAS_RA_DATA_BUF_PROP_DECLARE(n)                                                    \
	.rx_frame = eth##n##_rx_frame, .tx_frame = eth##n##_tx_frame
#endif /* CONFIG_ETH_RENESAS_RA_USE_ZERO_COPY */

#define ETH_RENESAS_RA_INIT(n)                                                                     \
	BUILD_ASSERT(DT_INST_PROP(n, channel) <= BSP_FEATURE_ETHER_NUM_CHANNELS);                  \
	BUILD_ASSERT(                                                                              \
		(ETH_TX_QUEUE_NUM(n) + ETH_RX_QUEUE_NUM(n) <=                                      \
		 LAYER3_SWITCH_CFG_AVAILABLE_QUEUE_NUM) &&                                         \
			(LAYER3_SWITCH_CFG_AVAILABLE_QUEUE_NUM < BSP_FEATURE_ESWM_MAX_QUEUE_NUM),  \
		"invalid queue settings");                                                         \
	BUILD_ASSERT(ETH_DESC_NUM(n) <= ETH_BUF_NUM(n), "invalid buffer settings");                \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	LISTIFY(ETH_RX_BUF_NUM(n), ETH_RX_BUF_DECLARE, (), n);                                     \
	LISTIFY(ETH_TX_BUF_NUM(n), ETH_TX_BUF_DECLARE, (), n);                                     \
	static uint8_t *eth##n##_pp_buffers[] = {                                                  \
		LISTIFY(ETH_RX_BUF_NUM(n), ETH_RX_BUF_PTR_DECLARE, (,), n),                        \
		LISTIFY(ETH_TX_BUF_NUM(n), ETH_TX_BUF_PTR_DECLARE, (,), n),			   \
	};                                                                                         \
                                                                                                   \
	LISTIFY(ETH_RX_QUEUE_NUM(n), ETH_RX_DESC_DECLARE, (), n);                                  \
	LISTIFY(ETH_TX_QUEUE_NUM(n), ETH_TX_DESC_DECLARE, (), n);                                  \
	static rmac_queue_info_t eth##n##_rx_queue_list[ETH_RX_QUEUE_NUM(n)] = {                   \
		LISTIFY(ETH_RX_QUEUE_NUM(n), ETH_RX_QUEUE_DECLARE, (,), n),                        \
	};                                                                                         \
	static rmac_queue_info_t eth##n##_tx_queue_list[ETH_TX_QUEUE_NUM(n)] = {                   \
		LISTIFY(ETH_TX_QUEUE_NUM(n), ETH_TX_QUEUE_DECLARE, (,), n),                        \
	};                                                                                         \
	static rmac_buffer_node_t eth##n##_buffer_node_list[ETH_BUF_NUM(n)];			   \
	ETH_RENESAS_RA_DATA_BUF_DECLARE(n);							   \
                                                                                                   \
	static rmac_instance_ctrl_t eth##n##_ctrl;                                                 \
	static rmac_extended_cfg_t eth##n##_ext_cfg = {                                            \
		.p_ether_switch = &eswm_inst,                                                      \
		.tx_queue_num = ETH_TX_QUEUE_NUM(n),                                               \
		.rx_queue_num = ETH_RX_QUEUE_NUM(n),                                               \
		.p_tx_queue_list = eth##n##_tx_queue_list,                                         \
		.p_rx_queue_list = eth##n##_rx_queue_list,                                         \
		.p_buffer_node_list = eth##n##_buffer_node_list,                                   \
		.buffer_node_num = ETH_BUF_NUM(n),                                                 \
		.rmpi_irq = FSP_INVALID_VECTOR,                                                    \
		.rmpi_ipl = BSP_IRQ_DISABLED,                                                      \
	};                                                                                         \
	static ether_cfg_t eth##n##_cfg = {                                                        \
		.channel = DT_INST_PROP(n, channel),                                               \
		.num_tx_descriptors = ETH_TX_BUF_NUM(n),                                           \
		.num_rx_descriptors = ETH_RX_BUF_NUM(n),                                           \
		.pp_ether_buffers = eth##n##_pp_buffers,                                           \
		.ether_buffer_size = ETH_BUF_SIZE,                                                 \
		.padding = ETHER_PADDING_DISABLE,                                                  \
		.zerocopy = ETH_RENESAS_RA_DATA_BUF_MODE,					   \
		.multicast = ETHER_MULTICAST_ENABLE,                                               \
		.promiscuous = ETHER_PROMISCUOUS_DISABLE,                                          \
		.flow_control = ETHER_FLOW_CONTROL_DISABLE,                                        \
		.p_callback = &eth_rmac_cb,                                                        \
		.p_context = (void *)DEVICE_DT_INST_GET(n),                                        \
		.p_extend = &eth##n##_ext_cfg,                                                     \
	};                                                                                         \
	static struct eth_renesas_ra_data eth##n##_renesas_ra_data = {                             \
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),                            \
		.fsp_ctrl = &eth##n##_ctrl,                                                        \
		.fsp_cfg = &eth##n##_cfg,                                                          \
		ETH_RENESAS_RA_DATA_BUF_PROP_DECLARE(n)};					   \
	static struct eth_renesas_ra_config eth##n##_renesas_ra_config = {                         \
		.random_mac = DT_INST_PROP(n, zephyr_random_mac_address),                          \
		.valid_mac = NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)),                              \
		.mii_type = ETH_PHY_CONN_TYPE(n),                                                  \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                          \
		.phy_clock = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_clock)),                         \
		.phy_clock_type = ETH_PHY_CLOCK_TYPE(n),                                           \
	};                                                                                         \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, renesas_ra_eth_init, NULL, &eth##n##_renesas_ra_data,     \
				      &eth##n##_renesas_ra_config, CONFIG_ETH_INIT_PRIORITY,       \
				      &eth_renesas_ra_api, NET_ETH_MTU /*MTU*/);

DT_INST_FOREACH_STATUS_OKAY(ETH_RENESAS_RA_INIT);
