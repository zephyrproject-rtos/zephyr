/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Renesas RA Ethernet Switch Module (ESWM) DSA driver.
 *
 * The renesas,ra-eswm-dsa node is the switch core and the conduit. Its
 * renesas,ra-eswm-switch child holds the DSA ports: each Ethernet port is a
 * user port driven by its own FSP RMAC instance, and the CPU port links the
 * switch to the conduit. Frames are not forwarded between the ports, every
 * port forwards only to the CPU.
 */

#include <errno.h>
#include <string.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/ethernet/dsa_tag_proto.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util.h>
#include <ethernet/eth_stats.h>
#include "r_rmac.h"
#include "r_layer3_switch.h"
#include "../eth_renesas_ra_rmac.h"

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
#include <zephyr/drivers/ptp_clock.h>
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

#define DT_DRV_COMPAT renesas_ra_eswm_dsa

LOG_MODULE_REGISTER(dsa_renesas_ra_eswm, CONFIG_ETHERNET_LOG_LEVEL);

/* FSP only supports one descriptor per frame, so buffers hold a full frame */
#define ESWM_BUF_SIZE 1536

#if defined(CONFIG_NOCACHE_MEMORY)
#define __eswm_desc __nocache __aligned(32)
#define __eswm_buf  __nocache __aligned(32)
#else /* CONFIG_NOCACHE_MEMORY */
#define __eswm_desc
#define __eswm_buf
#endif /* CONFIG_NOCACHE_MEMORY */

#define ETHPHYCLK_25MHZ MHZ(25)
#define ETHPHYCLK_50MHZ MHZ(50)

#define ESWM_RX_BUF_TIMEOUT K_MSEC(100)
#define ESWM_TX_TIMEOUT     K_MSEC(1)

/* Index of phy-connection-type in ethernet-controller.yaml */
#define ESWM_PHY_CONN_MII   0
#define ESWM_PHY_CONN_RMII  1
#define ESWM_PHY_CONN_GMII  2
#define ESWM_PHY_CONN_RGMII 3

#define ESWM_TX_QUEUE_NUM CONFIG_DSA_RENESAS_RA_ESWM_TX_QUEUE_NUM
#define ESWM_RX_QUEUE_NUM CONFIG_DSA_RENESAS_RA_ESWM_RX_QUEUE_NUM
#define ESWM_TX_QUEUE_LEN CONFIG_DSA_RENESAS_RA_ESWM_TX_QUEUE_LEN
#define ESWM_RX_QUEUE_LEN CONFIG_DSA_RENESAS_RA_ESWM_RX_QUEUE_LEN
#define ESWM_TX_BUF_NUM   CONFIG_DSA_RENESAS_RA_ESWM_TX_BUF_NUM
#define ESWM_RX_BUF_NUM   CONFIG_DSA_RENESAS_RA_ESWM_RX_BUF_NUM
#define ESWM_BUF_NUM      (ESWM_TX_BUF_NUM + ESWM_RX_BUF_NUM)
#define ESWM_DESC_NUM                                                                              \
	((ESWM_TX_QUEUE_NUM * (ESWM_TX_QUEUE_LEN - 1)) +                                           \
	 (ESWM_RX_QUEUE_NUM * (ESWM_RX_QUEUE_LEN - 1)))

BUILD_ASSERT(ESWM_DESC_NUM <= ESWM_BUF_NUM, "more descriptors than buffers");

#define ESWM_NODE        DT_INST(0, renesas_ra_eswm_dsa)
#define ESWM_SWITCH_NODE DT_INST(0, renesas_ra_eswm_switch)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(renesas_ra_eswm_dsa) == 1, "only one ESWM is supported");
BUILD_ASSERT(DT_SAME_NODE(DT_PARENT(ESWM_SWITCH_NODE), ESWM_NODE),
	     "renesas,ra-eswm-switch must be a child of the renesas,ra-eswm-dsa node");

/* The CPU port has an ethernet phandle to the conduit, the user ports do not */
#define ESWM_PORT_IS_CPU(port)  DT_NODE_HAS_PROP(port, ethernet)
#define ESWM_PORT_IS_USER(port) COND_CODE_1(ESWM_PORT_IS_CPU(port), (0), (1))

/* gPTP timer number from the ptp-clock property, 0 if there is none */
#define ESWM_PTP_CLOCK_NODE(port) DT_PHANDLE(port, ptp_clock)
#define ESWM_PTP_TIMER_OF_CLOCK(clock)                                                             \
	COND_CODE_1(DT_NODE_HAS_COMPAT(clock, renesas_ra_gptp_timer), (DT_REG_ADDR(clock)), (0))
#define ESWM_PORT_PTP_TIMER(port)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(port, ptp_clock),                                             \
		    (ESWM_PTP_TIMER_OF_CLOCK(ESWM_PTP_CLOCK_NODE(port))), (0))

struct eswm_dsa_config {
	const struct device *gwcaclk_dev;
	const struct device *pclk_dev;
	const struct device *eswclk_dev;
	const struct device *ethphyclk_dev;
	struct clock_control_ra_subsys_cfg ethphyclk_subsys;
	bool ethphyclk_enable;
	const struct pinctrl_dev_config *pin_cfg;
	struct net_eth_mac_config mcfg;
	void (*en_irq)(void);
};

struct eswm_dsa_data {
	layer3_switch_instance_ctrl_t *fsp_ctrl;
	ether_switch_cfg_t *fsp_cfg;
	/* FSP starts TX queues through GWCA registers shared by the ports */
	struct k_mutex tx_lock;
};

