/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODEM_NORDIC_NRF91_SLM_H_
#define MODEM_NORDIC_NRF91_SLM_H_

#include <zephyr/drivers/cellular.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/net/dns_resolve.h>

#include "../modem_socket.h"

#define NRF91_SLM_IMEI_LEN         (16)
#define NRF91_SLM_MODEL_ID_LEN     (65)
#define NRF91_SLM_IMSI_LEN         (23)
#define NRF91_SLM_ICCID_LEN        (22)
#define NRF91_SLM_MANUFACTURER_LEN (65)
#define NRF91_SLM_FW_VERSION_LEN   (65)

#ifdef __cplusplus
extern "C" {
#endif

enum nrf91_slm_state {
	NRF91_SLM_STATE_IDLE = 0,
	NRF91_SLM_STATE_RESET_PULSE,
	NRF91_SLM_STATE_POWER_ON_PULSE,
	NRF91_SLM_STATE_AWAIT_POWER_ON,
	NRF91_SLM_STATE_RUN_INIT_SCRIPT,
	NRF91_SLM_STATE_RUN_DIAL_SCRIPT,
	NRF91_SLM_STATE_AWAIT_REGISTERED,
	NRF91_SLM_STATE_DISCONNECT_PPP,
	NRF91_SLM_STATE_CARRIER_ON,
	NRF91_SLM_STATE_INIT_POWER_OFF,
	NRF91_SLM_STATE_POWER_OFF_PULSE,
	NRF91_SLM_STATE_AWAIT_POWER_OFF,
};

struct nrf91_slm_data {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_MODEM_NRF91_SLM_UART_BUFFER_SIZES];
	uint8_t uart_backend_transmit_buf[CONFIG_MODEM_NRF91_SLM_UART_BUFFER_SIZES];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CONFIG_MODEM_NRF91_SLM_CHAT_BUFFER_SIZES];
	uint8_t *chat_delimiter;
	uint8_t *chat_filter;
	uint8_t *chat_argv[32];
	struct k_mutex chat_lock;

	/* Socket chat script */
	uint8_t sock_receive_buf[CONFIG_MODEM_NRF91_SLM_UART_BUFFER_SIZES];
	uint8_t sock_recv_rb_buf[CONFIG_MODEM_NRF91_SLM_UART_BUFFER_SIZES];
	struct ring_buf sock_recv_rb;
	struct k_sem sock_recv_sem;
	struct k_sem sock_send_sem;
	const uint8_t *sock_send_buf;
	size_t sock_send_buf_len;
	size_t sock_send_count;

	/* Status */
	enum cellular_registration_status registration_status_gsm;
	enum cellular_registration_status registration_status_gprs;
	enum cellular_registration_status registration_status_lte;
	uint8_t rssi;
	uint8_t rsrp;
	uint8_t rsrq;
	uint8_t imei[NRF91_SLM_IMEI_LEN];
	uint8_t model_id[NRF91_SLM_MODEL_ID_LEN];
	uint8_t imsi[NRF91_SLM_IMSI_LEN];
	uint8_t iccid[NRF91_SLM_ICCID_LEN];
	uint8_t manufacturer[NRF91_SLM_MANUFACTURER_LEN];
	uint8_t fw_version[NRF91_SLM_FW_VERSION_LEN];

	enum nrf91_slm_state state;
	const struct device *dev;
	struct k_work_delayable timeout_work;

	/* Power management */
	struct k_sem suspended_sem;

	/* Event dispatcher */
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct ring_buf event_rb;
	struct k_mutex event_rb_lock;

	/* Network interface */
	struct net_if *netif;
	uint8_t mac_addr[6];

	/* DNS */
	struct zsock_addrinfo dns_result;
	struct sockaddr dns_result_addr;
	char dns_result_canonname[DNS_MAX_NAME_SIZE + 1];

	/* Poll */
	struct zsock_pollfd *poll_fds;
	int poll_nfds;
	int poll_count;

	/* Context for the offloaded socket API. */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[1];
};

int nrf91_slm_socket(struct nrf91_slm_data *data, int family, int type, int proto);

int nrf91_slm_connect(struct nrf91_slm_data *data, void *obj, const struct sockaddr *addr,
		      socklen_t addrlen);

ssize_t nrf91_slm_recvfrom(struct nrf91_slm_data *data, void *obj, void *buf, size_t max_len,
			   int flags, struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t nrf91_slm_sendto(struct nrf91_slm_data *data, void *obj, const void *buf, size_t len,
			 int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

int nrf91_slm_close(struct nrf91_slm_data *data, void *obj);

int nrf91_slm_poll(struct nrf91_slm_data *data, struct zsock_pollfd *fds, int nfds, int timeout_ms);

int nrf91_slm_getaddrinfo(struct nrf91_slm_data *data, const char *node, const char *service,
			  const struct zsock_addrinfo *hints, struct zsock_addrinfo **res);

void nrf91_slm_freeaddrinfo(struct nrf91_slm_data *data, struct zsock_addrinfo *res);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_NORDIC_NRF91_SLM_H_ */
