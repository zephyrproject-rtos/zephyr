/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/math_extras.h>

#include "siwx91x_nwp.h"
#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp_api.h"

#include "device/silabs/si91x/wireless/ble/inc/rsi_bt_common.h"
#include "device/silabs/si91x/wireless/socket/inc/sl_si91x_socket_constants.h"
#include "service/network_manager/inc/sli_net_constants.h"
#include "sli_wifi/inc/sli_wifi_types.h"

LOG_MODULE_DECLARE(siwx91x_nwp, CONFIG_SIWX91X_NWP_LOG_LEVEL);

/* Contains low level helpers for NWP. Each function will run one (and only one) NWP command. This
 * is a first layer of abstraction for the driver.
 *
 * The function tries to expose only the relevant arguments to the upper layers. In some case,
 * several functions may be mapped to the same NWP API. It happens when API has several mode and
 * some argument are only meaningful in some mode (typically, siwx91x_nwp_fw_upgrade_write vs
 * siwx91x_nwp_fw_upgrade_start or siwx91x_nwp_flash_write vs siwx91x_nwp_flash_erase).
 *
 * We prefer to expose NWP agnostic parameters, however, for the complex API, rather than exposing
 * ridiculous number of parameters, we may expose the NWP structure to the caller.
 */


/* sli_si91x_driver_send_command */
static uint32_t siwx91x_nwp_send_cmd(const struct device *dev,
				     const void *data_buf, size_t data_len,
				     uint16_t command, int queue_id, uint8_t flags,
				     struct net_buf **reply_buf)
{
	struct siwx91x_frame_desc desc_buf;
	NET_BUF_DEFINE_STACK(data_container, data_buf, data_len, 0);
	NET_BUF_DEFINE_STACK(desc_container, &desc_buf, sizeof(desc_buf), sizeof(void *));
	struct net_buf *local_reply;
	uint32_t status;

	__ASSERT(!(flags & SIWX91X_FRAME_FLAG_ASYNC), "Invalid call");
	__ASSERT(!(flags & SIWX91X_FRAME_FLAG_NO_HDR_RESET), "Invalid call");
	net_buf_frag_add(desc_container, net_buf_ref(data_container));
	net_buf_unref(data_container);

	local_reply = siwx91x_nwp_send_frame(dev, desc_container, command, queue_id, flags);
	__ASSERT(local_reply != NULL, "Corrupted state");
	status = *(uint16_t *)(&((struct siwx91x_frame_desc *)local_reply->data)->reserved[8]);
	if (reply_buf) {
		*reply_buf = net_buf_ref(local_reply);
	}
	net_buf_unref(local_reply);
	return status ? (0x10000 | status) : 0;
}

static uint32_t siwx91x_nwp_send_cmd2(const struct device *dev,
				      const void *data1_buf, size_t data1_len,
				      const void *data2_buf, size_t data2_len,
				      uint16_t command, int queue_id, uint8_t flags,
				      struct net_buf **reply_buf)
{
	struct siwx91x_frame_desc desc_buf;
	NET_BUF_DEFINE_STACK(data2_container, data2_buf, data2_len, 0);
	NET_BUF_DEFINE_STACK(data1_container, data1_buf, data1_len, 0);
	NET_BUF_DEFINE_STACK(desc_container, &desc_buf, sizeof(desc_buf), sizeof(void *));
	struct net_buf *local_reply;
	uint32_t status;

	__ASSERT(!(flags & SIWX91X_FRAME_FLAG_ASYNC), "Invalid call");
	__ASSERT(!(flags & SIWX91X_FRAME_FLAG_NO_HDR_RESET), "Invalid call");
	net_buf_frag_add(desc_container, net_buf_ref(data1_container));
	net_buf_frag_add(desc_container, net_buf_ref(data2_container));
	net_buf_unref(data1_container);
	net_buf_unref(data2_container);

