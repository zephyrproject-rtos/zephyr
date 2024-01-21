/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_MODEM_MURATA_1SC_H_
#define ZEPHYR_DRIVERS_MODEM_MURATA_1SC_H_
#include <zephyr/kernel.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/socket_offload.h>
#include "modem_context.h"
#include "modem_socket.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

#include <zephyr/drivers/modem/murata-1sc.h>

#define GSM_MODEM_DEVICE_NAME "murata 1sc"

#define MDM_UART_DEV_NAME		  DT_INST_BUS_LABEL(0)
#define MDM_UART_NODE			  DT_INST_BUS(0)
#define MDM_UART_DEV			  DEVICE_DT_GET(MDM_UART_NODE)
#define MDM_CMD_TIMEOUT			  K_SECONDS(5)
#define MDM_REGISTRATION_TIMEOUT	  K_SECONDS(10)
#define MDM_SENDMSG_SLEEP		  K_MSEC(1)
#define MDM_MAX_DATA_LENGTH		  1500
#define MDM_RECV_MAX_BUF		  20
#define MDM_RECV_BUF_SIZE		  1500
#define MDM_MAX_SOCKETS			  CONFIG_MODEM_MURATA_1SC_SOCKET_COUNT
#define MDM_BASE_SOCKET_NUM		  1
#define MDM_NETWORK_RETRY_COUNT		  10
#define MDM_INIT_RETRY_COUNT		  10
#define MDM_PDP_ACT_RETRY_COUNT		  3
#define MDM_WAIT_FOR_RSSI_COUNT		  10
#define MDM_WAIT_FOR_RSSI_DELAY		  K_SECONDS(2)
#define BUF_ALLOC_TIMEOUT		  K_SECONDS(1)
#define MDM_MAX_BOOT_TIME		  K_SECONDS(50)

/* Default lengths of certain things. */
#define MDM_MANUFACTURER_LENGTH		  40
#define MDM_MODEL_LENGTH		  16
#define MDM_REVISION_LENGTH		  32
#define MDM_SIM_INFO_LENGTH		  64
#define MDM_APN_LENGTH			  64
#define RSSI_TIMEOUT_SECS		  30
#define MDM_IP_LENGTH                     16
#define MDM_IP6_LENGTH                    16
#define MDM_GW_LENGTH                     16
#define MDM_MASK_LENGTH                   16
#define MDM_CARRIER_LENGTH                16
#define CHKSUM_ABILITY_MAX_LEN            64
#define CMD_FULL_ACCESS_MAX_LEN           64
#define MAX_CARRIER_RESP_SIZE	          64
#define MAX_SIGSTR_RESP_SIZE              32
#define MDM_EDRX_LENGTH                   128
#define MDM_PSM_LENGTH                    128
#define PSM_TIME_LEN                      11
#define MAX_CMD_BUF_SIZE                  256
#define DELIVER_PDU_MAX_SIZE              327
#define MDM_MAX_RST_TRIES                 5
#define MAX_FILENAME_LEN                  32
#define MDM_BOOT_DELAY                    6	   /* seconds */
#define MDM_WAKE_DELAY                    5000 /* milliseconds */
#define MDM_CMD_RSP_TIME                  K_SECONDS(2)
#define MDM_CMD_LONG_RSP_TIME             K_SECONDS(6)
#define FW_HEADER_SIZE                    256

#define MAX_BANDS_STR_SZ                  64
#define MAX_AUTOCONN_STR_SZ               16
#define MAX_SLEEP_MODE_STR_SZ             16
#define MAX_PM_MODE_STR_SZ                16
#define MAX_HIFC_MODE_STR_SZ              16
#define PDN_QUERY_RESPONSE_LEN            256
#define PDN_BUF_SZ                        256
#define MAX_PDNSET_STR_SZ                 128
#define IP_STR_LEN                        100
#define DNS_QUERY_RESPONSE_LEN            128
#define MIN_SS                            -113 /* dBm */
#define NO_SIG_RAW                        99
#define NO_SIG_RET                        -120

#if DT_INST_NODE_HAS_PROP(0, mdm_wake_mdm_gpios)
#define HIFC_AVAILABLE
#if DT_INST_NODE_HAS_PROP(0, mdm_wake_host_gpios)
#if DT_INST_NODE_HAS_PROP(0, mdm_rx_gpios) && DT_INST_NODE_HAS_PROP(0, mdm_rx_gpios)
#define MAX_HIFC_SUPPORTED 'A'
#define MAX_PM_MODE "dh0"
#else
#define MAX_PM_MODE "dh2"
#define MAX_HIFC_SUPPORTED 'B'
#endif
#else
#define MAX_PM_MODE "dh2"
#define MAX_HIFC_SUPPORTED 'C'
#endif
#endif

