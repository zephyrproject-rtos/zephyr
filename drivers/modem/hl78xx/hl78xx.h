/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HL78XX_H
#define HL78XX_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>

#include "../modem_context.h"
#include "../modem_socket.h"
#include <stdint.h>
/* clang-format off */
#define MDM_CMD_TIMEOUT                      (40)  /*K_SECONDS*/
#define MDM_DNS_TIMEOUT                      (70)  /*K_SECONDS*/
#define MDM_CELL_BAND_SEARCH_TIMEOUT         (60)  /*K_SECONDS*/
#define MDM_CMD_CONN_TIMEOUT                 (120) /*K_SECONDS*/
#define MDM_REGISTRATION_TIMEOUT             (180) /*K_SECONDS*/
#define MDM_PROMPT_CMD_DELAY                 (50)  /*K_MSEC*/
#define MDM_RESET_LOW_TIME                   (1)   /*K_MSEC*/
#define MDM_RESET_HIGH_TIME                  (10)  /*K_MSEC*/
#define MDM_BOOT_TIME                        (12)  /*K_SECONDS*/
#define MDM_DNS_ADD_TIMEOUT                  (100) /*K_MSEC*/
#define MODEM_HL78XX_PERIODIC_SCRIPT_TIMEOUT K_MSEC(CONFIG_MODEM_HL78XX_PERIODIC_SCRIPT_MS)

#ifdef CONFIG_HL78XX_GNSS
/**
 * GNSS $PSGSA sentence comes with more arguments
 * So we need a larger argv buffer
 * see some discussion about this:
 * https://forum.sierrawireless.com/t/clarification-on-proprietary-nmea-sentence-hl7812-5-5-14-0/34478
 */
#define MDM_CHAT_ARGV_BUFFER_SIZE 40
#else
#define MDM_CHAT_ARGV_BUFFER_SIZE 32
#endif /* CONFIG_HL78XX_GNSS */
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN

#define NTN_POSITION_METHOD_IGSS         "IGSS"
#define NTN_POSITION_METHOD_MANUAL       "MANUAL"
#define NTN_POSITION_METHOD_TEXT_MAX_LEN sizeof(NTN_POSITION_METHOD_MANUAL)

#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
#define MDM_MAX_DATA_LENGTH CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES

#define MDM_MAX_SOCKETS           CONFIG_MODEM_HL78XX_NUM_SOCKETS
#define MDM_MAX_PDP_CONTEXTS      CONFIG_MODEM_HL78XX_MAX_PDP_CONTEXTS
#define MDM_BASE_SOCKET_NUM       1
#define MDM_BAND_BITMAP_LEN_BYTES 32
#define MDM_BAND_HEX_STR_LEN      (MDM_BAND_BITMAP_LEN_BYTES * 2 + 1)

#define MDM_KBND_BITMAP_MAX_ARRAY_SIZE 64

#define ADDRESS_FAMILY_IP         "IP"
#define ADDRESS_FAMILY_IP4        "IPV4"
#define ADDRESS_FAMILY_IPV6       "IPV6"
#define ADDRESS_FAMILY_IPV4V6     "IPV4V6"
#define MDM_HL78XX_SOCKET_AF_IPV4 0
#define MDM_HL78XX_SOCKET_AF_IPV6 1
#if defined(CONFIG_MODEM_HL78XX_ADDRESS_FAMILY_IP)
#define MODEM_HL78XX_ADDRESS_FAMILY ADDRESS_FAMILY_IP
#define MODEM_HL78XX_ADDRESS_FAMILY_FORMAT "###.###.###.###"
#define MODEM_HL78XX_ADDRESS_FAMILY_FORMAT_LEN sizeof(MODEM_HL78XX_ADDRESS_FAMILY_FORMAT)
#elif defined(CONFIG_MODEM_HL78XX_ADDRESS_FAMILY_IPV4V6)
#define MODEM_HL78XX_ADDRESS_FAMILY        ADDRESS_FAMILY_IPV4V6
#define MODEM_HL78XX_ADDRESS_FAMILY_FORMAT "####:####:####:####:####:####:####:####"
#define MODEM_HL78XX_ADDRESS_FAMILY_FORMAT_LEN                                                     \
	sizeof("a01.a02.a03.a04.a05.a06.a07.a08.a09.a10.a11.a12.a13.a14.a15.a16")
#elif defined(CONFIG_MODEM_HL78XX_ADDRESS_FAMILY_IPV4)
#define MODEM_HL78XX_ADDRESS_FAMILY            ADDRESS_FAMILY_IPV4
#define MODEM_HL78XX_ADDRESS_FAMILY_FORMAT     "###.###.###.###"
#define MODEM_HL78XX_ADDRESS_FAMILY_FORMAT_LEN sizeof(MODEM_HL78XX_ADDRESS_FAMILY_FORMAT)

#else
#define MODEM_HL78XX_ADDRESS_FAMILY ADDRESS_FAMILY_IPV6
#endif

/* Modem Communication Patterns */
#define EOF_PATTERN         "--EOF--Pattern--"
#define TERMINATION_PATTERN "+++"
#define CONNECT_STRING      "CONNECT"
#define CME_ERROR_STRING    "+CME ERROR: "
#define ERROR_STRING        "ERROR"
#define OK_STRING           "OK"

/* RAT (Radio Access Technology) commands */
#define SET_RAT_M1_CMD_LEGACY    "AT+KSRAT=0"
#define SET_RAT_NB1_CMD_LEGACY   "AT+KSRAT=1"
#define SET_RAT_GSM_CMD_LEGACY   "AT+KSRAT=2"
#define SET_RAT_NBNTN_CMD_LEGACY "AT+KSRAT=3"

