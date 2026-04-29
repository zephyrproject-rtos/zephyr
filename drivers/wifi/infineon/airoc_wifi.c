/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief AIROC Wi-Fi driver.
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>
#include <airoc_wifi.h>
#include <airoc_whd_hal_common.h>
#include <whd_wlioctl.h>

LOG_MODULE_REGISTER(infineon_airoc_wifi, CONFIG_WIFI_LOG_LEVEL);

#ifndef AIROC_WIFI_TX_PACKET_POOL_COUNT
#define AIROC_WIFI_TX_PACKET_POOL_COUNT (10)
#endif

#ifndef AIROC_WIFI_RX_PACKET_POOL_COUNT
#define AIROC_WIFI_RX_PACKET_POOL_COUNT (10)
#endif

#ifndef AIROC_WIFI_PACKET_POOL_SIZE
#define AIROC_WIFI_PACKET_POOL_SIZE (2400)
#endif

#ifndef AIROC_WIFI_ASSOC_INFO_MAX_SIZE
#define AIROC_WIFI_ASSOC_INFO_MAX_SIZE (512)
#endif

#ifndef AIROC_WIFI_TX_BIT_RATE_CONVERTER
#define AIROC_WIFI_TX_BIT_RATE_CONVERTER (500)
#endif

#ifndef AIROC_WIFI_MGMT_TX_MAX_SIZE
#define AIROC_WIFI_MGMT_TX_MAX_SIZE (256)
#endif

#define AIROC_WIFI_PACKET_POOL_COUNT                                                               \
	(AIROC_WIFI_TX_PACKET_POOL_COUNT + AIROC_WIFI_RX_PACKET_POOL_COUNT)

#define AIROC_WIFI_WAIT_SEMA_MS    (30 * 1000)
#define AIROC_WIFI_SCAN_TIMEOUT_MS (12 * 1000)

/* AIROC private functions */
static whd_result_t airoc_wifi_host_buffer_get(whd_buffer_t *buffer, whd_buffer_dir_t direction,
					       uint16_t size, uint32_t timeout_ms);
static void airoc_wifi_buffer_release(whd_buffer_t buffer, whd_buffer_dir_t direction);
static uint8_t *airoc_wifi_buffer_get_current_piece_data_pointer(whd_buffer_t buffer);
static uint16_t airoc_wifi_buffer_get_current_piece_size(whd_buffer_t buffer);
static whd_result_t airoc_wifi_buffer_set_size(whd_buffer_t buffer, unsigned short size);
static whd_result_t airoc_wifi_buffer_add_remove_at_front(whd_buffer_t *buffer,
							  int32_t add_remove_amount);
static void airoc_wifi_network_process_ethernet_data(whd_interface_t interface,
						     whd_buffer_t buffer);
int airoc_wifi_init_primary(const struct device *dev, whd_interface_t *interface,
			    whd_netif_funcs_t *netif_funcs, whd_buffer_funcs_t *buffer_if);

/* Allocate network pool */
NET_BUF_POOL_FIXED_DEFINE(airoc_pool, AIROC_WIFI_PACKET_POOL_COUNT, AIROC_WIFI_PACKET_POOL_SIZE, 0,
			  NULL);

/* AIROC globals */
static uint16_t ap_event_handler_index = 0xFF;

/* Use global iface pointer to support any Ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *airoc_wifi_iface;

static whd_interface_t airoc_if;
static whd_interface_t airoc_sta_if;
static whd_interface_t airoc_ap_if;

static const whd_event_num_t sta_link_events[] = {WLC_E_LINK,
						  WLC_E_ASSOC,
						  WLC_E_SET_SSID,
						  WLC_E_JOIN,
						  WLC_E_EAPOL_MSG,
						  WLC_E_ESCAN_RESULT,
						  WLC_E_SCAN_COMPLETE,
						  WLC_E_DEAUTH_IND,
						  WLC_E_DISASSOC_IND,
						  WLC_E_PSK_SUP,
						  WLC_E_CSA_COMPLETE_IND,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
						  WLC_E_EXT_AUTH_REQ,
						  WLC_E_EXT_AUTH_FRAME_RX,
#endif
						  WLC_E_NONE};

static const whd_event_num_t ap_link_events[] = {WLC_E_DISASSOC_IND, WLC_E_DEAUTH_IND,
						 WLC_E_ASSOC_IND,    WLC_E_REASSOC_IND,
						 WLC_E_AUTHORIZED,   WLC_E_NONE};

static uint16_t sta_event_handler_index = 0xFF;
static void airoc_event_task(void);
static struct airoc_wifi_data airoc_wifi_data = {0};

#if defined(SPI_DATA_IRQ_SHARED)
PINCTRL_DT_INST_DEFINE(0);
#endif

static struct airoc_wifi_config airoc_wifi_config = {
#if defined(CONFIG_AIROC_WIFI_BUS_SDIO)
	.bus_dev.bus_sdio = DEVICE_DT_GET(DT_INST_PARENT(0)),
#elif defined(CONFIG_AIROC_WIFI_BUS_SPI)
	.bus_dev.bus_spi = SPI_DT_SPEC_GET(DT_DRV_INST(0), AIROC_WIFI_SPI_OPERATION),
	.bus_select_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(0), bus_select_gpios, {0}),
#if defined(SPI_DATA_IRQ_SHARED)
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#endif
#endif
	.wifi_reg_on_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(0), wifi_reg_on_gpios, {0}),
	.wifi_host_wake_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(0), wifi_host_wake_gpios, {0}),
	.wifi_dev_wake_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(0), wifi_dev_wake_gpios, {0}),
};

static whd_buffer_funcs_t airoc_wifi_buffer_if_default = {
	.whd_host_buffer_get = airoc_wifi_host_buffer_get,
	.whd_buffer_release = airoc_wifi_buffer_release,
	.whd_buffer_get_current_piece_data_pointer =
		airoc_wifi_buffer_get_current_piece_data_pointer,
	.whd_buffer_get_current_piece_size = airoc_wifi_buffer_get_current_piece_size,
	.whd_buffer_set_size = airoc_wifi_buffer_set_size,
	.whd_buffer_add_remove_at_front = airoc_wifi_buffer_add_remove_at_front,
};

static whd_netif_funcs_t airoc_wifi_netif_if_default = {
	.whd_network_process_ethernet_data = airoc_wifi_network_process_ethernet_data,
};

K_MSGQ_DEFINE(airoc_wifi_msgq, sizeof(whd_event_header_t), 10, 4);
K_THREAD_STACK_DEFINE(airoc_wifi_event_stack, CONFIG_AIROC_WIFI_EVENT_TASK_STACK_SIZE);
static struct k_thread airoc_wifi_event_thread;

struct airoc_wifi_event_t {
	uint8_t is_ap_event;
	whd_event_data_t *whd_event_data;
};

/*
 * AIROC Wi-Fi helper functions
 */
whd_interface_t airoc_wifi_get_whd_interface(void)
{
	return airoc_if;
}

static void airoc_wifi_scan_cb_search(whd_scan_result_t **result_ptr, void *user_data,
				      whd_scan_status_t status)
{
	if (status == WHD_SCAN_ABORTED) {
		k_sem_give(&airoc_wifi_data.sema_scan);
		return;
	}

	if (status == WHD_SCAN_COMPLETED_SUCCESSFULLY) {
		k_sem_give(&airoc_wifi_data.sema_scan);
	} else if ((status == WHD_SCAN_INCOMPLETE) && (user_data != NULL) &&
		   ((**result_ptr).SSID.length == ((whd_scan_result_t *)user_data)->SSID.length)) {
		if (strncmp(((whd_scan_result_t *)user_data)->SSID.value, (**result_ptr).SSID.value,
			    (**result_ptr).SSID.length) == 0) {
			memcpy(user_data, *result_ptr, sizeof(whd_scan_result_t));
		}
	}
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
static void airoc_wifi_supp_scan2_cb(whd_scan_result_t **result_ptr, void *user_data,
				     whd_scan_status_t status)
{
	struct airoc_wifi_data *data = &airoc_wifi_data;

	if ((result_ptr == NULL) || (*result_ptr == NULL)) {
		/* Check for scan complete */
		if (status == WHD_SCAN_COMPLETED_SUCCESSFULLY || status == WHD_SCAN_ABORTED) {
			LOG_DBG("WiFi Scan Done");
		}
		return;
	}

	whd_scan_result_t *sr = *result_ptr;
	wl_bss_info_t *bss_info = &(sr->bss_info);

	if (status == WHD_SCAN_ABORTED) {
		LOG_DBG("WHD_SCAN_ABORTED");
		return;
	}

	if (user_data == NULL) {
		LOG_DBG("WHD-SCAN: user_data NULL");
		return;
	}

	if (status == WHD_SCAN_COMPLETED_SUCCESSFULLY) {
		k_sem_give(&airoc_wifi_data.sema_scan);
	} else if ((status == WHD_SCAN_INCOMPLETE) && (user_data != NULL) &&
		   ((**result_ptr).SSID.length == data->ssid.length)) {
		if (strncmp(data->ssid.value, (**result_ptr).SSID.value,
			    (**result_ptr).SSID.length) == 0) {
			union wpa_event_data event;
			struct scan_info *info = NULL;

			memcpy(&(data->scan_result), *result_ptr, sizeof(whd_scan_result_t));
			memcpy(&(data->scan_bss_info), bss_info, sizeof(wl_bss_info_t));
			memset(&event, 0, sizeof(event));

			info = &event.scan_info;
			info->aborted = 0;
			info->external_scan = 0;
			info->nl_scan_event = 1;

			airoc_wifi_data.airoc_wifi_wpa_supp_dev_callbk_fns.scan_done(
				data->supp_if_ctx, &event);
		}
	} else {
		/* Do nothing for this case now */
		return;
	}
}
#endif

static int convert_whd_security_to_zephyr(whd_security_t security)
{
	int zephyr_security = WIFI_SECURITY_TYPE_UNKNOWN;

	switch (security) {
	case WHD_SECURITY_OPEN:
		zephyr_security = WIFI_SECURITY_TYPE_NONE;
		break;

	case WHD_SECURITY_WEP_PSK:
	case WHD_SECURITY_WEP_SHARED:
		zephyr_security = WIFI_SECURITY_TYPE_WEP;
		break;

	case WHD_SECURITY_WPA2_WPA_MIXED_PSK:
	case WHD_SECURITY_WPA2_WPA_AES_PSK:
	case WHD_SECURITY_WPA3_WPA2_PSK:
		zephyr_security = WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL;
		break;

	case WHD_SECURITY_WPA2_AES_PSK:
	case WHD_SECURITY_WPA2_MIXED_PSK:
		zephyr_security = WIFI_SECURITY_TYPE_PSK;
		break;

	case WHD_SECURITY_WPA2_AES_PSK_SHA256:
		zephyr_security = WIFI_SECURITY_TYPE_PSK_SHA256;
		break;

	case WHD_SECURITY_WPA3_SAE:
		zephyr_security = WIFI_SECURITY_TYPE_SAE;
		break;

	case WHD_SECURITY_WPA_TKIP_PSK:
	case WHD_SECURITY_WPA_MIXED_PSK:
	case WHD_SECURITY_WPA_AES_PSK:
		zephyr_security = WIFI_SECURITY_TYPE_WPA_PSK;
		break;

	default:
		if ((security & ENTERPRISE_ENABLED) != 0) {
			zephyr_security = WIFI_SECURITY_TYPE_EAP;
		}
		break;
	}
	return zephyr_security;
}

static whd_security_t convert_zephyr_security_to_whd(int security)
{
	whd_security_t whd_security = WIFI_SECURITY_TYPE_UNKNOWN;

	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		whd_security = WHD_SECURITY_OPEN;
		break;

	case WIFI_SECURITY_TYPE_WEP:
		whd_security = WHD_SECURITY_WEP_PSK;
		break;

	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		whd_security = WHD_SECURITY_WPA3_WPA2_PSK;
		break;

	case WIFI_SECURITY_TYPE_PSK:
		whd_security = WHD_SECURITY_WPA2_AES_PSK;
		break;

	case WIFI_SECURITY_TYPE_PSK_SHA256:
		whd_security = WIFI_SECURITY_TYPE_PSK_SHA256;
		break;

	case WIFI_SECURITY_TYPE_SAE:
		whd_security = WHD_SECURITY_WPA3_SAE;
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
		whd_security = WHD_SECURITY_WPA_AES_PSK;
		break;

	default:
		break;
	}
	return whd_security;
}