struct eswm_port_config {
	const struct pinctrl_dev_config *pin_cfg;
	uint8_t phy_conn_type;
#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	const struct device *ptp_clock;
	/* gPTP timer number of ptp_clock */
	uint8_t ptp_timer;
#endif /* CONFIG_ETH_RENESAS_RA_PTP */
};

struct eswm_port_data {
	struct net_if *iface;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	uint8_t *const *tx_bufs;
	uint8_t tx_buf_idx;
	uint8_t tx_buf_num;
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_DSA_RENESAS_RA_ESWM_RX_THREAD_STACK_SIZE);
	struct k_sem rx_sem;
#if defined(CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK)
	struct k_sem tx_sem;
#endif /* CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK */
	rmac_instance_ctrl_t fsp_ctrl;
	ether_cfg_t fsp_cfg;
	ether_callback_args_t fsp_cb;
	bool phy_link_up;
#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	rmac_timestamp_t rx_timestamp;
#endif /* CONFIG_ETH_RENESAS_RA_PTP */
};

struct eswm_port {
	const struct eswm_port_config *config;
	struct eswm_port_data *data;
};

extern void layer3_switch_gwdi_isr(void);
extern void rmac_init_buffers(rmac_instance_ctrl_t *const p_instance_ctrl);
extern void rmac_init_descriptors(rmac_instance_ctrl_t *const p_instance_ctrl);
extern void rmac_configure_reception_filter(rmac_instance_ctrl_t const *const p_instance_ctrl);
extern void r_rmac_disable_reception(rmac_instance_ctrl_t *p_instance_ctrl);
extern fsp_err_t rmac_do_link(rmac_instance_ctrl_t *const p_instance_ctrl,
			      const layer3_switch_magic_packet_detection_t mode);

/* NULL for the CPU port */
static inline struct eswm_port *eswm_port_get(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;

	return cfg->prv_config;
}

static void eswm_rmac_cb(ether_callback_args_t *args)
{
	struct eswm_port *port = args->p_context;

	switch (args->event) {
#if defined(CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK)
	case ETHER_EVENT_TX_COMPLETE:
		k_sem_give(&port->data->tx_sem);
		break;
#endif /* CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK */
	case ETHER_EVENT_RX_MESSAGE_LOST:
	case ETHER_EVENT_RX_COMPLETE:
		k_sem_give(&port->data->rx_sem);
		break;
	default:
		break;
	}
}

static void eswm_switch_cb(ether_switch_callback_args_t *args)
{
	ARG_UNUSED(args);
}

static void eswm_port_mii_setting(const struct eswm_port *port)
{
	uint8_t channel = port->data->fsp_cfg.channel;
	uint8_t phymode = BIT(R_PMISC_PFENET_PHYMODE0_Pos + channel);
	volatile uint32_t *p_miicr_register = &(R_ESWM->MIICR0) + channel;

	/* PHYMODE selects MII, otherwise RMII */
	if (port->config->phy_conn_type == ESWM_PHY_CONN_MII) {
		R_PMISC->PFENET = (uint8_t)(R_PMISC->PFENET | phymode);
	} else {
		R_PMISC->PFENET = (uint8_t)(R_PMISC->PFENET & ~phymode);
	}

	switch (port->config->phy_conn_type) {
	case ESWM_PHY_CONN_MII:
	case ESWM_PHY_CONN_GMII:
		*p_miicr_register = 0;
		break;
	case ESWM_PHY_CONN_RMII:
		*p_miicr_register = 2;
		break;
	case ESWM_PHY_CONN_RGMII:
		*p_miicr_register = (R_ESWM_MIICR0_TXCIDE_Msk | 1);

		/* Enable TXC generation */
		R_ESWM->MIIRR = R_ESWM->MIIRR | (1 << (R_ESWM_MIIRR_RGRST_Pos + channel));
		break;
	default:
		LOG_ERR("Port %u: unsupported phy-connection-type", channel);
		break;
	}
}

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
/* The timestamp has only the lower 32 bits of seconds, take the upper bits from now */
static void eswm_ptp_time(const rmac_timestamp_t *ts, const struct net_ptp_time *now,
			  struct net_ptp_time *tm)
{
	uint64_t second = (now->second & ~(uint64_t)UINT32_MAX) | ts->sec_lower;

	if ((second > now->second) && (second > UINT32_MAX)) {
		second -= (uint64_t)UINT32_MAX + 1U;
	}

	tm->second = second;
	tm->nanosecond = ts->ns;
}

static void eswm_port_timestamp_rx_pkt(const struct eswm_port *port, struct net_pkt *pkt)
{
	const rmac_timestamp_t *ts = &port->data->rx_timestamp;
	struct net_ptp_time now;
	struct net_ptp_time tm;

	/* FSP returns 0 when the descriptor has no valid timestamp */
	if ((port->config->ptp_clock == NULL) || ((ts->sec_lower == 0U) && (ts->ns == 0U))) {
		return;
	}

	if (ptp_clock_get(port->config->ptp_clock, &now) != 0) {
		return;
	}

	eswm_ptp_time(ts, &now, &tm);
	net_pkt_set_timestamp(pkt, &tm);
	net_pkt_set_rx_timestamping(pkt, true);
}