#define KSRAT_QUERY      "AT+KSRAT?"
#define DISABLE_RAT_AUTO "AT+KSELACQ=0,0"

#define SET_RAT_M1_CMD    "AT+KSRAT=0,1"
#define SET_RAT_NB1_CMD   "AT+KSRAT=1,1"
#define SET_RAT_GSM_CMD   "AT+KSRAT=2,1"
#define SET_RAT_NBNTN_CMD "AT+KSRAT=3,1"

/* Enable/Disable RAT registration status */
#define ENABLE_LTE_REG_STATUS_CMD          "AT+CEREG=5"
#define ENABLE_GSM_REG_STATUS_CMD          "AT+CREG=3"
#define DISABLE_LTE_REG_STATUS_CMD         "AT+CEREG=0"
#define DISABLE_GSM_REG_STATUS_CMD         "AT+CREG=0"
/* Power mode commands */
#define SET_AIRPLANE_MODE_CMD_LEGACY       "AT+CFUN=4,0"
#define SET_AIRPLANE_MODE_CMD              "AT+CFUN=4,1"
#define SET_FULLFUNCTIONAL_MODE_CMD_LEGACY "AT+CFUN=1,0"
#define SET_FULLFUNCTIONAL_MODE_CMD        "AT+CFUN=1,1"
#define SET_SIM_PWR_OFF_MODE_CMD           "AT+CFUN=0"
#define GET_FULLFUNCTIONAL_MODE_CMD        "AT+CFUN?"
#define MDM_POWER_OFF_CMD_LEGACY           "AT+CPWROFF"
#define MDM_POWER_FAST_OFF_CMD_LEGACY      "AT+CPWROFF=1"
/* PDP Context commands */
#define DEACTIVATE_PDP_CONTEXT             "AT+CGACT=0"
#define ACTIVATE_PDP_CONTEXT               "AT+CGACT=1"
/* LTE coverage check command */
#define CHECK_LTE_COVERAGE_CMD             "AT+KCELLMEAS=1"
#define WAKE_LTE_LAYER_CMD                 "AT%PINGCMD=0,\"8.8.8.8\",1"

/**
 * Airvantage commands
 * User initiated connection start/stop
 */
#define WDSI_USER_INITIATED_CONNECTION_START_CMD "AT+WDSS=1,1"
#define WDSI_USER_INITIATED_CONNECTION_STOP_CMD  "AT+WDSS=1,0"
/* Baud rate commands */
#define GET_BAUDRATE_CMD                         "AT+IPR?"
#define SET_BAUDRATE_CMD_FMT                     "AT+IPR=%d"

/* Helper macros */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)
#define ATOD(s_, value_, desc_) modem_atod(s_, value_, desc_, __func__)
/* Default dynamic AT command maximum length */
#define HL78XX_AT_CMD_MAX_LEN       64

