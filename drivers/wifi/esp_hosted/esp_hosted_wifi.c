/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb_encode.h>
#include <pb_decode.h>
#include <esp_hosted_proto.pb.h>

#include <esp_hosted_wifi.h>
#include <esp_hosted_hal.h>
#include <esp_hosted_util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted, CONFIG_WIFI_LOG_LEVEL);

static struct k_thread esp_hosted_event_thread;
K_THREAD_STACK_DEFINE(esp_hosted_event_stack, CONFIG_WIFI_ESP_HOSTED_EVENT_TASK_STACK_SIZE);

K_MSGQ_DEFINE(esp_hosted_msgq, sizeof(CtrlMsg), 8, 4);

static esp_hosted_config_t esp_hosted_config = {
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
	.dataready_gpio = GPIO_DT_SPEC_INST_GET(0, dataready_gpios),
	.handshake_gpio = GPIO_DT_SPEC_INST_GET(0, handshake_gpios),
	.spi_bus = SPI_DT_SPEC_INST_GET(0, ESP_HOSTED_SPI_CONFIG)};

static esp_hosted_data_t esp_hosted_data = {0};

static int esp_hosted_recv(struct net_if *, void *, size_t);

static size_t esp_hosted_get_iface(const struct device *dev)
{
	return (strcmp(dev->name, "sta_if") == 0) ? ESP_HOSTED_STA_IF : ESP_HOSTED_SAP_IF;
}

static int esp_hosted_request(const struct device *dev, CtrlMsgId msg_id, CtrlMsg *ctrl_msg,
			      uint32_t timeout)
{
	size_t payload_size = 0;
	esp_frame_t frame = {0};
	esp_hosted_data_t *data = (esp_hosted_data_t *)dev->data;

	/* Init control message */
	ctrl_msg->msg_type = CtrlMsgType_Req;
	ctrl_msg->msg_id = msg_id;
	ctrl_msg->which_payload = msg_id;

	/* Get packed protobuf size */
	pb_get_encoded_size(&payload_size, CtrlMsg_fields, ctrl_msg);

	if ((payload_size + TLV_HEADER_SIZE) > ESP_FRAME_MAX_PAYLOAD) {
		LOG_ERR("payload size > max payload %d", msg_id);
		return -1;
	}

	/* Create frame. */
	frame.if_type = ESP_HOSTED_SERIAL_IF;
	frame.len = payload_size + TLV_HEADER_SIZE;
	frame.offset = ESP_FRAME_HEADER_SIZE;
	frame.seq_num = data->seq_num++;

	frame.ep_type = TLV_HEADER_TYPE_EP;
	frame.ep_length = 8;
	memcpy(frame.ep_value, TLV_HEADER_EP_RESP, 8);
	frame.data_type = TLV_HEADER_TYPE_DATA;
	frame.data_length = payload_size;

	pb_ostream_t stream = pb_ostream_from_buffer(frame.data_value, frame.data_length);

	if (!pb_encode(&stream, CtrlMsg_fields, ctrl_msg)) {
		LOG_ERR("failed to encode protobuf");
		return -1;
	}

	/* Update frame checksum and send the frame. */
	frame.checksum = esp_hosted_frame_checksum(&frame);
	if (esp_hosted_hal_spi_transfer(dev, &frame, NULL, ESP_FRAME_SIZE_ROUND(frame))) {
		LOG_ERR("request %d failed", msg_id);
		return -1;
	}
	return 0;
}

static int esp_hosted_response(const struct device *dev, CtrlMsgId msg_id, CtrlMsg *ctrl_msg,
			       uint32_t timeout)
{
	if (k_msgq_get(&esp_hosted_msgq, ctrl_msg, K_MSEC(timeout))) {
		if (timeout == 0) {
			return 0;
		}
		LOG_ERR("failed to receive response for %d", msg_id);
		return -1;
	}

	if (ctrl_msg->msg_id != msg_id) {
		LOG_ERR("expected id %u got id %u", msg_id, ctrl_msg->msg_id);
		return -1;
	}

	/* If message type is a response, check the response struct's return value. */
	if (ctrl_msg->msg_type == CtrlMsgType_Resp && esp_hosted_ctrl_response(ctrl_msg)) {
		LOG_ERR("response %d failed %d", msg_id, esp_hosted_ctrl_response(ctrl_msg));
		return -1;
	}

	return 0;
}