static uint8_t convert_whd_band_to_zephyr(whd_802_11_band_t band)
{
	uint8_t zephyr_band = WIFI_FREQ_BAND_UNKNOWN;

	switch (band) {
	case WHD_802_11_BAND_2_4GHZ:
		zephyr_band = WIFI_FREQ_BAND_2_4_GHZ;
		break;

	case WHD_802_11_BAND_5GHZ:
		zephyr_band = WIFI_FREQ_BAND_5_GHZ;
		break;

	case WHD_802_11_BAND_6GHZ:
		zephyr_band = WIFI_FREQ_BAND_6_GHZ;
		break;
	}
	return zephyr_band;
}

static void parse_scan_result(whd_scan_result_t *p_whd_result, struct wifi_scan_result *p_zy_result)
{
	if (p_whd_result->SSID.length != 0) {
		p_zy_result->ssid_length = p_whd_result->SSID.length;
		strncpy(p_zy_result->ssid, p_whd_result->SSID.value, p_whd_result->SSID.length);
		p_zy_result->channel = p_whd_result->channel;
		p_zy_result->band = convert_whd_band_to_zephyr(p_whd_result->band);
		p_zy_result->security = convert_whd_security_to_zephyr(p_whd_result->security);
		p_zy_result->rssi = (int8_t)p_whd_result->signal_strength;
		p_zy_result->mac_length = 6;
		memcpy(p_zy_result->mac, &p_whd_result->BSSID, 6);
	}
}

static void scan_callback(whd_scan_result_t **result_ptr, void *user_data, whd_scan_status_t status)
{
	struct airoc_wifi_data *data = user_data;
	whd_scan_result_t whd_scan_result;
	struct wifi_scan_result zephyr_scan_result = {0};

	if (status == WHD_SCAN_COMPLETED_SUCCESSFULLY || status == WHD_SCAN_ABORTED) {
		data->scan_rslt_cb(data->iface, 0, NULL);
		data->scan_rslt_cb = NULL;
		/* NOTE: It is complete of scan packet, do not need to clean result_ptr,
		 * WHD will release result_ptr buffer
		 */
		return;
	}

	/* We received scan data so process it */
	if ((result_ptr != NULL) && (*result_ptr != NULL)) {
		memcpy(&whd_scan_result, *result_ptr, sizeof(whd_scan_result_t));
		parse_scan_result(&whd_scan_result, &zephyr_scan_result);
		data->scan_rslt_cb(data->iface, 0, &zephyr_scan_result);
	}
	memset(*result_ptr, 0, sizeof(whd_scan_result_t));
}

/*
 * Implement WHD network buffers functions
 */
static whd_result_t airoc_wifi_host_buffer_get(whd_buffer_t *buffer, whd_buffer_dir_t direction,
					       uint16_t size, uint32_t timeout_ms)
{
	ARG_UNUSED(direction);
	ARG_UNUSED(timeout_ms);
	struct net_buf *buf;

	buf = net_buf_alloc_len(&airoc_pool, size, K_NO_WAIT);
	if ((buf == NULL) || (buf->size < size)) {
		return WHD_BUFFER_ALLOC_FAIL;
	}
	*buffer = buf;

	/* Set buffer size */
	(void)airoc_wifi_buffer_set_size(*buffer, size);

	return WHD_SUCCESS;
}

static void airoc_wifi_buffer_release(whd_buffer_t buffer, whd_buffer_dir_t direction)
{
	CY_UNUSED_PARAMETER(direction);
	(void)net_buf_destroy((struct net_buf *)buffer);
}

static uint8_t *airoc_wifi_buffer_get_current_piece_data_pointer(whd_buffer_t buffer)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf *buf = (struct net_buf *)buffer;

	return (uint8_t *)buf->data;
}

static uint16_t airoc_wifi_buffer_get_current_piece_size(whd_buffer_t buffer)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf *buf = (struct net_buf *)buffer;

	return (uint16_t)buf->size;
}

static whd_result_t airoc_wifi_buffer_set_size(whd_buffer_t buffer, unsigned short size)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf *buf = (struct net_buf *)buffer;

	buf->size = size;
	return CY_RSLT_SUCCESS;
}

static whd_result_t airoc_wifi_buffer_add_remove_at_front(whd_buffer_t *buffer,
							  int32_t add_remove_amount)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf **buf = (struct net_buf **)buffer;

	if (add_remove_amount > 0) {
		(*buf)->len = (*buf)->size;
		(*buf)->data = net_buf_pull(*buf, add_remove_amount);
	} else {
		(*buf)->data = net_buf_push(*buf, -add_remove_amount);
		(*buf)->len = (*buf)->size;
	}
	return WHD_SUCCESS;
}

static int airoc_mgmt_send(const struct device *dev, struct net_pkt *pkt)
{
	struct airoc_wifi_data *data = dev->data;
	cy_rslt_t ret;
	size_t pkt_len = net_pkt_get_len(pkt);
	struct net_buf *buf = NULL;

	/* Read the packet payload */
	if (net_pkt_read(pkt, data->frame_buf, pkt_len) < 0) {
		LOG_ERR("net_pkt_read failed");
		return -EIO;
	}

	/* Allocate Network Buffer from pool with Packet Length + Data Header */
	ret = airoc_wifi_host_buffer_get((whd_buffer_t *)&buf, WHD_NETWORK_TX,
					 pkt_len + sizeof(data_header_t), 0);
	if ((ret != WHD_SUCCESS) || (buf == NULL)) {
		return -EIO;
	}

	/* Reserve the buffer Headroom for WHD Data header */
	net_buf_reserve(buf, sizeof(data_header_t));

	/* Copy the buffer to network Buffer pointer */
	(void)memcpy(buf->data, data->frame_buf, pkt_len);

	/* Call WHD API to send out the Packet */
	ret = whd_network_send_ethernet_data(airoc_if, (void *)buf);
	if (ret != CY_RSLT_SUCCESS) {
		LOG_ERR("whd_network_send_ethernet_data failed");
#if defined(CONFIG_NET_STATISTICS_WIFI)
		data->stats.errors.tx++;
#endif
		return -EIO;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.bytes.sent += pkt_len;
	data->stats.pkts.tx++;
#endif

	return 0;
}

static void airoc_wifi_network_process_ethernet_data(whd_interface_t interface, whd_buffer_t buffer)
{
	struct net_pkt *pkt;
	uint8_t *data = whd_buffer_get_current_piece_data_pointer(interface->whd_driver, buffer);
	uint32_t len = whd_buffer_get_current_piece_size(interface->whd_driver, buffer);
	bool net_pkt_unref_flag = false;

	if ((airoc_wifi_iface != NULL) && net_if_flag_is_set(airoc_wifi_iface, NET_IF_UP)) {

		pkt = net_pkt_rx_alloc_with_buffer(airoc_wifi_iface, len, NET_AF_UNSPEC, 0,
						   K_NO_WAIT);

		if (pkt != NULL) {
			if (net_pkt_write(pkt, data, len) < 0) {
				LOG_ERR("Failed to write pkt");
				net_pkt_unref_flag = true;
			}

			if ((net_pkt_unref_flag) || (net_recv_data(airoc_wifi_iface, pkt) < 0)) {
				LOG_ERR("Failed to push received data");
				net_pkt_unref_flag = true;
			}
		} else {
			LOG_ERR("Failed to get net buffer");
		}
	}

	/* Release a packet buffer */
	airoc_wifi_buffer_release(buffer, WHD_NETWORK_RX);

#if defined(CONFIG_NET_STATISTICS_WIFI)
	airoc_wifi_data.stats.bytes.received += len;
	airoc_wifi_data.stats.pkts.rx++;
#endif

	if (net_pkt_unref_flag) {
		net_pkt_unref(pkt);
#if defined(CONFIG_NET_STATISTICS_WIFI)
		airoc_wifi_data.stats.errors.rx++;
#endif
	}
}

static enum ethernet_hw_caps airoc_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_FILTERING;
}

static int airoc_set_config(const struct device *dev, enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	whd_mac_t whd_mac_addr;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_FILTER:
		for (int i = 0; i < WHD_ETHER_ADDR_LEN; i++) {
			whd_mac_addr.octet[i] = config->filter.mac_address.addr[i];
		}
		if (config->filter.set) {
			whd_wifi_register_multicast_address(airoc_if, &whd_mac_addr);
		} else {
			whd_wifi_unregister_multicast_address(airoc_if, &whd_mac_addr);
		}
		return 0;
	default:
		break;
	}
	return -ENOTSUP;
}

