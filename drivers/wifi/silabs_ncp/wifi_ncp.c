/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wi-Fi station running on a Silicon Labs network co-processor.
 * Ethernet frames and the commands of the network processor are exchanged on
 * two CPC endpoints of the "silabs,ncp" parent device.
 */

#define DT_DRV_COMPAT silabs_ncp_wifi

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/silabs_ncp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/random/random.h>

#include "wifi_ncp.h"
#include "wifi_ncp_sta.h"
#include "wifi_ncp_supp.h"

LOG_MODULE_REGISTER(wifi_ncp, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_NCP_CONNECT_TIMEOUT K_SECONDS(5)

BUILD_ASSERT(CONFIG_WIFI_SILABS_NCP_INIT_PRIORITY < CONFIG_NET_INIT_PRIO,
	     "The driver must be initialized before the network interfaces");
/* The receive slots are handed to CPC as the buffer descriptor followed by
 * its data
 */
BUILD_ASSERT(offsetof(struct wifi_ncp_data_rx_slot, data) ==
	     offsetof(struct wifi_ncp_data_rx_slot, buf) + sizeof(sl_cpc_buf_t));
BUILD_ASSERT(offsetof(struct wifi_ncp_mgmt_rx_slot, data) ==
	     offsetof(struct wifi_ncp_mgmt_rx_slot, buf) + sizeof(sl_cpc_buf_t));

/* Events common to both endpoints. The endpoints are connected once for all
 * during the device initialization: losing one is fatal.
 */
static void wifi_ncp_ep_link_event(const struct device *dev, sl_cpc_ep_t *ep,
				   sl_cpc_ep_event_type_t type, const sl_cpc_ep_event_t *event)
{
	struct wifi_ncp_data *data = dev->data;

	switch (type) {
	case SL_CPC_EP_EVENT_CONNECTED:
		LOG_INF("%s: endpoint %u connected", dev->name, sl_cpc_ep_get_id(ep));
		k_sem_give(&data->connected);
		break;
	case SL_CPC_EP_EVENT_CLOSED:
		LOG_ERR("%s: endpoint %u closed", dev->name, sl_cpc_ep_get_id(ep));
		net_if_carrier_off(data->iface);
		break;
	case SL_CPC_EP_EVENT_ERROR:
		LOG_ERR("%s: endpoint %u error: %d", dev->name, sl_cpc_ep_get_id(ep),
			event->error.status);
		net_if_carrier_off(data->iface);
		break;
	default:
		break;
	}
}

/* Handle the received management frames in order, on the work queue, then
 * give the buffers back to CPC.
 */
static void wifi_ncp_mgmt_rx_work(struct k_work *work)
{
	struct wifi_ncp_data *data = CONTAINER_OF(work, struct wifi_ncp_data, mgmt_rx_work);
	const struct device *dev = data->dev;
	struct wifi_ncp_mgmt_rx_slot *slot;
	int status;

	while ((slot = k_fifo_get(&data->mgmt_rx_pending, K_NO_WAIT))) {
		wifi_ncp_sta_rx(dev, slot->buf.ptr, slot->buf.len);

		status = sl_cpc_ep_push_recv_buf(&data->mgmt_ep, &slot->buf);
		if (status) {
			LOG_ERR("%s: mgmt: buffer lost: %d", dev->name, status);
		}
	}
}

int wifi_ncp_nwp_send(const struct device *dev, const struct wifi_ncp_nwp_hdr *hdr,
		      const void *head, size_t head_len, const void *body, size_t body_len)
{
	const struct wifi_ncp_config *cfg = dev->config;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_mgmt_tx_slot *slot;
	size_t len = sizeof(struct wifi_ncp_nwp_hdr) + head_len + body_len;
	int status;
	int ret;

	if (len > WIFI_NCP_MGMT_MTU) {
		return -EMSGSIZE;
	}

	ret = k_mem_slab_alloc(cfg->mgmt_tx_slab, (void **)&slot, WIFI_NCP_CONNECT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s: no management TX slot", dev->name);
		return -ENOMEM;
	}

	memcpy(slot->data, hdr, sizeof(struct wifi_ncp_nwp_hdr));
	if (head_len > 0) {
		memcpy(slot->data + sizeof(struct wifi_ncp_nwp_hdr), head, head_len);
	}
	if (body_len > 0) {
		memcpy(slot->data + sizeof(struct wifi_ncp_nwp_hdr) + head_len, body, body_len);
	}
	sl_cpc_buf_init(&slot->buf, slot->data, len);
	memset(&slot->frame, 0, sizeof(slot->frame));
	status = sl_cpc_ep_send(&data->mgmt_ep, &slot->buf, &slot->frame, NULL);
	if (status) {
		LOG_ERR("%s: mgmt: send failed: %d", dev->name, status);
		k_mem_slab_free(cfg->mgmt_tx_slab, slot);
		return -EIO;
	}

	return 0;
}

/* Runs in the CPC context: keep it short */
static void wifi_ncp_mgmt_ep_event(sl_cpc_ep_t *ep, sl_cpc_ep_event_type_t type,
				   const sl_cpc_ep_event_t *event, void *arg)
{
	const struct device *dev = arg;
	const struct wifi_ncp_config *cfg = dev->config;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_mgmt_rx_slot *rx;
	struct wifi_ncp_mgmt_tx_slot *tx;

	switch (type) {
	case SL_CPC_EP_EVENT_RECV:
		LOG_DBG("%s: mgmt: %u bytes received", dev->name, event->recv.buf->len);
		rx = CONTAINER_OF(event->recv.buf, struct wifi_ncp_mgmt_rx_slot, buf);
		k_fifo_put(&data->mgmt_rx_pending, rx);
		k_work_submit_to_queue(&data->wq, &data->mgmt_rx_work);
		break;
	case SL_CPC_EP_EVENT_SEND_DONE:
		if (event->send_done.status) {
			LOG_ERR("%s: mgmt: send failed: %d",
				dev->name, event->send_done.status);
		}
		tx = CONTAINER_OF(event->send_done.frame, struct wifi_ncp_mgmt_tx_slot, frame);
		k_mem_slab_free(cfg->mgmt_tx_slab, tx);
		break;
	default:
		wifi_ncp_ep_link_event(dev, ep, type, event);
		break;
	}
}

/* Hand a received Ethernet frame to the network stack. The CPC buffer is
 * given back to the endpoint whatever the outcome.
 */
static void wifi_ncp_data_recv(const struct device *dev, const sl_cpc_buf_t *buf)
{
	struct wifi_ncp_data *data = dev->data;
	const struct wifi_ncp_nwp_hdr *hdr = buf->ptr;
	size_t len;
	struct net_pkt *pkt;

	if (buf->len < sizeof(struct wifi_ncp_nwp_hdr) ||
	    hdr->command_id != WIFI_NCP_NWP_CMD_DATA) {
		LOG_WRN("%s: unexpected data frame (%u bytes), dropping", dev->name, buf->len);
		return;
	}
	len = buf->len - sizeof(struct wifi_ncp_nwp_hdr);

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_DBG("%s: no packet buffer, dropping %u bytes", dev->name, len);
		return;
	}

	if (net_pkt_write(pkt, hdr + 1, len) < 0) {
		LOG_ERR("%s: packet write failed", dev->name);
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(data->iface, pkt) < 0) {
		LOG_DBG("%s: packet rejected by the stack", dev->name);
		net_pkt_unref(pkt);
	}
}