	local_reply = siwx91x_nwp_send_frame(dev, desc_container, command, queue_id, flags);
	__ASSERT(local_reply != NULL, "Corrupted state");
	status = *(uint16_t *)(&((struct siwx91x_frame_desc *)local_reply->data)->reserved[8]);
	if (reply_buf) {
		*reply_buf = net_buf_ref(local_reply);
	}
	net_buf_unref(local_reply);
	return status ? (0x10000 | status) : 0;
}

/* NWP has a weird way to represent IPv6 addresses */
static void siwx91x_nwp_convert_ipv6_format(void *out, const void *in)
{
	uint32_t *out32 = out;
	const uint32_t *in32 = in;

	for (int i = 0; i < 4; i++) {
		out32[i] = __builtin_bswap32(in32[i]);
	}
}


#define SCAN_FEATURE_QUICK_SCAN       BIT(0)
#define SCAN_FEATURE_RESULTS_TO_HOST  BIT(1)
#define SCAN_FEATURE_TX_POWER_MASK    0xF8

#define PASSIVE_SCAN_ENABLE           BIT(7)
int siwx91x_nwp_scan(const struct device *dev, uint16_t channel_list, const char *ssid,
		      bool passive, bool internal)
{
	sli_wifi_request_scan_t params = {
		.scan_feature_bitmap = FIELD_PREP(SCAN_FEATURE_TX_POWER_MASK, 31),
	};
	uint32_t status;

	__ASSERT(strlen(ssid) < sizeof(params.ssid), "Corrupted data");
	if (ssid) {
		strcpy(params.ssid, ssid);
	}
	if (passive) {
		params.pscan_bitmap[3] |= PASSIVE_SCAN_ENABLE;
	}
	if (ssid && channel_list) {
		params.scan_feature_bitmap |= SCAN_FEATURE_QUICK_SCAN;
	}
	if (!internal) {
		params.scan_feature_bitmap |= SCAN_FEATURE_RESULTS_TO_HOST;
	}

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_SCAN,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	/* Return an error if scan is requested while device is connected */
	return status ? -EINVAL : 0;
}

void siwx91x_nwp_set_psk(const struct device *dev, const char *psk)
{
	sli_wifi_request_psk_t params = {
		.type = 1, /* PSK */
	};
	uint32_t status;

	__ASSERT(psk, "Invalid arguments");
	__ASSERT(strlen(psk) < sizeof(params.psk_or_pmk), "Invalid arguments");
	strcpy(params.psk_or_pmk, psk);

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_HOST_PSK,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

#define JOIN_POWER_LEVEL_MASK    0x7C

int siwx91x_nwp_join(const struct device *dev, const char *ssid,
		     const uint8_t *bssid, int security_type)
{
	sli_wifi_join_request_t params = {
		.power_level = 0x80 | FIELD_PREP(JOIN_POWER_LEVEL_MASK, 31),
		.security_type = security_type,
		.ssid_len = strlen(ssid),
		.join_feature_bitmap = 0, /* FIXME: See SL_WIFI_JOIN_FEAT_* */
	};
	uint32_t status;

	__ASSERT(ssid, "Invalid arguments");
	strncpy(params.ssid, ssid, sizeof(params.ssid));
	if (bssid) {
		memcpy(params.join_bssid, bssid, sizeof(params.join_bssid));
	}

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_JOIN,
				      SLI_WLAN_MGMT_Q, 0, NULL);

	switch (status) {
	case 0:
		return 0;
	case SL_STATUS_SI91X_REJOIN_FAILURE:
		return -ENOENT;
	case SL_STATUS_SI91X_COMMAND_GIVEN_IN_INVALID_STATE:
		return -EBADFD;
	default:
		return -EINVAL;
	}
}