static void *link_events_handler(whd_interface_t ifp, const whd_event_header_t *event_header,
				 const uint8_t *event_data, void *handler_user_data)
{
	ARG_UNUSED(ifp);
	ARG_UNUSED(handler_user_data);
	struct airoc_wifi_event_t airoc_event = {0};

	airoc_event.is_ap_event = 0;
	airoc_event.whd_event_data = (whd_event_data_t *)whd_mem_malloc(sizeof(whd_event_data_t));

	if (airoc_event.whd_event_data) {
		airoc_event.whd_event_data->event_header = (whd_event_header_t *)event_header;
		airoc_event.whd_event_data->event_data = (uint8_t *)event_data;
	}

	k_msgq_put(&airoc_wifi_msgq, &airoc_event, K_FOREVER);
	return NULL;
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
static void airoc_event_handler(whd_event_data_t *event_data)
{
	const struct device *dev = net_if_get_device(airoc_wifi_iface);
	struct airoc_wifi_data *data = dev->data;
	whd_event_header_t *event_header;
	union wpa_event_data event;

	event_header = event_data->event_header;

	switch (event_header->event_type) {
	case WLC_E_SET_SSID:
		LOG_DBG("WLC_E_SET_SSID");
		break;
	case WLC_E_JOIN:
		LOG_DBG("WLC_E_JOIN");
		break;
	case WLC_E_LINK:
		if (event_header->status == WLC_E_STATUS_SUCCESS &&
		    event_header->reason != WLC_E_LINK_DISASSOC) {
			wifi_mgmt_raise_connect_result_event(data->iface, 0);
			LOG_DBG("LINK UP");
		} else if (event_header->reason == WLC_E_LINK_DISASSOC) {
			if (data->is_sta_connected) {
				data->is_sta_connected = false;
			}
			LOG_DBG("LINK DOWN");
		} else {
			LOG_DBG("Unhandled WLC_E_LINK status");
		}
		break;
	case WLC_E_ASSOC:
		if (data->is_sta_connected == false) {
			data->is_sta_connected = true;
			net_if_dormant_off(airoc_wifi_iface);
			memset(&event, 0, sizeof(event));
			event.assoc_info.addr = data->bssid;
			event.assoc_info.authorized = 0;
			event.assoc_info.freq =
				whd_channel2freq((uint)(data->scan_bss_info.ctl_ch));

			if (!whd_get_assoc_ie_info(airoc_sta_if, &data->assoc_info)) {
				if (data->assoc_info.req_ies > 0) {
					event.assoc_info.req_ies = data->assoc_info.req_ies;
					event.assoc_info.req_ies_len = data->assoc_info.req_ies_len;
				}

				if (data->assoc_info.resp_ies > 0) {
					event.assoc_info.resp_ies = data->assoc_info.resp_ies;
					event.assoc_info.resp_ies_len =
						data->assoc_info.resp_ies_len;
				}
			}
			data->airoc_wifi_wpa_supp_dev_callbk_fns.assoc_resp(
				data->supp_if_ctx, &event, WLAN_STATUS_SUCCESS);
		} else {
			LOG_DBG("WLC_E_ASSOC - STA might be already connected");
		}
		break;
	case WLC_E_EAPOL_MSG:
		/* Send associated evet to supp, if its not done */
		if (data->is_sta_connected == false) {
			data->is_sta_connected = true;
			net_if_dormant_off(airoc_wifi_iface);
			memset(&event, 0, sizeof(event));
			event.assoc_info.addr = data->bssid;
			event.assoc_info.authorized = 0;
			event.assoc_info.freq =
				whd_channel2freq((uint)(data->scan_bss_info.ctl_ch));

			if (!whd_get_assoc_ie_info(airoc_sta_if, &data->assoc_info)) {
				if (data->assoc_info.req_ies > 0) {
					event.assoc_info.req_ies = data->assoc_info.req_ies;
					event.assoc_info.req_ies_len = data->assoc_info.req_ies_len;
				}

				if (data->assoc_info.resp_ies > 0) {
					event.assoc_info.resp_ies = data->assoc_info.resp_ies;
					event.assoc_info.resp_ies_len =
						data->assoc_info.resp_ies_len;
				}
			}
			data->airoc_wifi_wpa_supp_dev_callbk_fns.assoc_resp(
				data->supp_if_ctx, &event, WLAN_STATUS_SUCCESS);
		} else {
			LOG_DBG("WLC_E_EAPOL_MSG - STA already associated\n");
		}
		/* No need to handle EAPOL packets here, as it is send to supplicant as L2 packet */
		break;
	case WLC_E_DEAUTH_IND:
	case WLC_E_DISASSOC_IND:
		if (data->is_sta_connected) {
			data->is_sta_connected = false;
		}
		break;
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	case WLC_E_EXT_AUTH_REQ:
		memset(&event, 0, sizeof(event));

		event.external_auth.action = EXT_AUTH_START;
		event.external_auth.ssid_len = data->ssid.length;
		event.external_auth.ssid = data->ssid.value;
		event.external_auth.bssid = data->bssid;
		event.external_auth.key_mgmt_suite = 0x8ac0f00;
		event.external_auth.mld_addr = NULL;

		data->airoc_wifi_wpa_supp_dev_callbk_fns.ext_auth_req(data->supp_if_ctx, &event);
		break;
	case WLC_E_EXT_AUTH_FRAME_RX:
		uint16_t frame_len = event_header->datalen - sizeof(wl_rx_mgmt_data_t);
		int freq = whd_channel2freq((uint)(data->scan_bss_info.ctl_ch));

		data->airoc_wifi_wpa_supp_dev_callbk_fns.mgmt_rx(
			data->supp_if_ctx, &event_data->event_data[sizeof(wl_rx_mgmt_data_t)],
			frame_len, freq, data->scan_bss_info.RSSI);
		break;
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT */
	default:
		break;
	}
}

#if defined(CONFIG_IFX_WIFI_SOFTAP_SUPPORT)
static void airoc_ap_event_handler(whd_event_data_t *event_data)
{
	const struct device *dev = net_if_get_device(airoc_wifi_iface);
	struct airoc_wifi_data *data = dev->data;
	whd_event_header_t *event_header = event_data->event_header;

	switch (event_header->event_type) {
	case WLC_E_ASSOC:
		break;
	case WLC_E_ASSOC_IND:
		/* AP mode: New station associated */
		if (data->supp_if_ctx) {
			union wpa_event_data event;
			/* Allocate buffer for station MAC (was memcpy to NULL pointer) */
			u8 *sta_mac = whd_mem_malloc(ETH_ALEN);

			if (!sta_mac) {
				LOG_DBG("Failed to alloc sta_mac");
				break;
			}

			memset(&event, 0, sizeof(event));

			memcpy(sta_mac, event_header->addr.octet, ETH_ALEN);
			event.assoc_info.addr = sta_mac;

			/* For AP mode in wpa_supplicant, we still need to use assoc_resp callback
			 * but we need to ensure the correct MAC address is passed
			 */
			if (data->airoc_wifi_wpa_supp_dev_callbk_fns.assoc_resp) {
				data->airoc_wifi_wpa_supp_dev_callbk_fns.assoc_resp(
					data->supp_if_ctx, &event, WLAN_STATUS_SUCCESS);
			}
			whd_mem_free(sta_mac);
		}
		break;
	case WLC_E_DEAUTH_IND:
	case WLC_E_DISASSOC_IND:
		break;
	default:
		LOG_ERR("Unhandled AP event");
		break;
	}
}
#endif
#endif

static void airoc_event_task(void)
{
	struct airoc_wifi_event_t airoc_event;
	whd_event_header_t event_header;

	while (1) {
		k_msgq_get(&airoc_wifi_msgq, &airoc_event, K_FOREVER);
		if (!airoc_event.whd_event_data) {
			LOG_ERR("airoc_event.whd_event_data is NULL");
			continue;
		}
		memcpy(&event_header, airoc_event.whd_event_data->event_header,
		       sizeof(whd_event_header_t));
		LOG_DBG("Event type in airoc: %d, is_ap_event: %d\n",
			(whd_event_num_t)event_header.event_type, airoc_event.is_ap_event);
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
		if (!airoc_event.is_ap_event) {
			airoc_event_handler(airoc_event.whd_event_data);
		}
#if defined(CONFIG_IFX_WIFI_SOFTAP_SUPPORT)
		else {
			airoc_ap_event_handler(airoc_event.whd_event_data);
		}
#endif
#endif
		switch ((whd_event_num_t)event_header.event_type) {
		case WLC_E_LINK:
			break;

		case WLC_E_DEAUTH_IND:
		case WLC_E_DISASSOC_IND:
			net_if_dormant_on(airoc_wifi_iface);
			break;

		default:
			break;
		}
		whd_mem_free(airoc_event.whd_event_data);
	}
}

static void airoc_mgmt_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct airoc_wifi_data *data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	data->iface = iface;
	airoc_wifi_iface = iface;
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	wifi_nm_register_mgd_type_iface(wifi_nm_get_instance("wifi_supplicant"), WIFI_TYPE_STA,
					iface);
#endif
	/* Read WLAN MAC Address */
	if (whd_wifi_get_mac_address(airoc_sta_if, &airoc_sta_if->mac_addr) != WHD_SUCCESS) {
		LOG_ERR("Failed to get mac address");
	} else {
		(void)memcpy(&data->mac_addr, &airoc_sta_if->mac_addr,
			     sizeof(airoc_sta_if->mac_addr));
	}

	/* Assign link local address. */
	if (net_if_set_link_addr(iface, data->mac_addr, 6, NET_LINK_ETHERNET)) {
		LOG_ERR("Failed to set link addr");
	}

	/* Initialize Ethernet L2 stack */
	ethernet_init(iface);

	/* Not currently connected to a network */
	net_if_dormant_on(iface);
}

static int airoc_mgmt_scan(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	struct airoc_wifi_data *data = dev->data;
	enum wifi_scan_type scan_type = WIFI_SCAN_TYPE_ACTIVE;

	if (params != NULL) {
		scan_type = params->scan_type;
	}

	if (data->scan_rslt_cb != NULL) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	data->scan_rslt_cb = cb;
	memset(&data->scan_result, 0, sizeof(whd_scan_result_t));
	memset(&data->ssid, 0, sizeof(whd_ssid_t));

	/* Connect to the network */
	if (whd_wifi_scan(airoc_sta_if, scan_type, WHD_BSS_TYPE_ANY, &(data->ssid), NULL, NULL,
			  NULL, scan_callback, &(data->scan_result), data) != WHD_SUCCESS) {
		LOG_ERR("Failed to start scan");
		k_sem_give(&data->sema_common);
		return -EAGAIN;
	}

	k_sem_give(&data->sema_common);
	return 0;
}

static bool is_invalid_security(int security, uint8_t psk_length)
{
	return ((security == WIFI_SECURITY_TYPE_NONE) && (psk_length > 0));
}

static int airoc_mgmt_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)dev->data;
	int ret = 0;
	whd_scan_result_t scan_result;
	whd_scan_result_t usr_result = {0};
	/* Try to scan ssid to define security */
	whd_scan_result_t tmp_result = {0};

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	if (data->is_sta_connected) {
		LOG_ERR("Already connected");
		ret = -EALREADY;
		goto error;
	}

	if (data->is_ap_up) {
		LOG_ERR("Network interface is busy AP. Please first disable AP.");
		ret = -EBUSY;
		goto error;
	}

	usr_result.SSID.length = params->ssid_length;
	memcpy(usr_result.SSID.value, params->ssid, params->ssid_length);
	usr_result.security = convert_zephyr_security_to_whd(params->security);

	if (is_invalid_security(params->security, params->psk_length)) {

		if (whd_wifi_scan(airoc_sta_if, WHD_SCAN_TYPE_ACTIVE, WHD_BSS_TYPE_ANY, NULL, NULL,
				  NULL, NULL, airoc_wifi_scan_cb_search, &scan_result,
				  &(tmp_result)) != WHD_SUCCESS) {
			LOG_ERR("Failed start scan");
			ret = -EAGAIN;
			goto error;
		}

		if (k_sem_take(&airoc_wifi_data.sema_scan, K_MSEC(AIROC_WIFI_SCAN_TIMEOUT_MS)) !=
		    0) {
			whd_wifi_stop_scan(airoc_sta_if);
			ret = -EAGAIN;
			goto error;
		}
	} else {
		/* Fallback to user input */
		if (tmp_result.security == WHD_SECURITY_UNKNOWN) {
			usr_result.security = tmp_result.security;
		}
	}

	if (usr_result.security == WHD_SECURITY_UNKNOWN) {
		ret = -EAGAIN;
		LOG_ERR("Could not scan device");
		goto error;
	}

	if (usr_result.security == WHD_SECURITY_WPA3_WPA2_PSK) {
		LOG_WRN("Given security is WPA2-WPA3 mixed, selecting WPA2-AES");
		usr_result.security = WHD_SECURITY_WPA2_AES_PSK;
	}

	/* Connect to the network */
	if (whd_wifi_join(airoc_sta_if, &usr_result.SSID, usr_result.security, params->psk,
			  params->psk_length) != WHD_SUCCESS) {
		LOG_ERR("Failed to connect with network");

		ret = -EAGAIN;
		goto error;
	}

error:
	if (ret < 0) {
		net_if_dormant_on(data->iface);
	} else {
		net_if_dormant_off(data->iface);
		data->is_sta_connected = true;
#if defined(CONFIG_NET_DHCPV4)
		net_dhcpv4_restart(data->iface);
#endif /* defined(CONFIG_NET_DHCPV4) */
	}

	wifi_mgmt_raise_connect_result_event(data->iface, ret);
	k_sem_give(&data->sema_common);
	return ret;
}