/* Free this port's stale TX timestamps, the queue is shared by the ports of a gPTP timer */
static void eswm_port_tx_timestamp_flush(const struct eswm_port *port)
{
	const rmac_extended_cfg_t *ext_cfg = port->data->fsp_cfg.p_extend;
	const layer3_switch_descriptor_queue_cfg_t *queue_cfg = &ext_cfg->p_ts_queue->queue_cfg;

	/* Skip the last descriptor, it is the LINKFIX */
	for (uint32_t i = 0U; i < (queue_cfg->array_length - 1U); i++) {
		struct st_ts_reception_descriptor_result *result =
			&queue_cfg->p_ts_descriptor_array[i].ts_reception_descriptor_result;

		if ((result->dt != LAYER3_SWITCH_DESCRIPTOR_TYPE_FEMPTY_ND) &&
		    (result->dpn == port->data->fsp_cfg.channel)) {
			LOG_DBG("Drop stale TX timestamp, tsun %u", result->tsun);
			result->dt = LAYER3_SWITCH_DESCRIPTOR_TYPE_FEMPTY_ND;
		}
	}
}

static int eswm_port_tx_timestamp_request(struct eswm_port *port, bool enable)
{
	rmac_write_cfg_t write_cfg = {
		.tx_timestamp_enable = enable ? 1U : 0U,
	};

	return (R_RMAC_SetWriteConfig(&port->data->fsp_ctrl, &write_cfg) == FSP_SUCCESS) ? 0
											  : -EIO;
}

/* FSP polls for the TX timestamp in R_RMAC_Write() */
static void eswm_port_tx_timestamp(struct eswm_port *port, struct net_pkt *pkt)
{
	rmac_timestamp_t ts = {0};
	struct net_ptp_time now;
	struct net_ptp_time tm;

	/* FSP returns 0 when no timestamp was found */
	if ((R_RMAC_GetTxTimestamp(&port->data->fsp_ctrl, &ts) != FSP_SUCCESS) ||
	    ((ts.sec_lower == 0U) && (ts.ns == 0U))) {
		LOG_DBG("No TX timestamp");
		return;
	}

	if (ptp_clock_get(port->config->ptp_clock, &now) != 0) {
		return;
	}

	eswm_ptp_time(&ts, &now, &tm);
	net_pkt_set_timestamp(pkt, &tm);
	net_if_add_tx_timestamp(pkt);
}

/* Returns true when a TX timestamp was requested for pkt */
static bool eswm_port_tx_timestamp_start(struct eswm_port *port, struct net_pkt *pkt)
{
	if ((port->config->ptp_clock == NULL) || !net_pkt_is_tx_timestamping(pkt)) {
		return false;
	}

	eswm_port_tx_timestamp_flush(port);

	return eswm_port_tx_timestamp_request(port, true) == 0;
}

static void eswm_port_tx_timestamp_end(struct eswm_port *port, struct net_pkt *pkt, bool sent)
{
	/* FSP only clears the request when R_RMAC_Write() starts the TX queue */
	if (eswm_port_tx_timestamp_request(port, false) != 0) {
		LOG_DBG("Failed to clear TX timestamp request");
	}

	if (sent) {
		eswm_port_tx_timestamp(port, pkt);
	}
}
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

static bool eswm_port_rx(struct eswm_port *port)
{
	struct eswm_port_data *data = port->data;
	struct net_pkt *pkt = NULL;
	uint32_t len = 0U;
	fsp_err_t fsp_err;
	int ret = 0;
	uint8_t *rx_buf;

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	data->rx_timestamp = (rmac_timestamp_t){0};
	/* This function just saves the pointer, so R_RMAC_Read() can store the timestamp */
	(void)R_RMAC_GetRxTimestamp(&data->fsp_ctrl, &data->rx_timestamp);
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

	fsp_err = R_RMAC_Read(&data->fsp_ctrl, &rx_buf, &len);
	if (fsp_err == FSP_ERR_ETHER_ERROR_NO_DATA) {
		/* All descriptors of the queue were read */
		return false;
	} else if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Failed to read from FIFO");
		ret = -EIO;
		goto rx_end;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, NET_AF_UNSPEC, 0,
					   ESWM_RX_BUF_TIMEOUT);
	if (pkt == NULL) {
		(void)R_RMAC_RxBufferUpdate(&data->fsp_ctrl, rx_buf);
		LOG_DBG("Failed to obtain RX buffer");
		ret = -ENOBUFS;
		goto rx_end;
	}

	ret = net_pkt_write(pkt, rx_buf, len);
	if (ret < 0) {
		(void)R_RMAC_RxBufferUpdate(&data->fsp_ctrl, rx_buf);
		LOG_DBG("Failed to append RX buffer to packet");
		net_pkt_unref(pkt);
		goto rx_end;
	}

	fsp_err = R_RMAC_RxBufferUpdate(&data->fsp_ctrl, rx_buf);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Failed to release RX buffer");
	}

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	eswm_port_timestamp_rx_pkt(port, pkt);
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

	/* No tag: the frame goes straight to the user port it was received on */
	ret = net_recv_data(data->iface, pkt);
	if (ret < 0) {
		LOG_DBG("Failed to push pkt to network stack");
		net_pkt_unref(pkt);
	}

rx_end:
	if (ret < 0) {
		eth_stats_update_errors_rx(data->iface);
	}

	return true;
}

static void eswm_port_rx_thread(void *arg1, void *unused1, void *unused2)
{
	struct eswm_port *port = arg1;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		k_sem_take(&port->data->rx_sem, K_FOREVER);
		while (eswm_port_rx(port)) {
		}
	}
}

static int eswm_port_tx(struct eswm_port *port, struct net_pkt *pkt)
{
	struct eswm_port_data *data = port->data;
	size_t len = net_pkt_get_len(pkt);
	fsp_err_t fsp_err = FSP_ERR_NOT_INITIALIZED;
	uint8_t *tx_buf;
	int ret;
#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	bool tx_ts = false;
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		LOG_DBG("TX packet too large");
		return -EINVAL;
	}

	tx_buf = data->tx_bufs[data->tx_buf_idx];
	data->tx_buf_idx = (data->tx_buf_idx + 1U) % data->tx_buf_num;