static int esp_hosted_ctrl(const struct device *dev, CtrlMsgId req_id, CtrlMsg *ctrl_msg,
			   uint32_t timeout)
{
	uint32_t res_id = (req_id - CtrlMsgId_Req_Base) + CtrlMsgId_Resp_Base;

	/* Wait for event message. */
	if (req_id > CtrlMsgId_Event_Base && req_id < CtrlMsgId_Event_Max &&
	    esp_hosted_response(dev, req_id, ctrl_msg, timeout)) {
		return -1;
	}

	/* Regular control message. */
	if (req_id > CtrlMsgId_Req_Base && req_id < CtrlMsgId_Req_Max &&
	    esp_hosted_request(dev, req_id, ctrl_msg, timeout)) {
		return -1;
	}

	if (res_id > CtrlMsgId_Resp_Base && res_id < CtrlMsgId_Resp_Max &&
	    esp_hosted_response(dev, res_id, ctrl_msg, timeout)) {
		return -1;
	}

	return 0;
}

static void esp_hosted_event_task(const struct device *dev, void *p2, void *p3)
{
	esp_hosted_data_t *data = dev->data;

	for (;; k_msleep(CONFIG_WIFI_ESP_HOSTED_EVENT_TASK_POLL_MS)) {
		esp_frame_t frame = {0};

		do {
			esp_frame_t ffrag = {0};

			if (((ESP_FRAME_SIZE * 2) - frame.len) < ESP_FRAME_SIZE) {
				/* This shouldn't happen, but if it did stop the thread. */
				LOG_ERR("spi buffer overflow offset: %d", frame.len);
				return;
			}

			if (esp_hosted_hal_spi_transfer(dev, NULL, &ffrag, ESP_FRAME_SIZE)) {
				goto restart;
			}

			if (!ESP_FRAME_CHECK_VALID(ffrag)) {
				goto restart;
			}

			if (ffrag.checksum != esp_hosted_frame_checksum(&ffrag)) {
				LOG_ERR("invalid checksum");
				goto restart;
			}

			if (frame.len == 0) {
				/* First frame */
				memcpy(&frame, &ffrag, sizeof(esp_frame_t));
			} else {
				/* Fragmented frame */
				if ((frame.seq_num + 1) != ffrag.seq_num) {
					LOG_ERR("unexpected fragmented frame sequence");
					goto restart;
				}

				/* Append the current fragment's payload. */
				memcpy(frame.payload + frame.len, ffrag.payload, ffrag.len);

				/* Update frame */
				frame.len += ffrag.len;
				frame.seq_num++;
				frame.flags = ffrag.flags;
				LOG_DBG("received fragmented frame, length: %d", frame.len);
			}
		} while (frame.flags & ESP_FRAME_FLAGS_FRAGMENT);

		switch (frame.if_type) {
		case ESP_HOSTED_SAP_IF:
		case ESP_HOSTED_STA_IF: {
			esp_hosted_recv(data->iface[frame.if_type], frame.payload, frame.len);
			continue;
		}
		case ESP_HOSTED_PRIV_IF: {
			if (frame.priv_pkt_type == ESP_PACKET_TYPE_EVENT &&
			    frame.event_type == ESP_PRIV_EVENT_INIT) {
				LOG_INF("chip id %d spi_mhz %d caps 0x%x", frame.event_data[2],
					frame.event_data[5], frame.event_data[8]);
			}
			continue;
		}
		case ESP_HOSTED_SERIAL_IF: /* Requires further processing */
			break;
		default:
			LOG_ERR("unexpected interface type %d", frame.if_type);
			continue;
		}

#if CONFIG_WIFI_ESP_HOSTED_DEBUG
		esp_hosted_frame_dump(&frame);
#endif

		CtrlMsg ctrl_msg = CtrlMsg_init_zero;
		pb_istream_t stream = pb_istream_from_buffer(frame.data_value, frame.data_length);

		if (!pb_decode(&stream, CtrlMsg_fields, &ctrl_msg)) {
			LOG_ERR("failed to decode protobuf");
			continue;
		}

		switch (ctrl_msg.msg_id) {
		case CtrlMsgId_Event_Heartbeat:
			data->last_hb_ms = k_uptime_get();
			continue;
		case CtrlMsgId_Event_StationDisconnectFromAP:
			data->state[ESP_HOSTED_STA_IF] = WIFI_STATE_DISCONNECTED;
			net_if_dormant_on(data->iface[ESP_HOSTED_STA_IF]);
			wifi_mgmt_raise_disconnect_result_event(data->iface[ESP_HOSTED_STA_IF], 0);
			continue;
		case CtrlMsgId_Event_StationDisconnectFromESPSoftAP: {
			struct wifi_ap_sta_info sta_info = {0};

			sta_info.link_mode = WIFI_LINK_MODE_UNKNOWN;
			sta_info.twt_capable = false;
			sta_info.mac_length = WIFI_MAC_ADDR_LEN;
			esp_hosted_str_to_mac(
				ctrl_msg.event_station_disconnect_from_ESP_SoftAP.mac.bytes,
				sta_info.mac);
			wifi_mgmt_raise_ap_sta_disconnected_event(data->iface[ESP_HOSTED_SAP_IF],
								  &sta_info);
			continue;
		}
		case CtrlMsgId_Resp_ConnectAP: {
			int ret = esp_hosted_ctrl_response(&ctrl_msg);

			if (!ret) {
				data->state[ESP_HOSTED_STA_IF] = WIFI_STATE_COMPLETED;
				net_if_dormant_off(data->iface[ESP_HOSTED_STA_IF]);
			}
			wifi_mgmt_raise_connect_result_event(data->iface[ESP_HOSTED_STA_IF], ret);
			continue;
		}
		default: /* Unhandled events/responses will be queued. */
			break;
		}

		/* Queue control message resp/event for further processing. */
		if (k_msgq_put(&esp_hosted_msgq, &ctrl_msg, K_FOREVER)) {
			LOG_ERR("Failed to enqueue message");
			return;
		}

		LOG_DBG("pushed msg_type %u msg_id %u", ctrl_msg.msg_type, ctrl_msg.msg_id);
restart:;
	}
}

