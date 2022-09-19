/*
 * Copyright (c) 2019 Tobias Svehagen
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_AT_ESP_H_
#define ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_AT_ESP_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>

#include "modem_context.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define the commands that differ between the AT versions */
#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
#define _CWMODE "CWMODE_CUR"
#define _CWSAP  "CWSAP_CUR"
#define _CWJAP  "CWJAP_CUR"
#define _CIPSTA "CIPSTA_CUR"
#define _CIPSTAMAC "CIPSTAMAC_CUR"
#define _CIPRECVDATA "+CIPRECVDATA,"
#define _CIPRECVDATA_END ':'
#else
#define _CWMODE "CWMODE"
#define _CWSAP  "CWSAP"
#define _CWJAP  "CWJAP"
#define _CIPSTA "CIPSTA"
#define _CIPSTAMAC "CIPSTAMAC"
#define _CIPRECVDATA "+CIPRECVDATA:"
#define _CIPRECVDATA_END ','
#endif

/*
 * Passive mode differs a bit between firmware versions and the macro
 * ESP_PROTO_PASSIVE is therefore used to determine what protocol operates in
 * passive mode. For AT version 1.7 passive mode only affects TCP but in AT
 * version 2.0 it affects both TCP and UDP.
 */
#if defined(CONFIG_WIFI_ESP_AT_PASSIVE_MODE)
#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
#define ESP_PROTO_PASSIVE(proto) (proto == IPPROTO_TCP)
#else
#define ESP_PROTO_PASSIVE(proto) \
	(proto == IPPROTO_TCP || proto == IPPROTO_UDP)
#endif /* CONFIG_WIFI_ESP_AT_VERSION_1_7 */
#else
#define ESP_PROTO_PASSIVE(proto) 0
#endif /* CONFIG_WIFI_ESP_AT_PASSIVE_MODE */

#define ESP_BUS DT_INST_BUS(0)

#if DT_PROP(ESP_BUS, hw_flow_control) == 1
#define _FLOW_CONTROL "3"
#else
#define _FLOW_CONTROL "0"
#endif

#if DT_INST_NODE_HAS_PROP(0, target_speed)
#define _UART_BAUD	DT_INST_PROP(0, target_speed)
#else
#define _UART_BAUD	DT_PROP(ESP_BUS, current_speed)
#endif

#define _UART_CUR \
	STRINGIFY(_UART_BAUD)",8,1,0,"_FLOW_CONTROL

#define CONN_CMD_MAX_LEN (sizeof("AT+"_CWJAP"=\"\",\"\"") + \
			  WIFI_SSID_MAX_LEN + WIFI_PSK_MAX_LEN)

#if defined(CONFIG_WIFI_ESP_AT_DNS_USE)
#define ESP_MAX_DNS	MIN(3, CONFIG_DNS_RESOLVER_MAX_SERVERS)
#endif

#define ESP_MAX_SOCKETS 5

/* Maximum amount that can be sent with CIPSEND and read with CIPRECVDATA */
#define ESP_MTU		2048
#define CIPRECVDATA_MAX_LEN	ESP_MTU

#define INVALID_LINK_ID		255

#define MDM_RING_BUF_SIZE	CONFIG_WIFI_ESP_AT_MDM_RING_BUF_SIZE
#define MDM_RECV_MAX_BUF	CONFIG_WIFI_ESP_AT_MDM_RX_BUF_COUNT
#define MDM_RECV_BUF_SIZE	CONFIG_WIFI_ESP_AT_MDM_RX_BUF_SIZE

#define ESP_CMD_TIMEOUT		K_SECONDS(10)
#define ESP_SCAN_TIMEOUT	K_SECONDS(10)
#define ESP_CONNECT_TIMEOUT	K_SECONDS(20)
#define ESP_INIT_TIMEOUT	K_SECONDS(10)

#define ESP_MODE_NONE		0
#define ESP_MODE_STA		1
#define ESP_MODE_AP		2
#define ESP_MODE_STA_AP		3

#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7) || defined(CONFIG_WIFI_ESP_AT_VERSION_2_0)
#define ESP_CMD_CWMODE(mode) "AT+"_CWMODE"="STRINGIFY(_CONCAT(ESP_MODE_, mode))
#else
#define ESP_CMD_CWMODE(mode) "AT+"_CWMODE"="STRINGIFY(_CONCAT(ESP_MODE_, mode))",0"
#endif

#define ESP_CWDHCP_MODE_STATION		"1"
#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
#define ESP_CWDHCP_MODE_SOFTAP		"0"
#else
#define ESP_CWDHCP_MODE_SOFTAP		"2"
#endif

#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
#define _ESP_CMD_DHCP_ENABLE(mode, enable) \
			  "AT+CWDHCP_CUR=" mode "," STRINGIFY(enable)