static int airoc_mgmt_disconnect(const struct device *dev)
{
	int ret = 0;
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)dev->data;

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	if (whd_wifi_leave(airoc_sta_if) != WHD_SUCCESS) {
		k_sem_give(&data->sema_common);
		ret = -EAGAIN;
	} else {
		data->is_sta_connected = false;
		net_if_dormant_on(data->iface);
	}

	wifi_mgmt_raise_disconnect_result_event(data->iface, ret);
	k_sem_give(&data->sema_common);

	return ret;
}

static void *airoc_wifi_ap_link_events_handler(whd_interface_t ifp,
					       const whd_event_header_t *event_header,
					       const uint8_t *event_data, void *handler_user_data)
{
	struct airoc_wifi_event_t airoc_event = {0};

	airoc_event.is_ap_event = 1;
	airoc_event.whd_event_data = (whd_event_data_t *)whd_mem_malloc(sizeof(whd_event_data_t));

	if (airoc_event.whd_event_data) {
		airoc_event.whd_event_data->event_header = (whd_event_header_t *)event_header;
		airoc_event.whd_event_data->event_data = (uint8_t *)event_data;
	}
	k_msgq_put(&airoc_wifi_msgq, &airoc_event, K_FOREVER);

	return NULL;
}

static int airoc_mgmt_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct airoc_wifi_data *data = dev->data;
	whd_security_t security;
	whd_ssid_t ssid;
	uint16_t chanspec;
	int ret = 0;

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	if (data->is_sta_connected) {
		LOG_ERR("Network interface is busy in STA mode. Please first disconnect STA.");
		ret = -EBUSY;
		goto error;
	}

	if (data->is_ap_up) {
		LOG_ERR("Already AP is on - first disable");
		ret = -EAGAIN;
		goto error;
	}

	if (!data->second_interface_init) {
		if (whd_add_secondary_interface(data->whd_drv, NULL, &airoc_ap_if) !=
		    CY_RSLT_SUCCESS) {
			LOG_ERR("Error Unable to bring up the whd secondary interface");
			ret = -EAGAIN;
			goto error;
		}
		data->second_interface_init = true;
	}

	ssid.length = params->ssid_length;
	memcpy(ssid.value, params->ssid, ssid.length);

	/* make sure to set valid channels for 2G and 5G:
	 * - 2G channels from 1 to 11,
	 * - 5G channels from 36 to 165
	 */
	if ((params->channel > 0) && (params->channel < 12)) {
		chanspec = params->channel | GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BAND_2G);
	} else if ((params->channel > 35) && (params->channel < 166)) {
		chanspec = params->channel | GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BAND_5G);
	} else {
		chanspec = 1 | GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BAND_2G);
		LOG_WRN("Discard of setting unsupported channel: %u (will set 1)", params->channel);
	}

	switch (params->bandwidth) {
	case WIFI_FREQ_BANDWIDTH_20MHZ:
		chanspec |= GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BW_20);
		break;
	case WIFI_FREQ_BANDWIDTH_40MHZ:
		chanspec |= GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BW_40);
		break;
	case WIFI_FREQ_BANDWIDTH_80MHZ:
		chanspec |= GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BW_80);
		break;
	default:
		chanspec |= GET_C_VAR(airoc_ap_if->whd_driver, CHANSPEC_BW_20);
		LOG_WRN("Discard of setting unsupported bandwidth: %u (will set 20MHz)",
			params->bandwidth);
		break;
	}

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		security = WHD_SECURITY_OPEN;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		security = WHD_SECURITY_WPA2_AES_PSK;
		break;
	case WIFI_SECURITY_TYPE_SAE:
		security = WHD_SECURITY_WPA3_SAE;
		break;
	default:
		goto error;
	}

	if (whd_wifi_init_ap(airoc_ap_if, &ssid, security, (const uint8_t *)params->psk,
			     params->psk_length, chanspec) != 0) {
		LOG_ERR("Failed to init whd ap interface");
		ret = -EAGAIN;
		goto error;
	}

	if (whd_wifi_start_ap(airoc_ap_if) != 0) {
		LOG_ERR("Failed to start whd ap interface");
		ret = -EAGAIN;
		goto error;
	}

	/* set event handler */
	if (whd_management_set_event_handler(airoc_ap_if, ap_link_events,
					     airoc_wifi_ap_link_events_handler, NULL,
					     &ap_event_handler_index) != 0) {
		whd_wifi_stop_ap(airoc_ap_if);
		ret = -EAGAIN;
		goto error;
	}

	data->is_ap_up = true;
	airoc_if = airoc_ap_if;
	net_if_dormant_off(data->iface);
error:

	k_sem_give(&data->sema_common);
	return ret;
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int airoc_mgmt_wifi_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	struct airoc_wifi_data *data = dev->data;

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

static int airoc_mgmt_ap_disable(const struct device *dev)
{
	cy_rslt_t whd_ret;
	struct airoc_wifi_data *data = dev->data;

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	if (whd_wifi_deregister_event_handler(airoc_ap_if, ap_event_handler_index)) {
		LOG_ERR("Can't whd_wifi_deregister_event_handler");
	}

	whd_ret = whd_wifi_stop_ap(airoc_ap_if);
	if (whd_ret == CY_RSLT_SUCCESS) {
		data->is_ap_up = false;
		airoc_if = airoc_sta_if;
		net_if_dormant_on(data->iface);
	} else {
		LOG_ERR("Can't stop wifi ap: %u", whd_ret);
	}

	k_sem_give(&data->sema_common);

	if (whd_ret != CY_RSLT_SUCCESS) {
		return -ENODEV;
	}

	return 0;
}

static int airoc_iface_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct airoc_wifi_data *data = dev->data;
	whd_result_t result;
	wl_bss_info_t bss_info;
	whd_security_t security_info = 0;
	uint32_t wpa_data_rate_value = 0;
	uint32_t join_status;

	if (airoc_if == NULL) {
		return -ENOTSUP;
	}

	status->iface_mode =
		(data->is_ap_up ? WIFI_MODE_AP
				: (data->is_sta_connected ? WIFI_MODE_INFRA : WIFI_MODE_UNKNOWN));

	join_status = whd_wifi_is_ready_to_transceive(airoc_if);

	if (join_status == WHD_SUCCESS) {
		status->state = WIFI_STATE_COMPLETED;
	} else if (join_status == WHD_JOIN_IN_PROGRESS) {
		status->state = WIFI_STATE_ASSOCIATING;
	} else if (join_status == WHD_NOT_KEYED) {
		status->state = WIFI_STATE_AUTHENTICATING;
	} else {
		status->state = WIFI_STATE_DISCONNECTED;
	}

	result = whd_wifi_get_ap_info(airoc_if, &bss_info, &security_info);

	if (result == WHD_SUCCESS) {
		memcpy(&(status->bssid[0]), &(bss_info.BSSID), sizeof(whd_mac_t));

		whd_wifi_get_channel(airoc_if, (int *)&status->channel);

		status->band = (status->channel <= CH_MAX_2G_CHANNEL) ? WIFI_FREQ_BAND_2_4_GHZ
								      : WIFI_FREQ_BAND_5_GHZ;

		status->rssi = (int)bss_info.RSSI;

		status->ssid_len = bss_info.SSID_len;
		strncpy(status->ssid, bss_info.SSID, status->ssid_len);

		status->security = convert_whd_security_to_zephyr(security_info);

		status->beacon_interval = (unsigned short)bss_info.beacon_period;
		status->dtim_period = (unsigned char)bss_info.dtim_period;

		status->twt_capable = false;
	}

	whd_wifi_get_ioctl_value(airoc_if, WLC_GET_RATE, &wpa_data_rate_value);
	status->current_phy_tx_rate = wpa_data_rate_value;

	/* Unbelievably, this appears to be the only way to determine the phy mode with
	 *  the whd SDK that we're currently using. Note that the logic below is only valid on
	 *  devices that are limited to the 2.4Ghz band. Other versions of the SDK and chip
	 * evidently allow one to obtain a phy_mode value directly from bss_info
	 */
	if (wpa_data_rate_value > 54) {
		status->link_mode = WIFI_4;
	} else if (wpa_data_rate_value == 6 || wpa_data_rate_value == 9 ||
		   wpa_data_rate_value == 12 || wpa_data_rate_value == 18 ||
		   wpa_data_rate_value == 24 || wpa_data_rate_value == 36 ||
		   wpa_data_rate_value == 48 || wpa_data_rate_value == 54) {
		status->link_mode = WIFI_3;
	} else {
		status->link_mode = WIFI_1;
	}

	return 0;
}

