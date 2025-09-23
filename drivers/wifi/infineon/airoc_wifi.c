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

LOG_MODULE_REGISTER(infineon_airoc_wifi, CONFIG_WIFI_LOG_LEVEL);

#ifndef AIROC_WIFI_TX_PACKET_POOL_COUNT
#define AIROC_WIFI_TX_PACKET_POOL_COUNT (10)
#endif

#ifndef AIROC_WIFI_RX_PACKET_POOL_COUNT
#define AIROC_WIFI_RX_PACKET_POOL_COUNT (10)
#endif

#ifndef AIROC_WIFI_PACKET_POOL_SIZE
#define AIROC_WIFI_PACKET_POOL_SIZE     (1600)
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
NET_BUF_POOL_FIXED_DEFINE(airoc_pool, AIROC_WIFI_PACKET_POOL_COUNT,
				AIROC_WIFI_PACKET_POOL_SIZE, 0, NULL);

/* AIROC globals */
static uint16_t ap_event_handler_index = 0xFF;

/* Use global iface pointer to support any Ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *airoc_wifi_iface;

static whd_interface_t airoc_if;
static whd_interface_t airoc_sta_if;
static whd_interface_t airoc_ap_if;

static const whd_event_num_t sta_link_events[] = {
	WLC_E_LINK,    WLC_E_DEAUTH_IND,       WLC_E_DISASSOC_IND,
	WLC_E_PSK_SUP, WLC_E_CSA_COMPLETE_IND, WLC_E_NONE};

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
	.bus_dev.bus_spi = SPI_DT_SPEC_GET(DT_DRV_INST(0), AIROC_WIFI_SPI_OPERATION, 0),
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
	uint32_t event_type;
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

static void parse_scan_result(whd_scan_result_t *p_whd_result, struct wifi_scan_result *p_zy_result)
{
	if (p_whd_result->SSID.length != 0) {
		p_zy_result->ssid_length = p_whd_result->SSID.length;
		strncpy(p_zy_result->ssid, p_whd_result->SSID.value, p_whd_result->SSID.length);
		p_zy_result->channel = p_whd_result->channel;
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
	(void) airoc_wifi_buffer_set_size(*buffer, size);

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
	ret = airoc_wifi_host_buffer_get((whd_buffer_t *) &buf, WHD_NETWORK_TX,
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

		pkt = net_pkt_rx_alloc_with_buffer(airoc_wifi_iface, len, AF_UNSPEC, 0, K_NO_WAIT);

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

static int airoc_set_config(const struct device *dev,
			    enum ethernet_config_type type,
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
	ARG_UNUSED(event_data);
	ARG_UNUSED(handler_user_data);

	k_msgq_put(&airoc_wifi_msgq, event_header, K_FOREVER);
	return NULL;
}

static void airoc_event_task(void)
{
	whd_event_header_t event_header;

	while (1) {
		k_msgq_get(&airoc_wifi_msgq, &event_header, K_FOREVER);

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

	/* L1 network layer (physical layer) is up */
	net_if_carrier_on(data->iface);
}

static int airoc_mgmt_scan(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	struct airoc_wifi_data *data = dev->data;

	if (data->scan_rslt_cb != NULL) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	if (k_sem_take(&data->sema_common, K_MSEC(AIROC_WIFI_WAIT_SEMA_MS)) != 0) {
		return -EAGAIN;
	}

	data->scan_rslt_cb = cb;

	/* Connect to the network */
	if (whd_wifi_scan(airoc_sta_if, params->scan_type, WHD_BSS_TYPE_ANY, &(data->ssid), NULL,
			  NULL, NULL, scan_callback, &(data->scan_result), data) != WHD_SUCCESS) {
		LOG_ERR("Failed to start scan");
		k_sem_give(&data->sema_common);
		return -EAGAIN;
	}

	k_sem_give(&data->sema_common);
	return 0;
}

static int airoc_mgmt_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct airoc_wifi_data *data = (struct airoc_wifi_data *)dev->data;
	int ret = 0;
	whd_scan_result_t scan_result;
	whd_scan_result_t usr_result = {0};

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

	if ((params->security == WIFI_SECURITY_TYPE_NONE) && (params->psk_length > 0)) {
		/* Try to scan ssid to define security */

		if (whd_wifi_scan(airoc_sta_if, WHD_SCAN_TYPE_ACTIVE, WHD_BSS_TYPE_ANY, NULL, NULL,
				  NULL, NULL, airoc_wifi_scan_cb_search, &scan_result,
				  &(usr_result)) != WHD_SUCCESS) {
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
		/* Get security from user, convert it to  */
		usr_result.security = convert_zephyr_security_to_whd(params->security);
	}

	if (usr_result.security == WHD_SECURITY_UNKNOWN) {
		ret = -EAGAIN;
		LOG_ERR("Could not scan device");
		goto error;
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
	struct airoc_wifi_event_t airoc_event = {
		.is_ap_event = 1,
		.event_type = event_header->event_type
	};

	k_msgq_put(&airoc_wifi_msgq, &airoc_event, K_FOREVER);

	return NULL;
}

static int airoc_mgmt_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct airoc_wifi_data *data = dev->data;
	whd_security_t security;
	whd_ssid_t ssid;
	uint8_t channel;
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
	if (((params->channel > 0) && (params->channel < 12)) ||
	    ((params->channel > 35) && (params->channel < 166))) {
		channel = params->channel;
	} else {
		channel = 1;
		LOG_WRN("Discard of setting unsupported channel: %u (will set 1)",
			params->channel);
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
			     params->psk_length, channel) != 0) {
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

	whd_ret = whd_management_set_event_handler(airoc_sta_if, sta_link_events,
				link_events_handler, NULL, &sta_event_handler_index);
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

	return 0;
}

static const struct wifi_mgmt_ops airoc_wifi_mgmt = {
	.scan = airoc_mgmt_scan,
	.connect = airoc_mgmt_connect,
	.disconnect = airoc_mgmt_disconnect,
	.ap_enable = airoc_mgmt_ap_enable,
	.ap_disable = airoc_mgmt_ap_disable,
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
};

NET_DEVICE_DT_INST_DEFINE(0, airoc_init, NULL, &airoc_wifi_data, &airoc_wifi_config,
			  CONFIG_WIFI_INIT_PRIORITY, &airoc_api, ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), WHD_LINK_MTU);

CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