#if defined(CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK)
	k_sem_reset(&data->tx_sem);
#endif /* CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK */

	ret = net_pkt_read(pkt, tx_buf, len);
	if (ret < 0) {
		LOG_DBG("Failed to read TX packet");
		return ret;
	}

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	tx_ts = eswm_port_tx_timestamp_start(port, pkt);
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

	fsp_err = R_RMAC_Write(&data->fsp_ctrl, tx_buf, (uint32_t)len);
	if (fsp_err != FSP_SUCCESS) {
		ret = (fsp_err == FSP_ERR_ETHER_ERROR_TRANSMIT_BUFFER_FULL) ? -ENOBUFS : -EIO;
		LOG_DBG("Write to FIFO failed, err=%d", fsp_err);
	}

#if defined(CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK)
	if (fsp_err == FSP_SUCCESS) {
		uint8_t *sent_buf = NULL;

		/* Wait for the descriptor write back */
		ret = k_sem_take(&data->tx_sem, ESWM_TX_TIMEOUT);

		fsp_err = R_RMAC_TxStatusGet(&data->fsp_ctrl, &sent_buf);
		if ((fsp_err != FSP_SUCCESS) || (sent_buf != tx_buf)) {
			ret = -EIO;
		}
	}
#endif /* CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK */

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	if (tx_ts) {
		eswm_port_tx_timestamp_end(port, pkt, ret == 0);
	}
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

	return ret;
}

/* DSA switch API */

static int eswm_port_init(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;
	struct eswm_port *port = cfg->prv_config;
	struct eswm_port_data *data;
	fsp_err_t fsp_err;
	int ret;

	if (port == NULL) {
		/* CPU port */
		return 0;
	}

	data = port->data;

	if (!device_is_ready(DEVICE_DT_GET(ESWM_NODE))) {
		LOG_ERR("ESWM is not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(port->config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	data->iface = net_if_lookup_by_dev(dev);

	/* The interface address is written to the RMAC again at link up */
	(void)net_eth_mac_load(&cfg->mcfg, data->mac_addr);
	data->fsp_cfg.p_mac_address = data->mac_addr;

	fsp_err = R_RMAC_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("RMAC open failed, err=%d", fsp_err);
		return -EIO;
	}

	eswm_port_mii_setting(port);

	fsp_err = R_RMAC_CallbackSet(&data->fsp_ctrl, eswm_rmac_cb, port, &data->fsp_cb);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("RMAC set cb failed, err=%d", fsp_err);
		return -EIO;
	}

	k_sem_init(&data->rx_sem, 0, K_SEM_MAX_LIMIT);
#if defined(CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK)
	k_sem_init(&data->tx_sem, 0, 1);
#endif /* CONFIG_DSA_RENESAS_RA_ESWM_USE_HW_WRITEBACK */

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), eswm_port_rx_thread, port,
			NULL, NULL, K_PRIO_COOP(CONFIG_DSA_RENESAS_RA_ESWM_RX_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "dsa_eswm_rx");

	return 0;
}

/* The DSA core sets the carrier after this */
static void eswm_port_phylink_change(const struct device *phy_dev, struct phy_link_state *state,
				     const struct device *dev)
{
	struct eswm_port *port = eswm_port_get(dev);
	struct eswm_port_data *data;
	struct net_linkaddr *lladdr;
	fsp_err_t fsp_err;

	ARG_UNUSED(phy_dev);

	if (port == NULL) {
		return;
	}

	data = port->data;

	if (!state->is_up) {
		if (data->phy_link_up) {
			r_rmac_disable_reception(&data->fsp_ctrl);
			data->phy_link_up = false;
			data->fsp_ctrl.link_establish_status = ETHER_LINK_ESTABLISH_STATUS_DOWN;
		}
		return;
	}

	data->fsp_ctrl.link_establish_status = ETHER_LINK_ESTABLISH_STATUS_UP;

	/* rmac_do_link() writes this address to the RMAC, keep it equal to the interface's */
	lladdr = net_if_get_link_addr(data->iface);
	if ((lladdr != NULL) && (lladdr->len == NET_ETH_ADDR_LEN)) {
		memcpy(data->mac_addr, lladdr->addr, NET_ETH_ADDR_LEN);
	}

	/* Change ETHA to config mode */
	r_rmac_phy_set_operation_mode(data->fsp_cfg.channel, RENESAS_RA_ETHA_DISABLE_MODE);
	r_rmac_phy_set_operation_mode(data->fsp_cfg.channel, RENESAS_RA_ETHA_CONFIG_MODE);

	switch (state->speed) {
	case LINK_HALF_10BASE:
	case LINK_FULL_10BASE:
		data->fsp_ctrl.p_reg_rmac->MPIC_b.LSC = RENESAS_RA_MPIC_LSC_10;
		break;
	case LINK_HALF_100BASE:
	case LINK_FULL_100BASE:
		data->fsp_ctrl.p_reg_rmac->MPIC_b.LSC = RENESAS_RA_MPIC_LSC_100;
		break;
	case LINK_HALF_1000BASE:
	case LINK_FULL_1000BASE:
		data->fsp_ctrl.p_reg_rmac->MPIC_b.LSC = RENESAS_RA_MPIC_LSC_1000;
		break;
	default:
		LOG_DBG("Phy link speed is not supported");
		break;
	}

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
	/* RX timestamp timer, MTRC is only writable in CONFIG mode */
	data->fsp_ctrl.p_reg_rmac->MTRC_b.DTN = port->config->ptp_timer;
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

	/* Change ETHA to operation mode */
	r_rmac_phy_set_operation_mode(data->fsp_cfg.channel, RENESAS_RA_ETHA_DISABLE_MODE);
	r_rmac_phy_set_operation_mode(data->fsp_cfg.channel, RENESAS_RA_ETHA_OPERATION_MODE);

	rmac_init_buffers(&data->fsp_ctrl);
	rmac_init_descriptors(&data->fsp_ctrl);
	rmac_configure_reception_filter(&data->fsp_ctrl);

	fsp_err = rmac_do_link(&data->fsp_ctrl, LAYER3_SWITCH_MAGIC_PACKET_DETECTION_DISABLE);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Link MAC failed, err=%d", fsp_err);
		return;
	}

	data->phy_link_up = true;
}