static void wifi_ncp_data_ep_event(sl_cpc_ep_t *ep, sl_cpc_ep_event_type_t type,
				   const sl_cpc_ep_event_t *event, void *arg)
{
	const struct device *dev = arg;
	const struct wifi_ncp_config *cfg = dev->config;
	struct wifi_ncp_data_tx_slot *tx;
	int status;

	switch (type) {
	case SL_CPC_EP_EVENT_RECV:
		wifi_ncp_data_recv(dev, event->recv.buf);
		status = sl_cpc_ep_push_recv_buf(ep, event->recv.buf);
		if (status) {
			LOG_ERR("%s: data: buffer lost: %d", dev->name, status);
		}
		break;
	case SL_CPC_EP_EVENT_SEND_DONE:
		if (event->send_done.status) {
			LOG_ERR("%s: data: send failed: %d",
				dev->name, event->send_done.status);
		}
		tx = CONTAINER_OF(event->send_done.frame, struct wifi_ncp_data_tx_slot, frame);
		k_mem_slab_free(cfg->data_tx_slab, tx);
		break;
	default:
		wifi_ncp_ep_link_event(dev, ep, type, event);
		break;
	}
}

static int wifi_ncp_ep_open(const struct device *dev, sl_cpc_ep_t *ep, uint8_t id, uint16_t rx_size,
			    sl_cpc_buf_t *bufs, size_t stride, size_t count,
			    sl_cpc_ep_event_cb_t *cb)
{
	const struct wifi_ncp_config *cfg = dev->config;
	int status;

	status = sl_cpc_ep_init(ep, id, rx_size, cb, (void *)dev);
	if (status) {
		LOG_ERR("%s: endpoint %u init failed: %d", dev->name, id, status);
		return -EIO;
	}

	for (size_t i = 0; i < count; i++) {
		sl_cpc_buf_t *buf = (sl_cpc_buf_t *)((uint8_t *)bufs + i * stride);

		sl_cpc_buf_init(buf, buf + 1, rx_size);
		status = sl_cpc_ep_push_recv_buf(ep, buf);
		if (status) {
			LOG_ERR("%s: endpoint %u buffer rejected: %d",
				dev->name, id, status);
			sl_cpc_ep_deinit(ep);
			return -EIO;
		}
	}

	status = sl_cpc_ep_connect(ep, mfd_silabs_ncp_cpc_bus(cfg->parent));
	if (status) {
		LOG_ERR("%s: endpoint %u connect failed: %d",
			dev->name, id, status);
		sl_cpc_ep_deinit(ep);
		return -EIO;
	}

	return 0;
}