static int airoc_init(const struct device *dev)
{
	int ret;
	cy_rslt_t whd_ret;
	struct airoc_wifi_data *data = dev->data;

	k_tid_t tid = k_thread_create(
		&airoc_wifi_event_thread, airoc_wifi_event_stack,
		CONFIG_AIROC_WIFI_EVENT_TASK_STACK_SIZE, (k_thread_entry_t)airoc_event_task, NULL,
		NULL, NULL, CONFIG_AIROC_WIFI_EVENT_TASK_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	if (!tid) {
		LOG_ERR("ERROR spawning tx thread");
		return -EAGAIN;
	}
	k_thread_name_set(tid, "airoc_event");

	whd_ret = airoc_wifi_init_primary(dev, &airoc_sta_if, &airoc_wifi_netif_if_default,
					  &airoc_wifi_buffer_if_default);
	if (whd_ret != CY_RSLT_SUCCESS) {
		LOG_ERR("airoc_wifi_init_primary failed ret = %d \r\n", whd_ret);
		return -EAGAIN;
	}
	airoc_if = airoc_sta_if;

	whd_ret = whd_management_set_event_handler(
		airoc_sta_if, sta_link_events, link_events_handler, NULL, &sta_event_handler_index);
	if (whd_ret != CY_RSLT_SUCCESS) {
		LOG_ERR("whd_management_set_event_handler failed ret = %d \r\n", whd_ret);
		return -EAGAIN;
	}

	ret = k_sem_init(&data->sema_common, 1, 1);
	if (ret != 0) {
		LOG_ERR("k_sem_init(sema_common) failure");
		return ret;
	}

	ret = k_sem_init(&data->sema_scan, 0, 1);
	if (ret != 0) {
		LOG_ERR("k_sem_init(sema_scan) failure");
		return ret;
	}

	if (ret != 0) {
		LOG_ERR("k_sem_init(sema_scan) failure");
		return ret;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	data->assoc_info.req_ies = whd_mem_malloc(AIROC_WIFI_ASSOC_INFO_MAX_SIZE);
	data->assoc_info.resp_ies = whd_mem_malloc(AIROC_WIFI_ASSOC_INFO_MAX_SIZE);
	data->mgmt_tx_data = whd_mem_malloc(AIROC_WIFI_MGMT_TX_MAX_SIZE);
	if (!data->assoc_info.req_ies || !data->assoc_info.resp_ies || !data->mgmt_tx_data) {
		if (data->assoc_info.req_ies) {
			whd_mem_free(data->assoc_info.req_ies);
		}

		if (data->assoc_info.resp_ies) {
			whd_mem_free(data->assoc_info.resp_ies);
		}

		if (data->mgmt_tx_data) {
			whd_mem_free(data->mgmt_tx_data);
		}

		LOG_ERR("assoc_info allocation memory failure");
		return -ENOMEM;
	}
#endif

	return 0;
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
void *airoc_wifi_wpa_supp_dev_init(void *supp_drv_if_ctx, const char *iface_name,
				   struct zep_wpa_supp_dev_callbk_fns *callbk_fns)
{
	const struct device *dev = device_get_binding(iface_name);

	if (dev == NULL) {
		LOG_ERR("Failed to get device, NULL");
		return &airoc_wifi_data;
	}

	struct airoc_wifi_data *data = (struct airoc_wifi_data *)dev->data;

	whd_wifi_read_fw_capabilities(airoc_sta_if);
	data->supp_if_ctx = supp_drv_if_ctx;
	whd_mem_memset(&data->capa, 0, sizeof(struct wpa_driver_capa));
	memcpy(&data->airoc_wifi_wpa_supp_dev_callbk_fns, callbk_fns,
	       sizeof(data->airoc_wifi_wpa_supp_dev_callbk_fns));
	return data;
}

int airoc_wifi_wpa_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa)
{
	memset(capa, 0, sizeof(*capa));

	capa->key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA | WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
			 WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK | WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_SAE | WPA_DRIVER_CAPA_KEY_MGMT_802_1X_SHA256 |
			 WPA_DRIVER_CAPA_KEY_MGMT_PSK_SHA256;

	capa->enc = WPA_DRIVER_CAPA_ENC_WEP40 | WPA_DRIVER_CAPA_ENC_WEP104 |
		    WPA_DRIVER_CAPA_ENC_TKIP | WPA_DRIVER_CAPA_ENC_CCMP |
		    WPA_DRIVER_CAPA_ENC_CCMP_256 | WPA_DRIVER_CAPA_ENC_GCMP |
		    WPA_DRIVER_CAPA_ENC_GCMP_256 | WPA_DRIVER_CAPA_ENC_BIP |
		    WPA_DRIVER_CAPA_ENC_BIP_GMAC_128 | WPA_DRIVER_CAPA_ENC_BIP_GMAC_256 |
		    WPA_DRIVER_CAPA_ENC_BIP_CMAC_256;

	capa->auth = WPA_DRIVER_AUTH_OPEN | WPA_DRIVER_AUTH_SHARED;

	capa->flags = WPA_DRIVER_FLAGS_KEY_MGMT_OFFLOAD | WPA_DRIVER_FLAGS_SAE |
		      WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE;

#if defined(CONFIG_IFX_WIFI_SOFTAP_SUPPORT)
	capa->flags |= WPA_DRIVER_FLAGS_AP;
	capa->flags |= WPA_DRIVER_FLAGS_AP_MLME;
	capa->flags |= WPA_DRIVER_FLAGS_AP_TEARDOWN_SUPPORT;
	capa->flags |= WPA_DRIVER_FLAGS_AP_CSA;
	capa->flags2 |= WPA_DRIVER_FLAGS2_AP_SME;
#endif

	return WHD_SUCCESS;
}

int airoc_wifi_wpa_supp_get_conn_info(void *if_priv, struct wpa_conn_info *info)
{
	int ret = WHD_SUCCESS;
	wl_bss_info_t bss_info;
	whd_security_t security_info = 0;

	if (!if_priv || !info) {
		LOG_ERR("%s Invalid params", __func__);
		return -EINVAL;
	}

	ret = whd_wifi_get_ap_info(airoc_if, &bss_info, &security_info);
	if (ret == WHD_SUCCESS) {
		info->beacon_interval = bss_info.beacon_period;
		info->dtim_period = bss_info.dtim_period;
		info->twt_capable = false;
	}

	return ret;
}

int airoc_wifi_wpa_supp_signal_poll(void *if_priv, struct wpa_signal_info *si, unsigned char *bssid)
{
	int ret = WHD_SUCCESS;
	wl_bss_info_t bss_info;
	whd_security_t security_info = 0;
	uint32_t tx_rate;

	if (!if_priv || !si || !bssid) {
		LOG_ERR("%s: Invalid params", __func__);
		return -EINVAL;
	}

	ret = whd_wifi_get_ap_info(airoc_if, &bss_info, &security_info);
	if (ret == WHD_SUCCESS) {
		si->data.signal = bss_info.RSSI;
		si->current_noise = bss_info.phy_noise;
		si->frequency = whd_channel2freq(bss_info.ctl_ch);
	}

	ret = whd_wifi_get_ioctl_value(airoc_if, WLC_GET_RATE, &tx_rate);
	if (ret == WHD_SUCCESS) {
		/* The data rate received is in units of 500Kbits per sec, convert
		 * to Kbits per sec.
		 */
		si->data.current_tx_rate = tx_rate * AIROC_WIFI_TX_BIT_RATE_CONVERTER;
	}

	return ret;
}

int airoc_wifi_wpa_supp_scan2(void *priv, struct wpa_driver_scan_params *params)
{
	int ret = WHD_SUCCESS;
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)priv;
	whd_ssid_t ssid = {0};
	whd_ssid_t *custom_ssid = &data->ssid;
	whd_scan_result_t usr_result = {0};
	whd_scan_result_t scan_result;

	if (data == NULL) {
		LOG_ERR("WHD-SCAN: data NULL");
		return -EINVAL;
	}

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		LOG_ERR("Scan, returning sem take failed\n");
		return -EAGAIN;
	}

	ssid.length = params->ssids[0].ssid_len;
	memcpy(ssid.value, params->ssids[0].ssid, params->ssids[0].ssid_len);

	if (params->ssids[0].ssid_len) {
		data->ssid.length = ssid.length;
		memcpy(data->ssid.value, ssid.value, ssid.length);
	}

	usr_result.SSID.length = ssid.length;
	memcpy(usr_result.SSID.value, ssid.value, ssid.length);

	if (whd_wifi_scan(airoc_sta_if, WHD_SCAN_TYPE_ACTIVE, WHD_BSS_TYPE_ANY, custom_ssid, NULL,
			  NULL, NULL, airoc_wifi_supp_scan2_cb, &scan_result,
			  &usr_result) != WHD_SUCCESS) {
		LOG_ERR("Failed start scan");
		k_sem_give(&data->sema_common);
		ret = -EAGAIN;
	}

	data->airoc_wifi_wpa_supp_dev_callbk_fns.scan_start(data->supp_if_ctx);

	if (k_sem_take(&airoc_wifi_data.sema_scan, K_MSEC(AIROC_WIFI_SCAN_TIMEOUT_MS)) != 0) {
		whd_wifi_stop_scan(airoc_sta_if);
		ret = -EAGAIN;
	}

	if ((!ret) && (data->scan_result.security == WHD_SECURITY_UNKNOWN)) {
		ret = -EAGAIN;
		LOG_ERR("Could not scan device");
	}
	k_sem_give(&data->sema_common);

	return ret;
}

int airoc_wifi_wpa_supp_scan_abort(void *if_priv)
{
	int ret = WHD_SUCCESS;

	ret = whd_wifi_stop_scan(airoc_sta_if);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Failed to abort scan, ret: %d", ret);
	}
	return WHD_SUCCESS;
}

int airoc_wifi_wpa_supp_get_scan_results2(void *if_priv)
{

	struct airoc_wifi_data *data = (struct airoc_wifi_data *)if_priv;
	struct wpa_scan_res *r = (struct wpa_scan_res *)whd_mem_malloc(sizeof(struct wpa_scan_res) +
								       data->scan_result.ie_len);

	if (r == NULL) {
		LOG_ERR("%s: allocation failed, size=%u", __func__, sizeof(struct wpa_scan_res));
		return WHD_MALLOC_FAILURE;
	}

	memset(r, 0, sizeof(struct wpa_scan_res) + data->scan_result.ie_len);

	r->ie_len = data->scan_result.ie_len;
	memcpy(r->bssid, data->scan_bss_info.BSSID.octet, ETH_ALEN);
	r->flags = data->scan_bss_info.flags;
	r->caps = data->scan_bss_info.capability;
	r->snr = data->scan_bss_info.SNR;
	r->noise = data->scan_bss_info.phy_noise;
	r->freq = whd_channel2freq((uint)(data->scan_bss_info.ctl_ch));
	memcpy((uint8_t *)(r + 1), data->scan_result.ie_ptr, r->ie_len);
	data->airoc_wifi_wpa_supp_dev_callbk_fns.scan_res(data->supp_if_ctx, r, false);

	return WHD_SUCCESS;
}

int airoc_wifi_wpa_supp_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
				const unsigned char *addr, int key_idx, int set_tx,
				const unsigned char *seq, size_t seq_len, const unsigned char *key,
				size_t key_len, enum key_flag key_flag)
{
	int ret = WHD_SUCCESS;
	int32_t val = 0;
	int32_t wsec = 0;
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)if_priv;
	struct whd_wpa_assoc_security *sec = &(data->assoc_sec);
	wl_wsec_key_t wsec_key;

	uint32_t algos = 0, mask = 0;

	whd_driver_t whd_driver = airoc_sta_if->whd_driver;

	memset(&wsec_key, 0, sizeof(wsec_key));

	LOG_DBG("%s: (%s) alg=%d addr=%p key_idx=%d "
		"set_tx=%d seq_len=%lu key_len=%lu key_flag=0x%x\n",
		__func__, ifname, alg, addr, key_idx, set_tx, (unsigned long)seq_len,
		(unsigned long)key_len, key_flag);

	if (check_key_flag(key_flag)) {
		LOG_ERR("%s: invalid key_flag", __func__);
		ret = -EINVAL;
		return ret;
	}

	if ((alg == WPA_ALG_CCMP || alg == WPA_ALG_CCMP_256) &&
	    key_flag == KEY_FLAG_PAIRWISE_RX_TX) {
		wsec_key.index = htod32(key_idx);
		if (!ETHER_ISMULTI(addr)) {
			memcpy((char *)&wsec_key.ea, (const void *)addr, ETHER_ADDR_LEN);
		}
		wsec_key.len = htod32(key_len);
		if (wsec_key.len > sizeof(wsec_key.data)) {
			LOG_ERR("Invalid key length %d", key_len);
			ret = -EAGAIN;
			return ret;
		}
		memcpy(wsec_key.data, key, wsec_key.len);
		if (seq && seq_len == 6) {
			const uint8_t *ivptr;

			ivptr = (const uint8_t *)seq;
			wsec_key.rxiv.hi =
				(ivptr[5] << 24) | (ivptr[4] << 16) | (ivptr[3] << 8) | ivptr[2];
			wsec_key.rxiv.lo = (ivptr[1] << 8) | ivptr[0];
			wsec_key.iv_initialized = 1;
		}
		wsec_key.algo = htod32(CRYPTO_ALGO_AES_CCM);
		swap_key_from_host(&wsec_key);
		k_sleep(K_MSEC(10));
		ret = whd_wifi_set_iovar_buffer(airoc_sta_if, "wsec_key", &wsec_key,
						sizeof(wsec_key));
		if (ret != WHD_SUCCESS) {
			LOG_ERR("wsec_key fail ret = %d", ret);
			return ret;
		}
		goto exit;
	}
	wsec_key.index = htod32(key_idx);
	wsec_key.len = htod32(key_len);
	whd_mem_memcpy(wsec_key.data, key, key_len);
	wsec_key.flags = htod32(WL_PRIMARY_KEY);

	switch (alg) {
	case WPA_ALG_NONE:
		wsec_key.algo = htod32(CRYPTO_ALGO_OFF);
		break;
	case WPA_ALG_CCMP:
	case WPA_ALG_BIP_CMAC_128:
		wsec_key.algo = htod32(CRYPTO_ALGO_AES_CCM);
		val = 4;
		break;
	default:
		LOG_ERR("%s: Unknown algorithm %d", __func__, alg);
		ret = -EAGAIN;
		return ret;
	}

	if ((key_flag != KEY_FLAG_GROUP_RX) && (key_flag != KEY_FLAG_PAIRWISE_RX_TX)) {
		wsec_key.algo = htod32(CRYPTO_ALGO_OFF);
	}
	swap_key_from_host(&wsec_key);
	k_sleep(K_MSEC(10));
	ret = whd_wifi_set_iovar_buffer(airoc_sta_if, "wsec_key", &wsec_key, sizeof(wsec_key));
	if (ret != WHD_SUCCESS) {
		LOG_ERR("wsec_key fail ret = %d\n", ret);
		return ret;
	}