#define HL78XX_LOG_DBG(str, ...)                                                                   \
	COND_CODE_1(CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG, \
		    (LOG_DBG(str, ##__VA_ARGS__)), \
		    ((void)0))

/* clang-format on */

/* HL78XX States */
enum hl78xx_state {
	MODEM_HL78XX_STATE_IDLE = 0,
	MODEM_HL78XX_STATE_RESET_PULSE,
	MODEM_HL78XX_STATE_POWER_ON_PULSE,
	MODEM_HL78XX_STATE_AWAIT_POWER_ON,
	MODEM_HL78XX_STATE_SET_BAUDRATE,
	MODEM_HL78XX_STATE_RUN_INIT_SCRIPT,
	MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT,
	MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT,
	MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT,
	MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT,
	/* Full functionality, searching
	 * CFUN=1
	 */
	MODEM_HL78XX_STATE_AWAIT_REGISTERED,
	MODEM_HL78XX_STATE_CARRIER_ON,
	MODEM_HL78XX_STATE_FOTA,
	/* Minimum functionality, SIM powered off, Modem Power down
	 * CFUN=0
	 */
	MODEM_HL78XX_STATE_CARRIER_OFF,
	MODEM_HL78XX_STATE_SIM_POWER_OFF,
	/* Minimum functionality / Airplane mode
	 * Sim still powered on
	 * CFUN=4
	 */
	MODEM_HL78XX_STATE_AIRPLANE,
	MODEM_HL78XX_STATE_SLEEP,
#ifdef CONFIG_HL78XX_GNSS
	MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT,
	MODEM_HL78XX_STATE_GNSS_SEARCH_STARTED,
#endif /* CONFIG_HL78XX_GNSS */
	MODEM_HL78XX_STATE_INIT_POWER_OFF,
	MODEM_HL78XX_STATE_POWER_OFF_PULSE,
	MODEM_HL78XX_STATE_AWAIT_POWER_OFF,
	MODEM_HL78XX_STATE_COUNT
};

enum hl78xx_event {
	MODEM_HL78XX_EVENT_RESUME = 0,
	MODEM_HL78XX_EVENT_SUSPEND,
	MODEM_HL78XX_EVENT_SCRIPT_SUCCESS,
	MODEM_HL78XX_EVENT_SCRIPT_FAILED,
	MODEM_HL78XX_EVENT_SCRIPT_REQUIRE_RESTART,
	MODEM_HL78XX_EVENT_TIMEOUT,
	MODEM_HL78XX_EVENT_REGISTERED,
	MODEM_HL78XX_EVENT_DEREGISTERED,
	MODEM_HL78XX_EVENT_BUS_OPENED,
	MODEM_HL78XX_EVENT_BUS_CLOSED,
	/* Modem unexpected restart event */
	MODEM_HL78XX_EVENT_MDM_RESTART,
	MODEM_HL78XX_EVENT_SOCKET_READY,
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
	MODEM_HL78XX_EVENT_NTN_POSREQ,
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
	MODEM_HL78XX_EVENT_PHONE_FUNCTIONALITY_CHANGED,
#ifdef CONFIG_HL78XX_GNSS
	MODEM_HL78XX_EVENT_GNSS_START_REQUESTED,
	MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED,
	MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED_FAILED,
	MODEM_HL78XX_EVENT_GNSS_FIX_ACQUIRED,
	MODEM_HL78XX_EVENT_GNSS_FIX_LOST,
	MODEM_HL78XX_EVENT_GNSS_STOP_REQUESTED,
	MODEM_HL78XX_EVENT_GNSS_STOPPED,
	/* Explicit GNSS mode switching events */
	MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED,
	MODEM_HL78XX_EVENT_GNSS_MODE_EXIT_REQUESTED,
#endif /* CONFIG_HL78XX_GNSS */
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	MODEM_HL78XX_EVENT_DEVICE_ASLEEP,
	MODEM_HL78XX_EVENT_DEVICE_AWAKE,
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE

	/* WDSI FOTA events */
	MODEM_HL78XX_EVENT_WDSI_UPDATE,
	MODEM_HL78XX_EVENT_WDSI_RESTART,
	MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_REQUEST,
	MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_PROGRESS,
	MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_COMPLETE,
	MODEM_HL78XX_EVENT_WDSI_INSTALL_REQUEST,
	MODEM_HL78XX_EVENT_WDSI_INSTALLING_FIRMWARE,
	MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_SUCCEEDED,
	MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_FAILED,
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
	MODEM_HL78XX_EVENT_AT_CMD_TIMEOUT,
	MODEM_HL78XX_EVENT_COUNT
};

enum hl78xx_tcp_notif {
	TCP_NOTIF_NETWORK_ERROR = 0,
	TCP_NOTIF_NO_MORE_SOCKETS = 1,
	TCP_NOTIF_MEMORY_PROBLEM = 2,
	TCP_NOTIF_DNS_ERROR = 3,
	TCP_NOTIF_REMOTE_DISCONNECTION = 4,
	TCP_NOTIF_CONNECTION_ERROR = 5,
	TCP_NOTIF_GENERIC_ERROR = 6,
	TCP_NOTIF_ACCEPT_FAILED = 7,
	TCP_NOTIF_SEND_MISMATCH = 8,
	TCP_NOTIF_BAD_SESSION_ID = 9,
	TCP_NOTIF_SESSION_ALREADY_RUNNING = 10,
	TCP_NOTIF_ALL_SESSIONS_USED = 11,
	TCP_NOTIF_CONNECTION_TIMEOUT = 12,
	TCP_NOTIF_SSL_CONNECTION_ERROR = 13,
	TCP_NOTIF_SSL_INIT_ERROR = 14,
	TCP_NOTIF_SSL_CERT_ERROR = 15
};

/** Enum representing information transfer capability events */
enum hl78xx_info_transfer_event {
	EVENT_START_SCAN = 0,
	EVENT_FAIL_SCAN,
	EVENT_ENTER_CAMPED,
	EVENT_CONNECTION_ESTABLISHMENT,
	EVENT_START_RESCAN,
	EVENT_RRC_CONNECTED,
	EVENT_NO_SUITABLE_CELLS,
	EVENT_ALL_REGISTRATION_FAILED
};

struct kselacq_syntax {
	bool mode;
	enum hl78xx_cell_rat_mode rat1;
	enum hl78xx_cell_rat_mode rat2;
	enum hl78xx_cell_rat_mode rat3;
};

struct kband_syntax {
	uint8_t rat;
	/* Max 64 digits representation format is supported
	 * i.e: LTE Band 256 (2000MHz) :
	 * 80000000 00000000 00000000 00000000
	 * 00000000 00000000 00000000 00000000
	 * +
	 * NULL terminate
	 */
	uint8_t bnd_bitmap[MDM_BAND_HEX_STR_LEN];
};
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_PSM
struct hl78xx_psm_status {
	enum hl78xx_psmev_event current;
	enum hl78xx_psmev_event previous;
	bool is_psm_active;
};
#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_EDRX
struct hl78xx_edrx_status {
	enum hl78xx_edrx_event current;
	enum hl78xx_edrx_event previous;
	bool is_edrx_idle_requested;
};
#endif /* CONFIG_MODEM_HL78XX_EDRX */

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
struct hl78xx_power_down_status {
	enum power_down_event current;
	enum power_down_event previous;
	bool is_power_down_requested;
};
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

enum hl78xx_kedrx_mode {
	HL78XX_KEDRX_MODE_DISABLE = 0,
	HL78XX_KEDRX_MODE_ENABLE = 1,
	HL78XX_KEDRX_MODE_ENABLE_W_URC = 2,
	HL78XX_KEDRX_MODE_DISABLE_AND_ERASE_CFG = 3,
};

enum hl78xx_kedrx_ack_type {
	HL78XX_KEDRX_ACK_TYPE_CATM = 4,
	HL78XX_KEDRX_ACK_TYPE_NB = 5,
};
struct ksleep_syntax {
	uint8_t mngt;
	uint8_t level;
	uint8_t delay;
};
struct cpsms_syntax {
	/* Indication to disable or enable the use of PSM in the UE; */
	bool mode;
	/* TAU value (T3412) */
	uint8_t periodic_tau;
	/* Active Time value (T3324) */
	uint8_t active_time;
};

struct kedrxcfg_syntax {
	enum hl78xx_kedrx_mode mode;
	enum hl78xx_kedrx_ack_type ack_type;
	uint8_t requested_edrx;
};
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
enum apn_state_enum_t {
	APN_STATE_NOT_CONFIGURED = 0,
	APN_STATE_CONFIGURED,
	APN_STATE_REFRESH_REQUESTED,
	APN_STATE_REFRESH_IN_PROGRESS,
	APN_STATE_REFRESH_COMPLETED,
};

struct apn_state {
	enum apn_state_enum_t state;
};

struct registration_status {
	bool is_registered_currently;
	bool is_registered_previously;
	enum cellular_registration_status network_state_current;
	enum cellular_registration_status network_state_previous;
	enum hl78xx_cell_rat_mode rat_mode;
};

/* driver data */
struct modem_buffers {
	uint8_t uart_rx[CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES];
	uint8_t uart_tx[CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES];
	uint8_t chat_rx[CONFIG_MODEM_HL78XX_CHAT_BUFFER_SIZES];
	uint8_t cmd_buffer[CONFIG_MODEM_HL78XX_COMMAND_BUFFER_SIZE];
	size_t cmd_len;
	uint8_t *delimiter;
	uint8_t *filter;
	uint8_t *argv[MDM_CHAT_ARGV_BUFFER_SIZE];
	uint8_t *eof_pattern;
	uint8_t eof_pattern_size;
	uint8_t *termination_pattern;
	uint8_t termination_pattern_size;
};

struct modem_identity {
	uint8_t imei[MDM_IMEI_LENGTH];
	uint8_t model_id[MDM_MODEL_LENGTH];
	uint8_t imsi[MDM_IMSI_LENGTH];
	uint8_t iccid[MDM_ICCID_LENGTH];
	uint8_t manufacturer[MDM_MANUFACTURER_LENGTH];
	uint8_t fw_version[MDM_REVISION_LENGTH];
	uint8_t serial_number[MDM_SERIAL_NUMBER_LENGTH];
	char apn[MDM_APN_MAX_LENGTH];
};

struct hl78xx_phone_functionality_work {
	enum hl78xx_phone_functionality functionality;
	bool in_progress;
};

struct hl78xx_network_operator {
	char operator[MDM_MODEL_LENGTH];
	uint8_t format;
};
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN

struct ntn_rat_state {
	char pos_mode[NTN_POSITION_METHOD_TEXT_MAX_LEN];
	bool is_dynamic;
};

#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */

struct hl78xx_modem_boot_status {
	bool is_booted_previously;
	enum hl78xx_module_status status;
};

struct hl78xx_gprs_status {
	bool is_active;
	int8_t cid;
};

#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
/* WDSI FOTA states */
enum hl78xx_wdsi_fota_states {
	HL78XX_WDSI_FOTA_IDLE = 0,
	HL78XX_WDSI_FOTA_DOWNLOADING,
	HL78XX_WDSI_FOTA_DOWNLOAD_COMPLETED,
	HL78XX_WDSI_FOTA_INSTALLING,
	HL78XX_WDSI_FOTA_INSTALL_COMPLETED,
	HL78XX_WDSI_FOTA_INSTALL_FAILED
};

struct hl78xx_wdsi_status {
	enum wdsi_indication level;
	uint32_t data;
	size_t fota_size;
	bool in_progress;
	enum hl78xx_wdsi_fota_states fota_state;
	bool completed;
	int progress;
};

#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

struct hl78xx_modem_uart_status {
	uint32_t current_baudrate;
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
	uint32_t target_baudrate;
	uint8_t baudrate_detection_retry;
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */
};
struct modem_status {
	struct registration_status registration;
	int16_t rssi;
	uint8_t ksrep;
	int16_t rsrp;
	int16_t rsrq;
	uint16_t script_fail_counter;
	int variant;
	enum hl78xx_state state;
	struct kband_syntax kbndcfg[HL78XX_RAT_COUNT];
	struct hl78xx_gprs_status gprs[MDM_MAX_PDP_CONTEXTS];
	struct hl78xx_modem_boot_status boot;
	struct hl78xx_phone_functionality_work phone_functionality;
	struct apn_state apn;
	struct hl78xx_network_operator network_operator;
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
	struct hl78xx_wdsi_status wdsi;
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
	struct ntn_rat_state ntn_rat;
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
	struct hl78xx_modem_uart_status uart;
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	/* Power Management Control */
	struct ksleep_syntax pmc_sleep;
	struct cpsms_syntax pmc_cpsms;
	struct kedrxcfg_syntax pmc_kedrxcfg[2];
#ifdef CONFIG_MODEM_HL78XX_PSM
	struct hl78xx_psm_status psmev;
	bool awaiting_psm_confirmation;
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	struct hl78xx_power_down_status power_down;
	bool ignore_power_down_feeding;
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
	struct hl78xx_edrx_status edrxev;
	bool ignore_edrx_idle_feeding;
#endif /* CONFIG_MODEM_HL78XX_EDRX */
	bool lpm_restore_pending;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	bool rrc_idle;
	uint16_t kcellmeas_timeout;
	bool kcellmeas_bootstrap_done;
};

struct modem_gpio_callbacks {
	struct gpio_callback vgpio_cb;
	struct gpio_callback uart_dsr_cb;
	struct gpio_callback gpio6_cb;
	struct gpio_callback uart_cts_cb;
};

struct modem_event_system {
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct ring_buf event_rb;
	struct k_mutex event_rb_lock;
};

struct hl78xx_data {
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	struct modem_chat chat;
	struct modem_chat_script dynamic_script;
	struct modem_chat_script_chat dynamic_chat;

	struct k_mutex tx_lock;
	struct k_mutex api_lock;
	struct k_sem script_stopped_sem_tx_int;
	struct k_sem script_stopped_sem_rx_int;
	struct k_sem suspended_sem;
#ifdef CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING
	struct k_sem stay_in_boot_mode_sem;
#endif /* CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING */

	struct modem_buffers buffers;
	struct modem_identity identity;
	struct modem_status status;
	struct modem_gpio_callbacks gpio_cbs;
	struct modem_event_system events;
	struct k_work_delayable timeout_work;
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	struct k_work_delayable hl78xx_pwr_dwn_work;
#endif
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	struct k_work_delayable hl78xx_edrx_idle_work;
	struct k_work_delayable hl78xx_gpio6_debounce_work;
	bool hl78xx_gpio6_pending_state;
#endif
	/* Track leftover socket data state previously stored as a TU-global.
	 * Moving this into the per-modem data reduces global BSS and keeps
	 * state colocated with the modem instance.
	 */
	atomic_t state_leftover;
#if defined(CONFIG_MODEM_HL78XX_RSSI_WORK)
	struct k_work_delayable rssi_query_work;
#endif

	const struct device *dev;
	/* GNSS device */
	const struct device *gnss_dev;
	/* Offload device */
	const struct device *offload_dev;

	struct kselacq_syntax kselacq_data;
};

struct hl78xx_config {
	const struct device *uart;
	const struct hl78xx_variant_ops *variant;
	struct gpio_dt_spec mdm_gpio_reset;
	struct gpio_dt_spec mdm_gpio_wake;
	struct gpio_dt_spec mdm_gpio_pwr_on;
	struct gpio_dt_spec mdm_gpio_vgpio;
	struct gpio_dt_spec mdm_gpio_uart_cts;
	struct gpio_dt_spec mdm_gpio_gpio6;
	struct gpio_dt_spec mdm_gpio_fast_shutdown;
	struct gpio_dt_spec mdm_gpio_uart_dtr;
	struct gpio_dt_spec mdm_gpio_uart_dsr;
	struct gpio_dt_spec mdm_gpio_gpio8;
	struct gpio_dt_spec mdm_gpio_sim_switch;
	uint16_t power_pulse_duration_ms;
	uint16_t reset_pulse_duration_ms;
	uint16_t startup_time_ms;
	uint16_t shutdown_time_ms;

	bool autostarts;

	const struct modem_chat_script *init_chat_script;
	const struct modem_chat_script *periodic_chat_script;
};

/**
 * @brief Check whether a GPIO is available (has a valid port binding).
 */
static inline bool hl78xx_gpio_is_enabled(const struct gpio_dt_spec *gpio)
{
	return (gpio->port != NULL);
}

/* Forward-declare for variant ops */
struct hl78xx_socket_data;

/**
 * @brief Variant-specific operations for HL78xx modem family.
 *
 * Each module variant (HL7800, HL7812, etc.) provides a const instance
 * of this struct.  The common driver calls through these function pointers
 * at points where behaviour diverges between variants.
 */
struct hl78xx_variant_ops {
	/* True when modem-side socket context is lost in PSM/eDRX and must be recreated. */
	bool socket_lpm_recreate_required;

	/**
	 * @brief Select RAT command and requested RAT according to variant policy.
	 *
	 * @param[out] cmd_set_rat AT command string to apply selected RAT.
	 * @param[out] rat_request Selected RAT mode.
	 * @return 0 on success, -EINVAL when no valid RAT policy is configured.
	 */
	int (*cfg_select_rat)(struct hl78xx_data *data, const char **cmd_set_rat,
			      enum hl78xx_cell_rat_mode *rat_request);

	/**
	 * @brief Apply variant-specific registration-status script after RAT selection.
	 */
	int (*cfg_apply_rat_post_select)(struct hl78xx_data *data,
					 enum hl78xx_cell_rat_mode rat_request);

	/**
	 * @brief Return true if band configuration should be skipped for selected RAT.
	 */
	bool (*cfg_skip_band_for_rat)(struct hl78xx_data *data,
				      enum hl78xx_cell_rat_mode rat_request);

	/**
	 * @brief Handle +KSTATEV URC payload.
	 *
	 * Primarily used by HL7812 to update RAT-mode transitions.
	 */
	void (*on_kstatev_urc)(struct hl78xx_data *data, int state_value, int rat_mode);

	/**
	 * @brief Handle +PSMEV URC payload.
	 *
	 * Primarily used by HL7812 where +PSMEV is authoritative.
	 */
	void (*on_psmev_urc)(struct hl78xx_data *data, int psmev_value);

	/**
	 * @brief Handle parsed RRC status from AT%STATUS="RRC".
	 *
	 * Used by GNSS/eDRX gating flow before GNSS start.
	 */
	void (*on_rrc_status_urc)(struct hl78xx_data *data, bool is_idle);

	/**
	 * @brief Handle GPIO6 interrupt (sleep/wake detection).
	 *
	 * Called from the common GPIO6 ISR with the current pin state.
	 * HL7800: GPIO6 is the sole authoritative sleep/wake indicator.
	 * HL7812: Supplements +PSMEV URC; tracks eDRX/power-down.
	 */
	void (*gpio6_handler)(struct hl78xx_data *data, bool pin_state);

	/**
	 * @brief GPIO6 debounce delay in milliseconds.
	 *
	 * Set to 0 to disable debounce and process GPIO6 edges immediately.
	 */
	uint16_t gpio6_debounce_ms;

	/**
	 * @brief Interpret +KSUP during low-power operation.
	 *
	 * HL7800: +KSUP on PSM/eDRX exit is informational (AT settings
	 *         preserved). Routes GNSS-pending and config-restart cases.
	 * HL7812: Always treated as an unexpected restart (MDM_RESTART).
	 */
	void (*on_ksup_lpm)(struct hl78xx_data *data);

	/**
	 * @brief Variant-specific logic in await-registered state enter.
	 *
	 * HL7800: eDRX settling timer, PSM GNSS wake, "already registered"
	 *         shortcut.
	 * HL7812: PSM wake LTE layer only.
	 *
	 * @return 0 to continue default processing, 1 to skip it.
	 */
	int (*await_registered_enter_lpm)(struct hl78xx_data *data);

	/**
	 * @brief Variant-specific timeout handling in await-registered state.
	 *
	 * Called when MODEM_HL78XX_EVENT_TIMEOUT is received while waiting
	 * for registration in LPM flows.
	 *
	 * @return true if handled and caller should stop default timeout path.
	 */
	bool (*await_registered_timeout_lpm)(struct hl78xx_data *data);

	/**
	 * @brief Variant-specific carrier-on enter for LPM.
	 *
	 * Determines whether network setup (CGCONTRDP/DNS) is needed,
	 * handles PSM/eDRX context restoration.
	 *
	 * @param[out] is_lpm Set to true if LPM restore is needed.
	 * @return 0 to continue, 1 to return early from carrier_on_enter.
	 */
	int (*carrier_on_enter_lpm)(struct hl78xx_data *data, bool *is_lpm);

	/**
	 * @brief Post-DNS-success cleanup in carrier-on timeout.
	 *
	 * HL7800: Clears PSM/eDRX exit state, releases socket semaphore.
	 * HL7812: No-op (socket sem released via +KCELLMEAS).
	 */
	void (*carrier_on_dns_complete)(struct hl78xx_data *data);

	/**
	 * @brief Handle deregistered event for PSM detection.
	 *
	 * HL7800: Pull WAKE LOW to allow PSM entry.
	 * HL7812: Just log, wait for GPIO6/PSMEV.
	 */
	void (*carrier_on_deregistered_psm)(struct hl78xx_data *data);

	/**
	 * @brief Prepare socket layer on sleep entry.
	 *
	 * HL7800: Invalidate all modem-side socket contexts (destroyed
	 *         during PSM/eDRX).
	 * HL7812: No-op (contexts retained).
	 */
	void (*on_sleep_enter)(struct hl78xx_data *data);

	/**
	 * @brief Route state after sleep -> BUS_OPENED.
	 *
	 * HL7800 PSM/eDRX: Warm wake -> AWAIT_REGISTERED.
	 * HL7800 POWER_DOWN: Full init -> RUN_INIT_SCRIPT.
	 * HL7812: -> AWAIT_REGISTERED.
	 */
	void (*sleep_bus_opened_routing)(struct hl78xx_data *data);

	/**
	 * @brief Check if modem is in LPM and needs wakeup before socket I/O.
	 *
	 * Called by the socket offload layer before sendto().
	 *
	 * @param[out] in_lpm Set to true if modem is in LPM.
	 * @param[out] early_return Set to true to return 0 immediately
	 *             (modem is awake, no LPM processing needed).
	 */
	void (*check_lpm_state)(struct hl78xx_data *data, bool *in_lpm, bool *early_return);

	/**
	 * @brief Parse +CxREG ACT value into RAT mode for variant-specific URC formats.
	 *
	 * @param act_value ACT value parsed from +CxREG payload.
	 * @param[out] rat_mode Parsed RAT mode if handled.
	 * @return true when ACT parsing is supported by this variant.
	 */
	bool (*cxreg_try_parse_rat_mode)(struct hl78xx_data *data, int act_value,
					 enum hl78xx_cell_rat_mode *rat_mode);

	/**
	 * @brief Handle data-ready semantics when +CEREG/+CREG indicates registration.
	 *
	 * Some variants use registration URCs as an early data-ready signal
	 * in LPM+GNSS flows; others defer readiness to +KCELLMEAS.
	 */
	void (*on_registered_ready)(struct hl78xx_data *data);

	/**
	 * @brief Handle data-ready semantics when +KCELLMEAS indicates valid signal.
	 *
	 * Some variants release socket communications on this signal,
	 * while others complete restore later in carrier-on processing.
	 */
	void (*on_kcellmeas_ready)(struct hl78xx_data *data);

	/**
	 * @brief Handle pending GNSS mode request on CARRIER_ON entry.
	 *
	 * @return true if the variant handled routing and the caller should return.
	 */
	bool (*carrier_on_gnss_pending)(struct hl78xx_data *data);

	/**
	 * @brief Handle GNSS mode enter request while low power mode is enabled.
	 *
	 * @return true if the variant handled the request and caller should return.
	 */
	bool (*on_gnss_mode_enter_lpm)(struct hl78xx_data *data);
};

/* Variant ops instances (defined in variant-specific .c files) */
extern const struct hl78xx_variant_ops hl78xx_variant_ops_hl7812;
extern const struct hl78xx_variant_ops hl78xx_variant_ops_hl7800;

/* socket read callback data */
struct socket_read_data {
	char *recv_buf;
	size_t recv_buf_len;
	struct net_sockaddr *recv_addr;
	uint16_t recv_read_len;
};

/**
 * @brief Check if the cellular modem is registered on the network.
 *
 * This function checks the modem's current registration status and
 * returns true if the device is registered with a cellular network.
 *
 * @param data Pointer to the modem HL78xx driver data structure.
 *
 * @retval true if the modem is registered.
 * @retval false otherwise.
 */
bool hl78xx_is_registered(struct hl78xx_data *data);

/**
 * @brief DNS resolution work callback.
 *
 * @param dev Pointer to the device structure.
 * @param hard_reset Boolean indicating if a hard reset is required.
 * @return int 0 on success, negative errno code on failure.
 * Should be used internally to handle DNS resolution events.
 */
int dns_work_cb(const struct device *dev, bool hard_reset);

/**
 * @brief Callback to update and handle network interface status.
 *
 * This function is typically scheduled as work to check and respond to changes
 * in the modem's network interface state, such as registration, link readiness,
 * or disconnection events.
 *
 * @param data Pointer to the modem HL78xx driver data structure.
 */
int iface_status_work_cb(struct hl78xx_data *data, modem_chat_script_callback script_user_callback);

/**
 * @brief Parameters for dynamic modem command execution.
 */
struct hl78xx_dynamic_cmd_request {
	modem_chat_script_callback script_user_callback;
	const uint8_t *cmd;
	uint16_t cmd_len;
	const struct modem_chat_match *response_matches;
	uint16_t matches_size;
	uint16_t response_timeout;
	bool user_cmd;
};

/**
 * @brief Send a command to the modem and wait for matching response(s).
 *
 * Request-object API preferred for maintainability.
 */
int modem_dynamic_cmd_send_req(struct hl78xx_data *data,
			       const struct hl78xx_dynamic_cmd_request *req);

/**
 * @brief Send a command to the modem asynchronously and handle responses via callback.
 *
 * Request-object API preferred for maintainability.
 */
int modem_dynamic_cmd_send_async_req(struct hl78xx_data *data,
				     const struct hl78xx_dynamic_cmd_request *req);

/* Compatibility call-shapes while callsites migrate to *_req APIs. */
#define modem_dynamic_cmd_send(_data, _script_cb, _cmd, _cmd_len, _response_matches,               \
			       _matches_size, _response_timeout, _user_cmd)                        \
	modem_dynamic_cmd_send_req((_data), &(const struct hl78xx_dynamic_cmd_request){            \
						    .script_user_callback = (_script_cb),          \
						    .cmd = (const uint8_t *)(_cmd),                \
						    .cmd_len = (_cmd_len),                         \
						    .response_matches = (_response_matches),       \
						    .matches_size = (_matches_size),               \
						    .response_timeout = (_response_timeout),       \
						    .user_cmd = (_user_cmd),                       \
					    })

#define modem_dynamic_cmd_send_async(_data, _script_cb, _cmd, _cmd_len, _response_matches,         \
				     _matches_size, _response_timeout, _user_cmd)                  \
	modem_dynamic_cmd_send_async_req((_data), &(const struct hl78xx_dynamic_cmd_request){      \
							  .script_user_callback = (_script_cb),    \
							  .cmd = (const uint8_t *)(_cmd),          \
							  .cmd_len = (_cmd_len),                   \
							  .response_matches = (_response_matches), \
							  .matches_size = (_matches_size),         \
							  .response_timeout = (_response_timeout), \
							  .user_cmd = (_user_cmd),                 \
						  })