static enum ethernet_hw_caps eswm_port_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE;
}

static struct dsa_api eswm_dsa_api = {
	.port_init = eswm_port_init,
	.port_phylink_change = eswm_port_phylink_change,
	.get_capabilities = eswm_port_get_capabilities,
};

/* Conduit */

static void eswm_dsa_conduit_iface_init(struct net_if *iface)
{
	const struct eswm_dsa_config *config = net_if_get_device(iface)->config;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];

	/* The conduit sends nothing with its own address */
	if (net_eth_mac_load(&config->mcfg, mac_addr) >= 0) {
		net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
	}

	ethernet_init(iface);
}

static enum ethernet_hw_caps eswm_dsa_conduit_get_capabilities(const struct device *dev,
								 struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return (enum ethernet_hw_caps)0;
}

/*
 * Without a tag, dsa_xmit() passes the sender's packet itself with the user
 * port interface set, so a TX timestamp lands on the packet gPTP waits for.
 */
static int eswm_dsa_conduit_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eswm_dsa_data *data = dev->data;
	struct net_if *iface = net_pkt_iface(pkt);
	const struct ethernet_context *ctx = net_if_l2_data(iface);
	struct eswm_port *port;
	int ret;

	if ((ctx == NULL) || (ctx->dsa_port != DSA_USER_PORT)) {
		LOG_DBG("No user port to send to");
		return -EINVAL;
	}

	port = eswm_port_get(net_if_get_device(iface));

	(void)k_mutex_lock(&data->tx_lock, K_FOREVER);
	ret = eswm_port_tx(port, pkt);
	(void)k_mutex_unlock(&data->tx_lock);

	return ret;
}

static int eswm_dsa_init(const struct device *dev)
{
	struct eswm_dsa_data *data = dev->data;
	const struct eswm_dsa_config *config = dev->config;
	uint32_t gwcaclk = 0U;
	uint32_t pclk = 0U;
	uint32_t eswclk = 0U;
	uint32_t phy_ref_rate = 0U;
	fsp_err_t fsp_err;
	int ret;

	k_mutex_init(&data->tx_lock);

	(void)clock_control_get_rate(config->gwcaclk_dev, NULL, &gwcaclk);
	(void)clock_control_get_rate(config->pclk_dev, NULL, &pclk);
	(void)clock_control_get_rate(config->eswclk_dev, NULL, &eswclk);

	/* ESWCLK at most 1.5 times GWCACLK, PCLK below both */
	if (((2U * eswclk) > (3U * gwcaclk)) || (eswclk <= pclk) || (gwcaclk <= pclk)) {
		LOG_ERR("ESWM clock invalid");
		return -EIO;
	}

	fsp_err = R_LAYER3_SWITCH_Open(data->fsp_ctrl, data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("ESWM open failed, err=%d", fsp_err);
		return -EIO;
	}

	if (config->ethphyclk_enable) {
		(void)clock_control_get_rate(config->ethphyclk_dev, NULL, &phy_ref_rate);
		if ((phy_ref_rate != ETHPHYCLK_25MHZ) && (phy_ref_rate != ETHPHYCLK_50MHZ)) {
			LOG_DBG("ETHPHYCLK %u Hz is not 25 or 50 MHz", phy_ref_rate);
		}

		ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}

		ret = clock_control_on(config->ethphyclk_dev,
				       (clock_control_subsys_t)&config->ethphyclk_subsys);
		if (ret < 0) {
			LOG_DBG("Failed to start ETHPHYCLK, err=%d", ret);
		}
	} else {
		(void)clock_control_off(config->ethphyclk_dev,
					(clock_control_subsys_t)&config->ethphyclk_subsys);
	}

	config->en_irq();

	return 0;
}

static DEVICE_API(ethernet, eswm_dsa_conduit_api) = {
	.iface_api.init = eswm_dsa_conduit_iface_init,
	.get_capabilities = eswm_dsa_conduit_get_capabilities,
	.send = eswm_dsa_conduit_send,
};

#define ESWM_GPTP_TIMER_NUMBER_ENTRY(port)                                                         \
	IF_ENABLED(ESWM_PORT_IS_USER(port),                                                        \
		   ([DT_REG_ADDR(port)] = ESWM_PORT_PTP_TIMER(port),))

PINCTRL_DT_INST_DEFINE(0);

static void eswm_dsa_init_irq(void)
{
	R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(0, gwdi, irq)].IELS =
		BSP_PRV_IELS_ENUM(EVENT_ETHER_GWDI0);
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, gwdi, irq), DT_INST_IRQ_BY_NAME(0, gwdi, priority),
		    layer3_switch_gwdi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, gwdi, irq));
}