exit:
	if ((key_flag != KEY_FLAG_GROUP_RX) && (key_flag != KEY_FLAG_PAIRWISE_RX_TX)) {
		return ret;
	}

	ret = whd_wifi_get_iovar_value(airoc_sta_if, "wsec", &wsec);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("get wsec error ret = %d", ret);
		return ret;
	}

	wsec |= val;
	ret = whd_wifi_set_iovar_value(airoc_sta_if, "wsec", wsec);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("set wsec error ret = %d", ret);
		ret = -EAGAIN;
	}

	if (whd_driver->chip_info.fwcap_flags & (1 << WHD_FWCAP_GCMP)) {
		if (sec->pairwise_suite == WPA_CIPHER_GCMP) {
			algos = KEY_ALGO_MASK(CRYPTO_ALGO_AES_GCM256);
			mask = algos | KEY_ALGO_MASK(CRYPTO_ALGO_AES_CCM);
		}
		ret = whd_set_wsec_info_algos(airoc_sta_if, algos, mask);
	}
	if (ret == WHD_SUCCESS) {
		if (key_flag == KEY_FLAG_GROUP_RX) {
			ret = whd_wifi_set_ioctl_buffer(airoc_sta_if, WLC_SCB_AUTHORIZE,
							(uint8_t *)data->bssid, ETH_ALEN);
			if (ret != WHD_SUCCESS) {
				LOG_ERR("Failed to set SCB AUTHORIZE\n");
			}
			LOG_DBG("WLC_SCB_AUTHORIZE done");
		}
	} else {
		LOG_ERR("Failed to set wsec_info");
	}
	return ret;
}

static int wpa_auth_val(struct airoc_wifi_data *data, unsigned int *wpa_versions,
			unsigned int key_mgmt_suite, enum wps_mode wps)
{
	int val;
	/* Add WPA3 if needed */
	if ((*wpa_versions & WPA_PROTO_RSN) && (key_mgmt_suite & WPA_KEY_MGMT_SAE)) {
		*wpa_versions &= ~WPA_PROTO_RSN; /* Mark WPA3 use */
		*wpa_versions |= WPA_PROTO_WPA3; /* Mark WPA3 use */
	}
	val = 0;
	if (*wpa_versions & WPA_PROTO_WPA) { /* WPA */
		if (wpa_key_mgmt_wpa_psk(key_mgmt_suite)) {
			val |= WPA_AUTH_PSK;
		}
		if (wpa_key_mgmt_wpa_ieee8021x(key_mgmt_suite)) {
			val |= WPA_AUTH_UNSPECIFIED;
		}
	} else if (*wpa_versions & WPA_PROTO_RSN) { /* WPA2 */
		if (wpa_key_mgmt_wpa_psk(key_mgmt_suite)) {
			val |= WPA2_AUTH_PSK;
		}
		if (wpa_key_mgmt_wpa_ieee8021x(key_mgmt_suite)) {
			val |= WPA2_AUTH_UNSPECIFIED;
		}
	} else if (*wpa_versions & WPA_PROTO_WPA3) { /* WPA3 */
		val = WPA3_AUTH_SAE_PSK;
	} else {
		val = WPA_AUTH_DISABLED;
	}

	return val;
}

static int wpa_auth(struct airoc_wifi_data *data, unsigned int wpa_versions,
		    unsigned int key_mgmt_suite, enum wps_mode wps)
{
	int val;
	int ret = WHD_SUCCESS;
	struct whd_wpa_assoc_security *sec = &(data->assoc_sec);

	val = wpa_auth_val(data, &wpa_versions, key_mgmt_suite, wps);
	LOG_DBG("wpa auth val 0x%x", val);
	ret = whd_wifi_set_iovar_value(airoc_sta_if, "wpa_auth", val);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Failed to set wpa auth: %d ret: %d", val, ret);
		return ret;
	}

	sec->wpa_version = wpa_versions;

	return ret;
}

static int auth_type(struct airoc_wifi_data *data, int auth_alg)
{
	int type = -1;
	struct whd_wpa_assoc_security *sec = &(data->assoc_sec);
	int ret = WHD_SUCCESS;

	if (auth_alg & WPA_AUTH_ALG_OPEN) {
		type = WL_AUTH_OPEN_SYSTEM;
	}
	if (auth_alg & WPA_AUTH_ALG_SHARED) {
		type = WL_AUTH_SHARED_KEY;
	}
	if (auth_alg & WPA_AUTH_ALG_SAE) {
		type = WL_AUTH_SAE;
	}

	if (type == -1) {
		LOG_ERR("Unsupported ath alg = %d", auth_alg);
		return -1;
	}
	LOG_DBG("\nwpa auth type: auth 0x%x\n", type);
	ret = whd_wifi_set_iovar_value(airoc_sta_if, "auth", type);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Failed to set auth: %d ret: %d", type, ret);
		return ret;
	}

	sec->auth_alg = auth_alg;

	return ret;
}

static int set_cipher(struct airoc_wifi_data *data, unsigned int pairwise_suite,
		      unsigned int group_suite, enum wps_mode wps, bool isAp,
		      enum mfp_options mgmt_frame_protection, unsigned int key_mgmt_suite)
{
	int ret = WHD_SUCCESS;
	uint32_t pval = 0;
	uint32_t gval = 0;
	uint32_t wsec_val = 0;
	uint32_t mfp = 0;

	struct whd_wpa_assoc_security *sec = &(data->assoc_sec);

	switch (pairwise_suite) {
	case WPA_CIPHER_WEP104:
	case WPA_CIPHER_WEP40:
		pval = WEP_ENABLED;
		break;
	case WPA_CIPHER_TKIP:
		pval = TKIP_ENABLED;
		break;
	case WPA_CIPHER_CCMP:
	case WPA_CIPHER_AES_128_CMAC:
		pval = AES_ENABLED;
		break;
	case WPA_CIPHER_NONE:
		pval = 0;
		break;
	default:
		LOG_ERR("invalid cipher pairwise (%d)", pairwise_suite);
		return -EINVAL;
	}

	if (group_suite) {
		switch (group_suite) {
		case WPA_CIPHER_WEP104:
		case WPA_CIPHER_WEP40:
			gval = WEP_ENABLED;
			break;
		case WPA_CIPHER_TKIP:
			gval = TKIP_ENABLED;
			break;
		case WPA_CIPHER_CCMP:
		case WPA_CIPHER_AES_128_CMAC:
			gval = AES_ENABLED;
			break;
		case WPA_CIPHER_NONE:
			gval = 0;
			break;
		default:
			LOG_ERR("invalid cipher group (%d)", group_suite);
			return -EINVAL;
		}
	}

	wsec_val |= pval | gval;

	if (pval == AES_ENABLED && mgmt_frame_protection != NO_MGMT_FRAME_PROTECTION && !isAp) {
		ret = whd_wifi_get_iovar_value(airoc_sta_if, "mfp", &mfp);
		if (ret != WHD_SUCCESS) {
			LOG_ERR("Get MFP failed : err = %d", ret);
			return ret;
		}

		if (key_mgmt_suite == WPA_KEY_MGMT_PSK_SHA256 ||
		    key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SHA256) {
			wsec_val |= MFP_SHA256;
		}

		wsec_val |= MFP_CAPABLE;

		if (mgmt_frame_protection == MGMT_FRAME_PROTECTION_REQUIRED) {
			wsec_val |= MFP_REQUIRED;
		}
	}
	LOG_DBG("setting cipher wsec 0x%x", wsec_val);
	ret = whd_wifi_set_iovar_value(airoc_sta_if, "wsec", wsec_val);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Failed to set wsec: %d ret: %d", wsec_val, ret);
		return ret;
	}

	sec->pairwise_suite = pairwise_suite;
	sec->group_suite = group_suite;

	return ret;
}

static int set_key_mgmt(struct airoc_wifi_data *data, unsigned int key_mgmt_suite,
			int req_handshake_offload)
{
	uint32_t val = 0;
	int ret = WHD_SUCCESS;
	struct whd_wpa_assoc_security *sec = &(data->assoc_sec);

	if (key_mgmt_suite) {
		ret = whd_wifi_get_iovar_value(airoc_sta_if, "wpa_auth", &val);
		if (ret != WHD_SUCCESS) {
			LOG_ERR("could not get wpa_auth (%d)", ret);
			return ret;
		}
		if (val & (WPA_AUTH_PSK | WPA_AUTH_UNSPECIFIED)) {
			switch (key_mgmt_suite) {
			case WPA_KEY_MGMT_IEEE8021X:
				val = WPA_AUTH_UNSPECIFIED;
				break;
			case WPA_KEY_MGMT_PSK:
				val = WPA_AUTH_PSK;
				break;
			default:
				LOG_ERR("WPA invalid key_mgmt_suite (0x%x)", key_mgmt_suite);
				ret = -EINVAL;
				return ret;
			}
		} else if (val & (WPA2_AUTH_PSK | WPA2_AUTH_UNSPECIFIED)) {
			switch (key_mgmt_suite) {
			case WPA_KEY_MGMT_IEEE8021X:
				val = WPA2_AUTH_UNSPECIFIED;
				break;
			case WPA_KEY_MGMT_IEEE8021X_SHA256:
				val = WPA2_AUTH_1X_SHA256;
				break;
			case WPA_KEY_MGMT_PSK_SHA256:
				val = WPA2_AUTH_PSK_SHA256;
				break;
			case WPA_KEY_MGMT_PSK:
				val = WPA2_AUTH_PSK;
				break;
			default:
				LOG_ERR("WPA2 invalid key_mgmt_suite (0x%x)", key_mgmt_suite);
				ret = -EINVAL;
				return ret;
			}
		} else if (val & WPA3_AUTH_SAE_PSK) {
			switch (key_mgmt_suite) {
			case WPA_KEY_MGMT_IEEE8021X:
				val = WPA3_AUTH_1X_SUITE_B_SHA384;
				break;
			case WPA_KEY_MGMT_IEEE8021X_SHA256:
				val = WPA3_AUTH_1X_SHA256;
				break;
			case WPA_KEY_MGMT_SAE:
				val = WPA3_AUTH_SAE_PSK;
				break;
			default:
				LOG_ERR("WPA3 invalid key_mgmt_suite (0x%x)", key_mgmt_suite);
				ret = -EINVAL;
				return ret;
			}
		} else {
			LOG_ERR("Unhandled wpa_auth, can't proceed");
			ret = -EINVAL;
			return ret;
		}
		LOG_DBG("setting key_mgmt wpa_auth 0x%x", val);
		ret = whd_wifi_set_iovar_value(airoc_sta_if, "wpa_auth", val);
		if (ret != WHD_SUCCESS) {
			LOG_ERR("Could not set wpa_auth (0x%x)", ret);
			return ret;
		}
	}

	sec->key_mgmt_suite = key_mgmt_suite;

	return ret;
}

