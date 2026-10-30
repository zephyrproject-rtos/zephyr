/*
 * Copyright (c) 2026 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MXCHIP EMW3080 Wi-Fi driver, network bypass mode.
 *
 * The module handles 802.11 association only. Ethernet frames are tunnelled
 * through EMW3080_IPC_CMD_WIFI_BYPASS_OUT (TX) and EMW3080_IPC_EVENT_WIFI_BYPASS_INPUT
 * (RX), and the Zephyr IP stack runs on the host.
 */

#define DT_DRV_COMPAT mxchip_emw3080

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emw3080, CONFIG_WIFI_LOG_LEVEL);

#include "emw3080_api.h"
#include "emw3080_ipc.h"
#include "emw3080_spi.h"

#define EMW3080_DRV_REV    KERNEL_VERSION_STRING
#define EMW3080_FW_REV_LEN 32

enum emw3080_flags {
	EMW3080_READY,
	EMW3080_RESET_PENDING,
};

struct emw3080_info {
	uint8_t fw_rev[EMW3080_FW_REV_LEN];  /* Firmware version. */
	uint8_t mac[EMW3080_IPC_MAC_LEN];    /* Wi-Fi MAC address. */
	uint8_t ap_mac[EMW3080_IPC_MAC_LEN]; /* Wi-Fi softap MAC address. */
};

struct emw3080_data {
	struct net_if *iface;

	struct emw3080_info info;

	enum wifi_iface_state state;
	atomic_t flags;
	atomic_t cmd_failures;

	struct k_work_q wq;
	struct k_work scan_work;
	struct k_work connect_work;
	struct k_work disconnect_work;
	struct k_work link_up_work;
	struct k_work link_down_work;
	struct k_work recover_work;
	struct k_work_delayable connect_timeout_work;

	scan_result_cb_t scan_cb;
	emw3080_ipc_cmd_wifi_connect_t connect_req;

	struct k_mutex tx_lock;
	uint8_t tx_buf[EMW3080_IPC_MAX_PARAMS];
	uint8_t scan_buf[EMW3080_IPC_MAX_PARAMS];
};

static K_THREAD_STACK_DEFINE(emw3080_wq_stack, CONFIG_WIFI_EMW3080_WORKQ_STACK_SIZE);

static struct emw3080_data emw3080_data_0;

static bool emw3080_is_link_error(int ret)
{
	return ret == -ETIMEDOUT || ret == -EIO || ret == -EPROTO;
}

/*
 * Every command goes through here so that a module that stops answering is
 * detected. After CONFIG_WIFI_EMW3080_MAX_CMD_TIMEOUTS consecutive transport
 * failures the module is reset and reconfigured.
 */
static int emw3080_cmd(struct emw3080_data *data, uint16_t api_id, const void *req,
		       uint16_t req_len, void *res, uint16_t res_size, uint16_t *res_len,
		       uint32_t timeout_ms)
{
	int ret = emw3080_ipc_request(api_id, req, req_len, res, res_size, res_len, timeout_ms);

	if (ret == EMW3080_IPC_OK) {
		atomic_clear(&data->cmd_failures);
	} else if (emw3080_is_link_error(ret) && CONFIG_WIFI_EMW3080_MAX_CMD_TIMEOUTS > 0) {
		atomic_val_t failures = atomic_inc(&data->cmd_failures) + 1;

		if (failures >= CONFIG_WIFI_EMW3080_MAX_CMD_TIMEOUTS &&
		    !atomic_test_and_set_bit(&data->flags, EMW3080_RESET_PENDING)) {
			LOG_ERR("module not responding (%ld failures), resetting", (long)failures);
			k_work_submit_to_queue(&data->wq, &data->recover_work);
		}
	}

	return ret;
}

/* Commands answering a single int32 status. */
static int emw3080_cmd_status(struct emw3080_data *data, uint16_t api_id, const void *req,
			      uint16_t req_len, uint32_t timeout_ms)
{
	emw3080_ipc_rsp_status_t rsp = {.status = -1};
	int ret = emw3080_cmd(data, api_id, req, req_len, &rsp, sizeof(rsp), NULL, timeout_ms);

	if (ret != EMW3080_IPC_OK) {
		return ret;
	}
	if (rsp.status != 0) {
		LOG_ERR("api_id=0x%04x: rejected by module (status %d)", api_id, rsp.status);
		return -EIO;
	}
	return 0;
}