/* Each port forwards only to the CPU */
static layer3_switch_extended_cfg_t eswm_ext_cfg = {
	.p_ether_phy_instances = {NULL, NULL},
	.fowarding_target_port_masks = {LAYER3_SWITCH_PORT_BITMASK_PORT2,
					LAYER3_SWITCH_PORT_BITMASK_PORT2},
	.gptp_timer_numbers = {DT_FOREACH_CHILD_STATUS_OKAY(ESWM_SWITCH_NODE,
							     ESWM_GPTP_TIMER_NUMBER_ENTRY)},
};
static layer3_switch_instance_ctrl_t eswm_ctrl;
static ether_switch_cfg_t eswm_cfg = {
	.channel = 0,
	.p_extend = &eswm_ext_cfg,
	.p_callback = eswm_switch_cb,
	.p_context = (void *)DEVICE_DT_INST_GET(0),
	.irq = DT_INST_IRQ_BY_NAME(0, gwdi, irq),
	.ipl = DT_INST_IRQ_BY_NAME(0, gwdi, priority),
};
static ether_switch_instance_t eswm_inst = {
	.p_ctrl = (ether_switch_ctrl_t *)&eswm_ctrl,
	.p_cfg = (ether_switch_cfg_t *)&eswm_cfg,
	.p_api = (ether_switch_api_t *)&g_ether_switch_on_layer3_switch,
};
static struct eswm_dsa_data eswm_dsa_data = {
	.fsp_ctrl = &eswm_ctrl,
	.fsp_cfg = &eswm_cfg,
};
static const struct eswm_dsa_config eswm_dsa_config = {
	.gwcaclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, gwcaclk)),
	.pclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, pclk)),
	.eswclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, eswclk)),
	.ethphyclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, ethphyclk)),
	.ethphyclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(0, ethphyclk, mstp),
	.ethphyclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(0, ethphyclk, stop_bit),
	.ethphyclk_enable = DT_INST_PROP(0, renesas_ethphyclk_output),
	.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.mcfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0),
	.en_irq = eswm_dsa_init_irq,
};
ETH_NET_DEVICE_DT_INST_DEFINE(0, eswm_dsa_init, NULL, &eswm_dsa_data, &eswm_dsa_config,
			      CONFIG_ETH_INIT_PRIORITY, &eswm_dsa_conduit_api, NET_ETH_MTU);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_ra_eswm_switch

/* GWCA descriptor parts, see the GWDCCi.EDE and GWDCCi.ETS descriptor formats */
#define ESWM_DESC_BASIC_SIZE 8U /* DIE, DT, DS, INFO0, PTR */
#define ESWM_DESC_EXT_SIZE   8U /* INFO1, GWDCCi.EDE = 1 */
#define ESWM_DESC_TS_SIZE    8U /* TSNS, TSS, GWDCCi.ETS = 1 */

/* LAYER3_SWITCH_CFG_GPTP_ENABLE makes descriptors 24 bytes, which needs GWDCCi.ETS on all queues */
#if defined(CONFIG_ETH_RENESAS_RA_PTP)
#define ESWM_TIMESTAMP_STORAGE LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_ENABLE
#define ESWM_DESC_SIZE         (ESWM_DESC_BASIC_SIZE + ESWM_DESC_EXT_SIZE + ESWM_DESC_TS_SIZE)
#else
#define ESWM_TIMESTAMP_STORAGE LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE
#define ESWM_DESC_SIZE         (ESWM_DESC_BASIC_SIZE + ESWM_DESC_EXT_SIZE)
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

BUILD_ASSERT(sizeof(layer3_switch_descriptor_t) == ESWM_DESC_SIZE,
	     "descriptor size does not match the GWDCCi.ETS setting");

#define ESWM_USER_PORT_COUNT_ONE(port) ESWM_PORT_IS_USER(port)

BUILD_ASSERT((ESWM_TX_QUEUE_NUM + ESWM_RX_QUEUE_NUM) *
			     (DT_FOREACH_CHILD_STATUS_OKAY_SEP(ESWM_SWITCH_NODE,
							       ESWM_USER_PORT_COUNT_ONE, (+))) <=
		     LAYER3_SWITCH_CFG_AVAILABLE_QUEUE_NUM,
	     "not enough descriptor queues for the user ports");

#if defined(CONFIG_ETH_RENESAS_RA_PTP)
/* TX timestamp queue length, including the LINKFIX descriptor */
#define ESWM_TS_QUEUE_LEN 8

/* TX timestamp descriptors per gPTP timer, shared by the ports using that timer */
static layer3_switch_ts_reception_process_descriptor_t
	eswm_ts_desc_array[BSP_FEATURE_ESWM_GPTP_TIMER_NUM][ESWM_TS_QUEUE_LEN] __eswm_desc;

BUILD_ASSERT((sizeof(eswm_ts_desc_array[0]) % 32U) == 0U,
	     "TX timestamp queue size must be a multiple of 32 bytes");

#define ESWM_PTP_CLOCK_DEV(port)                                                                   \
	COND_CODE_1(DT_NODE_HAS_PROP(port, ptp_clock),                                             \
		    (DEVICE_DT_GET_OR_NULL(ESWM_PTP_CLOCK_NODE(port))), (NULL))
#endif /* CONFIG_ETH_RENESAS_RA_PTP */

/* ptp-clock must point to a gPTP timer of this ESWM */
#define ESWM_PTP_CLOCK_CHECK_COMPAT(port)                                                          \
	COND_CODE_1(DT_NODE_HAS_COMPAT(ESWM_PTP_CLOCK_NODE(port), renesas_ra_gptp_timer),          \
		    (BUILD_ASSERT(DT_SAME_NODE(DT_PARENT(DT_PARENT(ESWM_PTP_CLOCK_NODE(port))),    \
					       ESWM_NODE),                                         \
				  "ptp-clock must be a gPTP timer of the same ESWM");),            \
		    (BUILD_ASSERT(0, "ptp-clock must point to a renesas,ra-gptp-timer node");))

