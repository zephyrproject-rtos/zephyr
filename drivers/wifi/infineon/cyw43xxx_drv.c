/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief CYW43xxx wifi driver.
 */

#define DT_DRV_COMPAT infineon_cyw43xxx_wifi_sdio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(infineon_cyw43xxx_wifi, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/device.h>
#include <soc.h>
#include <cybsp_wifi.h>
#include <cyhal.h>
#include <whd_types.h>
#include <whd_events_int.h>
#include <whd_buffer_api.h>
#include <cy_network_buffer.h>
#include <cybsp_wifi.h>
#include <cybsp.h>

#ifndef CYW43XXX_TX_PACKET_POOL_COUNT
#define CYW43XXX_TX_PACKET_POOL_COUNT         (10)
#endif

#ifndef CYW43XXX_RX_PACKET_POOL_COUNT
#define CYW43XXX_RX_PACKET_POOL_COUNT         (10)
#endif

#ifndef CYW43XXX_PACKET_POOL_SIZE
#define CYW43XXX_PACKET_POOL_SIZE             (1600)
#endif

#define CYW43XXX_PACKET_POOL_COUNT            (CYW43XXX_TX_PACKET_POOL_COUNT + \
					       CYW43XXX_RX_PACKET_POOL_COUNT)

/* This macro is copy of NET_BUF_POOL_FIXED_DEFINE with aligning net_buf_data_##_name
 * WHD requires that network buffers is aligned, NET_BUF_POOL_FIXED_DEFINE does not
 * guarantees aligned.
 */
#define NET_BUF_POOL_FIXED_DEFINE_ALIGN(_name, _count, _data_size, _ud_size, _destroy)	  \
	_NET_BUF_ARRAY_DEFINE(_name, _count, _ud_size);					  \
	static uint8_t __noinit net_buf_data_##_name[_count][_data_size] __net_buf_align; \
	static const struct net_buf_pool_fixed net_buf_fixed_##_name = {		  \
		.data_size = _data_size,						  \
		.data_pool = (uint8_t *)net_buf_data_##_name,				  \
	};										  \
	static const struct net_buf_data_alloc net_buf_fixed_alloc_##_name = {		  \
		.cb = &net_buf_fixed_cb,						  \
		.alloc_data = (void *)&net_buf_fixed_##_name,				  \
	};										  \
	static STRUCT_SECTION_ITERABLE(net_buf_pool, _name) =				  \
		NET_BUF_POOL_INITIALIZER(_name, &net_buf_fixed_alloc_##_name,		  \
					 _net_buf_##_name, _count, _ud_size,		  \
					 _destroy)

/* Allocate network pool */
NET_BUF_POOL_FIXED_DEFINE_ALIGN(cyw43xxx_pool,
				CYW43XXX_PACKET_POOL_COUNT, CYW43XXX_PACKET_POOL_SIZE,
				0, NULL);

