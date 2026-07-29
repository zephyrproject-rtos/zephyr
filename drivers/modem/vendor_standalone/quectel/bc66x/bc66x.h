/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

#ifndef BC66X_H_
#define BC66X_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/gpio.h>

#include <errno.h>
#include <string.h>

#define BC66X_INIT_DELAY        K_MSEC(CONFIG_BC66X_INIT_DELAY)
#define BC66X_WAIT_FOR_HW_DELAY K_MSEC(CONFIG_BC66X_WAIT_FOR_HW_DELAY)
#define BC66X_INIT_TIMEOUT      K_SECONDS(CONFIG_BC66X_INIT_TIMEOUT)

#define BC66X_WAIT_FOR_CONNECTION_RETRIES 15
#define BC66X_WAIT_FOR_CONNECTION_DELAY   K_SECONDS(7)

#define BC66X_CHAT_SCRIPTS_TIMEOUT_S       15
#define BC66X_CHAT_SCRIPTS_SHORT_TIMEOUT_S 3

/* After entering light sleep mode */
#define BC66X_PM_DEEP_SLEEP_MODE_DELAY K_SECONDS(30)
/* After running a script, wait .. before running "put" */
#define BC66X_PM_SUSPEND_GRACE_PERIOD  K_SECONDS(60)

#define BC66X_PM_WAKE_UP_COMMAND_TIMEOUT_MS 300
#define BC66X_PM_WAKE_UP_MAX_RETRIES        10

#define BC66X_CMD_TIMEOUT_MS   5000
#define BC66X_SCRIPT_TIMEOUT_S 7

#define BC66X_FORCE_RESTART_AFTER_APN_SET false

#define BC66X_SMALL_CMD_BUFF_SIZE  64
#define BC66X_MEDIUM_CMD_BUFF_SIZE 256

#define BC66X_MAX_OPERATOR_LENGTH 16

/* Fixed by hw specs */
#define BC66X_MAX_SOCKETS                5
#define BC66X_MANUFACTURER_LENGTH        16
#define BC66X_MODEL_LENGTH               20
#define BC66X_REVISION_LENGTH            64
#define BC66X_IMEI_LENGTH                16
#define BC66X_IMSI_LENGTH                16
#define BC66X_IMEISV_LENGTH              32
#define BC66X_ICCID_LENGTH               32
#define BC66X_IP_ADDR_LENGTH             16
#define BC66X_OPERATOR_LENGTH            16
#define BC66X_SVN_LENGTH                 32
#define BC66X_SERIAL_LENGTH              32
#define BC66X_MTU                        1024
#define BC66X_MAX_HEX_SEND_LENGTH        512
#define BC66X_MAX_APN_LENGTH             99
#define BC66X_SOCKET_RECV_SLAB_SIZE      1024
#define BC66X_SOCKET_RECV_MAX_SIZE       512
#define BC66X_SOCKET_OPEN_TIMEOUT        K_MSEC(60500) /* specs recommends waiting up to 60 s */
#define BC66X_SOCKET_CLOSE_TIMEOUT_MS    5500          /* from specs 5s */
#define BC66X_SOCKET_SERVICE_TYPE_TCP    "TCP"
#define BC66X_SOCKET_SERVICE_TYPE_UDP    "UDP"
#define BC66_ONLY_SUPPORTED_CONTEXT_ID   1
#define BC660K_ONLY_SUPPORTED_CONTEXT_ID 0
#define BC66X_RSRQ_UNKNOWN_VALUE         255 /* "Not known or not detectable" */
#define BC66X_RSRP_UNKNOWN_VALUE         255 /* "Not known or not detectable" */
#define BC66X_RSSI_UNKNOWN_VALUE         99  /* "Not known or not detectable" */

/* Used to convert to dBm */
#define BC66X_RSSI_OFFSET -113
#define BC66X_RSSI_SCALE  2
#define BC66X_RSRQ_SCALE  5
#define BC66X_RSRQ_OFFSET -200
#define BC66X_RSRP_OFFSET -141

#define BC66X_UNKNOWN_VALUE -999

/* <stat> of +CEREG response */
#define BC66X_EPS_REG_STATUS_NOT_REGISTERED           0
#define BC66X_EPS_REG_STATUS_REGISTERED               1
#define BC66X_EPS_REG_STATUS_NOT_REGISTERED_SEARCHING 2
#define BC66X_EPS_REG_STATUS_REGISTRATION_DENIED      3
#define BC66X_EPS_REG_STATUS_UNKNOWN                  4
#define BC66X_EPS_REG_STATUS_REGISTERED_ROAMING       5

/* <AcT> of +CEREG response */
#define BC66X_ACCESS_TECHNOLOGY_E_UTRAN       7
#define BC66X_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1 9

#include "bc66x_net.h"

struct bc66x_operator {
	char name[BC66X_MAX_OPERATOR_LENGTH];
	char apn[BC66X_MAX_APN_LENGTH];
};