#define HASH_MULTIPLIER 37
/**
 * @brief Generate a 32-bit hash from a string.
 *
 * Useful for generating identifiers (e.g., MAC address suffix) from a given string.
 *
 * @param str Input string to hash.
 * @param len Length of the input string.
 *
 * @return 32-bit hash value.
 */
static inline uint32_t hash32(const char *str, int len)
{
	uint32_t h = 0;

	for (int i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}
	return h;
}

/**
 * @brief Generate a pseudo-random MAC address based on the modem's IMEI.
 *
 * This function creates a MAC address using a fixed prefix and a hash of the IMEI.
 * The resulting address is consistent for the same IMEI and suitable for use
 * in virtual or emulated network interfaces.
 *
 * @param mac_addr Pointer to a 6-byte buffer where the generated MAC address will be stored.
 * @param imei Null-terminated string containing the modem's IMEI.
 *
 * @return Pointer to the MAC address buffer.
 */
static inline uint8_t *modem_get_mac(uint8_t *mac_addr, char *imei)
{
	uint32_t hash_value;
	/* Define MAC address prefix */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x10;

	/* Generate MAC address based on IMEI */
	hash_value = hash32(imei, strlen(imei));
	UNALIGNED_PUT(hash_value, (uint32_t *)(mac_addr + 2));

	return mac_addr;
}