static void esp_hosted_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	esp_hosted_data_t *data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct wifi_nm_instance *nm = wifi_nm_get_instance("nm");
	size_t itf = esp_hosted_get_iface(dev);

	data->iface[itf] = iface;
	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;

	/* Set mac address. */
	net_if_set_link_addr(iface, data->mac_addr[itf], 6, NET_LINK_ETHERNET);

	/* Configure interface */
	ethernet_init(iface);
	net_if_dormant_on(iface);
	net_if_carrier_on(iface);

	/* Register a managed interface. */
	wifi_nm_register_mgd_type_iface(
		nm, itf == ESP_HOSTED_STA_IF ? WIFI_TYPE_STA : WIFI_TYPE_SAP, iface);
}

static int esp_hosted_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;

	ctrl_msg.req_connect_ap.listen_interval = 0;
	ctrl_msg.req_connect_ap.is_wpa3_supported = (bool)params->wpa3_ent_mode;

	strncpy(ctrl_msg.req_connect_ap.pwd, params->psk, ESP_HOSTED_MAX_PASS_LEN);
	strncpy(ctrl_msg.req_connect_ap.ssid, params->ssid, ESP_HOSTED_MAX_SSID_LEN);
	esp_hosted_mac_to_str(params->bssid, ctrl_msg.req_connect_ap.bssid.bytes);

	if (esp_hosted_ctrl(dev, CtrlMsgId_Req_ConnectAP, &ctrl_msg, 0)) {
		return -EAGAIN;
	}
	return 0;
}

static int esp_hosted_disconnect(const struct device *dev)
{
	int ret = 0;
	esp_hosted_data_t *data = dev->data;
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;
	struct net_if *iface = net_if_lookup_by_dev(dev);

	if (esp_hosted_ctrl(dev, CtrlMsgId_Req_DisconnectAP, &ctrl_msg, ESP_HOSTED_SYNC_TIMEOUT)) {
		ret = -EIO;
	}

	wifi_mgmt_raise_disconnect_result_event(iface, ret);
	data->state[ESP_HOSTED_STA_IF] = WIFI_STATE_DISCONNECTED;
	return ret;
}