struct bc66x_config {
	const struct gpio_dt_spec boot_gpio;
	const struct gpio_dt_spec rst_gpio;
	const struct gpio_dt_spec psm_gpio;
	const struct device *uart_dev;
	k_thread_stack_t *workq_stack;
	size_t workq_stack_size;
	int workq_priority;
	const bool is_offload_node;
	const bool is_bc66;
	const uint8_t supported_ctx_id;
	const char *send_cmd;
	const struct modem_chat_script *setup_script;
	const struct modem_chat_script *enable_light_sleep_script;
	const struct modem_chat_script *enable_deep_sleep_script;
};

/* Finite state machine possible states */
typedef enum {
	BC66X_NULL_STATE = 0,
	BC66X_INIT_STATE,
	BC66X_CONNECTING_STATE,
	BC66X_READY_STATE,
	BC66X_ERROR_STATE,
	BC66X_SLEEPING_STATE,
	BC66X_AWAKENING_STATE,
} bc66x_modem_state;

struct bc66x_connection_info {
	/* SIM INFO */
	char imsi[BC66X_IMSI_LENGTH];   /* IMSI */
	char iccid[BC66X_ICCID_LENGTH]; /* ICCID */

	/* CONNECTION INFO */
	char ip_addr[BC66X_IP_ADDR_LENGTH]; /* IP Address */
	struct bc66x_operator operator;     /* Operator */
	int16_t rsrp;                       /* RSRP (dBm) */
	int16_t rsrq_x10;                   /* RSRQ (dB) scaled x10 (195 --> 19.5 dB) */
	int16_t rssi;                       /* RSSI (dBm) */
	enum cellular_access_technology access_technology;     /* <AcT> of +COPS or +CEREG */
	enum cellular_registration_status registration_status; /* <stat> of +CEREG */

	/* TODO: implement these (not used yet) */
	uint32_t earfcn;       /* EARFCN - carrier frequency (0-262143) */
	uint8_t earfcn_offset; /* EARFCN offset */
	char cell_id[9];       /* Cell ID 4 bytes (28 bit) in hexadecimal */
	uint16_t pci;          /* Physical Cell ID (0-503) */
	int8_t sinr;           /* SINR (dB) */
	uint8_t band;          /* Current band */
	char tac[5];           /* TAC - Tracking Area Code in hexadecimal */
	uint8_t ecl;           /* ECL (Enhanced Coverage Level) */
	int16_t tx_power;      /* TX power (dBm, 128 = invalid value) */

	/* This should wake up the modem (for BC660K only), no response
	 * expected. After this, wait timeout before the next command(s)
	 */
	uint8_t operation_mode;
	uint8_t neighbor_cells; /* Number of neighbor cells (0-6) */
};

struct bc66x_data {
	const struct device *dev;

	struct modem_pipe *pipe;
	struct modem_backend_uart uart_backend;
	struct modem_chat chat;

	/* To prevent concurrent initialisation. Should not be needed... */
	struct k_sem init_lock;
	int init_retries;

	/* Note: Work items in the same queue are executed sequentially. */
	/* Shared workq for: init, enter deep sleep, set apn */
	struct k_work_q bc66x_workq;

	struct k_work_delayable init_work;

	struct k_sem script_lock;

	atomic_t is_initialized;   /* bool */
	atomic_t is_backend_ready; /* bool */

	uint8_t uart_rx_buf[CONFIG_MODEM_QUECTEL_BC66X_UART_RX_BUF_SIZE];
	uint8_t uart_tx_buf[CONFIG_MODEM_QUECTEL_BC66X_UART_TX_BUF_SIZE];

	uint8_t chat_recv_buf[CONFIG_MODEM_QUECTEL_BC66X_CHAT_BUF_SIZE];
	uint8_t *chat_argv[8];

	atomic_t modem_state; /* bc66x_modem_state */

	/* Not protected for now */
	char manufacturer[BC66X_MANUFACTURER_LENGTH];
	char model[BC66X_MODEL_LENGTH];
	char revision[BC66X_REVISION_LENGTH];
	char serial[BC66X_SERIAL_LENGTH];
	char imei[BC66X_IMEI_LENGTH];     /* IMEI */
	char imeisv[BC66X_IMEISV_LENGTH]; /* IMEISV */
	char svn[BC66X_SVN_LENGTH];       /* Software Version Number */
	struct bc66x_connection_info connection_info;

	struct bc66x_network_data net;

	/* PM */
	atomic_t is_in_deep_sleep;
	struct k_work_delayable deep_sleep_work;

	/* APN */
	atomic_t is_setting_apn;
	struct k_work_delayable set_apn_work;
	char new_apn_str[BC66X_MAX_APN_LENGTH];
};

/* Used by _net to send socket commands */
int bc66x_run_cmd(struct bc66x_data *data, const char *cmd, uint32_t script_timeout_s);
int bc66x_run_cmd_matches(struct bc66x_data *data, const char *cmd, uint32_t script_timeout_s,
			  const struct modem_chat_match *response_matches,
			  uint16_t response_matches_size,
			  const struct modem_chat_match *s_abort_matches,
			  uint16_t abort_matches_size);

#endif /* BC66X_H_ */