/* Use global iface pointer to support any Ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *infineon_cyw43xxx_wifi_iface;

struct infineon_cyw43xxx_wifi_runtime {
	struct net_if *iface;
	uint8_t mac_addr[6];
	bool tx_err;
	uint32_t tx_word;
	int tx_pos;
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE];
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

static whd_interface_t cyw43xxx_if;
static const whd_event_num_t sta_link_events[] = {
	WLC_E_LINK, WLC_E_DEAUTH_IND, WLC_E_DISASSOC_IND,
	WLC_E_PSK_SUP, WLC_E_CSA_COMPLETE_IND, WLC_E_NONE
};
static uint16_t sta_event_handler_index = 0xFF;
static void infineon_cyw43xxx_event_task(void);
static cyhal_sdio_t sdio_obj;

K_MSGQ_DEFINE(cyw43xxx_wifi_msgq, sizeof(whd_event_header_t), 10, 4);
K_THREAD_STACK_DEFINE(cyw43xxx_wifi_event_stack, CONFIG_CYW43XXX_WIFI_EVENT_TASK_STACK_SIZE);

static struct k_thread cyw43xxx_wifi_event_thread;

cyhal_sdio_t *cybsp_get_wifi_sdio_obj(void)
{
	return &sdio_obj;
}

whd_interface_t cyw43xx_get_whd_interface(void)
{
	return cyw43xxx_if;
}

whd_result_t cy_host_buffer_get(whd_buffer_t *buffer, whd_buffer_dir_t direction,
				unsigned short size, unsigned long timeout_ms)
{
	ARG_UNUSED(direction);
	ARG_UNUSED(timeout_ms);
	struct net_buf *buf;

	buf = net_buf_alloc_len(&cyw43xxx_pool, size, K_NO_WAIT);
	if (buf == NULL) {
		return WHD_BUFFER_ALLOC_FAIL;
	}
	*buffer = buf;
	return WHD_SUCCESS;
}

void cy_buffer_release(whd_buffer_t buffer, whd_buffer_dir_t direction)
{
	CY_UNUSED_PARAMETER(direction);
	(void)net_buf_destroy((struct net_buf *)buffer);
}

uint8_t *cy_buffer_get_current_piece_data_pointer(whd_buffer_t buffer)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf *buf = (struct net_buf *)buffer;

	return (uint8_t *)buf->data;
}

uint16_t cy_buffer_get_current_piece_size(whd_buffer_t buffer)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf *buf = (struct net_buf *)buffer;

	return (uint16_t)buf->size;
}

whd_result_t cy_buffer_set_size(whd_buffer_t buffer, unsigned short size)
{
	CY_ASSERT(buffer != NULL);
	struct net_buf *buf = (struct net_buf *)buffer;

	buf->size = size;
	return CY_RSLT_SUCCESS;
}

whd_result_t cy_buffer_add_remove_at_front(whd_buffer_t *buffer, int32_t add_remove_amount)
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

static int eth_infineon_cyw43xxx_send(const struct device *dev, struct net_pkt *pkt)
{
	struct infineon_cyw43xxx_wifi_runtime *data = dev->data;
	cy_rslt_t ret = CY_RSLT_SUCCESS;
	size_t pkt_len = net_pkt_get_len(pkt);
	struct net_buf *buf = NULL;

	/* Read the packet payload */
	if (net_pkt_read(pkt, data->frame_buf, pkt_len) < 0) {
		LOG_ERR("net_pkt_read failed");
		return -EIO;
	}
	/* Allocate Network Buffer from pool with Packet Length + Data Header */
	buf = net_buf_alloc_len(&cyw43xxx_pool, pkt_len + sizeof(data_header_t), K_NO_WAIT);

	/* Reserve the buffer Headroom for WHD Data header */
	net_buf_reserve(buf, sizeof(data_header_t));

	/* Copy the buffer to network Buffer pointer */
	(void)memcpy(buf->data, data->frame_buf, pkt_len);

	/* Call WHD API to send out the Packet */
	ret = whd_network_send_ethernet_data(cyw43xxx_if, (void *)buf);
	if (ret != CY_RSLT_SUCCESS) {
		LOG_ERR("whd_network_send_ethernet_data failed");
		return -EIO;
	}
	return 0;
}