#else
#define _ESP_CMD_DHCP_ENABLE(mode, enable) \
			  "AT+CWDHCP=" STRINGIFY(enable) "," mode
#endif

#define ESP_CMD_DHCP_ENABLE(mode, enable) \
	_ESP_CMD_DHCP_ENABLE(_CONCAT(ESP_CWDHCP_MODE_, mode), enable)

#define ESP_CMD_SET_IP(ip, gateway, mask) "AT+"_CIPSTA"=\"" \
			  ip "\",\""  gateway  "\",\""  mask "\""

#if defined(CONFIG_WIFI_ESP_AT_SCAN_PASSIVE)
#define ESP_CMD_CWLAP "AT+CWLAP=,,,1,,"
#else
#define ESP_CMD_CWLAP "AT+CWLAP"
#endif

#if defined(CONFIG_WIFI_ESP_AT_SCAN_RESULT_RSSI_ORDERED)
#define ESP_CMD_CWLAPOPT_ORDERED "1"
#else
#define ESP_CMD_CWLAPOPT_ORDERED "0"
#endif

#if defined(CONFIG_WIFI_ESP_AT_SCAN_MAC_ADDRESS)
/* We need ecn,ssid,rssi,mac,channel */
#define ESP_CMD_CWLAPOPT_MASK "31"
#else
/* no mac: only need ecn,ssid,rssi,channel */
#define ESP_CMD_CWLAPOPT_MASK "23"
#endif

#define ESP_CMD_CWLAPOPT(sort, mask) "AT+CWLAPOPT=" sort "," mask

extern struct esp_data esp_driver_data;

enum esp_socket_flags {
	ESP_SOCK_IN_USE     = BIT(1),
	ESP_SOCK_CONNECTING = BIT(2),
	ESP_SOCK_CONNECTED  = BIT(3),
	ESP_SOCK_CLOSE_PENDING = BIT(4),
	ESP_SOCK_WORKQ_STOPPED = BIT(5),
};

struct esp_socket {
	/* internal */
	struct k_mutex lock;
	atomic_t refcount;

	uint8_t idx;
	uint8_t link_id;
	atomic_t flags;

	/* socket info */
	struct sockaddr dst;

	/* sem */
	union {
		/* handles blocking receive */
		struct k_sem sem_data_ready;

		/* notifies about reaching 0 refcount */
		struct k_sem sem_free;
	};

	/* work */
	struct k_work connect_work;
	struct k_work recvdata_work;
	struct k_work send_work;
	struct k_work close_work;

	/* TX packets fifo */
	struct k_fifo tx_fifo;

	/* net context */
	struct net_context *context;
	net_context_connect_cb_t connect_cb;
	net_context_recv_cb_t recv_cb;

	/* callback data */
	void *conn_user_data;
	void *recv_user_data;
};

enum esp_data_flag {
	EDF_STA_CONNECTING = BIT(1),
	EDF_STA_CONNECTED  = BIT(2),
	EDF_STA_LOCK       = BIT(3),
	EDF_AP_ENABLED     = BIT(4),
};

/* driver data */
struct esp_data {
	struct net_if *net_iface;

	uint8_t flags;
	uint8_t mode;

	char conn_cmd[CONN_CMD_MAX_LEN];

	/* addresses  */
	struct in_addr ip;
	struct in_addr gw;
	struct in_addr nm;
	uint8_t mac_addr[6];
#if defined(ESP_MAX_DNS)
	struct sockaddr_in dns_addresses[ESP_MAX_DNS];
#endif

	/* modem context */
	struct modem_context mctx;

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	uint8_t iface_rb_buf[MDM_RING_BUF_SIZE];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE];

	/* socket data */
	struct esp_socket sockets[ESP_MAX_SOCKETS];
	struct esp_socket *rx_sock;

	/* work */
	struct k_work_q workq;
	struct k_work init_work;
	struct k_work_delayable ip_addr_work;
	struct k_work scan_work;
	struct k_work connect_work;
	struct k_work disconnect_work;
	struct k_work mode_switch_work;
	struct k_work dns_work;

	scan_result_cb_t scan_cb;

	/* semaphores */
	struct k_sem sem_tx_ready;
	struct k_sem sem_response;
	struct k_sem sem_if_ready;
};

int esp_offload_init(struct net_if *iface);

struct esp_socket *esp_socket_get(struct esp_data *data,
				  struct net_context *context);
int esp_socket_put(struct esp_socket *sock);
void esp_socket_init(struct esp_data *data);
void esp_socket_close(struct esp_socket *sock);
void esp_socket_rx(struct esp_socket *sock, struct net_buf *buf,
		   size_t offset, size_t len);