static int set_mfp(struct airoc_wifi_data *data, enum mfp_options mgmt_frame_protection)
{
	uint32_t mfp = WL_MFP_NONE;
	uint32_t current_mfp = WL_MFP_NONE;
	bool fw_support = false;
	int ret = WHD_SUCCESS;

	ret = whd_wifi_get_iovar_value(airoc_sta_if, "mfp", &current_mfp);
	if (ret == WHD_SUCCESS) {
		fw_support = true;
	}

	if (mgmt_frame_protection == MGMT_FRAME_PROTECTION_REQUIRED) {
		mfp = WL_MFP_REQUIRED;
	} else if (mgmt_frame_protection == MGMT_FRAME_PROTECTION_OPTIONAL) {
		mfp = WL_MFP_CAPABLE;
	} else {
		if (fw_support) {
			if (current_mfp != mfp) {
				LOG_DBG("setting mfp 0x%x", mfp);
				ret = whd_wifi_set_iovar_value(airoc_sta_if, "mfp", mfp);
				if (ret != WHD_SUCCESS) {
					LOG_ERR("Failed to set mfp: %d ret: %d", mfp, ret);
					return ret;
				}
			}
		}
	}

	if (mfp && !fw_support) {
		LOG_ERR("MFP capability found in mgmt_frame_protection. But fw doesn't seem to "
			"support MFP");
		return -EINVAL;
	}

	return ret;
}

static int whd_connect(struct airoc_wifi_data *data, struct wpa_driver_associate_params *params)
{
	int ret = WHD_SUCCESS;
	struct whd_wpa_assoc_security *sec = &(data->assoc_sec);
	wlc_ssid_t *ssid_params = NULL;
	uint32_t algos = 0, mask = 0;
	whd_driver_t whd_driver = airoc_sta_if->whd_driver;

	if (params->ssid == NULL) {
		LOG_ERR("%s: No SSID", __func__);
		return -EAGAIN;
	}

	if (data->is_sta_connected) {
		if (!memcmp(data->ssid.value, params->ssid, params->ssid_len)) {
			LOG_ERR("Already connected to the same AP");
		} else {
			LOG_ERR("Already connected to some AP");
			ret = whd_wifi_set_ioctl_buffer(airoc_sta_if, WLC_DISASSOC, NULL, 0);
			if (ret != WHD_SUCCESS) {
				LOG_ERR("\nDisassoc fail ret = %d", ret);
				return ret;
			}
			data->is_sta_connected = WHD_FALSE;
		}
	}

	if (data->is_sta_connected) {
		return ret;
	}

	memcpy(data->ssid.value, params->ssid, params->ssid_len);
	data->ssid.length = params->ssid_len;

	ret = wpa_auth(data, params->wpa_proto, params->key_mgmt_suite, params->wps);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("set wpa_auth error ret = %d", ret);
		return ret;
	}

	ret = auth_type(data, params->auth_alg);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("set auth error ret = %d", ret);
		return ret;
	}

	ret = set_cipher(data, params->pairwise_suite, params->group_suite, params->wps, false,
			 params->mgmt_frame_protection, params->key_mgmt_suite);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("set cipher error ret = %d", ret);
		return ret;
	}

	if (whd_driver->chip_info.fwcap_flags & (1 << WHD_FWCAP_GCMP)) {
		if (sec->pairwise_suite == WPA_CIPHER_GCMP) {
			algos = KEY_ALGO_MASK(CRYPTO_ALGO_AES_GCM256);
			mask = algos | KEY_ALGO_MASK(CRYPTO_ALGO_AES_CCM);
		}
		ret = whd_set_wsec_info_algos(airoc_sta_if, algos, mask);
		if (ret != WHD_SUCCESS) {
			LOG_ERR("Failed to wsec_info");
		}
	}

	ret = set_mfp(data, params->mgmt_frame_protection);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("MFP set failed err:%d", ret);
		return ret;
	}

	ret = set_key_mgmt(data, params->key_mgmt_suite, params->req_handshake_offload);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Invalid key mgmt");
		return ret;
	}

	ssid_params = (wlc_ssid_t *)whd_mem_malloc(sizeof(wlc_ssid_t));
	if (ssid_params == NULL) {
		LOG_ERR("%s: allocation failed, size=%u", __func__, sizeof(wlc_ssid_t));
		return WHD_MALLOC_FAILURE;
	}

	whd_mem_memset(ssid_params, 0, sizeof(wlc_ssid_t));
	ssid_params->SSID_len = data->ssid.length;
	whd_mem_memcpy(ssid_params->SSID, data->ssid.value, ssid_params->SSID_len);
	data->key_mgmt_suite = params->key_mgmt_suite;

	ret = whd_wifi_set_ioctl_buffer(airoc_sta_if, WLC_SET_SSID, ssid_params,
					sizeof(wlc_ssid_t));
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Failed to trigger join ret: %d", ret);
	}

	whd_mem_free(ssid_params);
	return ret;
}

int airoc_wifi_wpa_supp_associate(void *priv, struct wpa_driver_associate_params *params)
{
	int ret = WHD_SUCCESS;
	uint32_t infra;
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)priv;
	struct zep_drv_if_ctx *if_ctx = data->supp_if_ctx;
	uint32_t pm = (uint32_t)PM_OFF;

	ret = whd_wifi_set_ioctl_value(airoc_sta_if, WLC_SET_PM, pm);
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Failed to set PM to %d\n", pm);
	}

	if (params->mode == IEEE80211_MODE_IBSS || params->mode == IEEE80211_MODE_INFRA) {
		if (params->bssid) {
			whd_mem_memcpy(data->bssid, params->bssid, ETH_ALEN);
		}
		if (params->mode == IEEE80211_MODE_INFRA) {
			infra = 1;
			/* P2P to be handled here */
			/* WPS to be handled here */
		} else {
			infra = 0;
		}
		data->mode = params->mode;
		/* set infrastructure mode */
		infra = htod32(infra);
		ret = whd_wifi_set_ioctl_buffer(airoc_sta_if, WLC_SET_INFRA, &infra, sizeof(int));
		if (ret != WHD_SUCCESS) {
			LOG_ERR("Failed to set infra: %d ret: %d", infra, ret);
			goto exit;
		}
		memcpy(if_ctx->ssid, params->ssid, params->ssid_len);
		if_ctx->ssid_len = params->ssid_len;
		ret = whd_connect(data, params);
	}

exit:
	if (ret != WHD_SUCCESS) {
		whd_mem_memset(data->bssid, 0, ETH_ALEN);
		LOG_ERR("whd_connect() failed, ret: %d", ret);
	}
	return ret;
}

int airoc_wifi_wpa_supp_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
				     struct wpa_bss *curr_bss)
{
	/* Nothing to do here, everything will be handled in the associate op */
	return WHD_SUCCESS;
}

int airoc_wifi_wpa_supp_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	return WHD_SUCCESS;
}

int airoc_wifi_wpa_supp_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code)
{
	int ret = WHD_SUCCESS;
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)if_priv;
	scb_val_t scb_val;

	os_memcpy(scb_val.ea.octet, addr, ETH_ALEN);
	scb_val.val = htod32(reason_code);

	ret = whd_wifi_set_ioctl_buffer(airoc_sta_if, WLC_DISASSOC, (void *)&scb_val,
					sizeof(scb_val_t));
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Dissasoc failed, ret: %d\n", ret);
	}

	ret = whd_wifi_set_ioctl_buffer(airoc_sta_if, WLC_SCB_DEAUTHENTICATE_FOR_REASON,
					(void *)&scb_val, sizeof(scb_val_t));
	if (ret != WHD_SUCCESS) {
		LOG_ERR("Deauth failed, ret: %d\n", ret);
	}
	data->is_sta_connected = false;

	return ret;
}

int airoc_wifi_wpa_supp_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
				  unsigned int freq, int no_cck, int offchanok,
				  unsigned int wait_time, int cookie)
{
	const struct device *dev = net_if_get_device(airoc_wifi_iface);
	struct airoc_wifi_data *airoc_data = dev->data;
	int ret = WHD_SUCCESS;
	const struct ieee80211_mgmt *mgmt;
	uint32_t auth_params_len = 0;
	whd_auth_params_t *auth_params;

	if (data == NULL || data_len == 0) {
		LOG_ERR("send_mlme: data/data_len is NULL");
		return WHD_BADARG;
	}

	mgmt = (const struct ieee80211_mgmt *)data;
	auth_params_len = offsetof(whd_auth_params_t, data) + data_len;
	auth_params = (whd_auth_params_t *)whd_mem_malloc(auth_params_len);
	if (auth_params == NULL) {
		LOG_ERR("%s: allocation failed, size=%u", __func__, auth_params_len);
		return WHD_MALLOC_FAILURE;
	}

	auth_params->len = data_len + DOT11_MGMT_HDR_LEN;
	auth_params->fc = mgmt->frame_control;
	auth_params->dwell_time = MGMT_AUTH_FRAME_DWELL_TIME;
	auth_params->channel = airoc_data->scan_bss_info.ctl_ch;

	memcpy(auth_params->da.octet, &mgmt->da[0], ETHER_ADDR_LEN);
	memcpy(auth_params->bssid.octet, &mgmt->bssid[0], ETHER_ADDR_LEN);

	cookie = (int)auth_params->data;
	auth_params->packetId = htod32(cookie);
	memcpy(auth_params->data, data, data_len);

	memcpy(airoc_data->mgmt_tx_data, data, data_len);
	airoc_data->mgmt_tx_data_len = data_len;

	ret = whd_wifi_send_auth_frame(airoc_sta_if, auth_params);

	if (auth_params) {
		whd_mem_free(auth_params);
	}

	return ret;
}

int airoc_wifi_wpa_supp_send_ext_auth_sts(void *if_priv, struct external_auth *params)
{
	whd_buffer_t buffer;
	whd_auth_req_status_t *auth_status;

	if (params == NULL) {
		LOG_ERR("Invalid param in func %s at line %d", __func__, __LINE__);
		return WHD_WLAN_BADARG;
	}

	auth_status = (whd_auth_req_status_t *)whd_cdc_get_iovar_buffer(
		airoc_sta_if->whd_driver, &buffer, sizeof(whd_auth_req_status_t),
		IOVAR_STR_AUTH_STATUS);

	CHECK_IOCTL_BUFFER(auth_status);

	auth_status->ssid_len = params->ssid_len;

	whd_mem_memcpy(auth_status->ssid, params->ssid, auth_status->ssid_len);
	whd_mem_memcpy(auth_status->peer_mac.octet, params->bssid, 6);

	if (params->status == DOT11_SC_SUCCESS) {
		auth_status->flags = WL_EXTAUTH_SUCCESS;
	} else {
		auth_status->flags = WL_EXTAUTH_FAIL;
	}

	RETURN_WITH_ASSERT(whd_cdc_send_iovar(airoc_sta_if, CDC_SET, buffer, NULL));

	return 0;
}
#endif

#if defined(CONFIG_IFX_WIFI_SOFTAP_SUPPORT)
int airoc_wifi_wpa_supp_init_ap(void *if_priv, struct wpa_driver_associate_params *params)
{
	struct airoc_wifi_data *data = NULL;
	struct zep_drv_if_ctx *if_ctx = NULL;
	int ret = -1;

	if (!if_priv || !params) {
		LOG_ERR("%s: Invalid params", __func__);
		return ret;
	}
	if (params->mode != IEEE80211_MODE_AP) {
		LOG_ERR("%s: Invalid mode", __func__);
		return ret;
	}

	data = (struct airoc_wifi_data *)if_priv;
	if_ctx = data->supp_if_ctx;

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	if (data->is_sta_connected) {
		ret = -EBUSY;
		goto out;
	}

	if (data->is_ap_up) {
		ret = -EAGAIN;
		goto out;
	}

	if (!data->second_interface_init) {
		if (whd_add_secondary_interface(data->whd_drv, NULL, &airoc_ap_if) !=
		    CY_RSLT_SUCCESS) {
			ret = -EAGAIN;
			goto out;
		}
		data->second_interface_init = true;
	}
	ret = 0;
out:
	k_sem_give(&data->sema_common);

	return ret;
}