static void wifi_ncp_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_ncp_config *cfg = dev->config;
	struct wifi_ncp_data *data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	uint8_t mac[NET_ETH_ADDR_LEN];

	data->iface = iface;

	/* The address of the co-processor, else the devicetree one, else a
	 * locally administered one
	 */
	static const uint8_t no_mac[NET_ETH_ADDR_LEN];

	if (memcmp(data->mac, no_mac, sizeof(no_mac))) {
		memcpy(mac, data->mac, sizeof(mac));
	} else if (net_eth_mac_load(&cfg->mac, mac) < 0) {
		sys_rand_get(mac, sizeof(mac));
		mac[0] &= ~0x01U;
		mac[0] |= 0x02U;
	}
	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	ethernet_init(iface);

	/* The co-processor is reachable, there is no L2 until associated */
	net_if_carrier_on(iface);
	net_if_dormant_on(iface);
}

static enum ethernet_hw_caps wifi_ncp_get_capabilities(const struct device *dev,
						       struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return 0;
}

/* The Ethernet frame is copied behind its descriptor: CPC needs a
 * contiguous, aligned payload that stays valid until the send-done event,
 * while the packet is released by the caller on return.
 */
static int wifi_ncp_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct wifi_ncp_config *cfg = dev->config;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_data_tx_slot *slot;
	size_t len = net_pkt_get_len(pkt);
	int status;
	int ret;

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		return -EMSGSIZE;
	}

	ret = k_mem_slab_alloc(cfg->data_tx_slab, (void **)&slot, K_NO_WAIT);
	if (ret < 0) {
		LOG_DBG("%s: no TX slot", dev->name);
		return -ENOMEM;
	}

	memset(slot->data, 0, sizeof(struct wifi_ncp_nwp_hdr));
	net_pkt_cursor_init(pkt);
	ret = net_pkt_read(pkt, slot->data + sizeof(struct wifi_ncp_nwp_hdr), len);
	if (ret < 0) {
		k_mem_slab_free(cfg->data_tx_slab, slot);
		return ret;
	}

	sl_cpc_buf_init(&slot->buf, slot->data, sizeof(struct wifi_ncp_nwp_hdr) + len);
	memset(&slot->frame, 0, sizeof(slot->frame));
	status = sl_cpc_ep_send(&data->data_ep, &slot->buf, &slot->frame, NULL);
	if (status) {
		LOG_ERR("%s: send failed: %d", dev->name, status);
		k_mem_slab_free(cfg->data_tx_slab, slot);
		return -EIO;
	}

	return 0;
}

static uint32_t wifi_ncp_get_iface_caps(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return BIT(WIFI_TYPE_STA);
}

static int wifi_ncp_iface_status(const struct device *dev, struct net_if *iface,
				 struct wifi_iface_status *status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	memset(status, 0, sizeof(*status));
	status->state = WIFI_STATE_DISCONNECTED;
	status->iface_mode = WIFI_MODE_INFRA;
	status->band = WIFI_FREQ_BAND_UNKNOWN;

	return 0;
}

/* Open both endpoints and wait for their connection: the device is ready only
 * when the co-processor answers.
 */
