/*
 * Copyright (C) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SIMCOM_SIM7080_H
#define SIMCOM_SIM7080_H

#include <zephyr/kernel.h>
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/modem/simcom-sim7080.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/socket_offload.h>

#include "modem_context.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"
#include "modem_socket.h"

#define MDM_UART_NODE DT_INST_BUS(0)
#define MDM_UART_DEV DEVICE_DT_GET(MDM_UART_NODE)
#define MDM_MAX_DATA_LENGTH 1024
#define MDM_RECV_BUF_SIZE 1024
#define MDM_MAX_SOCKETS 5
#define MDM_BASE_SOCKET_NUM 0
#define MDM_RECV_MAX_BUF 30
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)
#define MDM_CMD_TIMEOUT K_SECONDS(10)
#define MDM_REGISTRATION_TIMEOUT K_SECONDS(180)
#define MDM_CONNECT_TIMEOUT K_SECONDS(90)
#define MDM_PDP_TIMEOUT K_SECONDS(120)
#define MDM_DNS_TIMEOUT K_SECONDS(210)
#define MDM_WAIT_FOR_RSSI_DELAY K_SECONDS(2)
#define MDM_WAIT_FOR_RSSI_COUNT 30
#define MDM_MAX_AUTOBAUD 5
#define MDM_MAX_CEREG_WAITS 40
#define MDM_MAX_CGATT_WAITS 40
#define MDM_BOOT_TRIES 2
#define MDM_GNSS_PARSER_MAX_LEN 128
#define MDM_APN CONFIG_MODEM_SIMCOM_SIM7080_APN
#define MDM_LTE_BANDS CONFIG_MODEM_SIMCOM_SIM7080_LTE_BANDS
#define RSSI_TIMEOUT_SECS 30

/*
 * Default length of modem data.
 */
#define MDM_MANUFACTURER_LENGTH 12
#define MDM_MODEL_LENGTH 16
#define MDM_REVISION_LENGTH 64
#define MDM_IMEI_LENGTH 16
#define MDM_IMSI_LENGTH 16
#define MDM_ICCID_LENGTH 32

/* Possible states of the ftp connection. */
enum sim7080_ftp_connection_state {
	/* Not connected yet. */
	SIM7080_FTP_CONNECTION_STATE_INITIAL = 0,
	/* Connected and still data available. */
	SIM7080_FTP_CONNECTION_STATE_CONNECTED,
	/* All data transferred. */
	SIM7080_FTP_CONNECTION_STATE_FINISHED,
	/* Something went wrong. */
	SIM7080_FTP_CONNECTION_STATE_ERROR,
};

enum sim7080_status_flags {
	SIM7080_STATUS_FLAG_POWER_ON = 0x01,
	SIM7080_STATUS_FLAG_CPIN_READY = 0x02,
	SIM7080_STATUS_FLAG_ATTACHED = 0x04,
	SIM7080_STATUS_FLAG_PDP_ACTIVE = 0x08,
};

/*
 * Driver data.
 */
struct sim7080_data {
	/*
	 * Network interface of the sim module.
	 */
	struct net_if *netif;
	uint8_t mac_addr[6];
	/*
	 * Uart interface of the modem.
	 */
	struct modem_iface_uart_data iface_data;
	uint8_t iface_rb_buf[MDM_MAX_DATA_LENGTH];
	/*
	 * Modem command handler.
	 */
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];
	/*
	 * Modem socket data.
	 */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	/*
	 * Current state of the modem.
	 */
	enum sim7080_state state;
	/*
	 * RSSI work
	 */
	struct k_work_delayable rssi_query_work;
	/*
	 * Information over the modem.
	 */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char mdm_imsi[MDM_IMSI_LENGTH];
	char mdm_iccid[MDM_ICCID_LENGTH];
#endif /* #if defined(CONFIG_MODEM_SIM_NUMBERS) */
	int mdm_rssi;
	/*
	 * Current operating socket and statistics.
	 */
	int current_sock_fd;
	int current_sock_written;
	size_t tx_space_avail;
	uint8_t socket_open_rc;
	/*
	 * Network registration of the modem.
	 */
	uint8_t mdm_registration;
	/* Modem status flags */
	uint32_t status_flags;
	/* SMS buffer structure provided by read. */
	struct sim7080_sms_buffer *sms_buffer;
	/* Position in the sms buffer. */
	uint8_t sms_buffer_pos;
	/* Status of the last http operation */
	uint16_t http_status;
	/* DNS related variables */
	struct {
		/* Number of DNS retries */
		uint8_t recount;
		/* Timeout in milliseconds */
		uint16_t timeout;
	} dns;
	/* Ftp related variables. */
	struct {
		/* User buffer for ftp data. */
		char *read_buffer;
		/* Length of the read buffer/number of bytes read. */
		size_t nread;
		/* State of the ftp connection. */
		enum sim7080_ftp_connection_state state;
	} ftp;
	/*
	 * Semaphore(s).
	 */
	struct k_sem sem_response;
	struct k_sem sem_tx_ready;
	struct k_sem sem_dns;
	struct k_sem sem_ftp;
	struct k_sem sem_http;
	struct k_sem boot_sem;
	struct k_sem pdp_sem;
};

/*
 * Socket read callback data.
 */
struct socket_read_data {
	char *recv_buf;
	size_t recv_buf_len;
	struct sockaddr *recv_addr;
	uint16_t recv_read_len;
};

/*
 * Driver internals
 */
extern struct sim7080_data mdata;
extern struct modem_context mctx;
extern const struct socket_op_vtable offload_socket_fd_op_vtable;
extern const struct socket_dns_offload offload_dns_ops;
extern struct k_work_q modem_workq;

extern void sim7080_rssi_query_work(struct k_work *work);

enum sim7080_state sim7080_get_state(void);

void sim7080_change_state(enum sim7080_state state);

int sim7080_pdp_activate(void);
int sim7080_pdp_deactivate(void);

int sim7080_offload_socket(int family, int type, int proto);

void sim7080_handle_sock_data_indication(int fd);

void sim7080_handle_sock_state(int fd, uint8_t state);

int sim7080_utils_parse_time(uint8_t *date, uint8_t *time_str, struct tm *t);

#endif /* SIMCOM_SIM7080_H */