#define ESWM_PTP_CLOCK_CHECK(port)                                                                 \
	COND_CODE_1(DT_NODE_HAS_PROP(port, ptp_clock), (ESWM_PTP_CLOCK_CHECK_COMPAT(port)), ())

/* Buffers */
#define ESWM_TX_BUF_DECLARE(idx, port)                                                             \
	static uint8_t eswm_##port##_tx_buf##idx[ESWM_BUF_SIZE] __eswm_buf;
#define ESWM_RX_BUF_DECLARE(idx, port)                                                             \
	static uint8_t eswm_##port##_rx_buf##idx[ESWM_BUF_SIZE] __eswm_buf;
#define ESWM_TX_BUF_PTR(idx, port) &eswm_##port##_tx_buf##idx[0]
#define ESWM_RX_BUF_PTR(idx, port) &eswm_##port##_rx_buf##idx[0]

/* Descriptors */
#define ESWM_TX_DESC_DECLARE(idx, port)                                                            \
	static layer3_switch_descriptor_t eswm_##port##_tx_desc##idx[ESWM_TX_BUF_NUM] __eswm_desc;
#define ESWM_RX_DESC_DECLARE(idx, port)                                                            \
	static layer3_switch_descriptor_t eswm_##port##_rx_desc##idx[ESWM_RX_BUF_NUM] __eswm_desc;

/* Queues */
#define ESWM_QUEUE_INFO(port, len, desc, queue_type)                                               \
	{                                                                                          \
		.queue_cfg = {                                                                     \
			.array_length = (len),                                                     \
			.p_descriptor_array = (desc),                                              \
			.p_ts_descriptor_array = NULL,                                             \
			.ports = BIT(DT_REG_ADDR(port)),                                           \
			.type = (queue_type),                                                      \
			.write_back_mode = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,                     \
			.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,            \
			.rx_timestamp_storage = ESWM_TIMESTAMP_STORAGE,                            \
		},                                                                                 \
	}
#define ESWM_TX_QUEUE_INFO(idx, port)                                                              \
	ESWM_QUEUE_INFO(port, ESWM_TX_QUEUE_LEN, eswm_##port##_tx_desc##idx,                       \
			LAYER3_SWITCH_QUEUE_TYPE_TX)
#define ESWM_RX_QUEUE_INFO(idx, port)                                                              \
	ESWM_QUEUE_INFO(port, ESWM_RX_QUEUE_LEN, eswm_##port##_rx_desc##idx,                       \
			LAYER3_SWITCH_QUEUE_TYPE_RX)

/* For a TX timestamp queue, FSP ignores type and p_descriptor_array */
#define ESWM_TS_QUEUE_DECLARE(port)                                                                \
	BUILD_ASSERT(ESWM_PORT_PTP_TIMER(port) < BSP_FEATURE_ESWM_GPTP_TIMER_NUM,                  \
		     "ptp-clock is not a gPTP timer of this device");                              \
	static rmac_queue_info_t eswm_##port##_ts_queue = {                                        \
		.queue_cfg = {                                                                     \
			.array_length = ESWM_TS_QUEUE_LEN,                                         \
			.p_descriptor_array = NULL,                                                \
			.p_ts_descriptor_array = eswm_ts_desc_array[ESWM_PORT_PTP_TIMER(port)],    \
			.ports = BIT(DT_REG_ADDR(port)),                                           \
			.type = LAYER3_SWITCH_QUEUE_TYPE_RX,                                       \
			.descriptor_format = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_TX_TIMESTAMP,        \
		},                                                                                 \
	};