static int wifi_ncp_init(const struct device *dev)
{
	const struct wifi_ncp_config *cfg = dev->config;
	struct wifi_ncp_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_sem_init(&data->connected, 0, 2);
	k_fifo_init(&data->mgmt_rx_pending);
	k_work_init(&data->mgmt_rx_work, wifi_ncp_mgmt_rx_work);
	k_mutex_init(&data->cmd.lock);
	k_sem_init(&data->cmd.done, 0, 1);
	k_mutex_init(&data->sta.lock);

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("%s: co-processor not ready", dev->name);
		return -ENODEV;
	}

	k_work_queue_init(&data->wq);
	k_work_queue_start(&data->wq, data->wq_stack, K_KERNEL_STACK_SIZEOF(data->wq_stack),
			   K_PRIO_PREEMPT(CONFIG_WIFI_SILABS_NCP_WQ_PRIORITY), NULL);

	ret = wifi_ncp_ep_open(dev, &data->mgmt_ep, WIFI_NCP_MGMT_EP_ID, WIFI_NCP_MGMT_MTU,
			       &data->mgmt_rx[0].buf, sizeof(data->mgmt_rx[0]),
			       ARRAY_SIZE(data->mgmt_rx), wifi_ncp_mgmt_ep_event);
	if (ret < 0) {
		return ret;
	}

	ret = wifi_ncp_ep_open(dev, &data->data_ep, WIFI_NCP_DATA_EP_ID, WIFI_NCP_DATA_MTU,
			       &data->data_rx[0].buf, sizeof(data->data_rx[0]),
			       ARRAY_SIZE(data->data_rx), wifi_ncp_data_ep_event);
	if (ret < 0) {
		sl_cpc_ep_close(&data->mgmt_ep);
		return ret;
	}

	for (int i = 0; i < 2; i++) {
		if (k_sem_take(&data->connected, WIFI_NCP_CONNECT_TIMEOUT)) {
			LOG_ERR("%s: endpoint connection timed out", dev->name);
			sl_cpc_ep_close(&data->data_ep);
			sl_cpc_ep_close(&data->mgmt_ep);
			return -ETIMEDOUT;
		}
	}

	return wifi_ncp_sta_init(dev);
}

/* Operations the Zephyr supplicant calls on the driver */
static const struct zep_wpa_supp_dev_ops wifi_ncp_supp_ops = {
	.init = wifi_ncp_supp_init,
	.deinit = wifi_ncp_supp_deinit,
	.get_capa = wifi_ncp_supp_get_capa,
	.get_wiphy = wifi_ncp_supp_get_wiphy,
	.scan2 = wifi_ncp_sta_scan,
	.scan_abort = wifi_ncp_sta_scan_abort,
	.get_scan_results2 = wifi_ncp_sta_get_scan_results,
	.associate = wifi_ncp_sta_associate,
	.deauthenticate = wifi_ncp_sta_deauthenticate,
	.set_key = wifi_ncp_sta_set_key,
	.set_supp_port = wifi_ncp_sta_set_supp_port,
	.tx_control_port = wifi_ncp_sta_tx_control_port,
	.signal_poll = wifi_ncp_sta_signal_poll,
	.get_conn_info = wifi_ncp_sta_get_conn_info,
};

static const struct wifi_mgmt_ops wifi_ncp_mgmt_ops = {
	.iface_status = wifi_ncp_iface_status,
	.get_iface_caps = wifi_ncp_get_iface_caps,
};

static const struct net_wifi_mgmt_offload wifi_ncp_api = {
	.wifi_iface.iface_api.init = wifi_ncp_iface_init,
	.wifi_iface.get_capabilities = wifi_ncp_get_capabilities,
	.wifi_iface.send = wifi_ncp_send,
	.wifi_mgmt_api = &wifi_ncp_mgmt_ops,
	.wifi_drv_ops = &wifi_ncp_supp_ops,
};

#define WIFI_NCP_DEFINE(inst)                                                                      \
	K_MEM_SLAB_DEFINE_STATIC(wifi_ncp_data_tx_slab##inst,                                      \
				 sizeof(struct wifi_ncp_data_tx_slot),                             \
				 CONFIG_WIFI_SILABS_NCP_DATA_TX_COUNT, SL_CPC_BUF_MIN_ALIGNMENT);  \
	K_MEM_SLAB_DEFINE_STATIC(wifi_ncp_mgmt_tx_slab##inst,                                      \
				 sizeof(struct wifi_ncp_mgmt_tx_slot),                             \
				 CONFIG_WIFI_SILABS_NCP_MGMT_TX_COUNT, SL_CPC_BUF_MIN_ALIGNMENT);  \
	static struct wifi_ncp_data wifi_ncp_data##inst;                                           \
	static const struct wifi_ncp_config wifi_ncp_config##inst = {                              \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.mac = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                      \
		.data_tx_slab = &wifi_ncp_data_tx_slab##inst,                                      \
		.mgmt_tx_slab = &wifi_ncp_mgmt_tx_slab##inst,                                      \
	};                                                                                         \
	NET_DEVICE_DT_INST_DEFINE(inst, wifi_ncp_init, NULL, &wifi_ncp_data##inst,                 \
				  &wifi_ncp_config##inst, CONFIG_WIFI_SILABS_NCP_INIT_PRIORITY,    \
				  &wifi_ncp_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),    \
				  NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(WIFI_NCP_DEFINE)