/**
 * @brief  Convert string to long integer, but handle errors
 *
 * @param  s: string with representation of integer number
 * @param  err_value: on error return this value instead
 * @param  desc: name the string being converted
 * @param  func: function where this is called (typically __func__)
 *
 * @retval return integer conversion on success, or err_value on error
 */
static inline int modem_atoi(const char *s, const int err_value, const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		return err_value;
	}
	return ret;
}

/**
 * @brief Convert a string to an double with error handling.
 *
 * Similar to atoi, but allows specifying an error fallback and logs errors.
 *
 * @param s Input string to convert.
 * @param err_value Value to return on failure.
 * @param desc Description of the value for logging purposes.
 * @param func Function name for logging purposes.
 *
 * @return Converted double on success, or err_value on failure.
 */
static inline double modem_atod(const char *s, const double err_value, const char *desc,
				const char *func)
{
	double ret;
	char *endptr;

	ret = strtod(s, &endptr);
	if (!endptr || *endptr != '\0') {
		return err_value;
	}
	return ret;
}
/**
 * @brief Small utility: safe strncpy that always NUL-terminates the destination.
 * This function copies a string from src to dst, ensuring that the destination
 * buffer is always NUL-terminated, even if the source string is longer than
 * the destination buffer.
 * @param dst Destination buffer.
 * @param src Source string.
 * @param dst_size Size of the destination buffer.
 */