void cy_network_process_ethernet_data(whd_interface_t interface, whd_buffer_t buffer)
{
	struct net_pkt *pkt;
	uint8_t *data = whd_buffer_get_current_piece_data_pointer(interface->whd_driver, buffer);
	uint32_t len = whd_buffer_get_current_piece_size(interface->whd_driver, buffer);
	bool net_pkt_unref_flag = false;

	if (infineon_cyw43xxx_wifi_iface == NULL) {
		LOG_ERR("network interface unavailable");
		cy_buffer_release(buffer, WHD_NETWORK_RX);
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(infineon_cyw43xxx_wifi_iface, len,
					   AF_UNSPEC, 0, K_NO_WAIT);

	if (pkt != NULL) {
		if (net_pkt_write(pkt, data, len) < 0) {
			LOG_ERR("Failed to write pkt");
			net_pkt_unref_flag = true;
		}

		if ((net_pkt_unref_flag) ||
		    (net_recv_data(infineon_cyw43xxx_wifi_iface, pkt) < 0)) {
			LOG_ERR("Failed to push received data");
			net_pkt_unref_flag = true;
		}
	} else {
		LOG_ERR("Failed to get net buffer");
	}

	/* Release a packet buffer */
	cy_buffer_release(buffer, WHD_NETWORK_RX);

	if (net_pkt_unref_flag) {
		net_pkt_unref(pkt);
	}
}

static void *link_events_handler(whd_interface_t ifp,
				 const whd_event_header_t *event_header,
				 const uint8_t *event_data, void
				 *handler_user_data)
{
	ARG_UNUSED(ifp);
	ARG_UNUSED(event_data);
	ARG_UNUSED(handler_user_data);

	(void)k_msgq_put(&cyw43xxx_wifi_msgq, event_header, K_FOREVER);
	return NULL;
}

static void infineon_cyw43xxx_event_task(void)
{
	whd_event_header_t event_header;

	while (1) {
		(void)k_msgq_get(&cyw43xxx_wifi_msgq, &event_header, K_FOREVER);

		switch ((whd_event_num_t)event_header.event_type) {
		case WLC_E_LINK:
			(void)net_if_up(infineon_cyw43xxx_wifi_iface);
			break;

		case WLC_E_DEAUTH_IND:
		case WLC_E_DISASSOC_IND:
			(void)net_if_down(infineon_cyw43xxx_wifi_iface);
			break;

		default:
			break;
		}
	}
}

static void eth_infineon_cyw43xxx_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct infineon_cyw43xxx_wifi_runtime *dev_data = dev->data;

	dev_data->iface = iface;
	infineon_cyw43xxx_wifi_iface = iface;

	/* Start interface when we are actually connected with WiFi network */
	(void)net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	/* Read WLAN MAC Address */
	(void)whd_wifi_get_mac_address(cyw43xxx_if, &cyw43xxx_if->mac_addr);
	(void)memcpy(&dev_data->mac_addr, &cyw43xxx_if->mac_addr, sizeof(cyw43xxx_if->mac_addr));

	/* Assign link local address. */
	(void)net_if_set_link_addr(iface,
				   dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	(void)ethernet_init(iface);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_infineon_cyw43xxx_stats(const struct device *dev)
{
	return &(((struct infineon_cyw43xxx_wifi_runtime *)dev->data)->stats);
}
#endif

static int eth_infineon_cyw43xxx_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	cy_rslt_t ret = CY_RSLT_SUCCESS;
	cyhal_gpio_t sdio_cmd =
		DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_sdio_cmd_gpios);

	cyhal_gpio_t sdio_clk =
		DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_sdio_clk_gpios);

	cyhal_gpio_t sdio_d0 =
		DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_sdio_d0_gpios);

	cyhal_gpio_t sdio_d1 =
		DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_sdio_d1_gpios);

	cyhal_gpio_t sdio_d2 =
		DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_sdio_d2_gpios);

	cyhal_gpio_t sdio_d3 =
		DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(DT_WIFI_SDIO_NODE, wifi_sdio_d3_gpios);

	ret = cyhal_sdio_init(&sdio_obj, sdio_cmd, sdio_clk,
			      sdio_d0, sdio_d1, sdio_d2, sdio_d3);
	if (ret != CY_RSLT_SUCCESS) {
		LOG_ERR("cyhal_sdio_init failed ret = %d \r\n", ret);
	}

	k_tid_t tid = k_thread_create(&cyw43xxx_wifi_event_thread,
				      cyw43xxx_wifi_event_stack,
				      CONFIG_CYW43XXX_WIFI_EVENT_TASK_STACK_SIZE,
				      (k_thread_entry_t)infineon_cyw43xxx_event_task,
				      NULL, NULL, NULL,
				      CONFIG_CYW43XXX_WIFI_EVENT_TASK_PRIO,
				      K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_name_set(tid, "cyw43xxx_event");

	ret = cybsp_wifi_init_primary_extended(&cyw43xxx_if, NULL, NULL, NULL, NULL);
	if (ret != CY_RSLT_SUCCESS) {
		LOG_ERR("cybsp_wifi_init_primary_extended failed ret = %d \r\n", ret);
	}
	ret = whd_management_set_event_handler(cyw43xxx_if, sta_link_events,
					       link_events_handler, NULL,
					       &sta_event_handler_index);
	if (ret != CY_RSLT_SUCCESS) {
		LOG_ERR("whd_management_set_event_handler failed ret = %d \r\n", ret);
	}

	return ret;
}

static struct infineon_cyw43xxx_wifi_runtime eth_data;
static const struct ethernet_api eth_infineon_cy43xxx_apis = {
	.iface_api.init = eth_infineon_cyw43xxx_init,
	.send =  eth_infineon_cyw43xxx_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_infineon_cyw43xxx_stats,
#endif
};

whd_result_t cy_buffer_pool_init(void *tx_packet_pool, void *rx_packet_pool)
{
	CY_UNUSED_PARAMETER(tx_packet_pool);
	CY_UNUSED_PARAMETER(rx_packet_pool);

	return 0;
}

/* workaround for malloc */
void *malloc(size_t size)
{
	return k_malloc(size);
}

void free(void *buff)
{
	k_free(buff);
}

NET_DEVICE_DT_INST_DEFINE(0,
			  eth_infineon_cyw43xxx_dev_init, NULL,
			  &eth_data, NULL, CONFIG_ETH_INIT_PRIORITY,
			  &eth_infineon_cy43xxx_apis, ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