static enum wifi_security_type emw3080_security_to_zephyr(enum emw3080_ipc_security sec)
{
	switch (sec) {
	case EMW3080_IPC_SEC_NONE:
		return WIFI_SECURITY_TYPE_NONE;
	case EMW3080_IPC_SEC_WEP:
		return WIFI_SECURITY_TYPE_WEP;
	case EMW3080_IPC_SEC_WPA_TKIP:
	case EMW3080_IPC_SEC_WPA_AES:
		return WIFI_SECURITY_TYPE_WPA_PSK;
	case EMW3080_IPC_SEC_WPA2_TKIP:
	case EMW3080_IPC_SEC_WPA2_AES:
	case EMW3080_IPC_SEC_WPA2_MIXED:
		return WIFI_SECURITY_TYPE_PSK;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

static enum wifi_frequency_bands emw3080_channel_to_band(int32_t channel)
{
	return (channel >= 1 && channel <= 14) ? WIFI_FREQ_BAND_2_4_GHZ : WIFI_FREQ_BAND_5_GHZ;
}

static void emw3080_ap_to_scan_result(const emw3080_ipc_ap_info_t *ap, struct wifi_scan_result *res)
{
	size_t ssid_len = MIN(strnlen(ap->ssid, sizeof(ap->ssid)), EMW3080_IPC_SSID_LEN);

	memset(res, 0, sizeof(*res));

	memcpy(res->ssid, ap->ssid, ssid_len);
	res->ssid_length = ssid_len;
	memcpy(res->mac, ap->bssid, WIFI_MAC_ADDR_LEN);
	res->mac_length = WIFI_MAC_ADDR_LEN;
	res->rssi = (int8_t)CLAMP(ap->rssi, INT8_MIN, INT8_MAX);
	res->channel = ap->channel;
	res->band = emw3080_channel_to_band(ap->channel);
	res->security = emw3080_security_to_zephyr(ap->security);
	res->mfp = WIFI_MFP_UNKNOWN;
}

/*
 * Move to DISCONNECTED and report the outcome matching the previous state:
 * a failed connection attempt or the loss of an established link.
 */
static void emw3080_link_set_down(struct emw3080_data *data, enum wifi_conn_status conn_status,
				  enum wifi_disconn_reason reason)
{
	enum wifi_iface_state prev = data->state;

	data->state = WIFI_STATE_DISCONNECTED;
	(void)k_work_cancel_delayable(&data->connect_timeout_work);

	if (data->iface == NULL) {
		return;
	}

	net_if_dormant_on(data->iface);
	if (IS_ENABLED(CONFIG_WIFI_STA_AUTO_DHCPV4)) {
		net_dhcpv4_stop(data->iface);
	}

	if (prev == WIFI_STATE_COMPLETED) {
		wifi_mgmt_raise_disconnect_result_event(data->iface, reason);
	} else if (prev == WIFI_STATE_ASSOCIATING) {
		wifi_mgmt_raise_connect_result_event(data->iface, conn_status);
	}
}

static void emw3080_link_up_work(struct k_work *work)
{
	struct emw3080_data *data = CONTAINER_OF(work, struct emw3080_data, link_up_work);
	enum wifi_iface_state prev = data->state;

	(void)k_work_cancel_delayable(&data->connect_timeout_work);
	data->state = WIFI_STATE_COMPLETED;

	if (prev == WIFI_STATE_COMPLETED || data->iface == NULL) {
		return;
	}

	net_if_dormant_off(data->iface);
	wifi_mgmt_raise_connect_result_event(data->iface, WIFI_STATUS_CONN_SUCCESS);
	if (IS_ENABLED(CONFIG_WIFI_STA_AUTO_DHCPV4)) {
		net_dhcpv4_start(data->iface);
	}
}

static void emw3080_link_down_work(struct k_work *work)
{
	struct emw3080_data *data = CONTAINER_OF(work, struct emw3080_data, link_down_work);

	emw3080_link_set_down(data, WIFI_STATUS_CONN_FAIL, WIFI_REASON_DISCONN_AP_LEAVING);
}

static void emw3080_connect_timeout_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct emw3080_data *data = CONTAINER_OF(dwork, struct emw3080_data, connect_timeout_work);

	if (data->state != WIFI_STATE_ASSOCIATING) {
		return;
	}

	LOG_WRN("connection timed out after %d ms", CONFIG_WIFI_EMW3080_CONNECT_TIMEOUT_MS);

	/* Abort the attempt so a late station up event does not follow. */
	(void)emw3080_cmd_status(data, EMW3080_IPC_CMD_WIFI_DISCONNECT, NULL, 0,
				 CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);

	emw3080_link_set_down(data, WIFI_STATUS_CONN_TIMEOUT, WIFI_REASON_DISCONN_UNSPECIFIED);
}

static void emw3080_bypass_rx(struct emw3080_data *data, const uint8_t *payload, size_t len)
{
	emw3080_bypass_input_event_payload_t hdr;
	struct net_pkt *pkt;

	if (len < sizeof(hdr)) {
		LOG_WRN("bypass rx: short event (%zu B)", len);
		return;
	}

	memcpy(&hdr, payload, sizeof(hdr));

	const uint8_t *frame = payload + sizeof(hdr);
	size_t frame_len = sys_le16_to_cpu(hdr.tot_len);

	if (frame_len == 0U || data->iface == NULL) {
		return;
	}
	if (frame_len > len - sizeof(hdr)) {
		LOG_WRN("bypass rx: tot_len %zu > %zu received", frame_len, len - sizeof(hdr));
		return;
	}

	/* Runs in the SPI transport thread: never wait for a buffer. */
	pkt = net_pkt_rx_alloc_with_buffer(data->iface, frame_len, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_WRN("bypass rx: no buffer, frame dropped");
		return;
	}

	if (net_pkt_write(pkt, frame, frame_len) < 0 || net_recv_data(data->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
}

/* Runs in the SPI transport thread: defer anything that blocks to the work queue. */
static void emw3080_on_event(uint16_t api_id, const uint8_t *payload, size_t len, void *user)
{
	struct emw3080_data *data = user;

	switch (api_id) {
	case EMW3080_IPC_EVENT_WIFI_BYPASS_INPUT:
		emw3080_bypass_rx(data, payload, len);
		break;
	case EMW3080_IPC_EVENT_WIFI_STATUS:
		if (len < 1U) {
			LOG_WRN("empty status event");
			break;
		}
		LOG_DBG("status event %u", payload[0]);
		if (payload[0] == EMW3080_IPC_EVENTP_WIFI_STA_UP) {
			k_work_submit_to_queue(&data->wq, &data->link_up_work);
		} else if (payload[0] == EMW3080_IPC_EVENTP_WIFI_STA_DOWN) {
			k_work_submit_to_queue(&data->wq, &data->link_down_work);
		}
		break;
	case EMW3080_IPC_EVENT_SYS_REBOOT:
		/* An unexpected reboot loses the bypass configuration. */
		if (atomic_test_bit(&data->flags, EMW3080_READY) &&
		    !atomic_test_bit(&data->flags, EMW3080_RESET_PENDING)) {
			LOG_WRN("module rebooted, reconfiguring");
			k_work_submit_to_queue(&data->wq, &data->recover_work);
		}
		break;
	default:
		LOG_DBG("unhandled event 0x%04x (%zu B)", api_id, len);
		break;
	}
}

/* Version check, MAC address and bypass mode. Uses ipc_request() directly. */
static int emw3080_module_setup(struct emw3080_data *data)
{
	emw3080_ipc_cmd_wifi_bypass_set_t bypass = {.mode = sys_cpu_to_le32(1)};
	emw3080_ipc_rsp_status_t status = {.status = -1};
	uint8_t mac[EMW3080_IPC_MAC_LEN];
	uint8_t fw_rev[EMW3080_FW_REV_LEN];
	uint16_t fw_rev_len;
	int ret;

	ret = emw3080_ipc_request(EMW3080_IPC_CMD_SYS_VERSION, NULL, 0, fw_rev, sizeof(fw_rev) - 1U,
				  &fw_rev_len, CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	if (ret != EMW3080_IPC_OK || fw_rev_len == 0U) {
		LOG_ERR("get version failed (%d)", ret);
		return (ret != EMW3080_IPC_OK) ? ret : -EBADMSG;
	}
	memcpy(data->info.fw_rev, fw_rev, fw_rev_len);
	data->info.fw_rev[fw_rev_len] = '\0';

	ret = emw3080_ipc_request(EMW3080_IPC_CMD_WIFI_GET_MAC, NULL, 0, mac, sizeof(mac), NULL,
				  CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	if (ret != EMW3080_IPC_OK) {
		LOG_ERR("get MAC failed (%d)", ret);
		return ret;
	}
	memcpy(data->info.mac, mac, sizeof(mac));

	ret = emw3080_ipc_request(EMW3080_IPC_CMD_WIFI_GET_SOFT_MAC, NULL, 0, mac, sizeof(mac),
				  NULL, CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	if (ret != EMW3080_IPC_OK) {
		LOG_ERR("get AP MAC failed (%d)", ret);
		return ret;
	}
	memcpy(data->info.ap_mac, mac, sizeof(mac));

	ret = emw3080_ipc_request(EMW3080_IPC_CMD_WIFI_BYPASS_SET, &bypass, sizeof(bypass), &status,
				  sizeof(status), NULL, CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	if (ret != EMW3080_IPC_OK) {
		LOG_ERR("enable bypass failed (%d)", ret);
		return ret;
	}
	if (status.status != 0) {
		LOG_ERR("enable bypass rejected (status %d)", status.status);
		return -EIO;
	}

	LOG_INF("EMW3080 Wi-Fi Driver Version: %s", EMW3080_DRV_REV);
	LOG_INF("EMW3080 Wi-Fi Firmware Version: %s", data->info.fw_rev);
	LOG_INF("EMW3080 MAC %02x:%02x:%02x:%02x:%02x:%02x", data->info.mac[0], data->info.mac[1],
		data->info.mac[2], data->info.mac[3], data->info.mac[4], data->info.mac[5]);

	return 0;
}

static void emw3080_recover_work(struct k_work *work)
{
	struct emw3080_data *data = CONTAINER_OF(work, struct emw3080_data, recover_work);
	int ret;

	emw3080_link_set_down(data, WIFI_STATUS_CONN_FAIL, WIFI_REASON_DISCONN_UNSPECIFIED);

	if (atomic_test_bit(&data->flags, EMW3080_RESET_PENDING)) {
		emw3080_spi_reset();
	}

	ret = emw3080_module_setup(data);
	atomic_clear_bit(&data->flags, EMW3080_RESET_PENDING);
	if (ret != 0) {
		LOG_ERR("module recovery failed (%d)", ret);
		return;
	}

	atomic_clear(&data->cmd_failures);
	LOG_INF("module recovered");
}

static void emw3080_scan_work(struct k_work *work)
{
	struct emw3080_data *data = CONTAINER_OF(work, struct emw3080_data, scan_work);
	const emw3080_ipc_rsp_wifi_scan_t *res =
		(const emw3080_ipc_rsp_wifi_scan_t *)data->scan_buf;
	emw3080_ipc_cmd_wifi_scan_t req = {0};
	struct wifi_scan_result result;
	scan_result_cb_t cb = data->scan_cb;
	uint16_t res_len;
	size_t count;
	int ret;

	ret = emw3080_cmd(data, EMW3080_IPC_CMD_WIFI_SCAN, &req, sizeof(req), data->scan_buf,
			  sizeof(data->scan_buf), &res_len, CONFIG_WIFI_EMW3080_SCAN_TIMEOUT_MS);
	if (ret != EMW3080_IPC_OK) {
		goto done;
	}
	if (res_len < sizeof(res->num)) {
		ret = -EBADMSG;
		goto done;
	}

	count = MIN(res->num, (res_len - sizeof(res->num)) / sizeof(emw3080_ipc_ap_info_t));
	for (size_t i = 0; i < count; i++) {
		emw3080_ap_to_scan_result(&res->ap[i], &result);
		cb(data->iface, 0, &result);
	}

done:
	data->scan_cb = NULL;
	cb(data->iface, ret, NULL);
}

static void emw3080_connect_work(struct k_work *work)
{
	struct emw3080_data *data = CONTAINER_OF(work, struct emw3080_data, connect_work);
	int ret;

	ret = emw3080_cmd_status(data, EMW3080_IPC_CMD_WIFI_CONNECT, &data->connect_req,
				 sizeof(data->connect_req), CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);

	memset(data->connect_req.key, 0, sizeof(data->connect_req.key));

	if (ret != 0) {
		emw3080_link_set_down(data, WIFI_STATUS_CONN_FAIL, WIFI_REASON_DISCONN_UNSPECIFIED);
		return;
	}

	/* The result comes with the station up event, unless this expires first. */
	if (data->state == WIFI_STATE_ASSOCIATING) {
		k_work_reschedule_for_queue(&data->wq, &data->connect_timeout_work,
					    K_MSEC(CONFIG_WIFI_EMW3080_CONNECT_TIMEOUT_MS));
	}
}

static void emw3080_disconnect_work(struct k_work *work)
{
	struct emw3080_data *data = CONTAINER_OF(work, struct emw3080_data, disconnect_work);
	int ret;

	ret = emw3080_cmd_status(data, EMW3080_IPC_CMD_WIFI_DISCONNECT, NULL, 0,
				 CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	if (ret != 0) {
		LOG_WRN("disconnect failed (%d), reporting link down anyway", ret);
	}

	emw3080_link_set_down(data, WIFI_STATUS_CONN_FAIL, WIFI_REASON_DISCONN_USER_REQUEST);
}

static int emw3080_mgmt_scan(const struct device *dev, struct net_if *iface,
			     struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct emw3080_data *data = dev->data;

	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	if (!atomic_test_bit(&data->flags, EMW3080_READY)) {
		return -ENETDOWN;
	}
	if (data->scan_cb != NULL) {
		return -EINPROGRESS;
	}

	data->scan_cb = cb;
	k_work_submit_to_queue(&data->wq, &data->scan_work);
	return 0;
}

static int emw3080_mgmt_connect(const struct device *dev, struct net_if *iface,
				struct wifi_connect_req_params *params)
{
	struct emw3080_data *data = dev->data;
	emw3080_ipc_cmd_wifi_connect_t *req = &data->connect_req;

	ARG_UNUSED(iface);

	if (!atomic_test_bit(&data->flags, EMW3080_READY)) {
		return -ENETDOWN;
	}
	if (data->state == WIFI_STATE_ASSOCIATING || data->state == WIFI_STATE_COMPLETED) {
		return -EALREADY;
	}
	if (params->ssid == NULL || params->ssid_length == 0U ||
	    params->ssid_length > EMW3080_IPC_SSID_LEN ||
	    params->psk_length > EMW3080_IPC_KEY_LEN) {
		return -EINVAL;
	}

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
	case WIFI_SECURITY_TYPE_WEP:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_PSK:
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		break;
	default:
		return -ENOTSUP;
	}

	memset(req, 0, sizeof(*req));
	memcpy(req->ssid, params->ssid, params->ssid_length);
	if (params->psk != NULL && params->psk_length > 0U) {
		memcpy(req->key, params->psk, params->psk_length);
		req->key_len = sys_cpu_to_le32(params->psk_length);
	}
	/* The module detects BSSID, channel and security; DHCP runs on the host. */
	req->use_attr = 0U;
	req->use_ip = 0U;

	data->state = WIFI_STATE_ASSOCIATING;
	k_work_submit_to_queue(&data->wq, &data->connect_work);
	return 0;
}

static int emw3080_mgmt_disconnect(const struct device *dev, struct net_if *iface)
{
	struct emw3080_data *data = dev->data;

	ARG_UNUSED(iface);

	if (data->state != WIFI_STATE_ASSOCIATING && data->state != WIFI_STATE_COMPLETED) {
		return -EALREADY;
	}

	k_work_submit_to_queue(&data->wq, &data->disconnect_work);
	return 0;
}

static int emw3080_mgmt_iface_status(const struct device *dev, struct net_if *iface,
				     struct wifi_iface_status *status)
{
	struct emw3080_data *data = dev->data;
	emw3080_ipc_rsp_wifi_get_linkinfo_t res = {.status = -1};
	int ret;

	ARG_UNUSED(iface);

	status->state = data->state;
	status->iface_mode = WIFI_MODE_INFRA;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->band = WIFI_FREQ_BAND_UNKNOWN;
	status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	status->mfp = WIFI_MFP_UNKNOWN;

	if (data->state != WIFI_STATE_COMPLETED) {
		return 0;
	}

	ret = emw3080_cmd(data, EMW3080_IPC_CMD_WIFI_GET_LINKINFO, NULL, 0, &res, sizeof(res), NULL,
			  CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	if (ret != EMW3080_IPC_OK) {
		return ret;
	}
	if (res.status != 0 || res.info.is_connected == 0) {
		LOG_WRN("link info unavailable (status %d)", res.status);
		return 0;
	}

	status->ssid_len = strnlen(res.info.ssid, MIN(sizeof(res.info.ssid), EMW3080_IPC_SSID_LEN));
	memcpy(status->ssid, res.info.ssid, status->ssid_len);
	status->ssid[status->ssid_len] = '\0';
	memcpy(status->bssid, res.info.bssid, WIFI_MAC_ADDR_LEN);
	status->channel = res.info.channel;
	status->band = emw3080_channel_to_band(res.info.channel);
	status->rssi = res.info.rssi;
	status->security = emw3080_security_to_zephyr(res.info.security);
	return 0;
}

static int emw3080_mgmt_get_version(const struct device *dev, struct net_if *iface,
				    struct wifi_version *params)
{
	ARG_UNUSED(iface);
	struct emw3080_data *data = dev->data;

	params->drv_version = EMW3080_DRV_REV;
	params->fw_version = data->info.fw_rev;
	return 0;
}

static int emw3080_send(const struct device *dev, struct net_pkt *pkt)
{
	struct emw3080_data *data = dev->data;
	emw3080_ipc_cmd_wifi_bypass_out_t *hdr = (emw3080_ipc_cmd_wifi_bypass_out_t *)data->tx_buf;
	const size_t hdr_len = sizeof(*hdr);
	const size_t frame_len = net_pkt_get_len(pkt);
	int ret;

	if (data->state != WIFI_STATE_COMPLETED) {
		return -ENETDOWN;
	}
	if (frame_len == 0U || frame_len > sizeof(data->tx_buf) - hdr_len) {
		LOG_WRN("bypass tx: bad frame length %zu", frame_len);
		return -EMSGSIZE;
	}

	k_mutex_lock(&data->tx_lock, K_FOREVER);

	memset(hdr, 0, hdr_len);
	hdr->idx = sys_cpu_to_le32(EMW3080_IPC_BYPASS_IDX_STA);
	hdr->data_len = sys_cpu_to_le16((uint16_t)frame_len);

	net_pkt_cursor_init(pkt);
	ret = net_pkt_read(pkt, data->tx_buf + hdr_len, frame_len);
	if (ret == 0) {
		ret = emw3080_cmd_status(data, EMW3080_IPC_CMD_WIFI_BYPASS_OUT, data->tx_buf,
					 (uint16_t)(hdr_len + frame_len),
					 CONFIG_WIFI_EMW3080_CMD_TIMEOUT_MS);
	}

	k_mutex_unlock(&data->tx_lock);
	return ret;
}

static void emw3080_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct emw3080_data *data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	data->iface = iface;

	net_if_set_link_addr(iface, data->info.mac, sizeof(data->info.mac), NET_LINK_ETHERNET);
	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	ethernet_init(iface);

	/* Operational only once associated. */
	net_if_dormant_on(iface);
}

static int emw3080_init(const struct device *dev)
{
	struct emw3080_data *data = dev->data;
	int ret;

	data->state = WIFI_STATE_UNKNOWN;

	k_mutex_init(&data->tx_lock);

	k_work_queue_init(&data->wq);
	k_work_queue_start(&data->wq, emw3080_wq_stack, K_THREAD_STACK_SIZEOF(emw3080_wq_stack),
			   K_PRIO_PREEMPT(CONFIG_WIFI_EMW3080_WORKQ_PRIORITY), NULL);
	k_thread_name_set(k_work_queue_thread_get(&data->wq), "emw3080_wq");

	k_work_init(&data->scan_work, emw3080_scan_work);
	k_work_init(&data->connect_work, emw3080_connect_work);
	k_work_init(&data->disconnect_work, emw3080_disconnect_work);
	k_work_init(&data->link_up_work, emw3080_link_up_work);
	k_work_init(&data->link_down_work, emw3080_link_down_work);
	k_work_init(&data->recover_work, emw3080_recover_work);
	k_work_init_delayable(&data->connect_timeout_work, emw3080_connect_timeout_work);

	ret = emw3080_ipc_init(emw3080_on_event, data);
	if (ret != 0) {
		LOG_ERR("transport init failed (%d)", ret);
		return ret;
	}

	ret = emw3080_module_setup(data);
	if (ret != 0) {
		return ret;
	}

	data->state = WIFI_STATE_DISCONNECTED;
	atomic_set_bit(&data->flags, EMW3080_READY);

	return 0;
}

static const struct wifi_mgmt_ops emw3080_mgmt_ops = {
	.scan = emw3080_mgmt_scan,
	.connect = emw3080_mgmt_connect,
	.disconnect = emw3080_mgmt_disconnect,
	.iface_status = emw3080_mgmt_iface_status,
	.get_version = emw3080_mgmt_get_version,
};

static const struct net_wifi_mgmt_offload emw3080_api = {
	.wifi_iface.iface_api.init = emw3080_iface_init,
	.wifi_iface.send = emw3080_send,
	.wifi_mgmt_api = &emw3080_mgmt_ops,
};

NET_DEVICE_DT_INST_DEFINE(0, emw3080_init, NULL, &emw3080_data_0, NULL, CONFIG_WIFI_INIT_PRIORITY,
			  &emw3080_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
