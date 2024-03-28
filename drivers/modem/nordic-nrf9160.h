/*
 * Copyright (c) 2023 Sendrato B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef MODEM_NRF9160_H
#define MODEM_NRF9160_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/backend/uart.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/socket_offload.h>

#include "modem_socket.h"

#define MDM_INIT_SCRIPT_TIMEOUT_SECONDS  10
#define MDM_DYNAMIC_SCRIPT_TIMEOUT_SEC   5
#define MDM_RECV_DATA_SCRIPT_TIMEOUT_SEC 2
#define MDM_RESET_SCRIPT_TIMEOUT_SEC     10
#define MDM_SCRIPT_DONE_TIMEOUT_SEC      (MDM_DYNAMIC_SCRIPT_TIMEOUT_SEC + 2)

#define MDM_SENDMSG_SLEEP         K_MSEC(1)
#define MDM_RECV_DATA_TIMEOUT_SEC 1
#define MDM_INIT_TIMEOUT_SEC      (MDM_INIT_SCRIPT_TIMEOUT_SECONDS + 2)
#define MDM_RESET_TIMEOUT_SEC     10

#define MDM_REQUEST_SCHED_DELAY_MSEC 500
#define MDM_REQUEST_WAIT_READY_MSEC  500

#define MDM_MAC_ADDR_LENGTH 6
#define MDM_MAX_DATA_LENGTH 1024
#define MDM_MAX_SOCKETS     3
#define MDM_BASE_SOCKET_NUM 0

/* Default lengths of modem info */
#define MDM_IMEI_LENGTH         15
#define MDM_MANUFACTURER_LENGTH 30
#define MDM_MODEL_LENGTH        24
#define MDM_REVISION_LENGTH     64

/* Setup AT commands */
/* System mode */
#if IS_ENABLED(CONFIG_MODEM_NRF9160_MODE_LTE_ONLY)
#define MDM_SETUP_CMD_SYSTEM_MODE "AT%XSYSTEMMODE=1,0,1,1"
#elif IS_ENABLED(CONFIG_MODEM_NRF9160_MODE_DUAL)
#define MDM_SETUP_CMD_SYSTEM_MODE "AT%XSYSTEMMODE=1,1,1,0"
#elif IS_ENABLED(CONFIG_MODEM_NRF9160_MODE_DUAL_LTE_PREF)
#define MDM_SETUP_CMD_SYSTEM_MODE "AT%XSYSTEMMODE=1,1,1,1"
#endif
/* PDP context */
#define MDM_SETUP_CMD_PDP_CTX "AT+CGDCONT=0,\"IP\",\"" CONFIG_MODEM_NRF9160_APN "\""

/* Default SLM data mode terminator command */
#define MDM_DATA_MODE_TERMINATOR "!~>&}@%"

/* Modem ATOI routine. */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)
/* Modem ATOL routine. */
#define ATOL(s_, desc_, res_)   modem_atol(s_, desc_, res_, __func__)

enum modem_event {
	MODEM_EVENT_RESUME = 0,
	MODEM_EVENT_SUSPEND,
	MODEM_EVENT_SCRIPT_SUCCESS,
	MODEM_EVENT_SCRIPT_FAILED,
	MODEM_EVENT_BUS_OPENED,
	MODEM_EVENT_BUS_CLOSED,
};

enum modem_request {
	MODEM_REQ_RESET,
	/* NetIF related requests */
	MODEM_REQ_IFACE_ENABLE,
	MODEM_REQ_IFACE_DISABLE,
	/* GNSS related requests */
	MODEM_REQ_GNSS_RESUME,
	MODEM_REQ_GNSS_SUSPEND,
	/* Sockets related requests */
	MODEM_REQ_OPEN_SOCK,
	MODEM_REQ_CLOSE_SOCK,
	MODEM_REQ_CONNECT_SOCK,
	MODEM_REQ_DATA_MODE,
	MODEM_REQ_SEND_DATA,
	MODEM_REQ_RECV_DATA,
	MODEM_REQ_SELECT_SOCK,
	MODEM_REQ_GET_ACTIVE_SOCK,
	/* DNS related requests */
	MODEM_REQ_GET_ADDRINFO,
};