static inline void safe_strncpy(char *dst, const char *src, size_t dst_size)
{
	size_t len = 0;

	if (dst == NULL || dst_size == 0) {
		return;
	}
	if (src == NULL) {
		dst[0] = '\0';
		return;
	}
	len = strlen(src);
	if (len >= dst_size) {
		len = dst_size - 1;
	}
	memcpy(dst, src, len);
	dst[len] = '\0';
}

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
/**
 * @brief Handle modem state update from +KSTATE URC (unsolicited result code).
 *
 * This function is called when a +KSTATE URC is received, indicating a change
 * in the modem's internal state. It updates the modem driver's state machine
 * accordingly.
 *
 * @param data Pointer to the HL78xx modem driver data structure.
 * @param state Integer value representing the new modem state as reported by the URC.
 */
void hl78xx_on_kstatev_parser(struct hl78xx_data *data, int state, int rat_mode);
#endif

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
/**
 * @brief Automatically detect and configure the modem's APN setting.
 *
 * Uses internal logic to determine the correct APN based on the modem's context
 * and network registration information.
 *
 * @param data Pointer to the modem HL78xx driver data structure.
 * @param associated_number Identifier (e.g., MCCMNC or IMSI) used for APN detection.
 *
 * @return 0 on success, negative errno code on failure.
 */
