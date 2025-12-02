/* Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STM87MXX_H
#define STM87MXX_H

#include <stdint.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_context.h>

#include "modem_context.h"
#include "modem_socket.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

#define MDM_CMD_TIMEOUT				K_SECONDS(10)
#define MDM_DNS_TIMEOUT				K_SECONDS(210)
#define MDM_REGISTRATION_TIMEOUT		K_SECONDS(180)
#define MDM_AT_CMD_WAKEUP_TIMEOUT		5000
#define MDM_RECV_MAX_BUF			30
#define MDM_RECV_BUF_SIZE			1024
#define MDM_WAIT_FOR_DATA_RETRIES		3
#define MDM_MAX_CEREG_WAITS			40
#define MDM_CONNECT_TIMEOUT			K_SECONDS(90)
#define MDM_SENDMSG_SLEEP			K_MSEC(1)

#define SOCKET_SEND_TIMEOUT			10
#define SOCKET_RECEIVE_TIMEOUT			10
#define SOCKET_FRAME_RECEIVED_URC		1

#define BUF_ALLOC_TIMEOUT			K_SECONDS(1)

#define MDM_MAX_SOCKETS				3
#define NO_TAG_CMD_MAX_LENGTH			32

/*
 * Default length of modem data.
 */
#define MDM_MANUFACTURER_LENGTH			18
#define MDM_MODEL_LENGTH			16
#define MDM_REVISION_LENGTH			64
#define MDM_IMEI_LENGTH				16
#define MDM_IMSI_LENGTH				16
#define MDM_ICCID_LENGTH			32
#define MDM_RSSI_LENGTH				32

#define MDM_UART_NODE				DT_INST_BUS(0)
#define MDM_UART_DEV				DEVICE_DT_GET(MDM_UART_NODE)

#define MDM_MAX_DATA_LENGTH			1024

#define ST87MXX_COLD_CONFIG_VERSION		0

#define ST87MXX_COLD_VERSION_INDEX		8
#define ST87MXX_COLD_VERSION_NVM_PAGE		5
#define ST87MXX_COLD_VERSION_NVM_OFFSET		12


/*******ST87MXX NVM config parameters: user modifiable *******/
/* Temperature limit */
/*
 * Signed integer type Temperature low threshold
 * in Celsius degree for shutdown display and shutdown
 */
#define TEMP_LOW_SHUTDOWN			-45
/*
 * Signed integer type Temperature high threshold
 * in Celsius degree for shutdown display and shutdown
 */
#define TEMP_HIGH_SHUTDONW			110
/*
 * Integer type 0: Disable shutdown if the shutdown threshold is reached
 * 1: Enable shutdown if the shutdown threshold is reached
 */
#define TEMP_SHUTDOWN				1

/* Battery level settings */
/* Integer type Low battery threshold in mV for shutdown display and shutdown */
#define VBAT_LOW_SHUTDOWN			2000
/* Integer type High battery threshold in mV for shutdown display and shutdown */
#define VBAT_HIGH_SHUTDOWN			3200
/*
 * Integer type 0: Disable shutdown if the shutdown threshold is reached
 * 1: Enable shutdown if the shutdown threshold is reached
 */
#define VBAT_SHUTDOWN				1

/****************************** NBIOT configuration ****************************/
/* Band selection : configure the band usage and split between various NMO */
#define BANDLIST	"20,8"			/* Band selected comma separated */
#define BANDCFG		"0,0,20,01,7910"	/* Band=20, Option=01, StartFreq=7910 */
#define BANDCFG_NMO1	"0,1,0,2,1,100,0"	/* Pref=0, Guard=2, In=1, BW=100, OffsetFreq=0 */
#define BANDCFG_NMO2	"0,2,0,2,1,100,100"	/* Pref=0, Guard=2, In=1, BW=100, OffsetFreq=100 */
#define BANDCFG_NMO3	"0,3,0,2,1,100,200"	/* Pref=0, Guard=2, In=1, BW=100, OffsetFreq=200 */

/* EDRX Setting */
/* Requested EDRX value "1011" -> 655.36s See 3GPP 24.008 Table 10.5.5.32 */
#define EDRX_VALUE		1011

/* Paging Time Window*/
/* PTW value "0011" -> 10.24s See 3GPP TS 24.008 Table 10.5.5.32 */
#define PTW_VALUE		0011

/* Power saving mode setting */
#define PSM_ENABLE		1
/*
 * TAU value (T3412) "00100001" -> 1H See 3GPP TS 24.008 Table 10.5.5.32
 * TAU value (T3412) "00111000" -> 24H See 3GPP TS 24.008 Table 10.5.5.32
 */
#define PERIODIC_TAU			"00100001"
/* Active time (T3324) "00000101" */
#define ACTIVE_TIME			"00000101"

/* IP configuration */
/*
 * Activation of the counting of the number of UDP packets actually received
 * and acknowledged by the eNodeB.
 * However, it does not guarantee that the packet has been received by the remote server.
 * If NB_PACKET_SENT is 1, counting is active.
 * If NB_PACKET_SENT is 0, counting is inactive.
 * NbUdpPacketsSent counter var in EC Lib State structure (ST87EC_Lib_Status_t)
 * keeps 0 value.
 */
#define NB_PACKET_SENT_ENABLE		1
#define DOMAIN_NAME			"8.8.8.8"	/* IP address for DNS resolution */

/****************************** ST87 configuration ****************************/
/* Sleep mode configuration */
/* Integer type Time in seconds between the last AT command and the sleep mode entry */
#define HOLD_TIME			10
/*
 * Integer type Define the timeout in seconds that the module is awake at each wake up
 * (telecom activity, AT command activity..)
 */
#define AWAKE_TIME			0

/*Parameters to be configured for Ring pin setup*/
/* Ring pin enable */
#define RING_PIN_ENABLE			0
/* Ring pin number to be set for ST87M01. The GPIO number between 8 and 31 */
#define RING_PIN_GPIO			10
/* Ring pin voltage polarity (0: active low and 1: active high) */
#define RING_PIN_POLARITY		1
/* Time in ms when ring pin is active (min: 10ms and max:300ms by 10ms steps) */
#define RING_PIN_DELAY			200

/* Types ---------------------------------------------------------------------*/

struct st87mxx_data {
	/*
	 * Network interface of the sim module.
	 */
	struct net_if *netif;
	uint8_t mac_addr[6];

	uint8_t context_id;
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

	int current_sock_written;

	struct mdm_receiver_context *mctx;
	struct gpio_dt_spec *reset_gpio;
	struct gpio_dt_spec *ring_gpio;

	uint8_t cold_init_version;

	char mdm_imei[MDM_IMEI_LENGTH];
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH+1];
	char mdm_model[MDM_MODEL_LENGTH];
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char mdm_iccid[MDM_ICCID_LENGTH];
	char mdm_imsi[MDM_IMSI_LENGTH];
#endif
	char mdm_revision[MDM_REVISION_LENGTH];
	int mdm_rssi;
	uint8_t mdm_registration;

	/*
	 * Semaphore(s).
	 */
	struct k_sem sem_response;
	struct k_sem sem_dns;
	struct k_sem sem_nvm;
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

#endif