int siwx91x_nwp_disconnect(const struct device *dev)
{
	sli_wifi_disassociation_request_t params = {
		.mode_flag = SL_WIFI_CLIENT_VAP_ID,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_DISCONNECT,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	/* Return an error if already disconnected */
	return status ? -EINVAL : 0;
}

void siwx91x_nwp_ap_config(const struct device *dev, sli_wifi_ap_config_request *params)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, params, sizeof(*params), SLI_WIFI_REQ_AP_CONFIGURATION,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

int siwx91x_nwp_ap_start(const struct device *dev, const char *ssid, int security_type)
{
	sli_wifi_join_request_t params = {
		/* I believe it does not make sense */
		// .join_feature_bitmap = SL_WIFI_JOIN_FEAT_LISTEN_INTERVAL_VALID,
		.power_level = 0x80 | FIELD_PREP(0x5C, 31),
		/* FIXME: This is already defined in ap_config */
		.security_type = security_type,
		.ssid_len = strlen(ssid),
	};
	struct net_buf *reply_buf;
	uint32_t status;
	uint8_t reply;

	__ASSERT(params.ssid_len < sizeof(params.ssid) - 1, "Corrupted argument");
	strcpy(params.ssid, ssid);

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_JOIN,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EINVAL;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	net_buf_linearize(&reply, sizeof(uint8_t),
			  reply_buf, sizeof(struct siwx91x_frame_desc), SIZE_MAX);
	net_buf_unref(reply_buf);
	if (reply != 'G') {
		return -EINVAL;
	}
	return 0;
}

void siwx91x_nwp_ap_stop(const struct device *dev)
{
	sli_wifi_disassociation_request_t params = {
		/* FIXME: the parameter seems ignored, but it would make sense to use
		 * SL_WIFI_AP_VAP_ID
		 */
		.mode_flag = SL_WIFI_CLIENT_VAP_ID,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_AP_STOP,
				   SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_sta_disconnect(const struct device *dev,
				const uint8_t remote[WIFI_MAC_ADDR_LEN])
{
	sli_wifi_disassociation_request_t params = {
		.mode_flag = SL_WIFI_AP_VAP_ID,
	};
	uint32_t status;

	memcpy(&params.client_mac_address, remote, WIFI_MAC_ADDR_LEN);

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_DISCONNECT,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_get_bss_info(const struct device *dev, sl_wifi_operational_statistics_t *reply)
{
	struct net_buf *reply_buf;
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_WIFI_REQ_GET_STATS, SLI_WLAN_MGMT_Q, 0,
				      &reply_buf);
	__ASSERT(!status, "Corrupted NWP reply");
	__ASSERT(reply_buf, "Corrupted NWP reply");

	net_buf_linearize(reply, sizeof(sl_wifi_operational_statistics_t),
			  reply_buf, sizeof(struct siwx91x_frame_desc), SIZE_MAX);
	net_buf_unref(reply_buf);
}

void siwx91x_nwp_ps_disable(const struct device *dev)
{
	sli_wifi_power_save_request_t params = { };
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_PWRMODE,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_ps_enable(const struct device *dev, sli_wifi_power_save_request_t *params)
{
	uint32_t status;

	__ASSERT(params->power_mode != 0, "Prefer siwx91x_nwp_disable_ps()");

	status = siwx91x_nwp_send_cmd(dev, params, sizeof(*params), SLI_WIFI_REQ_PWRMODE,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_twt_params(const struct device *dev, sl_wifi_twt_request_t *params)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, params, sizeof(*params), SLI_WIFI_REQ_TWT_PARAMS,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_set_region_ap(const struct device *dev)
{
	sli_wifi_set_region_ap_request_t params = {
		.set_region_code_from_user_cmd = SET_REGION_CODE_FROM_USER,
		.country_code = "EU ",
		.no_of_rules = 1,
		.channel_info[0].first_channel = 1,
		.channel_info[0].no_of_channels = 13,
		.channel_info[0].max_tx_power = 20,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_SET_REGION_AP,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_set_region_sta(const struct device *dev, sl_wifi_region_code_t region_code)
{
	sli_wifi_set_region_request_t params = {
		.set_region_code_from_user_cmd = SET_REGION_CODE_FROM_USER,
		.region_code = (uint8_t)region_code,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_SET_REGION,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

int siwx91x_nwp_wifi_init(const struct device *dev)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_WIFI_REQ_INIT,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	/* Return an error if already initialized */
	return status ? -EINVAL : 0;
}

void siwx91x_nwp_get_mac_address(const struct device *dev, uint8_t reply[NET_ETH_ADDR_LEN])
{
	struct net_buf *reply_buf;
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_WIFI_REQ_MAC_ADDRESS, SLI_WLAN_MGMT_Q, 0,
				      &reply_buf);
	__ASSERT(!status, "Corrupted NWP reply");
	__ASSERT(reply_buf, "Corrupted NWP reply");

	net_buf_linearize(reply, NET_ETH_ADDR_LEN,
			  reply_buf, sizeof(struct siwx91x_frame_desc), SIZE_MAX);
	net_buf_unref(reply_buf);
}

void siwx91x_nwp_set_sta_config(const struct device *dev)
{
	sli_wifi_rejoin_params_t params = {
		.max_retry_attempts = 1,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_REJOIN_PARAMS,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_set_band(const struct device *dev, sl_wifi_band_mode_t band)
{
	uint8_t params = band;
	uint32_t status;

	__ASSERT(band == SL_WIFI_BAND_MODE_2_4GHZ, "Unsupported option");

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_BAND,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_set_ht_caps(const struct device *dev, bool enabled)
{
	sli_wifi_request_ap_high_throughput_capability_t params = {
		.mode_11n_enable = true,
		.ht_caps_bitmap = SL_WIFI_HT_CAPS_NUM_RX_STBC |
				  SL_WIFI_HT_CAPS_SHORT_GI_20MHZ |
				  SL_WIFI_HT_CAPS_GREENFIELD_EN,
	};
	uint32_t status;

	__ASSERT(enabled == true, "Not supported");

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_HT_CAPABILITIES,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

int siwx91x_nwp_sock_config_ipv4(const struct device *dev, struct net_in_addr *addr,
				  struct net_in_addr *mask, struct net_in_addr *gw)
{
	sli_si91x_req_ipv4_params_t params = {
		.dhcp_mode = 1,
	};
	sli_si91x_rsp_ipv4_params_t *reply;
	struct net_buf *reply_buf;
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_IPCONFV4,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(reply_buf, sizeof(struct siwx91x_frame_desc));
	memcpy(gw, reply->gateway, sizeof(uint32_t));
	memcpy(addr, reply->ipaddr, sizeof(uint32_t));
	memcpy(mask, reply->netmask, sizeof(uint32_t));
	return 0;
}

int siwx91x_nwp_sock_config_ipv6(const struct device *dev, struct net_in6_addr *lua,
				 struct net_in6_addr *gua, int *prefix_len, struct net_in6_addr *gw)
{
	sli_si91x_req_ipv6_params_t params = {
		.mode[0] = 1, /* SLAAC donfiguration */
	};
	sli_si91x_rsp_ipv6_params_t *reply;
	struct net_buf *reply_buf;
	uint32_t status;

	LOG_WRN("IPv6 configuration is known to break NWP");

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_IPCONFV6,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(reply_buf, sizeof(struct siwx91x_frame_desc));
	siwx91x_nwp_convert_ipv6_format(gua, reply->global_address);
	siwx91x_nwp_convert_ipv6_format(lua, reply->link_local_address);
	siwx91x_nwp_convert_ipv6_format(gw, reply->gateway_address);
	*prefix_len = reply->prefixLength;
	return 0;
}

/* Create a UDP server connecttion */
int siwx91x_nwp_sock_bind(const struct device *dev, const struct net_sockaddr *remote)
{
	const struct net_sockaddr_in6 *remote6 = (const struct net_sockaddr_in6 *)remote;
	const struct net_sockaddr_in *remote4 = (const struct net_sockaddr_in *)remote;
	/* FIXME: Should we fill vap_id? */
	sli_si91x_socket_create_request_t params = {
		.socket_type = SLI_SI91X_SOCKET_UDP_CLIENT, /* For TCP, job is made by listen() */
		.socket_bitmap = SLI_SI91X_SOCKET_FEAT_SYNCHRONOUS,
	};
	sli_si91x_socket_create_response_t *reply;
	struct net_buf *reply_buf;
	uint32_t status;
	int newsockfd;

	switch (remote->sa_family) {
	case NET_AF_INET:
		params.ip_version = SL_IPV4_ADDRESS_LENGTH;
		params.local_port = net_ntohs(remote4->sin_port);
		break;
	case NET_AF_INET6:
		params.ip_version = SL_IPV6_ADDRESS_LENGTH;
		params.local_port = net_ntohs(remote6->sin6_port);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SOCKET_CREATE,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(reply_buf, sizeof(struct siwx91x_frame_desc));
	newsockfd = *(uint16_t *)reply->socket_id;
	net_buf_unref(reply_buf);
	return newsockfd;
}

/* Create a TCP server connecttion */
int siwx91x_nwp_sock_listen(const struct device *dev,
			    const struct net_sockaddr_ptr *local, int backlog)
{
	const struct net_sockaddr_in6_ptr *local6 = (const struct net_sockaddr_in6_ptr *)local;
	const struct net_sockaddr_in_ptr *local4 = (const struct net_sockaddr_in_ptr *)local;
	sli_si91x_socket_create_request_t params = {
		.socket_type = SLI_SI91X_SOCKET_TCP_SERVER,
		.socket_bitmap = SLI_SI91X_SOCKET_FEAT_SYNCHRONOUS |
				 SLI_SI91X_SOCKET_FEAT_LTCP_ACCEPT |
				 SLI_SI91X_SOCKET_FEAT_TCP_RX_WINDOW,
		.tcp_keepalive_initial_time = SLI_DEFAULT_TCP_KEEP_ALIVE_TIME,
		.rx_window_size = SLI_TCP_RX_WINDOW_SIZE,
		.max_tcp_retries_count = SLI_MAX_TCP_RETRY_COUNT,
		.max_count = backlog,
	};
	sli_si91x_socket_create_response_t *reply;
	struct net_buf *reply_buf;
	uint32_t status;
	int newsockfd;

	switch (local->family) {
	case NET_AF_INET:
		params.ip_version = SL_IPV4_ADDRESS_LENGTH;
		params.remote_port = net_ntohs(local4->sin_port);
		break;
	case NET_AF_INET6:
		params.ip_version = SL_IPV6_ADDRESS_LENGTH;
		params.remote_port = net_ntohs(local6->sin6_port);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SOCKET_CREATE,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(reply_buf, sizeof(struct siwx91x_frame_desc));
	newsockfd = *(uint16_t *)reply->socket_id;
	net_buf_unref(reply_buf);
	return newsockfd;
}

/* Create a client connection */
int siwx91x_nwp_sock_connect(const struct device *dev,
			     int socktype, const struct net_sockaddr *remote)
{
	const struct net_sockaddr_in6 *remote6 = (const struct net_sockaddr_in6 *)remote;
	const struct net_sockaddr_in *remote4 = (const struct net_sockaddr_in *)remote;
	sli_si91x_socket_create_request_t params = {
		.socket_bitmap = SLI_SI91X_SOCKET_FEAT_SYNCHRONOUS,
		.tcp_keepalive_initial_time = SLI_DEFAULT_TCP_KEEP_ALIVE_TIME,
		.rx_window_size = SLI_TCP_RX_WINDOW_SIZE,
		.max_tcp_retries_count = SLI_MAX_TCP_RETRY_COUNT,
	};
	sli_si91x_socket_create_response_t *reply;
	struct net_buf *reply_buf;
	uint32_t status;
	int newsockfd;

	switch (socktype) {
	case NET_SOCK_DGRAM:
		params.socket_type = SLI_SI91X_SOCKET_UDP_CLIENT;
		break;
	case NET_SOCK_STREAM:
		params.socket_type = SLI_SI91X_SOCKET_TCP_CLIENT;
		break;
	default:
		return -EAFNOSUPPORT;
	}

	switch (remote->sa_family) {
	case NET_AF_INET:
		params.ip_version = SL_IPV4_ADDRESS_LENGTH;
		params.remote_port = net_ntohs(remote4->sin_port);
		memcpy(&params.dest_ip_addr, &remote4->sin_addr, sizeof(remote4->sin_addr));
		break;
	case NET_AF_INET6:
		params.ip_version = SL_IPV6_ADDRESS_LENGTH;
		params.remote_port = net_ntohs(remote6->sin6_port);
		siwx91x_nwp_convert_ipv6_format(&params.dest_ip_addr, &remote6->sin6_addr);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SOCKET_CREATE,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(reply_buf, sizeof(struct siwx91x_frame_desc));
	newsockfd = *(uint16_t *)reply->socket_id;
	net_buf_unref(reply_buf);
	return newsockfd;
}

int siwx91x_nwp_sock_accept(const struct device *dev, int sockfd,
			    const struct net_sockaddr_ptr *local, struct net_sockaddr *remote)
{
	const struct net_sockaddr_in6_ptr *local6 = (const struct net_sockaddr_in6_ptr *)local;
	const struct net_sockaddr_in_ptr *local4 = (const struct net_sockaddr_in_ptr *)local;
	struct net_sockaddr_in6 *remote6 = (struct net_sockaddr_in6 *)remote;
	struct net_sockaddr_in *remote4 = (struct net_sockaddr_in *)remote;
	/* FIXME: Check the TCP option are passed to the accepted socket */
	sli_si91x_socket_accept_request_t params = {
		.socket_id = sockfd,
	};
	sli_si91x_rsp_ltcp_est_t *reply;
	struct net_buf *reply_buf;
	uint32_t status;
	int newsockfd;

	/* FIXME: Why NWP need that infomaiton? */
	switch (local->family) {
	case NET_AF_INET:
		params.source_port = net_ntohs(local4->sin_port);
		break;
	case NET_AF_INET6:
		params.source_port = net_ntohs(local6->sin6_port);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SOCKET_ACCEPT,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(reply_buf, sizeof(struct siwx91x_frame_desc));
	newsockfd = reply->socket_id;
	if (!remote) {
		goto end;
	}
	switch (reply->ip_version) {
	case SL_IPV4_ADDRESS_LENGTH:
		remote->sa_family = NET_AF_INET;
		remote4->sin_port = net_ntohs(reply->dest_port);
		memcpy(&remote4->sin_addr, &reply->dest_ip_addr, sizeof(remote4->sin_addr));
		break;
	case SL_IPV6_ADDRESS_LENGTH:
		remote->sa_family = NET_AF_INET6;
		remote6->sin6_port = net_ntohs(reply->dest_port);
		siwx91x_nwp_convert_ipv6_format(&remote6->sin6_addr, &reply->dest_ip_addr);
	default:
		__ASSERT(0, "Corrupted frame");
	}
end:
	net_buf_unref(reply_buf);
	return newsockfd;
}

int siwx91x_nwp_sock_close(const struct device *dev, int sockfd)
{
	sli_si91x_socket_close_request_t params = {
		.socket_id = sockfd,
		/* FIXME: what is the purpose of field port_number? */
		.port_number = 0,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SOCKET_CLOSE,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	return status ? -EIO : 0;
}

int siwx91x_nwp_sock_recv(const struct device *dev, int sockfd, struct net_buf **reply_buf)
{
	sli_si91x_req_socket_read_t params = {
		.socket_id = sockfd,
	};
	/* struct is identical for recv and send */
	sli_si91x_socket_send_request_t *reply;
	uint32_t status;

	__ASSERT(reply_buf, "Invalid call");
	sys_put_be32(NET_ETH_MTU, params.requested_bytes);

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SOCKET_READ_DATA,
				      SLI_WLAN_MGMT_Q, 0, reply_buf);
	if (status) {
		return -EIO;
	}
	__ASSERT(*reply_buf, "Corrupted NWP reply");

	reply = net_buf_pull(*reply_buf, sizeof(struct siwx91x_frame_desc));
	net_buf_pull(*reply_buf, reply->data_offset);
	return 0;
}

void siwx91x_nwp_sock_select(const struct device *dev, int select_id,
			     uint32_t read_fds, uint32_t write_fds)
{
	sli_si91x_socket_select_req_t params = {
		.select_id = select_id,
		.no_timeout = 1,
		.read_fds.fd_array[0] = read_fds,
		.write_fds.fd_array[0] = write_fds,
		.num_fd = 32 - MIN(u32_count_leading_zeros(read_fds),
				   u32_count_leading_zeros(write_fds)),
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_SELECT_REQUEST,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_set_config(const struct device *dev, uint16_t type, uint16_t value)
{
	sli_wifi_config_request_t params = {
		.config_type = type,
		.value = value,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WIFI_REQ_CONFIG,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_get_firmware_version(const struct device *dev,
				      struct siwx91x_nwp_version *reply)
{
	struct net_buf *reply_buf;
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_WLAN_REQ_FULL_FW_VERSION,
				      SLI_WLAN_MGMT_Q, 0, &reply_buf);
	__ASSERT(!status, "Corrupted NWP reply");

	net_buf_linearize(reply, sizeof(struct siwx91x_nwp_version),
			  reply_buf, sizeof(struct siwx91x_frame_desc), SIZE_MAX);
	net_buf_unref(reply_buf);
}

void siwx91x_nwp_opermode(const struct device *dev,
			  sl_wifi_system_boot_configuration_t *params)
{
	uint32_t status;

	__ASSERT(params->coex_mode != SL_SI91X_WLAN_MODE, "You mean SL_SI91X_WLAN_ONLY_MODE");
	__ASSERT(params->oper_mode != SL_SI91X_CONCURRENT_MODE, "Not supported");

	status = siwx91x_nwp_send_cmd(dev, params, sizeof(*params), SLI_WIFI_REQ_OPERMODE,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_dynamic_pool(const struct device *dev, int tx, int rx, int global)
{
	sl_wifi_system_dynamic_pool_t params = {
		.tx_ratio_in_buffer_pool = tx,
		.rx_ratio_in_buffer_pool = rx,
		.global_ratio_in_buffer_pool = global,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_WLAN_REQ_DYNAMIC_POOL,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

/* This is a redefintion of sli_si91x_request_ta2m4_t, but without allocating the full
 * input_data field.
 */
struct siwx91x_nwp_req_ta2m4 {
	uint8_t  sub_cmd;
	uint32_t addr;
	uint16_t input_buffer_length;
	uint8_t  flash_sector_erase_enable;
	uint8_t  input_data[];
} __packed;

int siwx91x_nwp_flash_erase(const struct device *dev, uint32_t dest, size_t len)
{
	struct siwx91x_nwp_req_ta2m4 params = {
		.sub_cmd = SL_SI91X_WRITE_TO_COMMON_FLASH,
		.flash_sector_erase_enable = 1,
		.input_buffer_length = len,
		.addr = dest,
	};
	uint32_t status;

	__ASSERT(len <= FLASH_SECTOR_SIZE, "Corrupted argument");

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_COMMON_REQ_TA_M4_COMMANDS,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	return status ? -EINVAL : 0;
}

int siwx91x_nwp_flash_write(const struct device *dev, uint32_t dest,
			    const void *data_buf, size_t data_len)
{
	struct siwx91x_nwp_req_ta2m4 params = {
		.sub_cmd = SL_SI91X_WRITE_TO_COMMON_FLASH,
		.flash_sector_erase_enable = 0,
		.input_buffer_length = data_len,
		.addr = dest,
	};
	uint32_t status;

	__ASSERT(data_len <= MAX_CHUNK_SIZE, "Corrupted argument");

	status = siwx91x_nwp_send_cmd2(dev, &params, sizeof(params), data_buf, data_len,
				       SLI_COMMON_REQ_TA_M4_COMMANDS, SLI_WLAN_MGMT_Q, 0, NULL);
	return status ? -EINVAL : 0;
}

/* This is a redefintion of sli_si91x_req_fwup_t, but without allocating the full content
 * field.
 */
struct siwx91x_nwp_req_fwup {
	uint16_t type;
	uint16_t length;
	uint8_t content[];
};

int siwx91x_nwp_fw_upgrade_start(const struct device *dev, const void *hdr)
{
	struct siwx91x_nwp_req_fwup params = {
		.type = SL_FWUP_RPS_HEADER,
		.length = SLI_RPS_HEADER_SIZE,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd2(dev, &params, sizeof(params), hdr, SLI_RPS_HEADER_SIZE,
				       SLI_WLAN_REQ_FWUP, SLI_WLAN_MGMT_Q, 0, NULL);

	switch (status) {
	case 0:
		return 0;
	case SL_STATUS_SI91X_FW_UPDATE_FAILED:
		return -EIO;
	default:
		return -EINVAL;
	}
}

int siwx91x_nwp_fw_upgrade_write(const struct device *dev, const void *buf, size_t len)
{
	struct siwx91x_nwp_req_fwup params = {
		.type = SL_FWUP_RPS_CONTENT,
		.length = len,
	};
	uint32_t status;

	if (len > SLI_MAX_FWUP_CHUNK_SIZE) {
		return -EINVAL;
	}

	status = siwx91x_nwp_send_cmd2(dev, &params, sizeof(params), buf, len,
				      SLI_WLAN_REQ_FWUP, SLI_WLAN_MGMT_Q, 0, NULL);

	switch (status) {
	case 0:
		return 0;
	case SL_STATUS_SI91X_FW_UPDATE_DONE:
		return 1;
	case SL_STATUS_SI91X_FW_UPDATE_FAILED:
		return -EIO;
	default:
		return -EINVAL;
	}
}

void siwx91x_nwp_feature(const struct device *dev, bool enable_pll)
{
	sli_si91x_feature_frame_request params = {
		.rf_type = RSI_INTERNAL_RF,
		.wireless_mode = SL_WIFI_HP_CHAIN,
		.enable_ppp = 0,
		.afe_type = 1,
		.feature_enables = SLI_FEAT_FRAME_PREAMBLE_DUTY_CYCLE |
				   SLI_FEAT_FRAME_LP_CHAIN |
				   SLI_FEAT_FRAME_IN_PACKET_DUTY_CYCLE,
		.pll_mode = enable_pll ? 1 : 0,
	};
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, &params, sizeof(params), SLI_COMMON_REQ_FEATURE_FRAME,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_soft_reset(const struct device *dev)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_COMMON_REQ_SOFT_RESET,
				      SLI_WLAN_MGMT_Q, SIWX91X_FRAME_FLAG_NO_LOCK, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}

void siwx91x_nwp_force_assert(const struct device *dev)
{
	uint32_t status;

	status = siwx91x_nwp_send_cmd(dev, NULL, 0, SLI_COMMON_REQ_ASSERT,
				      SLI_WLAN_MGMT_Q, 0, NULL);
	__ASSERT(!status, "Corrupted NWP reply");
}