#define ESWM_USER_PORT_DEFINE(port)                                                                \
	BUILD_ASSERT(DT_REG_ADDR(port) < BSP_FEATURE_ETHER_NUM_CHANNELS,                           \
		     "user port reg must be an Ethernet port number");                             \
	BUILD_ASSERT(DT_NODE_HAS_PROP(port, phy_handle), "user port needs phy-handle");            \
	BUILD_ASSERT(DT_NODE_HAS_PROP(port, phy_connection_type),                                  \
		     "user port needs phy-connection-type");                                       \
	ESWM_PTP_CLOCK_CHECK(port)                                                                 \
                                                                                                   \
	PINCTRL_DT_DEFINE(port);                                                                   \
	LISTIFY(ESWM_RX_BUF_NUM, ESWM_RX_BUF_DECLARE, (), port)                                    \
	LISTIFY(ESWM_TX_BUF_NUM, ESWM_TX_BUF_DECLARE, (), port)                                    \
	static uint8_t *eswm_##port##_buffers[] = {                                                \
		LISTIFY(ESWM_RX_BUF_NUM, ESWM_RX_BUF_PTR, (,), port),                              \
		LISTIFY(ESWM_TX_BUF_NUM, ESWM_TX_BUF_PTR, (,), port),                              \
	};                                                                                         \
                                                                                                   \
	LISTIFY(ESWM_RX_QUEUE_NUM, ESWM_RX_DESC_DECLARE, (), port)                                 \
	LISTIFY(ESWM_TX_QUEUE_NUM, ESWM_TX_DESC_DECLARE, (), port)                                 \
	static rmac_queue_info_t eswm_##port##_rx_queues[] = {                                     \
		LISTIFY(ESWM_RX_QUEUE_NUM, ESWM_RX_QUEUE_INFO, (,), port),                         \
	};                                                                                         \
	static rmac_queue_info_t eswm_##port##_tx_queues[] = {                                     \
		LISTIFY(ESWM_TX_QUEUE_NUM, ESWM_TX_QUEUE_INFO, (,), port),                         \
	};                                                                                         \
	static rmac_buffer_node_t eswm_##port##_buffer_nodes[ESWM_BUF_NUM];                        \
	IF_ENABLED(CONFIG_ETH_RENESAS_RA_PTP, (ESWM_TS_QUEUE_DECLARE(port)))                       \
                                                                                                   \
	static rmac_extended_cfg_t eswm_##port##_rmac_ext_cfg = {                                  \
		.p_ether_switch = &eswm_inst,                                                      \
		IF_ENABLED(CONFIG_ETH_RENESAS_RA_PTP, (.p_ts_queue = &eswm_##port##_ts_queue,))    \
		.tx_queue_num = ESWM_TX_QUEUE_NUM,                                                 \
		.rx_queue_num = ESWM_RX_QUEUE_NUM,                                                 \
		.p_tx_queue_list = eswm_##port##_tx_queues,                                        \
		.p_rx_queue_list = eswm_##port##_rx_queues,                                        \
		.p_buffer_node_list = eswm_##port##_buffer_nodes,                                  \
		.buffer_node_num = ESWM_BUF_NUM,                                                   \
		.rmpi_irq = FSP_INVALID_VECTOR,                                                    \
		.rmpi_ipl = BSP_IRQ_DISABLED,                                                      \
	};                                                                                         \
	static struct eswm_port eswm_##port;                                                       \
	static struct eswm_port_data eswm_##port##_data = {                                        \
		.tx_bufs = &eswm_##port##_buffers[ESWM_RX_BUF_NUM],                                \
		.tx_buf_num = ESWM_TX_BUF_NUM,                                                     \
		.fsp_cfg = {                                                                       \
			.channel = DT_REG_ADDR(port),                                              \
			.num_tx_descriptors = ESWM_TX_BUF_NUM,                                     \
			.num_rx_descriptors = ESWM_RX_BUF_NUM,                                     \
			.pp_ether_buffers = eswm_##port##_buffers,                                 \
			.ether_buffer_size = ESWM_BUF_SIZE,                                        \
			.padding = ETHER_PADDING_DISABLE,                                          \
			.zerocopy = ETHER_ZEROCOPY_ENABLE,                                         \
			.multicast = ETHER_MULTICAST_ENABLE,                                       \
			.promiscuous = ETHER_PROMISCUOUS_DISABLE,                                  \
			.flow_control = ETHER_FLOW_CONTROL_DISABLE,                                \
			.p_callback = eswm_rmac_cb,                                                \
			.p_context = &eswm_##port,                                                 \
			.p_extend = &eswm_##port##_rmac_ext_cfg,                                   \
		},                                                                                 \
	};                                                                                         \
	static const struct eswm_port_config eswm_##port##_config = {                              \
		.pin_cfg = PINCTRL_DT_DEV_CONFIG_GET(port),                                        \
		.phy_conn_type = DT_ENUM_IDX(port, phy_connection_type),                           \
		IF_ENABLED(CONFIG_ETH_RENESAS_RA_PTP,                                              \
			   (.ptp_clock = ESWM_PTP_CLOCK_DEV(port),                                 \
			    .ptp_timer = ESWM_PORT_PTP_TIMER(port),))                              \
	};                                                                                         \
	static struct eswm_port eswm_##port = {                                                    \
		.config = &eswm_##port##_config,                                                   \
		.data = &eswm_##port##_data,                                                       \
	};

#define ESWM_CPU_PORT_CHECK(port)                                                                  \
	BUILD_ASSERT(DT_REG_ADDR(port) == BSP_FEATURE_ESWM_GWCA_PORT,                              \
		     "CPU port reg must be the ESWM CPU port number");                             \
	BUILD_ASSERT(DT_SAME_NODE(DT_PHANDLE(port, ethernet), ESWM_NODE),                          \
		     "CPU port ethernet must point to the parent ESWM");

#define ESWM_DSA_PORT_INIT(port, n)                                                                \
	COND_CODE_1(ESWM_PORT_IS_CPU(port), (ESWM_CPU_PORT_CHECK(port)),                           \
		    (ESWM_USER_PORT_DEFINE(port)))                                                 \
	/* The CPU port has no tag, the port number travels in the descriptor */                   \
	BUILD_ASSERT(DT_PROP_OR(port, dsa_tag_protocol, DSA_TAG_PROTO_NOTAG) ==                    \
			     DSA_TAG_PROTO_NOTAG,                                                  \
		     "only DSA_TAG_PROTO_NOTAG is supported");                                     \
	static struct dsa_port_config eswm_##port##_dsa_config = {                                 \
		.mcfg = NET_ETH_MAC_DT_CONFIG_INIT(port),                                          \
		.port_idx = DT_REG_ADDR(port),                                                     \
		.phy_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, phy_handle)),                    \
		.phy_mode = DT_PROP_OR(port, phy_connection_type, ""),                             \
		.tag_proto = DSA_TAG_PROTO_NOTAG,                                                  \
		.ethernet_connection = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, ethernet)),          \
		IF_ENABLED(CONFIG_ETH_RENESAS_RA_PTP, (.ptp_clock = ESWM_PTP_CLOCK_DEV(port),))    \
		.prv_config = COND_CODE_1(ESWM_PORT_IS_CPU(port), (NULL), (&eswm_##port)),         \
	};                                                                                         \
	DSA_PORT_INST_INIT(port, n, &eswm_##port##_dsa_config)

#define ESWM_DSA_SWITCH_INIT(n) DSA_SWITCH_INST_INIT(n, &eswm_dsa_api, NULL, ESWM_DSA_PORT_INIT)

DT_INST_FOREACH_STATUS_OKAY(ESWM_DSA_SWITCH_INIT)