int airoc_wifi_wpa_supp_start_ap(void *if_priv, struct wpa_driver_ap_params *params)
{
	struct airoc_wifi_data *data = NULL;
	struct zep_drv_if_ctx *if_ctx = NULL;
	whd_security_t security = WHD_SECURITY_OPEN;
	whd_ssid_t ssid;
	uint8_t channel = 1;
	int ret = -1;

	if (!if_priv || !params) {
		LOG_ERR("%s: Invalid params", __func__);
		return ret;
	}
	data = (struct airoc_wifi_data *)if_priv;
	if_ctx = data->supp_if_ctx;

	if (!if_ctx) {
		LOG_ERR("%s: if_ctx is NULL", __func__);
		return ret;
	}

	if (!data->second_interface_init) {
		LOG_ERR("second_interface_init is down");
		return ret;
	}

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	ssid.length = params->ssid_len;
	memcpy(ssid.value, params->ssid, ssid.length);

	/* make sure to set valid channels for 2G and 5G:
	 * - 2G channels from 1 to 11,
	 * - 5G channels from 36 to 165
	 */
	if (params->freq) {
		if (((params->freq->channel > 0) && (params->freq->channel < 12)) ||
		    ((params->freq->channel > 35) && (params->freq->channel < 166))) {
			channel = params->freq->channel;
		} else {
			channel = 1;
			LOG_ERR("Discard of setting unsupported channel: %u (will set 1)",
				params->freq->channel);
		}
	}

	if (params->privacy == 0) {
		security = WHD_SECURITY_OPEN;
	} else if (params->wpa_version & WPA_PROTO_WPA3) {
		security = WHD_SECURITY_WPA3_SAE;
	} else if (params->wpa_version & WPA_PROTO_RSN) {
		security = WHD_SECURITY_WPA2_AES_PSK;
	} else if (params->wpa_version & WPA_PROTO_WPA) {
		security = WHD_SECURITY_WPA_AES_PSK;
	} else {
		LOG_ERR("Unknown security param, can't start_ap\n");
		return -1;
	}

	/* Use PSK for WPA2 */
	const uint8_t *key_data = NULL;
	size_t key_len = 0;

	if (security != WHD_SECURITY_OPEN) {
		if (params->psk_len >= 8 && params->psk_len <= 64) {
			key_data = params->psk;
			key_len = params->psk_len;

			if (params->psk_len == 32) {
				LOG_DBG("Using binary PSK, len=%d", key_len);
			} else {
				LOG_DBG("Using ASCII passphrase, len=%d", key_len);
			}
		} else {
			LOG_ERR("No valid PSK provided: psk=%p (len=%d)", params->psk,
				params->psk_len);
			ret = -EINVAL;
			goto out;
		}
	}

	if (whd_wifi_init_ap(airoc_ap_if, &ssid, security, key_data, key_len, channel) != 0) {
		LOG_ERR("Failed to init whd ap interface");
		ret = -EAGAIN;
		goto out;
	}

	if (whd_wifi_start_ap(airoc_ap_if) != 0) {
		LOG_ERR("Failed to start whd ap interface");
		ret = -EAGAIN;
		goto out;
	}

	/* set event handler */
	if (whd_management_set_event_handler(airoc_ap_if, ap_link_events,
					     airoc_wifi_ap_link_events_handler, NULL,
					     &ap_event_handler_index) != 0) {
		whd_wifi_stop_ap(airoc_ap_if);
		ret = -EAGAIN;
		goto out;
	}

	data->is_ap_up = true;
	airoc_if = airoc_ap_if;
	net_if_dormant_off(data->iface);

	ret = 0;
out:
	k_sem_give(&data->sema_common);
	return ret;
}

int airoc_wifi_wpa_supp_reg_mgmt_frame(void *if_priv, u16 frame_type, size_t match_len,
				       const u8 *match)
{
	if (!if_priv || (match_len && !match)) {
		LOG_ERR("%s: Invalid params", __func__);
		return 1;
	}
	return 0;
}
#endif

static inline unsigned short mhz_from_5g_channel(uint8_t ch)
{
	/* 5GHz channels: 36, 40, 44, 48, 149, 153, 157, 161, 165 */
	return (unsigned short)(5000 + 5 * ch);
}

static inline unsigned short mhz_from_2g_channel(uint8_t ch)
{
	return (ch == 14) ? 2484 : (unsigned short)(2412 + 5 * (ch - 1));
}

static void airoc_build_band_5g_min(struct wpa_supp_event_supported_band *band)
{
	static const uint8_t chs[] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
	static const unsigned short br[] = {60, 90, 120, 180, 240, 360, 480, 540};

	memset(band, 0, sizeof(*band));
	band->band = 1; /* 5GHz */
	band->wpa_supp_n_channels = (unsigned short)(ARRAY_SIZE(chs));
	band->wpa_supp_n_bitrates = (unsigned short)(ARRAY_SIZE(br));

	for (int i = 0; i < band->wpa_supp_n_channels; i++) {
		band->channels[i].ch_valid = 1;
		band->channels[i].center_frequency = mhz_from_5g_channel(chs[i]);
		band->channels[i].wpa_supp_flags = 0;
		band->channels[i].wpa_supp_max_power = 20;
		band->channels[i].dfs_state = 0;
	}

	for (int i = 0; i < band->wpa_supp_n_bitrates; i++) {
		band->bitrates[i].wpa_supp_bitrate = br[i];
		band->bitrates[i].wpa_supp_flags = 0;
	}
}

static void airoc_build_band_2g_min(struct wpa_supp_event_supported_band *band)
{
	static const unsigned short br[] = {10, 20, 55, 110, 60, 90, 120, 180, 240, 360, 480, 540};

	memset(band, 0, sizeof(*band));
	band->band = 0;                 /* 2.4GHz */
	band->wpa_supp_n_channels = 11; /* ch1..11 */
	band->wpa_supp_n_bitrates = (unsigned short)(ARRAY_SIZE(br));

	for (int i = 0; i < band->wpa_supp_n_channels; i++) {
		int ch = i + 1; /* 1..11 */

		band->channels[i].ch_valid = 1;
		band->channels[i].center_frequency = mhz_from_2g_channel((uint8_t)ch);
		band->channels[i].wpa_supp_flags = 0;
		band->channels[i].wpa_supp_max_power = 20;
		band->channels[i].dfs_state = 0;
	}

	for (int i = 0; i < band->wpa_supp_n_bitrates; i++) {
		band->bitrates[i].wpa_supp_bitrate = br[i];
		band->bitrates[i].wpa_supp_flags = 0;
	}
}

static int airoc_emit_minimal_wiphy(void *if_priv)
{
	struct airoc_wifi_data *data = NULL;
	struct wpa_supp_event_supported_band band2g;
	struct wpa_supp_event_supported_band band5g;

	airoc_build_band_2g_min(&band2g);
	airoc_build_band_5g_min(&band5g);
	data = (struct airoc_wifi_data *)if_priv;

	data->airoc_wifi_wpa_supp_dev_callbk_fns.get_wiphy_res(data->supp_if_ctx, &band2g);
	data->airoc_wifi_wpa_supp_dev_callbk_fns.get_wiphy_res(data->supp_if_ctx, &band5g);
	data->airoc_wifi_wpa_supp_dev_callbk_fns.get_wiphy_res(data->supp_if_ctx, NULL);
	return 0;
}

int airoc_wifi_wpa_supp_get_wiphy(void *if_priv)
{
	int status = -EAGAIN;
	struct airoc_wifi_data *data = NULL;

	if (!if_priv) {
		LOG_ERR("%s: Missing interface context", __func__);
		goto out;
	}

	data = (struct airoc_wifi_data *)if_priv;
	status = airoc_emit_minimal_wiphy(if_priv);
out:
	return status;
}

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
static const struct zep_wpa_supp_dev_ops airoc_wifi_wpa_drv_ops = {
	.init = airoc_wifi_wpa_supp_dev_init,
	.scan2 = airoc_wifi_wpa_supp_scan2,
	.scan_abort = airoc_wifi_wpa_supp_scan_abort,
	.get_scan_results2 = airoc_wifi_wpa_supp_get_scan_results2,
	.set_key = airoc_wifi_wpa_supp_set_key,
	.get_capa = airoc_wifi_wpa_supp_get_capa,
	.get_conn_info = airoc_wifi_wpa_supp_get_conn_info,
	.get_wiphy = airoc_wifi_wpa_supp_get_wiphy,
	.authenticate = airoc_wifi_wpa_supp_authenticate,
	.associate = airoc_wifi_wpa_supp_associate,
	.set_supp_port = airoc_wifi_wpa_supp_set_supp_port,
	.signal_poll = airoc_wifi_wpa_supp_signal_poll,
	.send_mlme = airoc_wifi_wpa_supp_send_mlme,
	.send_external_auth_status = airoc_wifi_wpa_supp_send_ext_auth_sts,
	.deauthenticate = airoc_wifi_wpa_supp_deauthenticate,
#if defined(CONFIG_IFX_WIFI_SOFTAP_SUPPORT)
	.init_ap = airoc_wifi_wpa_supp_init_ap,
	.start_ap = airoc_wifi_wpa_supp_start_ap,
	.register_mgmt_frame = airoc_wifi_wpa_supp_reg_mgmt_frame,
#endif
};
#endif

static const struct wifi_mgmt_ops airoc_wifi_mgmt = {
	.scan = airoc_mgmt_scan,
	.connect = airoc_mgmt_connect,
	.disconnect = airoc_mgmt_disconnect,
	.ap_enable = airoc_mgmt_ap_enable,
	.ap_disable = airoc_mgmt_ap_disable,
	.iface_status = airoc_iface_status,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = airoc_mgmt_wifi_stats,
#endif
};

static const struct net_wifi_mgmt_offload airoc_api = {
	.wifi_iface.iface_api.init = airoc_mgmt_init,
	.wifi_iface.send = airoc_mgmt_send,
	.wifi_iface.get_capabilities = airoc_get_capabilities,
	.wifi_iface.set_config = airoc_set_config,
	.wifi_mgmt_api = &airoc_wifi_mgmt,
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
	.wifi_drv_ops = &airoc_wifi_wpa_drv_ops,
#endif
};

#ifndef CONFIG_WIFI_NM_WPA_SUPPLICANT
NET_DEVICE_DT_INST_DEFINE(0, airoc_init, NULL, &airoc_wifi_data, &airoc_wifi_config,
			  CONFIG_WIFI_INIT_PRIORITY, &airoc_api, ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), WHD_LINK_MTU);

CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
#else
NET_DEVICE_INIT_INSTANCE(airoc_wifi_sta, "wlan0", 0, airoc_init, NULL, &airoc_wifi_data,
			 &airoc_wifi_config, CONFIG_WIFI_INIT_PRIORITY, &airoc_api, ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2), WHD_LINK_MTU)

CONNECTIVITY_WIFI_MGMT_BIND(airoc_wifi_sta);
#endif