enum modem_state {
	MODEM_STATE_IDLE = 0,
	MODEM_STATE_INIT,
	MODEM_STATE_READY,
};

struct net_if_data {
	const struct device *modem_dev;
};

struct offload_if {
	struct net_if *net_iface;
	uint8_t mac_addr[MDM_MAC_ADDR_LENGTH];
};

#define OFFLOADED_NETDEV_L2_CTX_TYPE struct offload_if

struct open_sock_t {
	int family;
	int type;
};

struct connect_sock_t {
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port;
};

struct socket_send_t {
	struct modem_socket *sock;
	const struct sockaddr *dst_addr;
	uint8_t *buf;
	size_t len;
	int sent;
};

struct get_addrinfo_t {
	const char *node;
};

struct recv_sock_t {
	int flags;
	/* Amount of bytes received */
	uint16_t nbytes;
};

struct select_sock_t {
	int sock_fd;
};

struct modem_data {
	/* Child node net_if */
	struct offload_if iface;
	/* Child node gnss device */
	const struct device *gnss_dev;

	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_MODEM_NORDIC_NRF9160_UART_RX_BUF_SIZE];
	uint8_t uart_backend_transmit_buf[CONFIG_MODEM_NORDIC_NRF9160_UART_TX_BUF_SIZE];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[128];
	uint8_t chat_delimiter[2];
	uint8_t *chat_argv[32];

	/* Modem info */
	uint8_t imei[MDM_IMEI_LENGTH];
	uint8_t manufacturer[MDM_MANUFACTURER_LENGTH];
	uint8_t model[MDM_MODEL_LENGTH];
	uint8_t revision[MDM_REVISION_LENGTH];

	/* Device node */
	const struct device *dev;
	enum modem_state state;
	bool connected;

	/* Event dispatcher */
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct ring_buf event_rb;
	struct k_mutex event_rb_lock;

	/* Request dispatcher */
	struct k_work_delayable request_dispatch_work;
	uint8_t request_buf[8];
	struct ring_buf request_rb;
	struct k_mutex request_rb_lock;

	/* Dynamic chat script */
	uint8_t dynamic_match_buf[32];
	uint8_t dynamic_separators_buf[2];
	uint8_t dynamic_request_buf[64];
	struct modem_chat_match dynamic_match;
	struct modem_chat_script_chat dynamic_script_chat;
	struct modem_chat_script dynamic_script;
	int dynamic_script_res;

	/* Socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	/* Active socket fd */
	int sock_fd;

	/* State semaphore */
	struct k_sem sem_state;
	/* Script exec semaphore */
	struct k_sem sem_script_exec;
	/* Script done semaphore */
	struct k_sem sem_script_done;
	/* Script sync semaphore */
	struct k_sem sem_script_sync;

	/* GNSS data */
	uint16_t gnss_interval;
	uint16_t gnss_timeout;

	/* Structs to offload socket operations */
	struct open_sock_t open_sock;
	struct connect_sock_t connect_sock;
	struct recv_sock_t recv_sock;
	struct socket_send_t send_sock;
	struct select_sock_t select_sock;
	/* Structs to offload DNS operations */
	struct get_addrinfo_t get_addrinfo;
};

struct modem_config {
	const struct device *uart;
	const struct gpio_dt_spec power_gpio;
	const struct gpio_dt_spec reset_gpio;
	const struct modem_chat_script *init_chat_script;
	const struct modem_chat_script *reset_chat_script;
	/* Offload DNS ops */
	const struct socket_dns_offload dns_ops;
	/* Socket create API */
	net_socket_create_t sock_create;
};

#endif // MODEM_NRF9160_H
