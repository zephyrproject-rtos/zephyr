/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_H_
#define ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_H_

#include <kernel.h>
#include <net/net_context.h>
#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/wifi_mgmt.h>

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
#if defined(CONFIG_WIFI_ESP_PASSIVE_MODE)
#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
#define ESP_PROTO_PASSIVE(proto) (proto == IPPROTO_TCP)
#else
#define ESP_PROTO_PASSIVE(proto) \
	(proto == IPPROTO_TCP || proto == IPPROTO_UDP)
#endif /* CONFIG_WIFI_ESP_AT_VERSION_1_7 */
#else
#define ESP_PROTO_PASSIVE(proto) 0
#endif /* CONFIG_WIFI_ESP_PASSIVE_MODE */

#define ESP_BUS DT_BUS(DT_DRV_INST(0))

#if DT_PROP(ESP_BUS, hw_flow_control) == 1
#define _FLOW_CONTROL "3"
#else
#define _FLOW_CONTROL "0"
#endif

#define _UART_CUR \
	STRINGIFY(DT_PROP(ESP_BUS, current_speed))",8,1,0,"_FLOW_CONTROL

#define CONN_CMD_MAX_LEN (sizeof("AT+"_CWJAP"=\"\",\"\"") + \
			  WIFI_SSID_MAX_LEN + WIFI_PSK_MAX_LEN)

#define ESP_MAX_SOCKETS 5

/* Maximum amount that can be sent with CIPSEND and read with CIPRECVDATA */
#define ESP_MTU		2048
#define CIPRECVDATA_MAX_LEN	ESP_MTU

#define INVALID_LINK_ID		255

#define MDM_RING_BUF_SIZE	1024
#define MDM_RECV_MAX_BUF	30
#define MDM_RECV_BUF_SIZE	128
#define CMD_BUF_ALLOC_TIMEOUT	K_SECONDS(1)

#define ESP_CMD_TIMEOUT		K_SECONDS(10)
#define ESP_SCAN_TIMEOUT	K_SECONDS(10)
#define ESP_CONNECT_TIMEOUT	K_SECONDS(20)
#define ESP_INIT_TIMEOUT	K_SECONDS(10)

extern struct esp_data esp_driver_data;

enum esp_socket_flags {
	ESP_SOCK_IN_USE     = BIT(1),
	ESP_SOCK_CONNECTING = BIT(2),
	ESP_SOCK_CONNECTED  = BIT(3)
};

struct esp_socket {
	/* internal */
	uint8_t idx;
	uint8_t link_id;
	uint8_t flags;

	/* socket info */
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;
	struct sockaddr src;
	struct sockaddr dst;

	/* for +CIPRECVDATA */
	size_t bytes_avail;

	/* packets */
	struct k_fifo fifo_rx_pkt;
	struct net_pkt *tx_pkt;

	/* sem */
	struct k_sem sem_data_ready;

	/* work */
	struct k_work connect_work;
	struct k_work send_work;
	struct k_work recv_work;
	struct k_work recvdata_work;

	/* net context */
	struct net_context *context;
	net_context_connect_cb_t connect_cb;
	net_context_send_cb_t send_cb;
	net_context_recv_cb_t recv_cb;

	/* callback data */
	void *conn_user_data;
	void *send_user_data;
	void *recv_user_data;
};

enum esp_data_flag {
	EDF_STA_CONNECTING = BIT(1),
	EDF_STA_CONNECTED  = BIT(2)
};

/* driver data */
struct esp_data {
	struct net_if *net_iface;

	uint8_t flags;

	char conn_cmd[CONN_CMD_MAX_LEN];

	/* addresses  */
	struct in_addr ip;
	struct in_addr gw;
	struct in_addr nm;
	uint8_t mac_addr[6];

	/* modem context */
	struct modem_context mctx;

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	uint8_t iface_isr_buf[MDM_RECV_BUF_SIZE];
	uint8_t iface_rb_buf[MDM_RING_BUF_SIZE];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_read_buf[MDM_RECV_BUF_SIZE];
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE];

	/* socket data */
	struct esp_socket sockets[ESP_MAX_SOCKETS];
	struct esp_socket *rx_sock;

	/* work */
	struct k_work_q workq;
	struct k_work init_work;
	struct k_delayed_work ip_addr_work;
	struct k_work scan_work;
	struct k_work connect_work;

	scan_result_cb_t scan_cb;

	/* semaphores */
	struct k_sem sem_tx_ready;
	struct k_sem sem_response;
	struct k_sem sem_if_ready;
	struct k_sem sem_if_up;
};

int esp_offload_init(struct net_if *iface);

struct net_pkt *esp_prepare_pkt(struct esp_data *dev, struct net_buf *src,
				size_t offset, size_t len);
struct esp_socket *esp_socket_get();
int esp_socket_put(struct esp_socket *sock);
struct esp_socket *esp_socket_from_link_id(struct esp_data *data,
					   uint8_t link_id);
void esp_socket_init(struct esp_data *data);

static inline struct esp_data *esp_socket_to_dev(struct esp_socket *sock)
{
	return CONTAINER_OF(sock - sock->idx, struct esp_data, sockets);
}

static inline bool esp_socket_in_use(struct esp_socket *sock)
{
	return (sock->flags & ESP_SOCK_IN_USE) != 0;
}

static inline bool esp_socket_connected(struct esp_socket *sock)
{
	return (sock->flags & ESP_SOCK_CONNECTED) != 0;
}

static inline void esp_flag_set(struct esp_data *dev,
				enum esp_data_flag flag)
{
	dev->flags |= flag;
}

static inline void esp_flag_clear(struct esp_data *dev,
				  enum esp_data_flag flag)
{
	dev->flags &= (~flag);
}

static inline bool esp_flag_is_set(struct esp_data *dev,
				   enum esp_data_flag flag)
{
	return (dev->flags & flag) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_H_ */