static int esp_hosted_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	int ret = 0;
	esp_hosted_data_t *data = dev->data;
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;

	uint32_t channel = params->channel;
	Ctrl_WifiSecProt security = esp_hosted_sec_to_prot(params->security);

	if (security == -1) {
		ret = -ENOTSUP;
		goto exit;
	}

	if (params->channel == WIFI_CHANNEL_ANY) {
		channel = CONFIG_WIFI_ESP_HOSTED_AP_CHANNEL_DEF;
	}

	ctrl_msg.req_start_softap.chnl = channel;
	ctrl_msg.req_start_softap.bw = Ctrl_WifiBw_HT40;
	ctrl_msg.req_start_softap.ssid_hidden = false;
	ctrl_msg.req_start_softap.sec_prot = security;
	ctrl_msg.req_start_softap.max_conn = CONFIG_WIFI_ESP_HOSTED_AP_CLIENTS_MAX;
	strncpy(ctrl_msg.req_start_softap.pwd, params->psk, ESP_HOSTED_MAX_PASS_LEN);
	strncpy(ctrl_msg.req_start_softap.ssid, params->ssid, ESP_HOSTED_MAX_SSID_LEN);

	/* Start Soft-AP mode. */
	if (esp_hosted_ctrl(dev, CtrlMsgId_Req_StartSoftAP, &ctrl_msg, ESP_HOSTED_SYNC_TIMEOUT)) {
		ret = -EIO;
	}

exit:
	if (!ret) {
		data->state[ESP_HOSTED_SAP_IF] = WIFI_STATE_COMPLETED;
		net_if_dormant_off(data->iface[ESP_HOSTED_SAP_IF]);
	}

	wifi_mgmt_raise_ap_enable_result_event(data->iface[ESP_HOSTED_SAP_IF], ret);
	return ret;
}

static int esp_hosted_ap_disable(const struct device *dev)
{
	int ret = 0;
	esp_hosted_data_t *data = dev->data;
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;

	if (esp_hosted_ctrl(dev, CtrlMsgId_Req_StopSoftAP, &ctrl_msg, ESP_HOSTED_SYNC_TIMEOUT)) {
		ret = -EIO;
	}

	data->state[ESP_HOSTED_SAP_IF] = WIFI_STATE_DISCONNECTED;
	wifi_mgmt_raise_ap_disable_result_event(data->iface[ESP_HOSTED_SAP_IF], ret);
	return ret;
}