int modem_detect_apn(struct hl78xx_data *data, const char *associated_number);
#endif
/**
 * @brief Get the default band configuration in hexadecimal string format for each band.
 *
 * Retrieves the modem's default band configuration as a hex string,
 * used for configuring or restoring band settings.
 *
 * @param rat The radio access technology mode for which to get the band configuration.
 * @param hex_bndcfg Buffer to store the resulting hex band configuration string.
 * @param size_in_bytes Size of the buffer in bytes.
 *
 * @retval 0 on success.
 * @retval Negative errno code on failure.
 */
int hl78xx_get_band_default_config_for_rat(enum hl78xx_cell_rat_mode rat, char *hex_bndcfg,
					   size_t size_in_bytes);

/**
 * @brief Convert a hexadecimal string to a binary bitmap.
 *
 * Parses a hexadecimal string and converts it into a binary bitmap array.
 *
 * @param hex_str Null-terminated string containing hexadecimal data.
 * @param bitmap_out Output buffer to hold the resulting binary bitmap.
 *
 * @retval 0 on success.
 * @retval Negative errno code on failure (e.g., invalid characters, overflow).
 */
int hl78xx_hex_string_to_bitmap(const char *hex_str, uint8_t *bitmap_out);

/**
 * @brief hl78xx_api_func_get_registration_status - Brief description of the function.
 * @param dev Description of dev.
 * @param tech Description of tech.
 * @param status Description of status.
 * @return int Description of return value.
 */