void esp_socket_workq_stop_and_flush(struct esp_socket *sock);
struct esp_socket *esp_socket_ref(struct esp_socket *sock);
void esp_socket_unref(struct esp_socket *sock);

static inline
struct esp_socket *esp_socket_ref_from_link_id(struct esp_data *data,
					       uint8_t link_id)
{
	if (link_id >= ARRAY_SIZE(data->sockets)) {
		return NULL;
	}

	return esp_socket_ref(&data->sockets[link_id]);
}

static inline atomic_val_t esp_socket_flags_update(struct esp_socket *sock,
						   atomic_val_t value,
						   atomic_val_t mask)
{
	atomic_val_t flags;

	do {
		flags = atomic_get(&sock->flags);
	} while (!atomic_cas(&sock->flags, flags, (flags & ~mask) | value));

	return flags;
}

static inline
atomic_val_t esp_socket_flags_clear_and_set(struct esp_socket *sock,
					    atomic_val_t clear_flags,
					    atomic_val_t set_flags)
{
	return esp_socket_flags_update(sock, set_flags,
				       clear_flags | set_flags);
}

static inline atomic_val_t esp_socket_flags_set(struct esp_socket *sock,
						atomic_val_t flags)
{
	return atomic_or(&sock->flags, flags);
}

static inline bool esp_socket_flags_test_and_clear(struct esp_socket *sock,
						   atomic_val_t flags)
{
	return (atomic_and(&sock->flags, ~flags) & flags);
}

static inline bool esp_socket_flags_test_and_set(struct esp_socket *sock,
						 atomic_val_t flags)
{
	return (atomic_or(&sock->flags, flags) & flags);
}

static inline atomic_val_t esp_socket_flags_clear(struct esp_socket *sock,
						  atomic_val_t flags)
{
	return atomic_and(&sock->flags, ~flags);
}

static inline atomic_val_t esp_socket_flags(struct esp_socket *sock)
{
	return atomic_get(&sock->flags);
}

static inline struct esp_data *esp_socket_to_dev(struct esp_socket *sock)
{
	return CONTAINER_OF(sock - sock->idx, struct esp_data, sockets);
}

static inline void __esp_socket_work_submit(struct esp_socket *sock,
					    struct k_work *work)
{
	struct esp_data *data = esp_socket_to_dev(sock);

	k_work_submit_to_queue(&data->workq, work);
}

static inline int esp_socket_work_submit(struct esp_socket *sock,
					  struct k_work *work)
{
	int ret = -EBUSY;

	k_mutex_lock(&sock->lock, K_FOREVER);
	if (!(esp_socket_flags(sock) & ESP_SOCK_WORKQ_STOPPED)) {
		__esp_socket_work_submit(sock, work);
		ret = 0;
	}
	k_mutex_unlock(&sock->lock);

	return ret;
}

static inline int esp_socket_queue_tx(struct esp_socket *sock,
				      struct net_pkt *pkt)
{
	int ret = -EBUSY;

	k_mutex_lock(&sock->lock, K_FOREVER);
	if (!(esp_socket_flags(sock) & ESP_SOCK_WORKQ_STOPPED)) {
		k_fifo_put(&sock->tx_fifo, pkt);
		__esp_socket_work_submit(sock, &sock->send_work);
		ret = 0;
	}
	k_mutex_unlock(&sock->lock);

	return ret;
}

static inline bool esp_socket_connected(struct esp_socket *sock)
{
	return (esp_socket_flags(sock) & ESP_SOCK_CONNECTED) != 0;
}

static inline void esp_flags_set(struct esp_data *dev, uint8_t flags)
{
	dev->flags |= flags;
}

static inline void esp_flags_clear(struct esp_data *dev, uint8_t flags)
{
	dev->flags &= (~flags);
}

static inline bool esp_flags_are_set(struct esp_data *dev, uint8_t flags)
{
	return (dev->flags & flags) != 0;
}

static inline enum net_sock_type esp_socket_type(struct esp_socket *sock)
{
	return net_context_get_type(sock->context);
}

static inline enum net_ip_protocol esp_socket_ip_proto(struct esp_socket *sock)
{
	return net_context_get_proto(sock->context);
}

static inline int esp_cmd_send(struct esp_data *data,
			       const struct modem_cmd *handlers,
			       size_t handlers_len, const char *buf,
			       k_timeout_t timeout)
{
	return modem_cmd_send(&data->mctx.iface, &data->mctx.cmd_handler,
			      handlers, handlers_len, buf, &data->sem_response,
			      timeout);
}

void esp_connect_work(struct k_work *work);
void esp_recvdata_work(struct k_work *work);
void esp_close_work(struct k_work *work);
void esp_send_work(struct k_work *work);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_AT_ESP_H_ */