static int esp_hosted_send(const struct device *dev, struct net_pkt *pkt)
{
	esp_frame_t frame = {0};
	size_t pkt_len = net_pkt_get_len(pkt);
	size_t itf = esp_hosted_get_iface(dev);
#if defined(CONFIG_NET_STATISTICS_WIFI)
	esp_hosted_data_t *data = dev->data;
#endif

	if (pkt_len > ESP_FRAME_MAX_PAYLOAD) {
		LOG_ERR("packet length > SPI buf length");
		return -ENOMEM;
	}

	/* Create frame. */
	frame.if_type = itf;
	frame.len = pkt_len;
	frame.offset = ESP_FRAME_HEADER_SIZE;

	/* Copy frame payload. */
	if (net_pkt_read(pkt, frame.payload, pkt_len) < 0) {
		LOG_ERR("net_pkt_read failed");
		return -EIO;
	}

	/* Update frame checksum and send the frame. */
	frame.checksum = esp_hosted_frame_checksum(&frame);
	if (esp_hosted_hal_spi_transfer(dev, &frame, NULL, ESP_FRAME_SIZE_ROUND(frame))) {
		LOG_ERR("spi_transfer failed");
		return -EIO;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.pkts.tx++;
	data->stats.bytes.sent += pkt_len;
#endif

	LOG_DBG("send eth frame size: %d", pkt_len);
	return 0;
}

static int esp_hosted_recv(struct net_if *iface, void *buf, size_t len)
{
	int ret = 0;
	struct net_pkt *pkt = NULL;
#if defined(CONFIG_NET_STATISTICS_WIFI)
	const struct device *dev = net_if_get_device(iface);
	esp_hosted_data_t *data = dev->data;
#endif

	if (!iface || !net_if_flag_is_set(iface, NET_IF_UP)) {
		return 0;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_MSEC(1000));
	if (pkt == NULL) {
		LOG_ERR("Failed to allocate net buffer");
		return -ENOMEM;
	}

	if (net_pkt_write(pkt, buf, len) < 0) {
		ret = -EIO;
		LOG_ERR("Failed to write to net buffer");
		goto error;
	}

	if (net_recv_data(iface, pkt) < 0) {
		ret = -EIO;
		LOG_ERR("Failed to push received data");
		goto error;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.pkts.rx++;
	data->stats.bytes.received += len;
#endif

	LOG_DBG("recv eth frame size: %d", len);
	return 0;

error:
	net_pkt_unref(pkt);
#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.errors.rx++;
#endif
	return ret;
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int esp_hosted_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	esp_hosted_data_t *data = dev->data;

	stats->bytes.received = data->stats.bytes.received;
	stats->bytes.sent = data->stats.bytes.sent;
	stats->pkts.rx = data->stats.pkts.rx;
	stats->pkts.tx = data->stats.pkts.tx;
	stats->errors.rx = data->stats.errors.rx;
	stats->errors.tx = data->stats.errors.tx;
	stats->broadcast.rx = data->stats.broadcast.rx;
	stats->broadcast.tx = data->stats.broadcast.tx;
	stats->multicast.rx = data->stats.multicast.rx;
	stats->multicast.tx = data->stats.multicast.tx;
	stats->sta_mgmt.beacons_rx = data->stats.sta_mgmt.beacons_rx;
	stats->sta_mgmt.beacons_miss = data->stats.sta_mgmt.beacons_miss;
	return 0;
}
#endif

static int esp_hosted_status(const struct device *dev, struct wifi_iface_status *status)
{
	esp_hosted_data_t *data = dev->data;
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;
	size_t itf = esp_hosted_get_iface(dev);

	status->state = data->state[itf];
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->wpa3_ent_type = WIFI_WPA3_ENTERPRISE_NA;
	status->mfp = WIFI_MFP_DISABLE;
	status->iface_mode = (itf == ESP_HOSTED_STA_IF) ? WIFI_MODE_INFRA : WIFI_MODE_AP;

	if (status->state == WIFI_STATE_DISCONNECTED) {
		return 0;
	}

	if (esp_hosted_ctrl(dev,
			    (itf == ESP_HOSTED_STA_IF) ? CtrlMsgId_Req_GetAPConfig
						       : CtrlMsgId_Req_GetSoftAPConfig,
			    &ctrl_msg, ESP_HOSTED_SYNC_TIMEOUT)) {
		return -EIO;
	}

	if (itf == ESP_HOSTED_STA_IF) {
		status->security = esp_hosted_prot_to_sec(ctrl_msg.resp_get_ap_config.sec_prot);
		status->channel = ctrl_msg.resp_get_ap_config.chnl;
		status->rssi = ctrl_msg.resp_get_ap_config.rssi;
		status->ssid_len = MIN(ctrl_msg.resp_get_ap_config.ssid.size, WIFI_SSID_MAX_LEN);
		memcpy(status->ssid, ctrl_msg.resp_get_ap_config.ssid.bytes, status->ssid_len);
		esp_hosted_str_to_mac(ctrl_msg.resp_get_ap_config.bssid.bytes, status->bssid);
	} else {
		status->security = esp_hosted_prot_to_sec(ctrl_msg.resp_get_softap_config.sec_prot);
		status->channel = ctrl_msg.resp_get_softap_config.chnl;
		status->ssid_len =
			MIN(ctrl_msg.resp_get_softap_config.ssid.size, WIFI_SSID_MAX_LEN);
		memcpy(status->ssid, ctrl_msg.resp_get_softap_config.ssid.bytes, status->ssid_len);
		memcpy(status->bssid, data->mac_addr[ESP_HOSTED_SAP_IF], ESP_HOSTED_MAC_ADDR_LEN);
	}
	return 0;
}

static int esp_hosted_scan(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;
	struct net_if *iface = net_if_lookup_by_dev(dev);

	if (esp_hosted_ctrl(dev, CtrlMsgId_Req_GetAPScanList, &ctrl_msg, ESP_HOSTED_SCAN_TIMEOUT)) {
		return -ETIMEDOUT;
	}

	size_t ap_count = ctrl_msg.resp_scan_ap_list.count;
	ScanResult *ap_list = ctrl_msg.resp_scan_ap_list.entries;

	for (size_t i = 0; i < ap_count; i++) {
		struct wifi_scan_result result = {0};

		result.rssi = ap_list[i].rssi;
		result.channel = ap_list[i].chnl;
		result.security = esp_hosted_prot_to_sec(ap_list[i].sec_prot);

		if (ap_list[i].ssid.size) {
			result.ssid_length = MIN(ap_list[i].ssid.size, ESP_HOSTED_MAX_SSID_LEN);
			memcpy(result.ssid, ap_list[i].ssid.bytes, result.ssid_length);
		}

		if (ap_list[i].bssid.size) {
			result.mac_length = MIN(ap_list[i].bssid.size, ESP_HOSTED_MAC_ADDR_LEN);
			esp_hosted_str_to_mac(ap_list[i].bssid.bytes, result.mac);
		}

		cb(iface, 0, &result);
		k_yield();
	}

	/* End of scan */
	cb(iface, 0, NULL);

	/* Entries are dynamically allocated. */
	pb_release(CtrlMsg_fields, &ctrl_msg);
	return 0;
}

static int esp_hosted_dev_init(const struct device *dev)
{
	esp_hosted_data_t *data = dev->data;

	/* Pins config and SPI init. */
	if (esp_hosted_hal_init(dev)) {
		return -EAGAIN;
	}

	/* Initialize semaphores. */
	if (k_sem_init(&data->bus_sem, 1, 1)) {
		LOG_ERR("k_sem_init() failed");
		return -EINVAL;
	}

	data->tid = k_thread_create(&esp_hosted_event_thread, esp_hosted_event_stack,
				    K_THREAD_STACK_SIZEOF(esp_hosted_event_stack),
				    (k_thread_entry_t)esp_hosted_event_task, (void *)dev, NULL,
				    NULL, CONFIG_WIFI_ESP_HOSTED_EVENT_TASK_PRIORITY,
				    K_INHERIT_PERMS, K_NO_WAIT);

	if (!data->tid) {
		LOG_ERR("ERROR spawning event processing thread");
		return -EAGAIN;
	}

	/* Wait for an ESPInit control event. */
	CtrlMsg ctrl_msg = CtrlMsg_init_zero;

	if (esp_hosted_ctrl(dev, CtrlMsgId_Event_ESPInit, &ctrl_msg, ESP_HOSTED_SYNC_TIMEOUT)) {
		return -EIO;
	}

	/* Set MAC addresses. */
	for (size_t i = 0; i < 2; i++) {
		ctrl_msg.req_get_mac_address.mode = i + 1;
		if (esp_hosted_ctrl(dev, CtrlMsgId_Req_GetMacAddress, &ctrl_msg,
				    ESP_HOSTED_SYNC_TIMEOUT)) {
			return -EIO;
		}
		esp_hosted_str_to_mac(ctrl_msg.resp_get_mac_address.mac.bytes, data->mac_addr[i]);
	}
	return 0;
}

static const struct wifi_mgmt_ops esp_hosted_mgmt = {
	.scan = esp_hosted_scan,
	.connect = esp_hosted_connect,
	.disconnect = esp_hosted_disconnect,
	.ap_enable = esp_hosted_ap_enable,
	.ap_disable = esp_hosted_ap_disable,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = esp_hosted_stats,
#endif
	.iface_status = esp_hosted_status,
};

static const struct net_wifi_mgmt_offload esp_hosted_api = {
	.wifi_iface.iface_api.init = esp_hosted_init,
	.wifi_iface.send = esp_hosted_send,
	.wifi_mgmt_api = &esp_hosted_mgmt,
};

NET_DEVICE_INIT_INSTANCE(0, "sta_if", 0, esp_hosted_dev_init, NULL, &esp_hosted_data,
			 &esp_hosted_config, CONFIG_WIFI_INIT_PRIORITY, &esp_hosted_api,
			 ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

NET_DEVICE_INIT_INSTANCE(1, "sap_if", 1, NULL, NULL, &esp_hosted_data, &esp_hosted_config,
			 CONFIG_WIFI_INIT_PRIORITY, &esp_hosted_api, ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

DEFINE_WIFI_NM_INSTANCE(nm, &esp_hosted_mgmt);
CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