int hl78xx_api_func_get_registration_status(const struct device *dev,
					    enum cellular_access_technology tech,
					    enum cellular_registration_status *status);

/**
 * @brief hl78xx_api_func_set_apn - Brief description of the function.
 * @param dev Description of dev.
 * @param apn Description of apn.
 * @return int Description of return value.
 */
int hl78xx_api_func_set_apn(const struct device *dev, const char *apn);

/**
 * @brief hl78xx_api_func_get_modem_info_standard - Brief description of the function.
 * @param dev Description of dev.
 * @param type Description of type.
 * @param info Description of info.
 * @param size Description of size.
 * @return int Description of return value.
 */
int hl78xx_api_func_get_modem_info_standard(const struct device *dev,
					    enum cellular_modem_info_type type, char *info,
					    size_t size);

/**
 * @brief hl78xx_enter_state - Brief description of the function.
 * @param data Description of data.
 * @param state Description of state.
 */
void hl78xx_enter_state(struct hl78xx_data *data, enum hl78xx_state state);

/**
 * @brief hl78xx_delegate_event - Brief description of the function.
 * @param data Description of data.
 * @param evt Description of evt.
 */
void hl78xx_delegate_event(struct hl78xx_data *data, enum hl78xx_event evt);

/**
 * @brief notif_carrier_off - Brief description of the function.
 * @param dev Description of dev.
 */
void notif_carrier_off(const struct device *dev);

/**
 * @brief notif_carrier_on - Brief description of the function.
 * @param dev Description of dev.
 */
void notif_carrier_on(const struct device *dev);

/**
 * @brief check_if_any_socket_connected - Brief description of the function.
 * @param dev Description of dev.
 * @return int Description of return value.
 */
int check_if_any_socket_connected(const struct device *dev);

/**
 * @brief Schedule a work to generate MODEM_HL78XX_EVENT_TIMEOUT
 * @param data pointer to hl78xx_data.
 * @param timeout the time to wait before submitting the work item.
 *
 */
void hl78xx_start_timer(struct hl78xx_data *data, k_timeout_t timeout);

/**
 * @brief Stop the timer.
 * @param data pointer to hl78xx_data.
 */
void hl78xx_stop_timer(struct hl78xx_data *data);

/**
 * @brief Dispatch an event
 * @param notif event information.
 */
void event_dispatcher_dispatch(struct hl78xx_evt *notif);

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE

#ifdef CONFIG_MODEM_HL78XX_PSM

static inline bool hl78xx_psm_is_active(struct hl78xx_data *data)
{
	return data->status.psmev.current != HL78XX_PSM_EVENT_NONE;
}

#endif /* CONFIG_MODEM_HL78XX_PSM */

void hl78xx_release_socket_comms(struct hl78xx_data *data);

#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

#endif /* HL78XX_H */