#define ATOI(s_, value_, desc_) murata_1sc_atoi(s_, value_, desc_, __func__)

/* pin settings */
enum mdm_control_pins {
	MDM_WAKE_HOST = 0,
	MDM_WAKE_MDM,
	MDM_RESET,
};


/* driver data */
struct murata_1sc_data {
	struct net_if *net_iface;
	uint8_t mac_addr[6];

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	uint8_t iface_rb_buf[MDM_MAX_DATA_LENGTH];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];

	/* RSSI work */
	struct k_work_delayable rssi_query_work;

	/* modem data */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_sim_info[MDM_SIM_INFO_LENGTH];
	char mdm_imei[MDM_1SC_IMEI_LENGTH];
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char mdm_imsi[MDM_1SC_IMSI_LENGTH];
	char mdm_iccid[MDM_1SC_ICCID_LENGTH];
#endif /* #if defined(CONFIG_MODEM_SIM_NUMBERS) */
	char mdm_ip[MDM_IP_LENGTH];
	char mdm_ip6[MDM_IP6_LENGTH];
	char mdm_gw[MDM_GW_LENGTH];
	char mdm_nmask[MDM_MASK_LENGTH];
	char mdm_phn[MDM_1SC_PHN_LENGTH];
	char mdm_carrier[MDM_CARRIER_LENGTH];
	char mdm_apn[MDM_APN_LENGTH];
	char mdm_psm[MDM_PSM_LENGTH];
	char mdm_edrx[MDM_EDRX_LENGTH];

	int mdm_rssi;


	/* Socket from which we are currently reading data. */
	int sock_fd;

	/* This buffer is shared by all sockets for rx and tx
	 * Therefore it must be semaphore protected.
	 *
	 * The size is 2x the max data length since binary data
	 * is being translated into byte-wise hex representation,
	 * plus extra for the SOCKETDATA command and params
	 */
	char xlate_buf[MDM_MAX_DATA_LENGTH * 2 + 64];

	/* Semaphores */
	struct k_sem sem_response;
	struct k_sem sem_sock_conn;
	struct k_sem sem_xlate_buf;
};

struct murata_1sc_config {
	/* Modem Pins */
	struct gpio_dt_spec wake_host_gpio;
	struct gpio_dt_spec wake_mdm_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec rst_done_gpio;
	struct gpio_dt_spec mdm_rx_gpio;
	struct gpio_dt_spec mdm_tx_gpio;
};

/* Socket read callback data */
struct socket_read_data {
	char		 *recv_buf;
	size_t		 recv_buf_len;
	struct sockaddr	 *recv_addr;
	uint16_t	 recv_read_len;
};

const struct mdmdata_cmd_t cmd_pool[] = {{"APN", apn_e},
					 {"ASLEEP", awake_e},
					 {"AWAKE", awake_e},
					 {"CONN_STS", connsts_e},
					 {"CONN", connsts_e},
					 {"EDRX", edrx_e},
					 {"GOLD", golden_e},
					 {"GOLDEN", golden_e},
					 {"ICCID", iccid_e},
					 {"IMEI", imei_e},
					 {"IMSI", imsi_e},
					 {"IP", ip_e},
					 {"IP6", ip6_e},
					 {"MSISDN", msisdn_e},
					 {"PSM", psm_e},
					 {"SLEEP", sleep_e},
					 {"SSI", ssi_e},
					 {"STAT", connsts_e},
					 {"SIM", sim_info_e},
					 {"VERSION", version_e},
					 {"VER", version_e},
					 {"WAKE", wake_e},
					 {}};

struct mdm_dns_resp_t {
	struct sockaddr_in ipv4;
	struct sockaddr_in6 ipv6;
};

struct aggr_ipv4_addr {
	struct in_addr ip;
	struct in_addr gw;
	struct in_addr nmask;
};

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)

#define CERTCMD_WRITE_SIZE 32 + MAX_FILENAME_LEN /* assume filename maxlen = 32 */
#define PEM_BUFF_SIZE	   3002			 /* terminate with \" & 0 */

/**
 * following struct may not have packed memory if it has something like
 * int follow by char then int,
 * since array of uchar plus array of uchar will be packed
 */
struct cert_cmd_t {
	uint8_t cert_cmd_write[CERTCMD_WRITE_SIZE];
	uint8_t pem_buf[PEM_BUFF_SIZE];
};

#endif /* defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */

#ifdef HIFC_AVAILABLE
enum handshake_state {
	init_suspend,
	init_resume,
	isr_init_resume,
	suspend_wait_modem_to_host,
	suspend_wait_rx,
	resume_wait_modem_to_host,
	resume_wait_rx,
	isr_resume_wait_rx,
};
#endif /* HIFC_AVAILABLE */
#endif
