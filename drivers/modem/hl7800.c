/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT swir_hl7800

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_hl7800, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/pm/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/net/net_context.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dns_resolve.h>
#if defined(CONFIG_NET_IPV6)
#include "ipv6.h"
#endif
#if defined(CONFIG_NET_IPV4)
#include "ipv4.h"
#endif
#if defined(CONFIG_NET_UDP)
#include "udp_internal.h"
#endif

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
#include <zephyr/fs/fs.h>
#endif

#include "modem_receiver.h"
#include <zephyr/drivers/modem/hl7800.h>

#define PREFIXED_SWITCH_CASE_RETURN_STRING(prefix, val)                        \
	case prefix##_##val: {                                                 \
		return #val;                                                   \
	}

/* Uncomment the #define below to enable a hexdump of all incoming
 * data from the modem receiver
 */
/* #define HL7800_ENABLE_VERBOSE_MODEM_RECV_HEXDUMP 1 */

#define HL7800_LOG_UNHANDLED_RX_MSGS 1

/* Uncomment the #define(s) below to enable extra debugging */
/* #define HL7800_RX_LOCK_LOG 1 */
/* #define HL7800_TX_LOCK_LOG 1 */
/* #define HL7800_IO_LOG 1 */

#define HL7800_RX_LOCK_DBG_LOG(fmt, ...)                                       \
	do {                                                                   \
		if (IS_ENABLED(HL7800_RX_LOCK_LOG)) {                          \
			LOG_DBG(fmt, ##__VA_ARGS__);                           \
		}                                                              \
	} while (false)

#define HL7800_TX_LOCK_DBG_LOG(fmt, ...)                                       \
	do {                                                                   \
		if (IS_ENABLED(HL7800_TX_LOCK_LOG)) {                          \
			LOG_DBG(fmt, ##__VA_ARGS__);                           \
		}                                                              \
	} while (false)

#define HL7800_IO_DBG_LOG(fmt, ...)                                            \
	do {                                                                   \
		if (IS_ENABLED(HL7800_IO_LOG)) {                               \
			LOG_DBG(fmt, ##__VA_ARGS__);                           \
		}                                                              \
	} while (false)

#if ((LOG_LEVEL == LOG_LEVEL_DBG) &&                                           \
     defined(CONFIG_MODEM_HL7800_LOW_POWER_MODE))
#define PRINT_AWAKE_MSG LOG_WRN("awake")
#define PRINT_NOT_AWAKE_MSG LOG_WRN("NOT awake")
#else
#define PRINT_AWAKE_MSG
#define PRINT_NOT_AWAKE_MSG
#endif

enum tcp_notif {
	HL7800_TCP_NET_ERR,
	HL7800_TCP_NO_SOCKS,
	HL7800_TCP_MEM,
	HL7800_TCP_DNS,
	HL7800_TCP_DISCON,
	HL7800_TCP_CONN,
	HL7800_TCP_ERR,
	HL7800_TCP_CLIENT_REQ,
	HL7800_TCP_DATA_SND,
	HL7800_TCP_ID,
	HL7800_TCP_RUNNING,
	HL7800_TCP_ALL_USED,
	HL7800_TCP_TIMEOUT,
	HL7800_TCP_SSL_CONN,
	HL7800_TCP_SSL_INIT
};

enum udp_notif {
	HL7800_UDP_NET_ERR = 0,
	HL7800_UDP_NO_SOCKS = 1,
	HL7800_UDP_MEM = 2,
	HL7800_UDP_DNS = 3,
	HL7800_UDP_CONN = 5,
	HL7800_UDP_ERR = 6,
	HL7800_UDP_DATA_SND = 8, /* this matches TCP_DATA_SND */
	HL7800_UDP_ID = 9,
	HL7800_UDP_RUNNING = 10,
	HL7800_UDP_ALL_USED = 11
};

enum socket_state {
	SOCK_IDLE,
	SOCK_RX,
	SOCK_TX,
	SOCK_SERVER_CLOSED,
	SOCK_CONNECTED,
};

enum hl7800_lpm {
	HL7800_LPM_NONE,
	HL7800_LPM_EDRX,
	HL7800_LPM_PSM,
};

struct mdm_control_pinconfig {
	char *dev_name;
	gpio_pin_t pin;
	gpio_flags_t config;
	gpio_flags_t irq_config;
};

#define PINCONFIG(name_, pin_, config_, irq_config_)                                               \
	{                                                                                          \
		.dev_name = name_, .pin = pin_, .config = config_, .irq_config = irq_config_       \
	}

/* pin settings */
enum mdm_control_pins {
	MDM_RESET = 0,
	MDM_WAKE,
	MDM_PWR_ON,
	MDM_FAST_SHUTD,
	MDM_VGPIO,
	MDM_UART_DSR,
	MDM_UART_CTS,
	MDM_GPIO6,
	MAX_MDM_CONTROL_PINS,
};

enum net_operator_status { NO_OPERATOR, REGISTERED };

enum device_service_indications {
	WDSI_PKG_DOWNLOADED = 3,
};

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
enum XMODEM_CONTROL_CHARACTERS {
	XM_SOH = 0x01,
	XM_SOH_1K = 0x02,
	XM_EOT = 0x04,
	XM_ACK = 0x06, /* 'R' */
	XM_NACK = 0x15, /* 'N' */
	XM_ETB = 0x17,
	XM_CAN = 0x18,
	XM_C = 0x43
};

#define XMODEM_DATA_SIZE 1024
#define XMODEM_PACKET_SIZE (XMODEM_DATA_SIZE + 4)
#define XMODEM_PAD_VALUE 26

struct xmodem_packet {
	uint8_t preamble;
	uint8_t id;
	uint8_t id_complement;
	uint8_t data[XMODEM_DATA_SIZE];
	uint8_t crc;
};
#endif

static const struct mdm_control_pinconfig pinconfig[] = {
	/* MDM_RESET */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_reset_gpios), DT_INST_GPIO_PIN(0, mdm_reset_gpios),
		  (GPIO_OUTPUT | GPIO_OPEN_DRAIN), 0),

	/* MDM_WAKE */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_wake_gpios), DT_INST_GPIO_PIN(0, mdm_wake_gpios),
		  (GPIO_OUTPUT | GPIO_OPEN_SOURCE), 0),

	/* MDM_PWR_ON */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_pwr_on_gpios), DT_INST_GPIO_PIN(0, mdm_pwr_on_gpios),
		  (GPIO_OUTPUT | GPIO_OPEN_DRAIN), 0),

	/* MDM_FAST_SHUTD */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_fast_shutd_gpios),
		  DT_INST_GPIO_PIN(0, mdm_fast_shutd_gpios), (GPIO_OUTPUT | GPIO_OPEN_DRAIN),
		  0),

	/* MDM_VGPIO */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_vgpio_gpios), DT_INST_GPIO_PIN(0, mdm_vgpio_gpios),
		  GPIO_INPUT, GPIO_INT_EDGE_BOTH),

	/* MDM_UART_DSR */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_uart_dsr_gpios),
		  DT_INST_GPIO_PIN(0, mdm_uart_dsr_gpios), GPIO_INPUT, GPIO_INT_EDGE_BOTH),

	/* MDM_UART_CTS */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_uart_cts_gpios),
		  DT_INST_GPIO_PIN(0, mdm_uart_cts_gpios), GPIO_INPUT, GPIO_INT_EDGE_BOTH),

	/* MDM_GPIO6 */
	PINCONFIG(DT_INST_GPIO_LABEL(0, mdm_gpio6_gpios), DT_INST_GPIO_PIN(0, mdm_gpio6_gpios),
		  GPIO_INPUT, GPIO_INT_EDGE_BOTH),
};

#define MDM_UART_DEV	DEVICE_DT_GET(DT_INST_BUS(0))

#define MDM_WAKE_ASSERTED 1 /* Asserted keeps the module awake */
#define MDM_WAKE_NOT_ASSERTED 0
#define MDM_RESET_ASSERTED 0
#define MDM_RESET_NOT_ASSERTED 1
#define MDM_PWR_ON_ASSERTED 0
#define MDM_PWR_ON_NOT_ASSERTED 1
#define MDM_FAST_SHUTD_ASSERTED 0
#define MDM_FAST_SHUTD_NOT_ASSERTED 1

#define MDM_SEND_OK_ENABLED 0
#define MDM_SEND_OK_DISABLED 1

#define MDM_CMD_SEND_TIMEOUT K_SECONDS(6)
#define MDM_IP_SEND_RX_TIMEOUT K_SECONDS(62)
#define MDM_SOCK_NOTIF_DELAY K_MSEC(150)
#define MDM_CMD_CONN_TIMEOUT K_SECONDS(31)

#define MDM_MAX_DATA_LENGTH 1500
#define MDM_MTU 1500
#define MDM_MAX_RESP_SIZE 128
#define MDM_IP_INFO_RESP_SIZE 256

#define MDM_HANDLER_MATCH_MAX_LEN 100

#define MDM_MAX_SOCKETS 6

/* Special value used to indicate that a socket is being created
 * and that its actual ID hasn't been assigned yet.
 */
#define MDM_CREATE_SOCKET_ID (MDM_MAX_SOCKETS + 1)

#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

#define SIZE_OF_NUL 1

#define SIZE_WITHOUT_NUL(v) (sizeof(v) - SIZE_OF_NUL)

#define CMD_HANDLER(cmd_, cb_)                                                 \
	{                                                                      \
		.cmd = cmd_, .cmd_len = (uint16_t)sizeof(cmd_) - 1,            \
		.func = on_cmd_##cb_                                           \
	}

#define MDM_MANUFACTURER_LENGTH 16
#define MDM_MODEL_LENGTH 7
#define MDM_SN_RESPONSE_LENGTH (MDM_HL7800_SERIAL_NUMBER_SIZE + 7)
#define MDM_NETWORK_STATUS_LENGTH 45

#define MDM_TOP_BAND_SIZE 4
#define MDM_MIDDLE_BAND_SIZE 8
#define MDM_BOTTOM_BAND_SIZE 8
#define MDM_TOP_BAND_START_POSITION 2
#define MDM_MIDDLE_BAND_START_POSITION 6
#define MDM_BOTTOM_BAND_START_POSITION 14
#define MDM_BAND_BITMAP_STR_LENGTH_MAX                                                             \
	(MDM_TOP_BAND_SIZE + MDM_MIDDLE_BAND_SIZE + MDM_BOTTOM_BAND_SIZE)
#define MDM_BAND_BITMAP_STR_LENGTH_MIN 1

#define MDM_DEFAULT_AT_CMD_RETRIES 3
#define MDM_WAKEUP_TIME K_SECONDS(12)
#define MDM_BOOT_TIME K_SECONDS(12)
#define MDM_WAKE_TO_CHECK_CTS_DELAY_MS K_MSEC(20)

#define MDM_WAIT_FOR_DATA_TIME K_MSEC(50)
#define MDM_RESET_LOW_TIME K_MSEC(50)
#define MDM_RESET_HIGH_TIME K_MSEC(10)
#define MDM_WAIT_FOR_DATA_RETRIES 3

#define RSSI_UNKNOWN -999

#define DNS_WORK_DELAY_SECS 1
#define IFACE_WORK_DELAY K_MSEC(500)
#define SOCKET_CLEANUP_WORK_DELAY K_MSEC(100)
#define WAIT_FOR_KSUP_RETRIES 5

#define CGCONTRDP_RESPONSE_NUM_DELIMS 7
#define COPS_RESPONSE_NUM_DELIMS 2
#define KCELLMEAS_RESPONSE_NUM_DELIMS 4

#define PROFILE_LINE_1                                                         \
	"E1 Q0 V1 X4 &C1 &D1 &R1 &S0 +IFC=2,2 &K3 +IPR=115200 +FCLASS0\r\n"
#define PROFILE_LINE_2                                                         \
	"S00:255 S01:255 S03:255 S04:255 S05:255 S07:255 S08:255 S10:255\r\n"

#define ADDRESS_FAMILY_IPV4 "IPV4"
#if defined(CONFIG_MODEM_HL7800_ADDRESS_FAMILY_IPV4V6)
#define MODEM_HL7800_ADDRESS_FAMILY "IPV4V6"
#elif defined(CONFIG_MODEM_HL7800_ADDRESS_FAMILY_IPV4)
#define MODEM_HL7800_ADDRESS_FAMILY ADDRESS_FAMILY_IPV4
#else
#define MODEM_HL7800_ADDRESS_FAMILY "IPV6"
#endif
#define MDM_HL7800_SOCKET_AF_IPV4 0
#define MDM_HL7800_SOCKET_AF_IPV6 1

#define SET_RAT_M1_CMD_LEGACY "AT+KSRAT=0"
#define SET_RAT_NB1_CMD_LEGACY "AT+KSRAT=1"
#define SET_RAT_M1_CMD "AT+KSRAT=0,1"
#define SET_RAT_NB1_CMD "AT+KSRAT=1,1"
#define NEW_RAT_CMD_MIN_VERSION "HL7800.4.5.4.0"
#define HL7800_VERSION_FORMAT "HL7800.%zu.%zu.%zu.%zu"

#define MAX_PROFILE_LINE_LENGTH                                                \
	MAX(sizeof(PROFILE_LINE_1), sizeof(PROFILE_LINE_2))

#define IPV6_ADDR_FORMAT "####:####:####:####:####:####:####:####"
#define HL7800_IPV6_ADDR_LEN                                                                       \
	sizeof("a01.a02.a03.a04.a05.a06.a07.a08.a09.a10.a11.a12.a13.a14.a15.a16")

#define MDM_ADDR_FAM_MAX_LEN sizeof("IPV4V6")

/* The ? can be a + or - */
static const char TIME_STRING_FORMAT[] = "\"yy/MM/dd,hh:mm:ss?zz\"";
#define TIME_STRING_DIGIT_STRLEN 2
#define TIME_STRING_SEPARATOR_STRLEN 1
#define TIME_STRING_PLUS_MINUS_INDEX (6 * 3)
#define TIME_STRING_FIRST_SEPARATOR_INDEX 0
#define TIME_STRING_FIRST_DIGIT_INDEX 1
#define TIME_STRING_TO_TM_STRUCT_YEAR_OFFSET (2000 - 1900)

/* Time structure min, max */
#define TM_YEAR_RANGE 0, 99
#define TM_MONTH_RANGE_PLUS_1 1, 12
#define TM_DAY_RANGE 1, 31
#define TM_HOUR_RANGE 0, 23
#define TM_MIN_RANGE 0, 59
#define TM_SEC_RANGE 0, 60 /* leap second */
#define QUARTER_HOUR_RANGE 0, 96
#define SECONDS_PER_QUARTER_HOUR (15 * 60)

#define SEND_AT_CMD_ONCE_EXPECT_OK(c)                                          \
	do {                                                                   \
		ret = send_at_cmd(NULL, (c), MDM_CMD_SEND_TIMEOUT, 0, false);  \
		if (ret < 0) {                                                 \
			LOG_ERR("%s result:%d", (c), ret);                     \
			goto error;                                            \
		}                                                              \
	} while (0)

#define SEND_AT_CMD_IGNORE_ERROR(c)                                            \
	do {                                                                   \
		ret = send_at_cmd(NULL, (c), MDM_CMD_SEND_TIMEOUT, 0, false);  \
		if (ret < 0) {                                                 \
			LOG_ERR("%s result:%d", (c), ret);                     \
		}                                                              \
	} while (0)

#define SEND_AT_CMD_EXPECT_OK(c)                                               \
	do {                                                                   \
		ret = send_at_cmd(NULL, (c), MDM_CMD_SEND_TIMEOUT,             \
				  MDM_DEFAULT_AT_CMD_RETRIES, false);          \
		if (ret < 0) {                                                 \
			LOG_ERR("%s result:%d", (c), ret);                     \
			goto error;                                            \
		}                                                              \
	} while (0)

/* Complex has "no_id_resp" set to true because the sending command
 * is the command used to process the response
 */
#define SEND_COMPLEX_AT_CMD(c)                                                 \
	do {                                                                   \
		ret = send_at_cmd(NULL, (c), MDM_CMD_SEND_TIMEOUT,             \
				  MDM_DEFAULT_AT_CMD_RETRIES, true);           \
		if (ret < 0) {                                                 \
			LOG_ERR("%s result:%d", (c), ret);                     \
			goto error;                                            \
		}                                                              \
	} while (0)

NET_BUF_POOL_DEFINE(mdm_recv_pool, CONFIG_MODEM_HL7800_RECV_BUF_CNT,
		    CONFIG_MODEM_HL7800_RECV_BUF_SIZE, 0, NULL);

static uint8_t mdm_recv_buf[MDM_MAX_DATA_LENGTH];

static K_SEM_DEFINE(hl7800_RX_lock_sem, 1, 1);
static K_SEM_DEFINE(hl7800_TX_lock_sem, 1, 1);
static K_MUTEX_DEFINE(cb_lock);

/* RX thread structures */
K_THREAD_STACK_DEFINE(hl7800_rx_stack, CONFIG_MODEM_HL7800_RX_STACK_SIZE);
struct k_thread hl7800_rx_thread;
#define RX_THREAD_PRIORITY K_PRIO_COOP(7)

/* RX thread work queue */
K_THREAD_STACK_DEFINE(hl7800_workq_stack,
		      CONFIG_MODEM_HL7800_RX_WORKQ_STACK_SIZE);
static struct k_work_q hl7800_workq;
#define WORKQ_PRIORITY K_PRIO_COOP(7)

static const char EOF_PATTERN[] = "--EOF--Pattern--";
static const char CONNECT_STRING[] = "CONNECT";
static const char OK_STRING[] = "OK";

struct hl7800_socket {
	struct net_context *context;
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;
	struct sockaddr src;
	struct sockaddr dst;

	bool created;
	bool reconfig;
	int socket_id;
	int rx_size;
	bool error;
	int error_val;
	enum socket_state state;

	/** semaphore */
	struct k_sem sock_send_sem;

	/** socket callbacks */
	struct k_work recv_cb_work;
	struct k_work rx_data_work;
	struct k_work_delayable notif_work;
	net_context_recv_cb_t recv_cb;
	struct net_pkt *recv_pkt;
	void *recv_user_data;
};

struct stale_socket {
	int reserved; /* first word of queue data item reserved for the kernel */
	enum net_sock_type type;
	uint8_t id;
	bool allocated;
};

#define NO_ID_RESP_CMD_MAX_LENGTH 32

struct hl7800_iface_ctx {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct in_addr ipv4Addr, subnet, gateway, dns_v4;
#ifdef CONFIG_NET_IPV6
	struct in6_addr ipv6Addr, dns_v6;
	char dns_v6_string[HL7800_IPV6_ADDR_LEN];
#endif
	bool restarting;
	bool initialized;
	bool wait_for_KSUP;
	uint32_t wait_for_KSUP_tries;
	bool reconfig_IP_connection;
	char dns_v4_string[NET_IPV4_ADDR_LEN];
	char no_id_resp_cmd[NO_ID_RESP_CMD_MAX_LENGTH];
	bool search_no_id_resp;

	/* GPIO PORT devices */
	const struct device *gpio_port_dev[MAX_MDM_CONTROL_PINS];
	struct gpio_callback mdm_vgpio_cb;
	struct gpio_callback mdm_uart_dsr_cb;
	struct gpio_callback mdm_gpio6_cb;
	struct gpio_callback mdm_uart_cts_cb;
	int vgpio_state;
	int dsr_state;
	int gpio6_state;
	int cts_state;

	/* RX specific attributes */
	struct mdm_receiver_context mdm_ctx;

	/* socket data */
	struct hl7800_socket sockets[MDM_MAX_SOCKETS];
	int last_socket_id;
	int last_error;
	struct stale_socket stale_sockets[MDM_MAX_SOCKETS];
	struct k_queue stale_socket_queue;

	/* semaphores */
	struct k_sem response_sem;
	struct k_sem mdm_awake;

	/* work */
	struct k_work_delayable rssi_query_work;
	struct k_work_delayable iface_status_work;
	struct k_work_delayable dns_work;
	struct k_work mdm_vgpio_work;
	struct k_work_delayable mdm_reset_work;
	struct k_work_delayable allow_sleep_work;
	struct k_work_delayable delete_untracked_socket_work;

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	/* firmware update */
	enum mdm_hl7800_fota_state fw_update_state;
	struct fs_file_t fw_update_file;
	struct xmodem_packet fw_packet;
	uint32_t fw_packet_count;
	int file_pos;
	struct k_work finish_fw_update_work;
	bool fw_updated;
#endif

	/* modem info */
	/* NOTE: make sure length is +1 for null char */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_HL7800_REVISION_MAX_SIZE];
	char mdm_imei[MDM_HL7800_IMEI_SIZE];
	char mdm_sn[MDM_HL7800_SERIAL_NUMBER_SIZE];
	char mdm_network_status[MDM_NETWORK_STATUS_LENGTH];
	char mdm_iccid[MDM_HL7800_ICCID_SIZE];
	enum mdm_hl7800_startup_state mdm_startup_state;
	enum mdm_hl7800_radio_mode mdm_rat;
	char mdm_active_bands_string[MDM_HL7800_LTE_BAND_STR_SIZE];
	char mdm_bands_string[MDM_HL7800_LTE_BAND_STR_SIZE];
	char mdm_imsi[MDM_HL7800_IMSI_MAX_STR_SIZE];
	int mdm_rssi;
	uint16_t mdm_bands_top;
	uint32_t mdm_bands_middle;
	uint32_t mdm_bands_bottom;
	int32_t mdm_sinr;
	bool mdm_echo_is_on;
	struct mdm_hl7800_apn mdm_apn;
	bool mdm_startup_reporting_on;
	int device_services_ind;
	bool new_rat_cmd_support;
	uint8_t operator_index;
	enum mdm_hl7800_functionality functionality;
	char mdm_pdp_addr_fam[MDM_ADDR_FAM_MAX_LEN];

	/* modem state */
	bool allow_sleep;
	bool uart_on;
	enum mdm_hl7800_sleep desired_sleep_level;
	enum mdm_hl7800_sleep sleep_state;
	enum hl7800_lpm low_power_mode;
	enum mdm_hl7800_network_state network_state;
	enum net_operator_status operator_status;
	struct tm local_time;
	int32_t local_time_offset;
	bool local_time_valid;
	bool configured;
	void (*wake_up_callback)(int state);
	void (*gpio6_callback)(int state);
	void (*cts_callback)(int state);

#ifdef CONFIG_MODEM_HL7800_GPS
	struct k_work_delayable gps_work;
	uint32_t gps_query_location_rate_seconds;
#endif
};

struct cmd_handler {
	const char *cmd;
	uint16_t cmd_len;
	bool (*func)(struct net_buf **buf, uint16_t len);
};

static sys_slist_t hl7800_event_callback_list =
	SYS_SLIST_STATIC_INIT(&hl7800_event_callback_list);

static struct hl7800_iface_ctx ictx;

static size_t hl7800_read_rx(struct net_buf **buf);
static char *get_network_state_string(enum mdm_hl7800_network_state state);
static char *get_startup_state_string(enum mdm_hl7800_startup_state state);
static char *get_sleep_state_string(enum mdm_hl7800_sleep state);
static void set_network_state(enum mdm_hl7800_network_state state);
static void set_startup_state(enum mdm_hl7800_startup_state state);
static void set_sleep_state(enum mdm_hl7800_sleep state);
static void generate_network_state_event(void);
static void generate_startup_state_event(void);
static void generate_sleep_state_event(void);
static int modem_boot_handler(char *reason);
static void mdm_vgpio_work_cb(struct k_work *item);
static void mdm_reset_work_callback(struct k_work *item);
static int write_apn(char *access_point_name);
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
static void mark_sockets_for_reconfig(void);
#endif
static void hl7800_build_mac(struct hl7800_iface_ctx *ictx);
static void rssi_query(void);

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
static void initialize_sleep_level(void);
static int set_sleep_level(void);
#endif

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
static char *get_fota_state_string(enum mdm_hl7800_fota_state state);
static void set_fota_state(enum mdm_hl7800_fota_state state);
static void generate_fota_state_event(void);
static void generate_fota_count_event(void);
#endif

static struct stale_socket *alloc_stale_socket(void)
{
	struct stale_socket *sock = NULL;

	for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
		if (!ictx.stale_sockets[i].allocated) {
			sock = &ictx.stale_sockets[i];
			sock->allocated = true;
			break;
		}
	}

	return sock;
}

static void free_stale_socket(struct stale_socket *sock)
{
	if (sock != NULL) {
		sock->allocated = false;
	}
}

static int queue_stale_socket(enum net_sock_type type, uint8_t id)
{
	int ret = 0;
	struct stale_socket *sock = NULL;

	sock = alloc_stale_socket();
	if (sock != NULL) {
		sock->type = type;
		sock->id = id;
		k_queue_append(&ictx.stale_socket_queue, (void *)sock);
	} else {
		LOG_ERR("Could not alloc stale socket");
		ret = -ENOMEM;
	}

	return ret;
}

static struct stale_socket *dequeue_stale_socket(void)
{
	struct stale_socket *sock = NULL;

	sock = (struct stale_socket *)k_queue_get(&ictx.stale_socket_queue, K_NO_WAIT);

	return sock;
}

static bool convert_time_string_to_struct(struct tm *tm, int32_t *offset,
					  char *time_string);
static int modem_reset_and_configure(void);

static int read_pin(int default_state, const struct device *port, gpio_pin_t pin)
{
	int state = gpio_pin_get(port, pin);

	if (state < 0) {
		LOG_ERR("Unable to read port: %s pin: %d status: %d", port->name, pin, state);
		state = default_state;
	}

	return state;
}

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
static bool is_cmd_ready(void)
{
	ictx.vgpio_state = read_pin(0, ictx.gpio_port_dev[MDM_VGPIO], pinconfig[MDM_VGPIO].pin);

	ictx.gpio6_state = read_pin(0, ictx.gpio_port_dev[MDM_GPIO6], pinconfig[MDM_GPIO6].pin);

	ictx.cts_state = read_pin(1, ictx.gpio_port_dev[MDM_UART_CTS], pinconfig[MDM_UART_CTS].pin);

	return ictx.vgpio_state && ictx.gpio6_state && !ictx.cts_state;
}
#endif

/**
 * The definition of awake is that the HL7800
 * is ready to receive AT commands successfully
 */
static void check_hl7800_awake(void)
{
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	bool is_cmd_rdy = is_cmd_ready();

	if (is_cmd_rdy && (ictx.sleep_state != HL7800_SLEEP_AWAKE) &&
	    !ictx.allow_sleep && !ictx.wait_for_KSUP) {
		PRINT_AWAKE_MSG;
		set_sleep_state(HL7800_SLEEP_AWAKE);
		k_sem_give(&ictx.mdm_awake);
	} else if (!is_cmd_rdy && ictx.sleep_state == HL7800_SLEEP_AWAKE &&
		   ictx.allow_sleep) {
		PRINT_NOT_AWAKE_MSG;

		if (ictx.desired_sleep_level == HL7800_SLEEP_HIBERNATE ||
		    ictx.desired_sleep_level == HL7800_SLEEP_LITE_HIBERNATE) {
			/* If the device is sleeping (not ready to receive commands)
			 * then the device may send +KSUP when waking up.
			 * We should wait for it.
			 */
			ictx.wait_for_KSUP = true;
			ictx.wait_for_KSUP_tries = 0;

			set_sleep_state(ictx.desired_sleep_level);

		} else if (ictx.desired_sleep_level == HL7800_SLEEP_SLEEP) {
			set_sleep_state(HL7800_SLEEP_SLEEP);
		}
	}
#endif
}

static int hl7800_RX_lock(void)
{
	HL7800_RX_LOCK_DBG_LOG("Locking RX [%p]...", k_current_get());
	int rc = k_sem_take(&hl7800_RX_lock_sem, K_FOREVER);

	if (rc != 0) {
		LOG_ERR("Unable to lock hl7800 (%d)", rc);
	} else {
		HL7800_RX_LOCK_DBG_LOG("Locked RX [%p]", k_current_get());
	}

	return rc;
}

static void hl7800_RX_unlock(void)
{
	HL7800_RX_LOCK_DBG_LOG("UNLocking RX [%p]...", k_current_get());
	k_sem_give(&hl7800_RX_lock_sem);
	HL7800_RX_LOCK_DBG_LOG("UNLocked RX [%p]", k_current_get());
}

static bool hl7800_RX_locked(void)
{
	if (k_sem_count_get(&hl7800_RX_lock_sem) == 0) {
		return true;
	} else {
		return false;
	}
}

static int hl7800_TX_lock(void)
{
	HL7800_TX_LOCK_DBG_LOG("Locking TX [%p]...", k_current_get());
	int rc = k_sem_take(&hl7800_TX_lock_sem, K_FOREVER);

	if (rc != 0) {
		LOG_ERR("Unable to lock hl7800 (%d)", rc);
	} else {
		HL7800_TX_LOCK_DBG_LOG("Locked TX [%p]", k_current_get());
	}

	return rc;
}

static void hl7800_TX_unlock(void)
{
	HL7800_TX_LOCK_DBG_LOG("UNLocking TX [%p]...", k_current_get());
	k_sem_give(&hl7800_TX_lock_sem);
	HL7800_TX_LOCK_DBG_LOG("UNLocked TX [%p]", k_current_get());
}

static bool hl7800_TX_locked(void)
{
	if (k_sem_count_get(&hl7800_TX_lock_sem) == 0) {
		return true;
	} else {
		return false;
	}
}

static void hl7800_lock(void)
{
	hl7800_TX_lock();
	hl7800_RX_lock();
}

static void hl7800_unlock(void)
{
	hl7800_RX_unlock();
	hl7800_TX_unlock();
}

static struct hl7800_socket *socket_get(void)
{
	int i;
	struct hl7800_socket *sock = NULL;

	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		if (!ictx.sockets[i].context) {
			sock = &ictx.sockets[i];
			break;
		}
	}

	return sock;
}

static struct hl7800_socket *socket_from_id(int socket_id)
{
	int i;
	struct hl7800_socket *sock = NULL;

	if (socket_id < 1) {
		return NULL;
	}

	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		if (ictx.sockets[i].socket_id == socket_id) {
			sock = &ictx.sockets[i];
			break;
		}
	}

	return sock;
}

static void socket_put(struct hl7800_socket *sock)
{
	if (!sock) {
		return;
	}

	sock->context = NULL;
	sock->socket_id = -1;
	sock->created = false;
	sock->reconfig = false;
	sock->error = false;
	sock->error_val = -1;
	sock->rx_size = 0;
	sock->state = SOCK_IDLE;
	(void)memset(&sock->src, 0, sizeof(struct sockaddr));
	(void)memset(&sock->dst, 0, sizeof(struct sockaddr));
}

char *hl7800_sprint_ip_addr(const struct sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		return net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf,
				     sizeof(buf));
	} else
#endif
#if defined(CONFIG_NET_IPV4)
		if (addr->sa_family == AF_INET) {
		return net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr, buf,
				     sizeof(buf));
	} else
#endif
	{
		LOG_ERR("Unknown IP address family:%d", addr->sa_family);
		return NULL;
	}
}

void mdm_hl7800_register_wake_test_point_callback(void (*func)(int state))
{
	ictx.wake_up_callback = func;
}

void mdm_hl7800_register_gpio6_callback(void (*func)(int state))
{
	ictx.gpio6_callback = func;
}

void mdm_hl7800_register_cts_callback(void (*func)(int state))
{
	ictx.cts_callback = func;
}

static void modem_assert_wake(bool assert)
{
	int state;

	if (assert) {
		HL7800_IO_DBG_LOG("MDM_WAKE_PIN -> ASSERTED");
		state = MDM_WAKE_ASSERTED;
	} else {
		HL7800_IO_DBG_LOG("MDM_WAKE_PIN -> NOT_ASSERTED");
		state = MDM_WAKE_NOT_ASSERTED;
	}

	gpio_pin_set(ictx.gpio_port_dev[MDM_WAKE], pinconfig[MDM_WAKE].pin, state);

	if (ictx.wake_up_callback != NULL) {
		ictx.wake_up_callback(state);
	}
}

static void modem_assert_pwr_on(bool assert)
{
	if (assert) {
		HL7800_IO_DBG_LOG("MDM_PWR_ON -> ASSERTED");
		gpio_pin_set(ictx.gpio_port_dev[MDM_PWR_ON],
			     pinconfig[MDM_PWR_ON].pin, MDM_PWR_ON_ASSERTED);
	} else {
		HL7800_IO_DBG_LOG("MDM_PWR_ON -> NOT_ASSERTED");
		gpio_pin_set(ictx.gpio_port_dev[MDM_PWR_ON],
			     pinconfig[MDM_PWR_ON].pin,
			     MDM_PWR_ON_NOT_ASSERTED);
	}
}

static void modem_assert_fast_shutd(bool assert)
{
	if (assert) {
		HL7800_IO_DBG_LOG("MDM_FAST_SHUTD -> ASSERTED");
		gpio_pin_set(ictx.gpio_port_dev[MDM_FAST_SHUTD],
			     pinconfig[MDM_FAST_SHUTD].pin,
			     MDM_FAST_SHUTD_ASSERTED);
	} else {
		HL7800_IO_DBG_LOG("MDM_FAST_SHUTD -> NOT_ASSERTED");
		gpio_pin_set(ictx.gpio_port_dev[MDM_FAST_SHUTD],
			     pinconfig[MDM_FAST_SHUTD].pin,
			     MDM_FAST_SHUTD_NOT_ASSERTED);
	}
}

static void allow_sleep_work_callback(struct k_work *item)
{
	ARG_UNUSED(item);
	LOG_DBG("Allow sleep");
	ictx.allow_sleep = true;
	set_sleep_state(ictx.desired_sleep_level);
	modem_assert_wake(false);
}

static void allow_sleep(bool allow)
{
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	if (allow) {
		k_work_reschedule_for_queue(&hl7800_workq,
					    &ictx.allow_sleep_work,
					    K_MSEC(CONFIG_MODEM_HL7800_ALLOW_SLEEP_DELAY_MS));
	} else {
		LOG_DBG("Keep awake");
		k_work_cancel_delayable(&ictx.allow_sleep_work);
		ictx.allow_sleep = false;
		modem_assert_wake(true);
	}
#endif
}

static void event_handler(enum mdm_hl7800_event event, void *event_data)
{
	sys_snode_t *node;
	struct mdm_hl7800_callback_agent *agent;

	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&hl7800_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct mdm_hl7800_callback_agent, node);
		if (agent->event_callback != NULL) {
			agent->event_callback(event, event_data);
		}
	}
	k_mutex_unlock(&cb_lock);
}

void mdm_hl7800_get_signal_quality(int *rsrp, int *sinr)
{
	if (CONFIG_MODEM_HL7800_RSSI_RATE_SECONDS == 0) {
		rssi_query();
	}

	*rsrp = ictx.mdm_rssi;
	*sinr = ictx.mdm_sinr;
}

void mdm_hl7800_wakeup(bool wakeup)
{
	allow_sleep(!wakeup);
}

/* Send an AT command with a series of response handlers */
static int send_at_cmd(struct hl7800_socket *sock, const uint8_t *data,
		       k_timeout_t timeout, int retries, bool no_id_resp)
{
	int ret = 0;

	ictx.last_error = 0;

	do {
		if (!sock) {
			k_sem_reset(&ictx.response_sem);
			ictx.last_socket_id = 0;
		} else {
			k_sem_reset(&sock->sock_send_sem);
			ictx.last_socket_id = sock->socket_id;
		}
		if (no_id_resp) {
			strncpy(ictx.no_id_resp_cmd, data,
				sizeof(ictx.no_id_resp_cmd) - 1);
			ictx.search_no_id_resp = true;
		}

		LOG_DBG("OUT: [%s]", log_strdup(data));
		mdm_receiver_send(&ictx.mdm_ctx, data, strlen(data));
		mdm_receiver_send(&ictx.mdm_ctx, "\r", 1);

		if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
			goto done;
		}

		if (!sock) {
			ret = k_sem_take(&ictx.response_sem, timeout);
		} else {
			ret = k_sem_take(&sock->sock_send_sem, timeout);
		}

		if (ret == 0) {
			ret = ictx.last_error;
		} else if (ret == -EAGAIN) {
			ret = -ETIMEDOUT;
		}

		retries--;
		if (retries < 0) {
			retries = 0;
		}
	} while (ret != 0 && retries > 0);
done:
	ictx.search_no_id_resp = false;
	return ret;
}

static int wakeup_hl7800(void)
{
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	int ret;

	allow_sleep(false);

	/* If modem is in sleep mode (not hibernate),
	 * then it can respond in ~10 ms.
	 */
	if (ictx.desired_sleep_level == HL7800_SLEEP_SLEEP) {
		k_sleep(MDM_WAKE_TO_CHECK_CTS_DELAY_MS);
	}

	if (!is_cmd_ready()) {
		LOG_DBG("Waiting to wakeup");
		ret = k_sem_take(&ictx.mdm_awake, MDM_WAKEUP_TIME);
		if (ret) {
			LOG_DBG("Err waiting for wakeup: %d", ret);
		}
	}
#endif
	return 0;
}

int32_t mdm_hl7800_send_at_cmd(const uint8_t *data)
{
	int ret;

	if (!data) {
		return -EINVAL;
	}

	hl7800_lock();
	wakeup_hl7800();
	ictx.last_socket_id = 0;
	ret = send_at_cmd(NULL, data, MDM_CMD_SEND_TIMEOUT, 0, false);
	allow_sleep(true);
	hl7800_unlock();
	return ret;
}

/* The access point name (and username and password) are stored in the modem's
 * non-volatile memory.
 */
int32_t mdm_hl7800_update_apn(char *access_point_name)
{
	int ret = -EINVAL;

	hl7800_lock();
	wakeup_hl7800();
	ictx.last_socket_id = 0;
	ret = write_apn(access_point_name);
	allow_sleep(true);
	hl7800_unlock();

	if (ret >= 0) {
		/* After a reset the APN will be re-read from the modem
		 * and an event will be generated.
		 */
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.mdm_reset_work,
					    K_NO_WAIT);
	}
	return ret;
}

bool mdm_hl7800_valid_rat(uint8_t value)
{
	if ((value == MDM_RAT_CAT_M1) || (value == MDM_RAT_CAT_NB1)) {
		return true;
	}
	return false;
}

int32_t mdm_hl7800_update_rat(enum mdm_hl7800_radio_mode value)
{
	int ret = -EINVAL;

	if (value == ictx.mdm_rat) {
		/* The set command will fail (in the modem)
		 * if the RAT isn't different.
		 */
		return 0;
	} else if (!mdm_hl7800_valid_rat(value)) {
		return ret;
	}

	hl7800_lock();
	wakeup_hl7800();
	ictx.last_socket_id = 0;

	if (value == MDM_RAT_CAT_M1) {
		if (ictx.new_rat_cmd_support) {
			SEND_AT_CMD_ONCE_EXPECT_OK(SET_RAT_M1_CMD);
		} else {
			SEND_AT_CMD_ONCE_EXPECT_OK(SET_RAT_M1_CMD_LEGACY);
		}
	} else { /* MDM_RAT_CAT_NB1 */
		if (ictx.new_rat_cmd_support) {
			SEND_AT_CMD_ONCE_EXPECT_OK(SET_RAT_NB1_CMD);
		} else {
			SEND_AT_CMD_ONCE_EXPECT_OK(SET_RAT_NB1_CMD_LEGACY);
		}
	}

error:
	if (ret >= 0) {
		/* Changing the RAT causes the modem to reset. */
		ret = modem_boot_handler("RAT changed");
	}

	allow_sleep(true);
	hl7800_unlock();

	/* A reset and reconfigure ensures the modem configuration and
	 * state are valid
	 */
	if (ret >= 0) {
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.mdm_reset_work,
					    K_NO_WAIT);
	}

	return ret;
}

int32_t mdm_hl7800_get_local_time(struct tm *tm, int32_t *offset)
{
	int ret;

	ictx.local_time_valid = false;

	hl7800_lock();
	wakeup_hl7800();
	ictx.last_socket_id = 0;
	ret = send_at_cmd(NULL, "AT+CCLK?", MDM_CMD_SEND_TIMEOUT, 0, false);
	allow_sleep(true);
	if (ictx.local_time_valid) {
		memcpy(tm, &ictx.local_time, sizeof(struct tm));
		memcpy(offset, &ictx.local_time_offset, sizeof(*offset));
	} else {
		ret = -EIO;
	}
	hl7800_unlock();
	return ret;
}

int32_t mdm_hl7800_get_operator_index(void)
{
	int ret;

	hl7800_lock();
	wakeup_hl7800();
	ictx.last_socket_id = 0;
	ret = send_at_cmd(NULL, "AT+KCARRIERCFG?", MDM_CMD_SEND_TIMEOUT, 0,
			  false);
	allow_sleep(true);
	hl7800_unlock();
	if (ret < 0) {
		return ret;
	} else {
		return ictx.operator_index;
	}
}

int32_t mdm_hl7800_get_functionality(void)
{
	int ret;

	hl7800_lock();
	wakeup_hl7800();
	ictx.last_socket_id = 0;
	ret = send_at_cmd(NULL, "AT+CFUN?", MDM_CMD_SEND_TIMEOUT, 0, false);
	allow_sleep(true);
	hl7800_unlock();

	if (ret < 0) {
		return ret;
	} else {
		return ictx.functionality;
	}
}

int32_t mdm_hl7800_set_functionality(enum mdm_hl7800_functionality mode)
{
	int ret;
	char buf[sizeof("AT+CFUN=###,0")] = { 0 };

	hl7800_lock();
	wakeup_hl7800();
	snprintk(buf, sizeof(buf), "AT+CFUN=%u,0", mode);
	ictx.last_socket_id = 0;
	ret = send_at_cmd(NULL, buf, MDM_CMD_SEND_TIMEOUT,
			  MDM_DEFAULT_AT_CMD_RETRIES, false);
	allow_sleep(true);
	hl7800_unlock();

	return ret;
}

#ifdef CONFIG_MODEM_HL7800_GPS
int32_t mdm_hl7800_set_gps_rate(uint32_t rate)
{
	int ret = -1;

	hl7800_lock();
	wakeup_hl7800();
	ictx.gps_query_location_rate_seconds = rate;

	/* Stopping first allows changing the rate between two non-zero values.
	 * Ignore error if GNSS isn't running.
	 */
	SEND_AT_CMD_IGNORE_ERROR("AT+GNSSSTOP");

	if (rate == 0) {
		SEND_AT_CMD_EXPECT_OK("AT+CFUN=1,0");
	} else {
		/* Navigation doesn't work when LTE is on. */
		SEND_AT_CMD_EXPECT_OK("AT+CFUN=4,0");

		SEND_AT_CMD_EXPECT_OK("AT+GNSSCONF=1,1");

		if (IS_ENABLED(CONFIG_MODEM_HL7800_USE_GLONASS)) {
			SEND_AT_CMD_EXPECT_OK("AT+GNSSCONF=10,1");
		}
		/* Enable all NMEA sentences */
		SEND_AT_CMD_EXPECT_OK("AT+GNSSNMEA=0,1000,0,1FF");
		/* Enable GPS */
		SEND_AT_CMD_EXPECT_OK("AT+GNSSSTART=0");
	}

error:
	if (rate && ret == 0) {
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.gps_work,
					    K_SECONDS(ictx.gps_query_location_rate_seconds));
	} else {
		k_work_cancel_delayable(&ictx.gps_work);
	}
	LOG_DBG("GPS status: %d rate: %u", ret, rate);

	allow_sleep(true);
	hl7800_unlock();
	return ret;
}
#endif /* CONFIG_MODEM_HL7800_GPS */

#ifdef CONFIG_MODEM_HL7800_POLTE
int32_t mdm_hl7800_polte_register(void)
{
	int ret = -1;

	hl7800_lock();
	wakeup_hl7800();
	/* register for events */
	SEND_AT_CMD_EXPECT_OK("AT%POLTEEV=\"REGISTER\",1");
	SEND_AT_CMD_EXPECT_OK("AT%POLTEEV=\"LOCATION\",1");
	/* register with polte.io */
	SEND_AT_CMD_EXPECT_OK("AT%POLTECMD=\"REGISTER\"");
error:
	LOG_DBG("PoLTE register status: %d", ret);
	allow_sleep(true);
	hl7800_unlock();
	return ret;
}

int32_t mdm_hl7800_polte_enable(char *user, char *password)
{
	int ret = -1;
	char buf[sizeof(MDM_HL7800_SET_POLTE_USER_AND_PASSWORD_FMT_STR) +
		 MDM_HL7800_MAX_POLTE_USER_ID_SIZE + MDM_HL7800_MAX_POLTE_PASSWORD_SIZE] = { 0 };

	hl7800_lock();
	wakeup_hl7800();

	/* register for events */
	SEND_AT_CMD_EXPECT_OK("AT%POLTEEV=\"REGISTER\",1");
	SEND_AT_CMD_EXPECT_OK("AT%POLTEEV=\"LOCATION\",1");
	/*  restore user and password (not saved in NV by modem) */
	snprintk(buf, sizeof(buf), MDM_HL7800_SET_POLTE_USER_AND_PASSWORD_FMT_STR, user, password);
	ret = send_at_cmd(NULL, buf, MDM_CMD_SEND_TIMEOUT, MDM_DEFAULT_AT_CMD_RETRIES, false);

error:
	LOG_DBG("PoLTE register status: %d", ret);
	allow_sleep(true);
	hl7800_unlock();
	return ret;
}

int32_t mdm_hl7800_polte_locate(void)
{
	int ret = -1;

	hl7800_lock();
	wakeup_hl7800();
	SEND_AT_CMD_EXPECT_OK("AT%POLTECMD=\"LOCATE\",2,1");
error:
	LOG_DBG("PoLTE locate status: %d", ret);
	allow_sleep(true);
	hl7800_unlock();
	return ret;
}

#endif /* CONFIG_MODEM_HL7800_POLTE */

/**
 * @brief Perform a site survey.
 *
 */
int32_t mdm_hl7800_perform_site_survey(void)
{
	int ret;

	hl7800_lock();
	wakeup_hl7800();
	ret = send_at_cmd(NULL, "at%meas=\"97\"", MDM_CMD_SEND_TIMEOUT, 0, false);
	allow_sleep(true);
	hl7800_unlock();
	return ret;
}

void mdm_hl7800_generate_status_events(void)
{
	hl7800_lock();
	generate_startup_state_event();
	generate_network_state_event();
	generate_sleep_state_event();
#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	generate_fota_state_event();
#endif
	event_handler(HL7800_EVENT_RSSI, &ictx.mdm_rssi);
	event_handler(HL7800_EVENT_SINR, &ictx.mdm_sinr);
	event_handler(HL7800_EVENT_APN_UPDATE, &ictx.mdm_apn);
	event_handler(HL7800_EVENT_RAT, &ictx.mdm_rat);
	event_handler(HL7800_EVENT_BANDS, ictx.mdm_bands_string);
	event_handler(HL7800_EVENT_ACTIVE_BANDS, ictx.mdm_active_bands_string);
	event_handler(HL7800_EVENT_REVISION, ictx.mdm_revision);
	hl7800_unlock();
}

static int send_data(struct hl7800_socket *sock, struct net_pkt *pkt)
{
	int ret;
	struct net_buf *frag;
	char dst_addr[NET_IPV6_ADDR_LEN];
	char buf[sizeof("AT+KUDPSND=##,\"" IPV6_ADDR_FORMAT "\",#####,####")];
	size_t send_len, actual_send_len;

	send_len = 0, actual_send_len = 0;

	if (!sock) {
		return -EINVAL;
	}

	ictx.last_error = 0;
	sock->state = SOCK_TX;

	frag = pkt->frags;
	send_len = net_buf_frags_len(frag);
	/* start sending data */
	k_sem_reset(&sock->sock_send_sem);
	if (sock->type == SOCK_STREAM) {
		snprintk(buf, sizeof(buf), "AT+KTCPSND=%d,%zu", sock->socket_id,
			 send_len);
	} else {
		if (!net_addr_ntop(sock->family, &net_sin(&sock->dst)->sin_addr,
				   dst_addr, sizeof(dst_addr))) {
			LOG_ERR("Invalid dst addr");
			return -EINVAL;
		}
		snprintk(buf, sizeof(buf), "AT+KUDPSND=%d,\"%s\",%u,%zu",
			 sock->socket_id, dst_addr,
			 net_sin(&sock->dst)->sin_port, send_len);
	}
	send_at_cmd(sock, buf, K_NO_WAIT, 0, false);

	/* wait for CONNECT  or error */
	ret = k_sem_take(&sock->sock_send_sem, MDM_IP_SEND_RX_TIMEOUT);
	if (ret) {
		LOG_ERR("Err waiting for CONNECT (%d)", ret);
		goto done;
	}
	/* check for error */
	if (ictx.last_error != 0) {
		ret = ictx.last_error;
		LOG_ERR("AT+K**PSND (%d)", ret);
		goto done;
	}

	/* Loop through packet data and send */
	while (frag) {
		actual_send_len += frag->len;
		mdm_receiver_send(&ictx.mdm_ctx, frag->data, frag->len);
		frag = frag->frags;
	}
	if (actual_send_len != send_len) {
		LOG_WRN("AT+K**PSND act: %zd exp: %zd", actual_send_len,
			send_len);
	}
	LOG_DBG("Sent %zu bytes", actual_send_len);

	/* Send EOF pattern to terminate data */
	k_sem_reset(&sock->sock_send_sem);
	mdm_receiver_send(&ictx.mdm_ctx, EOF_PATTERN, strlen(EOF_PATTERN));
	ret = k_sem_take(&sock->sock_send_sem, MDM_IP_SEND_RX_TIMEOUT);
	if (ret == 0) {
		ret = ictx.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}
done:
	if (sock->type == SOCK_STREAM) {
		sock->state = SOCK_CONNECTED;
	} else {
		sock->state = SOCK_IDLE;
	}

	return ret;
}

/*** NET_BUF HELPERS ***/

static bool is_crlf(uint8_t c)
{
	if (c == '\n' || c == '\r') {
		return true;
	} else {
		return false;
	}
}

static void net_buf_skipcrlf(struct net_buf **buf)
{
	/* chop off any /n or /r */
	while (*buf && is_crlf(*(*buf)->data)) {
		net_buf_pull_u8(*buf);
		if (!(*buf)->len) {
			*buf = net_buf_frag_del(NULL, *buf);
		}
	}
}

static uint16_t net_buf_findcrlf(struct net_buf *buf, struct net_buf **frag)
{
	uint16_t len = 0U, pos = 0U;

	while (buf && !is_crlf(*(buf->data + pos))) {
		if (pos + 1 >= buf->len) {
			len += buf->len;
			buf = buf->frags;
			pos = 0U;
		} else {
			pos++;
		}
	}

	if (buf && is_crlf(*(buf->data + pos))) {
		len += pos;
		*frag = buf;
		return len;
	}

	return 0;
}

static uint8_t net_buf_get_u8(struct net_buf **buf)
{
	uint8_t val = net_buf_pull_u8(*buf);

	if (!(*buf)->len) {
		*buf = net_buf_frag_del(NULL, *buf);
	}
	return val;
}

static uint32_t net_buf_remove(struct net_buf **buf, uint32_t len)
{
	uint32_t to_remove;
	uint32_t removed = 0;

	while (*buf && len > 0) {
		to_remove = (*buf)->len;
		if (to_remove > len) {
			to_remove = len;
		}
		net_buf_pull(*buf, to_remove);
		removed += to_remove;
		len -= to_remove;
		if (!(*buf)->len) {
			*buf = net_buf_frag_del(NULL, *buf);
		}
	}
	return removed;
}

/*** UDP / TCP Helper Function ***/

/* Setup IP header data to be used by some network applications.
 * While much is dummy data, some fields such as dst, port and family are
 * important.
 * Return the IP + protocol header length.
 */
static int pkt_setup_ip_data(struct net_pkt *pkt, struct hl7800_socket *sock)
{
	int hdr_len = 0;
	uint16_t src_port = 0U, dst_port = 0U;
#if defined(CONFIG_NET_TCP)
	struct net_tcp_hdr *tcp;
#endif

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_create(
			    pkt,
			    &((struct sockaddr_in6 *)&sock->dst)->sin6_addr,
			    &((struct sockaddr_in6 *)&sock->src)->sin6_addr)) {
			return -1;
		}
		src_port = ntohs(net_sin6(&sock->src)->sin6_port);
		dst_port = ntohs(net_sin6(&sock->dst)->sin6_port);

		hdr_len = sizeof(struct net_ipv6_hdr);
	}
#endif
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_create(
			    pkt, &((struct sockaddr_in *)&sock->dst)->sin_addr,
			    &((struct sockaddr_in *)&sock->src)->sin_addr)) {
			return -1;
		}
		src_port = ntohs(net_sin(&sock->src)->sin_port);
		dst_port = ntohs(net_sin(&sock->dst)->sin_port);

		hdr_len = sizeof(struct net_ipv4_hdr);
	}
#endif

#if defined(CONFIG_NET_UDP)
	if (sock->ip_proto == IPPROTO_UDP) {
		if (net_udp_create(pkt, dst_port, src_port)) {
			return -1;
		}

		hdr_len += NET_UDPH_LEN;
	}
#endif
#if defined(CONFIG_NET_TCP)
	if (sock->ip_proto == IPPROTO_TCP) {
		NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);

		tcp = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
		if (!tcp) {
			return -1;
		}

		(void)memset(tcp, 0, NET_TCPH_LEN);

		/* Setup TCP header */
		tcp->src_port = dst_port;
		tcp->dst_port = src_port;

		if (net_pkt_set_data(pkt, &tcp_access)) {
			return -1;
		}

		hdr_len += NET_TCPH_LEN;
	}
#endif /* CONFIG_NET_TCP */

	return hdr_len;
}

/*** MODEM RESPONSE HANDLERS ***/

static uint32_t wait_for_modem_data(struct net_buf **buf, uint32_t current_len,
				    uint32_t expected_len)
{
	uint32_t waitForDataTries = 0;

	while ((current_len < expected_len) &&
	       (waitForDataTries < MDM_WAIT_FOR_DATA_RETRIES)) {
		LOG_DBG("cur:%d, exp:%d", current_len, expected_len);
		k_sleep(MDM_WAIT_FOR_DATA_TIME);
		current_len += hl7800_read_rx(buf);
		waitForDataTries++;
	}

	return current_len;
}

static uint32_t wait_for_modem_data_and_newline(struct net_buf **buf,
						uint32_t current_len,
						uint32_t expected_len)
{
	return wait_for_modem_data(buf, current_len,
				   (expected_len + strlen("\r\n")));
}

/* Handler: AT+CGMI */
static bool on_cmd_atcmdinfo_manufacturer(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t out_len;
	int len_no_null = MDM_MANUFACTURER_LENGTH - 1;

	/* make sure revision data is received
	 *  waiting for: Sierra Wireless\r\n
	 */
	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_MANUFACTURER_LENGTH);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find mfg end");
		goto done;
	}
	if (len < len_no_null) {
		LOG_WRN("mfg too short (len:%d)", len);
	} else if (len > len_no_null) {
		LOG_WRN("mfg too long (len:%d)", len);
		len = MDM_MANUFACTURER_LENGTH;
	}

	out_len = net_buf_linearize(ictx.mdm_manufacturer,
				    sizeof(ictx.mdm_manufacturer) - 1, *buf, 0,
				    len);
	ictx.mdm_manufacturer[out_len] = 0;
	LOG_INF("Manufacturer: %s", log_strdup(ictx.mdm_manufacturer));
done:
	return true;
}

/* Handler: AT+CGMM */
static bool on_cmd_atcmdinfo_model(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t out_len;
	int len_no_null = MDM_MODEL_LENGTH - 1;

	/* make sure model data is received
	 *  waiting for: HL7800\r\n
	 */
	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_MODEL_LENGTH);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find model end");
		goto done;
	}
	if (len < len_no_null) {
		LOG_WRN("model too short (len:%d)", len);
	} else if (len > len_no_null) {
		LOG_WRN("model too long (len:%d)", len);
		len = MDM_MODEL_LENGTH;
	}

	out_len = net_buf_linearize(ictx.mdm_model, sizeof(ictx.mdm_model) - 1,
				    *buf, 0, len);
	ictx.mdm_model[out_len] = 0;
	LOG_INF("Model: %s", log_strdup(ictx.mdm_model));
done:
	return true;
}

/* Handler: AT+CGMR */
static bool on_cmd_atcmdinfo_revision(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t out_len;

	/* make sure revision data is received
	 *  waiting for something like: AHL7800.1.2.3.1.20171211\r\n
	 */
	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_REVISION_MAX_SIZE);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find rev end");
		goto done;
	}
	if (len == 0) {
		LOG_WRN("revision not found");
	} else if (len > MDM_HL7800_REVISION_MAX_STRLEN) {
		LOG_WRN("revision too long (len:%d)", len);
		len = MDM_HL7800_REVISION_MAX_STRLEN;
	}

	out_len = net_buf_linearize(
		ictx.mdm_revision, sizeof(ictx.mdm_revision) - 1, *buf, 0, len);
	ictx.mdm_revision[out_len] = 0;
	LOG_INF("Revision: %s", log_strdup(ictx.mdm_revision));
	event_handler(HL7800_EVENT_REVISION, ictx.mdm_revision);
done:
	return true;
}

/* Handler: AT+CGSN */
static bool on_cmd_atcmdinfo_imei(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t out_len;

	/* make sure IMEI data is received
	 *  waiting for: ###############\r\n
	 */
	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_IMEI_SIZE);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find IMEI end");
		goto done;
	}
	if (len < MDM_HL7800_IMEI_STRLEN) {
		LOG_WRN("IMEI too short (len:%d)", len);
	} else if (len > MDM_HL7800_IMEI_STRLEN) {
		LOG_WRN("IMEI too long (len:%d)", len);
		len = MDM_HL7800_IMEI_STRLEN;
	}

	out_len = net_buf_linearize(ictx.mdm_imei, sizeof(ictx.mdm_imei) - 1,
				    *buf, 0, len);
	ictx.mdm_imei[out_len] = 0;

	LOG_INF("IMEI: %s", log_strdup(ictx.mdm_imei));
done:
	return true;
}

/* Handler: +CCID: <ICCID> */
static bool on_cmd_atcmdinfo_iccid(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t out_len;

	/* make sure ICCID data is received
	 *  waiting for: <ICCID>\r\n
	 */
	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_ICCID_SIZE);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find ICCID end");
		goto done;
	}
	if (len > MDM_HL7800_ICCID_STRLEN) {
		LOG_WRN("ICCID too long (len:%d)", len);
		len = MDM_HL7800_ICCID_STRLEN;
	}

	out_len = net_buf_linearize(ictx.mdm_iccid, MDM_HL7800_ICCID_STRLEN,
				    *buf, 0, len);
	ictx.mdm_iccid[out_len] = 0;

	LOG_INF("ICCID: %s", log_strdup(ictx.mdm_iccid));
done:
	return true;
}

static bool on_cmd_atcmdinfo_imsi(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t out_len;

	/* The handler for the IMSI is based on the command.
	 *  waiting for: <IMSI>\r\n
	 */
	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_IMSI_MIN_STR_SIZE);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find IMSI end");
		goto done;
	}
	if (len > MDM_HL7800_IMSI_MAX_STRLEN) {
		LOG_WRN("IMSI too long (len:%d)", len);
		len = MDM_HL7800_IMSI_MAX_STRLEN;
	}

	out_len = net_buf_linearize(ictx.mdm_imsi, MDM_HL7800_IMSI_MAX_STR_SIZE,
				    *buf, 0, len);
	ictx.mdm_imsi[out_len] = 0;

	if (strstr(ictx.mdm_imsi, "ERROR") != NULL) {
		LOG_ERR("Unable to read IMSI");
		memset(ictx.mdm_imsi, 0, sizeof(ictx.mdm_imsi));
	}

	LOG_INF("IMSI: %s", log_strdup(ictx.mdm_imsi));
done:
	return true;
}

static void dns_work_cb(struct k_work *work)
{
#if defined(CONFIG_DNS_RESOLVER) && !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	int ret;
	struct dns_resolve_context *dnsCtx;
	static const char *const dns_servers_str[] = { ictx.dns_v4_string,
#ifdef CONFIG_NET_IPV6
						       ictx.dns_v6_string,
#endif
						       NULL };

	if (ictx.iface && net_if_is_up(ictx.iface)) {
		/* set new DNS addr in DNS resolver */
		LOG_DBG("Refresh DNS resolver");
		dnsCtx = dns_resolve_get_default();

		ret = dns_resolve_reconfigure(dnsCtx, (const char **)dns_servers_str, NULL);
		if (ret < 0) {
			LOG_ERR("dns_resolve_init fail (%d)", ret);
			return;
		}
	}
#endif
}

char *mdm_hl7800_get_iccid(void)
{
	return ictx.mdm_iccid;
}

char *mdm_hl7800_get_sn(void)
{
	return ictx.mdm_sn;
}

char *mdm_hl7800_get_imei(void)
{
	return ictx.mdm_imei;
}

char *mdm_hl7800_get_fw_version(void)
{
	return ictx.mdm_revision;
}

char *mdm_hl7800_get_imsi(void)
{
	return ictx.mdm_imsi;
}

/* Convert HL7800 IPv6 address string in format
 * a01.a02.a03.a04.a05.a06.a07.a08.a09.a10.a11.a12.a13.a14.a15.a16 to
 * an IPv6 address.
 */
static int hl7800_net_addr6_pton(const char *src, struct in6_addr *dst)
{
	int num_sections = 8;
	int i, len;
	uint16_t ipv6_section;

	len = strlen(src);
	for (i = 0; i < len; i++) {
		if (!(src[i] >= '0' && src[i] <= '9') && src[i] != '.') {
			return -EINVAL;
		}
	}

	for (i = 0; i < num_sections; i++) {
		if (!src || *src == '\0') {
			return -EINVAL;
		}

		ipv6_section = (uint16_t)strtol(src, NULL, 10);
		src = strchr(src, '.');
		src++;
		if (!src || *src == '\0') {
			return -EINVAL;
		}
		ipv6_section = (ipv6_section << 8) | (uint16_t)strtol(src, NULL, 10);
		UNALIGNED_PUT(htons(ipv6_section), &dst->s6_addr16[i]);

		src = strchr(src, '.');
		if (src) {
			src++;
		} else {
			if (i < num_sections - 1) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

/* Handler: +CGCONTRDP: <cid>,<bearer_id>,<apn>,<local_addr and subnet_mask>,
 *			<gw_addr>,<DNS_prim_addr>,<DNS_sec_addr>
 */
static bool on_cmd_atcmdinfo_ipaddr(struct net_buf **buf, uint16_t len)
{
	int ret;
	int num_delims = CGCONTRDP_RESPONSE_NUM_DELIMS;
	char *delims[CGCONTRDP_RESPONSE_NUM_DELIMS];
	size_t out_len;
	char value[MDM_IP_INFO_RESP_SIZE];
	char *search_start, *addr_start, *sm_start;
	struct in_addr new_ipv4_addr;
	struct in6_addr new_ipv6_addr;
	bool is_ipv4;
	int addr_len;
	char temp_addr_str[HL7800_IPV6_ADDR_LEN];
	k_timeout_t delay;

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;
	search_start = value;
	LOG_DBG("IP info: %s", log_strdup(value));

	/* find all delimiters (,) */
	for (int i = 0; i < num_delims; i++) {
		delims[i] = strchr(search_start, ',');
		if (!delims[i]) {
			LOG_ERR("Could not find delim %d, val: %s", i,
				log_strdup(value));
			return true;
		}
		/* Start next search after current delim location */
		search_start = delims[i] + 1;
	}

	/* determine if IPv4 or IPv6 by checking length of ip address plus
	 * gateway string.
	 */
	is_ipv4 = false;
	addr_len = delims[3] - delims[2];
	LOG_DBG("IP string len: %d", addr_len);
	if (addr_len <= (NET_IPV4_ADDR_LEN * 2)) {
		is_ipv4 = true;
	}

	/* Find start of subnet mask */
	addr_start = delims[2] + 1;
	if (is_ipv4) {
		num_delims = 4;
	} else {
		num_delims = 16;
	}
	search_start = addr_start;
	sm_start = addr_start;
	for (int i = 0; i < num_delims; i++) {
		sm_start = strchr(search_start, '.');
		if (!sm_start) {
			LOG_ERR("Could not find submask start");
			return true;
		}
		/* Start next search after current delim location */
		search_start = sm_start + 1;
	}

	/* get new IP addr */
	addr_len = sm_start - addr_start;
	strncpy(temp_addr_str, addr_start, addr_len);
	temp_addr_str[addr_len] = 0;
	LOG_DBG("IP addr: %s", log_strdup(temp_addr_str));
	if (is_ipv4) {
		ret = net_addr_pton(AF_INET, temp_addr_str, &new_ipv4_addr);
	} else {
		ret = hl7800_net_addr6_pton(temp_addr_str, &new_ipv6_addr);
	}
	if (ret < 0) {
		LOG_ERR("Invalid IP addr");
		return true;
	}

	if (is_ipv4) {
		/* move past the '.' */
		sm_start += 1;
		/* store new subnet mask */
		addr_len = delims[3] - sm_start;
		strncpy(temp_addr_str, sm_start, addr_len);
		temp_addr_str[addr_len] = 0;
		ret = net_addr_pton(AF_INET, temp_addr_str, &ictx.subnet);
		if (ret < 0) {
			LOG_ERR("Invalid subnet");
			return true;
		}

		/* store new gateway */
		addr_start = delims[3] + 1;
		addr_len = delims[4] - addr_start;
		strncpy(temp_addr_str, addr_start, addr_len);
		temp_addr_str[addr_len] = 0;
		ret = net_addr_pton(AF_INET, temp_addr_str, &ictx.gateway);
		if (ret < 0) {
			LOG_ERR("Invalid gateway");
			return true;
		}
	}

	/* store new dns */
	addr_start = delims[4] + 1;
	addr_len = delims[5] - addr_start;
	if (is_ipv4) {
		strncpy(ictx.dns_v4_string, addr_start, addr_len);
		ictx.dns_v4_string[addr_len] = 0;
		ret = net_addr_pton(AF_INET, ictx.dns_v4_string, &ictx.dns_v4);
		LOG_DBG("IPv4 DNS addr: %s", log_strdup(ictx.dns_v4_string));
	}
#ifdef CONFIG_NET_IPV6
	else {
		/* store HL7800 formatted IPv6 DNS string temporarily */
		strncpy(ictx.dns_v6_string, addr_start, addr_len);

		ret = hl7800_net_addr6_pton(ictx.dns_v6_string, &ictx.dns_v6);
		net_addr_ntop(AF_INET6, &ictx.dns_v6, ictx.dns_v6_string,
			      sizeof(ictx.dns_v6_string));
		LOG_DBG("IPv6 DNS addr: %s", log_strdup(ictx.dns_v6_string));
	}
#endif
	if (ret < 0) {
		LOG_ERR("Invalid dns");
		return true;
	}

	if (ictx.iface) {
		if (is_ipv4) {
#ifdef CONFIG_NET_IPV4
			/* remove the current IPv4 addr before adding a new one.
			 * We dont care if it is successful or not.
			 */
			net_if_ipv4_addr_rm(ictx.iface, &ictx.ipv4Addr);

			if (!net_if_ipv4_addr_add(ictx.iface, &new_ipv4_addr, NET_ADDR_DHCP, 0)) {
				LOG_ERR("Cannot set iface IPv4 addr");
			}

			net_if_ipv4_set_netmask(ictx.iface, &ictx.subnet);
			net_if_ipv4_set_gw(ictx.iface, &ictx.gateway);
#endif
			/* store the new IP addr */
			net_ipaddr_copy(&ictx.ipv4Addr, &new_ipv4_addr);
		} else {
#if CONFIG_NET_IPV6
			net_if_ipv6_addr_rm(ictx.iface, &ictx.ipv6Addr);
			if (!net_if_ipv6_addr_add(ictx.iface, &new_ipv6_addr, NET_ADDR_AUTOCONF,
						  0)) {
				LOG_ERR("Cannot set iface IPv6 addr");
			}
#endif
		}

		/* start DNS update work */
		delay = K_NO_WAIT;
		if (!ictx.initialized) {
			/* Delay this in case the network
			 * stack is still starting up
			 */
			delay = K_SECONDS(DNS_WORK_DELAY_SECS);
		}
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.dns_work,
					    delay);
	} else {
		LOG_ERR("iface NULL");
	}

	return true;
}

/* Handler1: +COPS: <mode>[,<format>,<oper>[,<AcT>]]
 *
 * Handler2:
 * +COPS: [list of supported (<stat>, long alphanumeric <oper>, short
 * alphanumeric <oper>, numeric <oper>[,< AcT>])s][,,
 * (list of supported <mode>s),(list of supported <format>s)]
 */
static bool on_cmd_atcmdinfo_operator_status(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];
	int num_delims = COPS_RESPONSE_NUM_DELIMS;
	char *delims[COPS_RESPONSE_NUM_DELIMS];
	char *search_start;
	int i;

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	/* For AT+COPS=?, result is most likely longer than size of log string */
	if (strchr(value, '(') != NULL) {
		LOG_HEXDUMP_DBG(value, out_len, "Operator: ");
		goto done;
	} else {
		LOG_INF("Operator: %s", log_strdup(value));
	}

	/* Process AT+COPS? */
	if (len == 1) {
		/* only mode was returned, there is no operator info */
		ictx.operator_status = NO_OPERATOR;
		goto done;
	}

	search_start = value;

	/* find all delimiters (,) */
	for (i = 0; i < num_delims; i++) {
		delims[i] = strchr(search_start, ',');
		if (!delims[i]) {
			LOG_ERR("Could not find delim %d, val: %s", i, log_strdup(value));
			goto done;
		}
		/* Start next search after current delim location */
		search_start = delims[i] + 1;
	}

	/* we found both delimiters, that means we have an operator */
	ictx.operator_status = REGISTERED;
done:
	return true;
}

/* Handler: +KGSN: T5640400011101 */
static bool on_cmd_atcmdinfo_serial_number(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	char value[MDM_SN_RESPONSE_LENGTH];
	size_t out_len;
	int sn_len;
	char *val_start;

	/* make sure SN# data is received.
	 *  we are waiting for: +KGSN: ##############\r\n
	 */
	wait_for_modem_data(buf, net_buf_frags_len(*buf),
			    MDM_SN_RESPONSE_LENGTH);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find sn end");
		goto done;
	}

	/* get msg data */
	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	/* find ':' */
	val_start = strchr(value, ':');
	if (!val_start) {
		LOG_ERR("Unable to find sn ':'");
		goto done;
	}
	/* Remove ": " chars */
	val_start += 2;

	sn_len = len - (val_start - value);
	if (sn_len < MDM_HL7800_SERIAL_NUMBER_STRLEN) {
		LOG_WRN("sn too short (len:%d)", sn_len);
	} else if (sn_len > MDM_HL7800_SERIAL_NUMBER_STRLEN) {
		LOG_WRN("sn too long (len:%d)", sn_len);
		sn_len = MDM_HL7800_SERIAL_NUMBER_STRLEN;
	}

	strncpy(ictx.mdm_sn, val_start, sn_len);
	ictx.mdm_sn[sn_len] = 0;
	LOG_INF("Serial #: %s", log_strdup(ictx.mdm_sn));
done:
	return true;
}

/* Handler: +KSRAT: # */
static bool on_cmd_radio_tech_status(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;
	ictx.mdm_rat = strtol(value, NULL, 10);
	LOG_INF("+KSRAT: %d", ictx.mdm_rat);
	event_handler(HL7800_EVENT_RAT, &ictx.mdm_rat);

	return true;
}

/* Handler: +KBNDCFG: #,####################### */
static bool on_cmd_radio_band_configuration(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];
	char n_tmp[sizeof("#########")];

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	if (value[0] != (ictx.mdm_rat == MDM_RAT_CAT_M1 ? '0' : '1')) {
		/* Invalid RAT */
		return true;
	} else if (strlen(value) < sizeof("#,###################")) {
		/* String size too short */
		return true;
	}

	memcpy(ictx.mdm_bands_string, &value[MDM_TOP_BAND_START_POSITION],
	       MDM_HL7800_LTE_BAND_STRLEN);

	memcpy(n_tmp, &value[MDM_TOP_BAND_START_POSITION], MDM_TOP_BAND_SIZE);
	n_tmp[MDM_TOP_BAND_SIZE] = 0;
	ictx.mdm_bands_top = strtoul(n_tmp, NULL, 16);

	memcpy(n_tmp, &value[MDM_MIDDLE_BAND_START_POSITION],
	       MDM_MIDDLE_BAND_SIZE);
	n_tmp[MDM_MIDDLE_BAND_SIZE] = 0;
	ictx.mdm_bands_middle = strtoul(n_tmp, NULL, 16);

	memcpy(n_tmp, &value[MDM_BOTTOM_BAND_START_POSITION],
	       MDM_BOTTOM_BAND_SIZE);
	n_tmp[MDM_BOTTOM_BAND_SIZE] = 0;
	ictx.mdm_bands_bottom = strtoul(n_tmp, NULL, 16);

	LOG_INF("Current band configuration: %04x %08x %08x",
		ictx.mdm_bands_top, ictx.mdm_bands_middle,
		ictx.mdm_bands_bottom);

	return true;
}

/* Handler: +KBND: #,####################### */
static bool on_cmd_radio_active_bands(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	if (strlen(value) < sizeof("#,###################")) {
		/* String size too short */
		return true;
	}

	memcpy(ictx.mdm_active_bands_string,
	       &value[MDM_TOP_BAND_START_POSITION], MDM_HL7800_LTE_BAND_STRLEN);
	event_handler(HL7800_EVENT_ACTIVE_BANDS, ictx.mdm_active_bands_string);

	return true;
}

static char *get_startup_state_string(enum mdm_hl7800_startup_state state)
{
	/* clang-format off */
	switch (state) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, READY);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, WAITING_FOR_ACCESS_CODE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, SIM_NOT_PRESENT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, SIMLOCK);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, UNRECOVERABLE_ERROR);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, UNKNOWN);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_STARTUP_STATE, INACTIVE_SIM);
	default:
		return "UNKNOWN";
	}
	/* clang-format on */
}

static void set_startup_state(enum mdm_hl7800_startup_state state)
{
	ictx.mdm_startup_state = state;
	generate_startup_state_event();
}

static void generate_startup_state_event(void)
{
	struct mdm_hl7800_compound_event event;

	event.code = ictx.mdm_startup_state;
	event.string = get_startup_state_string(ictx.mdm_startup_state);
	LOG_INF("Startup State: %s", event.string);
	event_handler(HL7800_EVENT_STARTUP_STATE_CHANGE, &event);
}

int mdm_hl7800_set_desired_sleep_level(enum mdm_hl7800_sleep level)
{
	int r = -EPERM;

#if CONFIG_MODEM_HL7800_LOW_POWER_MODE
	switch (level) {
	case HL7800_SLEEP_AWAKE:
	case HL7800_SLEEP_HIBERNATE:
	case HL7800_SLEEP_LITE_HIBERNATE:
	case HL7800_SLEEP_SLEEP:
		ictx.desired_sleep_level = level;
		r = 0;
		break;
	default:
		r = -EINVAL;
	}

	if (r == 0) {
		hl7800_lock();
		wakeup_hl7800();
		r = set_sleep_level();
		allow_sleep(true);
		hl7800_unlock();
	}
#endif

	return r;
}

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE

static void initialize_sleep_level(void)
{
	if (ictx.desired_sleep_level == HL7800_SLEEP_UNINITIALIZED) {
		if (IS_ENABLED(CONFIG_MODEM_HL7800_SLEEP_LEVEL_HIBERNATE)) {
			ictx.desired_sleep_level = HL7800_SLEEP_HIBERNATE;
		} else if (IS_ENABLED(CONFIG_MODEM_HL7800_SLEEP_LEVEL_LITE_HIBERNATE)) {
			ictx.desired_sleep_level = HL7800_SLEEP_LITE_HIBERNATE;
		} else if (IS_ENABLED(CONFIG_MODEM_HL7800_SLEEP_LEVEL_SLEEP)) {
			ictx.desired_sleep_level = HL7800_SLEEP_SLEEP;
		} else {
			ictx.desired_sleep_level = HL7800_SLEEP_AWAKE;
		}
	}
}

static int set_sleep_level(void)
{
	char cmd[sizeof("AT+KSLEEP=#,#,##")];
	static const char SLEEP_CMD_FMT[] = "AT+KSLEEP=%d,%d,%d";
	int delay = CONFIG_MODEM_HL7800_SLEEP_DELAY_AFTER_REBOOT;
	int ret = 0;

	/* AT+KSLEEP= <management>[,<level>[,<delay to sleep after reboot>]]
	 * management 1 means the HL7800 decides when it enters sleep mode
	 */
	switch (ictx.desired_sleep_level) {
	case HL7800_SLEEP_HIBERNATE:
		snprintk(cmd, sizeof(cmd), SLEEP_CMD_FMT, 1, 2, delay);
		break;
	case HL7800_SLEEP_LITE_HIBERNATE:
		snprintk(cmd, sizeof(cmd), SLEEP_CMD_FMT, 1, 1, delay);
		break;
	case HL7800_SLEEP_SLEEP:
		snprintk(cmd, sizeof(cmd), SLEEP_CMD_FMT, 1, 0, delay);
		break;
	default:
		/* don't sleep */
		snprintk(cmd, sizeof(cmd), SLEEP_CMD_FMT, 2, 0, delay);
		break;
	}

	SEND_AT_CMD_EXPECT_OK(cmd);

error:
	return ret;
}

#endif /* CONFIG_MODEM_HL7800_LOW_POWER_MODE */

static char *get_sleep_state_string(enum mdm_hl7800_sleep state)
{
	/* clang-format off */
	switch (state) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_SLEEP, UNINITIALIZED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_SLEEP, HIBERNATE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_SLEEP, LITE_HIBERNATE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_SLEEP, SLEEP);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_SLEEP, AWAKE);
	default:
		return "UNKNOWN";
	}
	/* clang-format on */
}

static void set_sleep_state(enum mdm_hl7800_sleep state)
{
	ictx.sleep_state = state;
	if (ictx.sleep_state != HL7800_SLEEP_AWAKE) {
		k_sem_reset(&ictx.mdm_awake);
	}
	generate_sleep_state_event();
}

static void generate_sleep_state_event(void)
{
	struct mdm_hl7800_compound_event event;

	event.code = ictx.sleep_state;
	event.string = get_sleep_state_string(ictx.sleep_state);
	LOG_INF("Sleep State: %s", event.string);
	event_handler(HL7800_EVENT_SLEEP_STATE_CHANGE, &event);
}

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
static char *get_fota_state_string(enum mdm_hl7800_fota_state state)
{
	/* clang-format off */
	switch (state) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, IDLE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, START);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, WIP);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, PAD);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, SEND_EOT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, FILE_ERROR);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, INSTALL);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, REBOOT_AND_RECONFIGURE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800_FOTA, COMPLETE);
	default:
		return "UNKNOWN";
	}
	/* clang-format on */
}

static void set_fota_state(enum mdm_hl7800_fota_state state)
{
	LOG_INF("FOTA state: %s->%s",
		log_strdup(get_fota_state_string(ictx.fw_update_state)),
		log_strdup(get_fota_state_string(state)));
	ictx.fw_update_state = state;
	generate_fota_state_event();
}

static void generate_fota_state_event(void)
{
	struct mdm_hl7800_compound_event event;

	event.code = ictx.fw_update_state;
	event.string = get_fota_state_string(ictx.fw_update_state);
	event_handler(HL7800_EVENT_FOTA_STATE, &event);
}

static void generate_fota_count_event(void)
{
	uint32_t count = ictx.fw_packet_count * XMODEM_DATA_SIZE;

	event_handler(HL7800_EVENT_FOTA_COUNT, &count);
}
#endif

/* Handler: +KSUP: # */
static bool on_cmd_startup_report(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];

	memset(value, 0, sizeof(value));
	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	if (out_len > 0) {
		set_startup_state(strtol(value, NULL, 10));
	} else {
		set_startup_state(HL7800_STARTUP_STATE_UNKNOWN);
	}

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	if (ictx.fw_updated) {
		ictx.fw_updated = false;
		set_fota_state(HL7800_FOTA_REBOOT_AND_RECONFIGURE);
		/* issue reset after a firmware update to reconfigure modem state */
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.mdm_reset_work,
					    K_NO_WAIT);
	} else
#endif
	{
		PRINT_AWAKE_MSG;
		ictx.wait_for_KSUP = false;
		ictx.mdm_startup_reporting_on = true;
		ictx.reconfig_IP_connection = true;
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
		mark_sockets_for_reconfig();
#endif
		set_sleep_state(HL7800_SLEEP_AWAKE);
		k_sem_give(&ictx.mdm_awake);
	}

	return true;
}

static bool profile_handler(struct net_buf **buf, uint16_t len,
			    bool active_profile)
{
	uint32_t size;
	int echo_state = -1;
	struct net_buf *frag = NULL;
	uint16_t line_length;
	char line[MAX_PROFILE_LINE_LENGTH];
	size_t output_length;

	/* Prepare net buffer for this parser. */
	net_buf_remove(buf, len);
	net_buf_skipcrlf(buf);

	size = wait_for_modem_data(buf, net_buf_frags_len(*buf),
				   sizeof(PROFILE_LINE_1));
	net_buf_skipcrlf(buf); /* remove any \r\n that are in the front */

	/* Parse configuration data to determine if echo is on/off. */
	line_length = net_buf_findcrlf(*buf, &frag);
	if (line_length) {
		memset(line, 0, sizeof(line));
		output_length = net_buf_linearize(line, SIZE_WITHOUT_NUL(line),
						  *buf, 0, line_length);
		LOG_DBG("length: %u: %s", line_length, log_strdup(line));

		/* Echo on off is the first thing on the line: E0, E1 */
		if (output_length >= SIZE_WITHOUT_NUL("E?")) {
			echo_state = (line[1] == '1') ? 1 : 0;
		}
	}
	LOG_DBG("echo: %d", echo_state);
	net_buf_remove(buf, line_length);
	net_buf_skipcrlf(buf);

	if (active_profile) {
		ictx.mdm_echo_is_on = (echo_state != 0);
	}

	/* Discard next line.  This waits for the longest possible response even
	 * though most registers won't have the value 0xFF. */
	size = wait_for_modem_data(buf, net_buf_frags_len(*buf),
				   sizeof(PROFILE_LINE_2));
	net_buf_skipcrlf(buf);
	len = net_buf_findcrlf(*buf, &frag);
	net_buf_remove(buf, len);
	net_buf_skipcrlf(buf);

	return false;
}

static bool on_cmd_atcmdinfo_active_profile(struct net_buf **buf, uint16_t len)
{
	return profile_handler(buf, len, true);
}

static bool on_cmd_atcmdinfo_stored_profile0(struct net_buf **buf, uint16_t len)
{
	return profile_handler(buf, len, false);
}

static bool on_cmd_atcmdinfo_stored_profile1(struct net_buf **buf, uint16_t len)
{
	return profile_handler(buf, len, false);
}

/* +WPPP: 1,1,"username","password" */
static bool on_cmd_atcmdinfo_pdp_authentication_cfg(struct net_buf **buf,
						    uint16_t len)
{
	struct net_buf *frag = NULL;
	uint16_t line_length;
	char line[MDM_HL7800_APN_CMD_MAX_SIZE];
	size_t output_length;
	size_t i;
	char *p;

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_APN_CMD_MAX_SIZE);

	line_length = net_buf_findcrlf(*buf, &frag);
	if (line_length) {
		memset(line, 0, sizeof(line));
		output_length = net_buf_linearize(line, SIZE_WITHOUT_NUL(line),
						  *buf, 0, line_length);
		LOG_DBG("length: %u: %s", line_length, log_strdup(line));
		if (output_length > 0) {
			memset(ictx.mdm_apn.username, 0,
			       sizeof(ictx.mdm_apn.username));
			memset(ictx.mdm_apn.password, 0,
			       sizeof(ictx.mdm_apn.password));

			i = 0;
			p = strchr(line, '"');
			if (p != NULL) {
				p += 1;
				i = 0;
				while ((p != NULL) && (*p != '"') &&
				       (i <
					MDM_HL7800_APN_USERNAME_MAX_STRLEN)) {
					ictx.mdm_apn.username[i++] = *p++;
				}
			}
			LOG_INF("APN Username: %s",
				log_strdup(ictx.mdm_apn.username));

			p = strchr(p + 1, '"');
			if (p != NULL) {
				p += 1;
				i = 0;
				while ((p != NULL) && (*p != '"') &&
				       (i <
					MDM_HL7800_APN_PASSWORD_MAX_STRLEN)) {
					ictx.mdm_apn.password[i++] = *p++;
				}
			}
			LOG_INF("APN Password: %s",
				log_strdup(ictx.mdm_apn.password));
		}
	}
	net_buf_remove(buf, line_length);
	net_buf_skipcrlf(buf);

	return false;
}

/* Only context 1 is used. Other contexts are unhandled.
 *
 * +CGDCONT: 1,"IP","access point name",,0,0,0,0,0,,0,,,,,
 */
static bool on_cmd_atcmdinfo_pdp_context(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	uint16_t line_length;
	char line[MDM_HL7800_APN_CMD_MAX_SIZE];
	size_t output_length;
	char *p;
	size_t i;

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_APN_CMD_MAX_SIZE);

	line_length = net_buf_findcrlf(*buf, &frag);
	if (line_length) {
		memset(line, 0, sizeof(line));
		output_length = net_buf_linearize(line, SIZE_WITHOUT_NUL(line),
						  *buf, 0, line_length);
		LOG_DBG("length: %u: %s", line_length, log_strdup(line));
		if (output_length > 0) {
			memset(ictx.mdm_apn.value, 0, sizeof(ictx.mdm_apn.value));
			memset(ictx.mdm_pdp_addr_fam, 0, MDM_ADDR_FAM_MAX_LEN);

			/* Address family after first , */
			p = strchr(line, ',');
			if (p == NULL) {
				LOG_WRN("Issue parsing APN response");
				goto done;
			}
			p += 2;
			i = 0;
			while ((p != NULL) && (*p != '"') && (i < MDM_ADDR_FAM_MAX_LEN)) {
				ictx.mdm_pdp_addr_fam[i++] = *p++;
			}
			LOG_DBG("PDP address family: %s", log_strdup(ictx.mdm_pdp_addr_fam));

			/* APN after second , " */
			p = strchr(p, ',');
			if (p == NULL) {
				LOG_WRN("Issue parsing APN response");
				goto done;
			}
			p++;
			if (*p == ',') {
				/* APN is blank */
				goto done;
			}
			if (*p == '"') {
				p++;
				i = 0;
				while ((p != NULL) && (*p != '"') &&
				       (i < MDM_HL7800_APN_MAX_STRLEN)) {
					ictx.mdm_apn.value[i++] = *p++;
				}
			}

			LOG_INF("APN: %s", log_strdup(ictx.mdm_apn.value));
		}
	}
done:
	net_buf_remove(buf, line_length);
	net_buf_skipcrlf(buf);

	return false;
}

static int hl7800_query_rssi(void)
{
	int ret;

	ret = send_at_cmd(NULL, "AT+KCELLMEAS=0", MDM_CMD_SEND_TIMEOUT, 1,
			  false);
	if (ret < 0) {
		LOG_ERR("AT+KCELLMEAS ret:%d", ret);
	}
	return ret;
}

static void hl7800_start_rssi_work(void)
{
	/* Rate is not checked here to allow one reading
	 * when going from network down->up
	 */
	k_work_reschedule_for_queue(&hl7800_workq, &ictx.rssi_query_work,
				    K_NO_WAIT);
}

static void hl7800_stop_rssi_work(void)
{
	int rc;

	rc = k_work_cancel_delayable(&ictx.rssi_query_work);
	if (rc != 0) {
		LOG_ERR("Could not cancel RSSI work [%d]", rc);
	}
}

static void rssi_query(void)
{
	hl7800_lock();
	wakeup_hl7800();
	hl7800_query_rssi();
	allow_sleep(true);
	hl7800_unlock();
}

static void hl7800_rssi_query_work(struct k_work *work)
{
	rssi_query();

	/* re-start RSSI query work */
	if (CONFIG_MODEM_HL7800_RSSI_RATE_SECONDS > 0) {
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.rssi_query_work,
					    K_SECONDS(CONFIG_MODEM_HL7800_RSSI_RATE_SECONDS));
	}
}

#ifdef CONFIG_MODEM_HL7800_GPS
/* Unsolicited notification
 * Handler: +GNSSEV: <eventType>,<eventStatus>
 */
static bool on_cmd_gps_event(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];
	char *start = NULL;
	char *end = NULL;
	int8_t event = -1;
	int8_t status = -1;

	memset(value, 0, sizeof(value));
	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	if (out_len > 0) {
		start = value;
		event = strtol(start, &end, 10);
		if (end == strchr(value, ',')) {
			start = end + 1;
			status = strtol(start, &end, 10);
		}
	}

	LOG_INF("GPS event: %d status: %d", event, status);

	if (event == HL7800_GNSS_EVENT_POSITION) {
		event_handler(HL7800_EVENT_GPS_POSITION_STATUS, &status);
	}

	return true;
}

static void gps_work_callback(struct k_work *work)
{
	ARG_UNUSED(work);
	int r;

	hl7800_lock();
	wakeup_hl7800();
	r = send_at_cmd(NULL, "AT+GNSSLOC?", MDM_CMD_SEND_TIMEOUT, 1, false);
	allow_sleep(true);
	hl7800_unlock();

	LOG_DBG("GPS location request status: %d", r);

	if (ictx.gps_query_location_rate_seconds) {
		k_work_reschedule_for_queue(&hl7800_workq, &ictx.gps_work,
					    K_SECONDS(ictx.gps_query_location_rate_seconds));
	}
}

/* The AT+GNSSLOC? command returns 1 of 2 things:
 *
 * +GNSSLOC:
 * Latitude: "49 Deg 10 Min 21.49 Sec N"
 * Longitude:  "123 Deg 4 Min 14.76 Sec W"
 * GpsTime: "yyyy mm dd hh:mm:ss"
 * FixType: "2D" or "3D"
 * HEPE: "8.485 m" (Horizontal Estimated Position Error)
 * Altitude: "-1 m"
 * AltUnc: "3.0 m"
 * Direction: "0.0 deg"
 * HorSpeed: "0.0 m/s"
 * VerSpeed: "0.0 m/s"
 * OK
 *
 * OR
 *
 * +GNSSLOC:
 * FIX NOT AVAILABLE
 * OK
 *
 * Since each response is on its own line, the command handler is used
 * to handle each one as an individual response.
 */
static bool gps_handler(struct net_buf **buf, uint16_t len,
			enum mdm_hl7800_gps_string_types str_type)
{
	struct mdm_hl7800_compound_event event;
	char gps_str[MDM_HL7800_MAX_GPS_STR_SIZE];
	size_t gps_len = sizeof(gps_str) - 1;
	struct net_buf *frag = NULL;
	size_t out_len;

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf), sizeof(gps_str));

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find end");
		goto done;
	}

	if (len > gps_len) {
		LOG_WRN("GPS string too long (len:%d)", len);
		len = gps_len;
	}

	out_len = net_buf_linearize(gps_str, gps_len, *buf, 0, len);
	gps_str[out_len] = 0;

	event.code = str_type;
	event.string = gps_str;
	event_handler(HL7800_EVENT_GPS, &event);
done:
	return true;
}

static bool on_cmd_latitude(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_LATITUDE);
}

static bool on_cmd_longitude(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_LONGITUDE);
}

static bool on_cmd_gps_time(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_GPS_TIME);
}

static bool on_cmd_fix_type(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_FIX_TYPE);
}

static bool on_cmd_hepe(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_HEPE);
}

static bool on_cmd_altitude(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_ALTITUDE);
}

static bool on_cmd_alt_unc(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_ALT_UNC);
}

static bool on_cmd_direction(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_DIRECTION);
}

static bool on_cmd_hor_speed(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_HOR_SPEED);
}

static bool on_cmd_ver_speed(struct net_buf **buf, uint16_t len)
{
	return gps_handler(buf, len, HL7800_GPS_STR_VER_SPEED);
}
#endif /* CONFIG_MODEM_HL7800_GPS */

#ifdef CONFIG_MODEM_HL7800_POLTE
/* Handler: %POLTEEVU: "REGISTER",0, <mqttAuthUser>, <mqttAuthPassword> */
static bool on_cmd_polte_registration(struct net_buf **buf, uint16_t len)
{
	char rsp[MDM_MAX_RESP_SIZE] = { 0 };
	size_t rsp_len = sizeof(rsp) - 1;
	char *rsp_end = rsp + rsp_len;
	struct mdm_hl7800_polte_registration_event_data data;
	struct net_buf *frag = NULL;
	size_t out_len;
	char *location;
	bool parsed;

	memset(&data, 0, sizeof(data));

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf), sizeof(rsp));

	location = rsp;
	parsed = false;
	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	do {
		if (!frag) {
			LOG_ERR("Unable to find end");
			break;
		}

		if (len > rsp_len) {
			LOG_WRN("string too long (len:%d)", len);
			len = rsp_len;
		}

		out_len = net_buf_linearize(rsp, rsp_len, *buf, 0, len);
		rsp[out_len] = 0;

		/* Command handler looks for string up to the user field */
		location = strstr(location, "\"");
		if (location != NULL && location < rsp_end) {
			location += 1;
			if (location >= rsp_end) {
				break;
			}
			data.user = location;
		} else {
			break;
		}

		/* Find end of user field and null terminate string */
		location = strstr(location, "\"");
		if (location != NULL && location < rsp_end) {
			*location = 0;
			location += 1;
			if (location >= rsp_end) {
				break;
			}
		} else {
			break;
		}

		location = strstr(location, ",\"");
		if (location != NULL && location < rsp_end) {
			location += 2;
			if (location >= rsp_end) {
				break;
			}
			data.password = location;

		} else {
			break;
		}

		location = strstr(location, "\"");
		if (location != NULL && location < rsp_end) {
			*location = 0;
		} else {
			break;
		}
		parsed = true;
	} while (0);

	if (parsed && data.user && data.password) {
		data.status = 0;
	} else {
		data.status = -1;
		LOG_ERR("Unable to parse PoLTE registration");
	}

	event_handler(HL7800_EVENT_POLTE_REGISTRATION, &data);

	return true;
}

/* Handler: %POLTECMD: "LOCATE",<res> */
static bool on_cmd_polte_locate_cmd_rsp(struct net_buf **buf, uint16_t len)
{
	char rsp[sizeof("99")] = { 0 };
	size_t rsp_len = sizeof(rsp) - 1;
	size_t out_len;
	struct net_buf *frag = NULL;
	struct mdm_hl7800_polte_location_data data;

	memset(&data, 0, sizeof(data));

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf), sizeof(rsp));

	data.status = -1;
	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	do {
		if (!frag) {
			LOG_ERR("Unable to find end");
			break;
		}

		if (len > rsp_len) {
			LOG_WRN("string too long (len:%d)", len);
			len = rsp_len;
		}

		out_len = net_buf_linearize(rsp, rsp_len, *buf, 0, len);
		rsp[out_len] = 0;

		data.status = (uint32_t)strtoul(rsp, NULL, 10);
	} while (0);

	event_handler(HL7800_EVENT_POLTE_LOCATE_STATUS, &data);

	return true;
}

/* Handler:
 * %POLTEEVU: "LOCATION",<stat>[,<latitude>,<longitude>,<time>,<confidence>]
 */
static bool on_cmd_polte_location(struct net_buf **buf, uint16_t len)
{
	char rsp[MDM_MAX_RESP_SIZE] = { 0 };
	size_t rsp_len = sizeof(rsp) - 1;
	char *rsp_end = rsp + rsp_len;
	struct net_buf *frag = NULL;
	size_t out_len = 0;
	char *start;
	char *end;
	bool parsed;
	struct mdm_hl7800_polte_location_data data;
	static const char POLTE_LOC_DELIMITER[] = "\",\"";

	memset(&data, 0, sizeof(data));

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf), sizeof(rsp));

	parsed = false;
	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	do {
		if (!frag) {
			LOG_ERR("Unable to find end");
			break;
		}

		if (len > rsp_len) {
			LOG_WRN("string too long (len:%d)", len);
			len = rsp_len;
		}

		out_len = net_buf_linearize(rsp, rsp_len, *buf, 0, len);
		rsp[out_len] = 0;

		data.status = -1;
		start = rsp;
		end = "";
		/* Comma isn't present when there is an error. */
		start = strstr(start, ",");
		if (start != NULL && start < rsp_end) {
			*start = ' ';
			start += 1;
		}
		data.status = (uint32_t)strtoul(rsp, &end, 10);
		if (data.status != 0) {
			LOG_WRN("Response not received from PoLTE server: %d", data.status);
			data.status = MDM_HL7800_POLTE_SERVER_ERROR;
			parsed = true;
			break;
		} else if (start >= rsp_end) {
			break;
		}

		start = strstr(start, "\"") + 1;
		end = strstr(start, POLTE_LOC_DELIMITER);
		if (start > rsp && start < rsp_end && end < rsp_end && end > start) {
			memcpy(data.latitude, start, MIN(end - start, sizeof(data.latitude) - 1));
		} else {
			break;
		}

		start = end + strlen(POLTE_LOC_DELIMITER);
		end = strstr(start, POLTE_LOC_DELIMITER);
		if (start > rsp && start < rsp_end && end < rsp_end && end > start) {
			memcpy(data.longitude, start, MIN(end - start, sizeof(data.longitude) - 1));
		} else {
			break;
		}

		start = end + strlen(POLTE_LOC_DELIMITER);
		end = strstr(start, POLTE_LOC_DELIMITER);
		if (start > rsp && start < rsp_end && end < rsp_end && end > start) {
			data.timestamp = (uint32_t)strtoul(start, NULL, 10);
		} else {
			break;
		}

		start = end + strlen(POLTE_LOC_DELIMITER);
		end = strstr(start, "\"");
		if (start > rsp && start < rsp_end && end < rsp_end && end > start) {
			memcpy(data.confidence_in_meters, start,
			       MIN(end - start, sizeof(data.confidence_in_meters) - 1));
		} else {
			break;
		}

		parsed = true;
	} while (0);

	if (!parsed) {
		LOG_HEXDUMP_ERR(rsp, out_len, "Unable to parse PoLTE location");
	} else {
		LOG_HEXDUMP_DBG(rsp, out_len, "PoLTE Location");
	}

	event_handler(HL7800_EVENT_POLTE, &data);

	return true;
}
#endif /* CONFIG_MODEM_HL7800_POLTE */

static void notify_all_tcp_sockets_closed(void)
{
	int i;
	struct hl7800_socket *sock = NULL;

	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		sock = &ictx.sockets[i];
		if ((sock->context != NULL) && (sock->type == SOCK_STREAM)) {
			sock->state = SOCK_SERVER_CLOSED;
			LOG_DBG("Sock %d closed", sock->socket_id);
			/* signal RX callback with null packet */
			if (sock->recv_cb) {
				sock->recv_cb(sock->context, sock->recv_pkt,
					      NULL, NULL, 0,
					      sock->recv_user_data);
			}
		}
	}
}

static void iface_status_work_cb(struct k_work *work)
{
	int ret;
	hl7800_lock();

	if (!ictx.initialized && ictx.restarting) {
		LOG_DBG("Wait for driver init, process network state later");
		/* we are not ready to process this yet, try again later */
		k_work_reschedule_for_queue(&hl7800_workq,
					    &ictx.iface_status_work,
					    IFACE_WORK_DELAY);
		goto done;
	} else if (ictx.wait_for_KSUP &&
		   ictx.wait_for_KSUP_tries < WAIT_FOR_KSUP_RETRIES) {
		LOG_DBG("Wait for +KSUP before updating network state");
		ictx.wait_for_KSUP_tries++;
		/* we have not received +KSUP yet, lets wait more time to receive +KSUP */
		k_work_reschedule_for_queue(&hl7800_workq,
					    &ictx.iface_status_work,
					    IFACE_WORK_DELAY);
		goto done;
	} else if (ictx.wait_for_KSUP &&
		   ictx.wait_for_KSUP_tries >= WAIT_FOR_KSUP_RETRIES) {
		/* give up waiting for KSUP */
		LOG_DBG("Give up waiting for");
		ictx.wait_for_KSUP = false;
		check_hl7800_awake();
	}

	wakeup_hl7800();

	LOG_DBG("Updating network state...");

	/* Query operator selection */
	ret = send_at_cmd(NULL, "AT+COPS?", MDM_CMD_SEND_TIMEOUT, 0, false);
	if (ret < 0) {
		LOG_ERR("AT+COPS ret:%d", ret);
	}

	/* bring iface up/down */
	switch (ictx.network_state) {
	case HL7800_HOME_NETWORK:
	case HL7800_ROAMING:
		if (ictx.iface && !net_if_is_up(ictx.iface)) {
			LOG_DBG("HL7800 iface UP");
			net_if_up(ictx.iface);
		}
		break;
	case HL7800_OUT_OF_COVERAGE:
	default:
		if (ictx.iface && net_if_is_up(ictx.iface) &&
		    (ictx.low_power_mode != HL7800_LPM_PSM)) {
			LOG_DBG("HL7800 iface DOWN");
			net_if_down(ictx.iface);
		}
		break;
	}

	if ((ictx.iface && !net_if_is_up(ictx.iface)) ||
	    (ictx.low_power_mode == HL7800_LPM_PSM &&
	     ictx.network_state == HL7800_OUT_OF_COVERAGE)) {
		hl7800_stop_rssi_work();
		notify_all_tcp_sockets_closed();
	} else if (ictx.iface && net_if_is_up(ictx.iface)) {
		hl7800_start_rssi_work();
		/* get IP address info */
		(void)send_at_cmd(NULL, "AT+CGCONTRDP=1", MDM_CMD_SEND_TIMEOUT,
				  CONFIG_MODEM_HL7800_GET_IP_ADDR_INFO_ATTEMPTS, false);
		/* get active bands */
		SEND_AT_CMD_IGNORE_ERROR("AT+KBND?");
	}
	LOG_DBG("Network state updated");
	allow_sleep(true);
done:
	hl7800_unlock();
}

static char *get_network_state_string(enum mdm_hl7800_network_state state)
{
	switch (state) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, NOT_REGISTERED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, HOME_NETWORK);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, SEARCHING);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, REGISTRATION_DENIED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, OUT_OF_COVERAGE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, ROAMING);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, EMERGENCY);
		PREFIXED_SWITCH_CASE_RETURN_STRING(HL7800, UNABLE_TO_CONFIGURE);
	default:
		return "UNKNOWN";
	}
}

static void set_network_state(enum mdm_hl7800_network_state state)
{
	ictx.network_state = state;
	generate_network_state_event();
}

static void generate_network_state_event(void)
{
	struct mdm_hl7800_compound_event event;

	event.code = ictx.network_state;
	event.string = get_network_state_string(ictx.network_state);
	LOG_INF("Network State: %d %s", ictx.network_state, event.string);
	event_handler(HL7800_EVENT_NETWORK_STATE_CHANGE, &event);
}

/* Handler: +CEREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>]
 *  [,[<cause_type>],[<reject_cause>] [,[<Active-Time>],[<Periodic-TAU>]]]]
 */
static bool on_cmd_network_report_query(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];
	char *pos;
	int l;
	char val[MDM_MAX_RESP_SIZE];

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	pos = strchr(value, ',');
	if (pos) {
		l = (value + out_len) - pos;
		strncpy(val, pos + 1, l);
		val[l] = 0;
		set_network_state(strtol(val, NULL, 0));

		/* start work to adjust iface */
		k_work_reschedule_for_queue(&hl7800_workq,
					    &ictx.iface_status_work,
					    IFACE_WORK_DELAY);
	}

	return true;
}

static bool on_cmd_operator_index_query(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	char carrier[MDM_HL7800_OPERATOR_INDEX_SIZE];
	size_t out_len;

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_OPERATOR_INDEX_SIZE);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find end of operator index response");
		goto done;
	}

	out_len = net_buf_linearize(carrier, MDM_HL7800_OPERATOR_INDEX_STRLEN,
				    *buf, 0, len);
	carrier[out_len] = 0;
	ictx.operator_index = (uint8_t)strtol(carrier, NULL, 10);

	LOG_INF("Operator Index: %u", ictx.operator_index);
done:
	return true;
}

static bool on_cmd_modem_functionality(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag;
	size_t out_len;
	char rsp[MDM_HL7800_MODEM_FUNCTIONALITY_SIZE];

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					MDM_HL7800_MODEM_FUNCTIONALITY_SIZE);

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find end of response");
		goto done;
	}

	out_len = net_buf_linearize(rsp, MDM_HL7800_MODEM_FUNCTIONALITY_STRLEN,
				    *buf, 0, len);
	rsp[out_len] = 0;
	ictx.functionality = strtol(rsp, NULL, 10);

	LOG_INF("Modem Functionality: %u", ictx.functionality);
done:
	return true;
}

/* There can be multiple responses from a single command.
 * %MEAS: EARFCN=5826, CellID=420, RSRP=-99, RSRQ=-15
 * %MEAS: EARFCN=6400, CellID=201, RSRP=-93, RSRQ=-21
 */
static bool on_cmd_survey_status(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	char response[sizeof("EARFCN=XXXXXXXXXXX, CellID=XXXXXXXXXXX, RSRP=-XXX, RSRQ=-XXX")];
	char *key;
	size_t out_len;
	char *value;
	struct mdm_hl7800_site_survey site_survey;

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					sizeof(response));

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_ERR("Unable to find end");
		goto done;
	}

	out_len = net_buf_linearize(response, sizeof(response), *buf, 0, len);
	LOG_HEXDUMP_DBG(response, out_len, "Site Survey");

	key = "EARFCN=";
	value = strstr(response, key);
	if (value == NULL) {
		goto done;
	} else {
		value += strlen(key);
		site_survey.earfcn = strtoul(value, NULL, 10);
	}

	key = "CellID=";
	value = strstr(response, key);
	if (value == NULL) {
		goto done;
	} else {
		value += strlen(key);
		site_survey.cell_id = strtoul(value, NULL, 10);
	}

	key = "RSRP=";
	value = strstr(response, key);
	if (value == NULL) {
		goto done;
	} else {
		value += strlen(key);
		site_survey.rsrp = strtol(value, NULL, 10);
	}

	key = "RSRQ=";
	value = strstr(response, key);
	if (value == NULL) {
		goto done;
	} else {
		value += strlen(key);
		site_survey.rsrq = strtol(value, NULL, 10);
	}

	event_handler(HL7800_EVENT_SITE_SURVEY, &site_survey);

done:
	return true;
}

/* Handler: +CCLK: "yy/MM/dd,hh:mm:ss±zz" */
static bool on_cmd_rtc_query(struct net_buf **buf, uint16_t len)
{
	struct net_buf *frag = NULL;
	size_t str_len = sizeof(TIME_STRING_FORMAT) - 1;
	char rtc_string[sizeof(TIME_STRING_FORMAT)];

	memset(rtc_string, 0, sizeof(rtc_string));
	ictx.local_time_valid = false;

	wait_for_modem_data_and_newline(buf, net_buf_frags_len(*buf),
					sizeof(TIME_STRING_FORMAT));

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		goto done;
	}
	if (len != str_len) {
		LOG_WRN("Unexpected length for RTC string %d (expected:%zu)",
			len, str_len);
	} else {
		net_buf_linearize(rtc_string, str_len, *buf, 0, str_len);
		LOG_INF("RTC string: '%s'", log_strdup(rtc_string));
		ictx.local_time_valid = convert_time_string_to_struct(
			&ictx.local_time, &ictx.local_time_offset, rtc_string);
	}
done:
	return true;
}

static bool valid_time_string(const char *time_string)
{
	size_t offset, i;

	/* Ensure the all the expected delimiters are present */
	offset = TIME_STRING_DIGIT_STRLEN + TIME_STRING_SEPARATOR_STRLEN;
	i = TIME_STRING_FIRST_SEPARATOR_INDEX;

	for (; i < TIME_STRING_PLUS_MINUS_INDEX; i += offset) {
		if (time_string[i] != TIME_STRING_FORMAT[i]) {
			return false;
		}
	}
	/* The last character is the offset from UTC and can be either
	 * positive or negative.  The last " is also handled here.
	 */
	if ((time_string[i] == '+' || time_string[i] == '-') &&
	    (time_string[i + offset] == '"')) {
		return true;
	}
	return false;
}

int get_next_time_string_digit(int *failure_cnt, char **pp, int min, int max)
{
	char digits[TIME_STRING_DIGIT_STRLEN + SIZE_OF_NUL];
	int result;

	memset(digits, 0, sizeof(digits));
	memcpy(digits, *pp, TIME_STRING_DIGIT_STRLEN);
	*pp += TIME_STRING_DIGIT_STRLEN + TIME_STRING_SEPARATOR_STRLEN;
	result = strtol(digits, NULL, 10);
	if (result > max) {
		*failure_cnt += 1;
		return max;
	} else if (result < min) {
		*failure_cnt += 1;
		return min;
	} else {
		return result;
	}
}

static bool convert_time_string_to_struct(struct tm *tm, int32_t *offset,
					  char *time_string)
{
	int fc = 0;
	char *ptr = time_string;

	if (!valid_time_string(ptr)) {
		return false;
	}
	ptr = &ptr[TIME_STRING_FIRST_DIGIT_INDEX];
	tm->tm_year = TIME_STRING_TO_TM_STRUCT_YEAR_OFFSET +
		      get_next_time_string_digit(&fc, &ptr, TM_YEAR_RANGE);
	tm->tm_mon =
		get_next_time_string_digit(&fc, &ptr, TM_MONTH_RANGE_PLUS_1) -
		1;
	tm->tm_mday = get_next_time_string_digit(&fc, &ptr, TM_DAY_RANGE);
	tm->tm_hour = get_next_time_string_digit(&fc, &ptr, TM_HOUR_RANGE);
	tm->tm_min = get_next_time_string_digit(&fc, &ptr, TM_MIN_RANGE);
	tm->tm_sec = get_next_time_string_digit(&fc, &ptr, TM_SEC_RANGE);
	tm->tm_isdst = 0;
	*offset = (int32_t)get_next_time_string_digit(&fc, &ptr,
						      QUARTER_HOUR_RANGE) *
		  SECONDS_PER_QUARTER_HOUR;
	if (time_string[TIME_STRING_PLUS_MINUS_INDEX] == '-') {
		*offset *= -1;
	}
	return (fc == 0);
}

/* Handler: +CEREG: <stat>[,[<lac>],[<ci>],[<AcT>]
 *  [,[<cause_type>],[<reject_cause>] [,[<Active-Time>],[<Periodic-TAU>]]]]
 */
static bool on_cmd_network_report(struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char *pos;
	int l;
	char val[MDM_MAX_RESP_SIZE];

	out_len = net_buf_linearize(ictx.mdm_network_status,
				    sizeof(ictx.mdm_network_status) - 1, *buf,
				    0, len);
	ictx.mdm_network_status[out_len] = 0;
	LOG_DBG("Network status: %s", log_strdup(ictx.mdm_network_status));
	pos = strchr(ictx.mdm_network_status, ',');
	if (pos) {
		l = pos - ictx.mdm_network_status;
		strncpy(val, ictx.mdm_network_status, l);
		val[l] = 0;
		set_network_state(strtol(val, NULL, 0));
	} else {
		set_network_state(strtol(ictx.mdm_network_status, NULL, 0));
	}

	/* keep HL7800 awake because we want to process the network state soon */
	allow_sleep(false);
	/* start work to adjust iface */
	k_work_reschedule_for_queue(&hl7800_workq, &ictx.iface_status_work,
				    IFACE_WORK_DELAY);

	return true;
}

/* Handler: +KCELLMEAS: <RSRP>,<Downlink Path Loss>,<PUSCH Tx Power>,
 *                       <PUCCH Tx Power>,<SiNR>
 */
static bool on_cmd_atcmdinfo_rssi(struct net_buf **buf, uint16_t len)
{
	/* number of ',' delimiters in this response */
	int num_delims = KCELLMEAS_RESPONSE_NUM_DELIMS;
	char *delims[KCELLMEAS_RESPONSE_NUM_DELIMS];
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];
	char *search_start;
	int i;

	out_len = net_buf_linearize(value, len, *buf, 0, len);
	value[out_len] = 0;
	search_start = value;

	/* find all delimiters */
	for (i = 0; i < num_delims; i++) {
		delims[i] = strchr(search_start, ',');
		if (!delims[i]) {
			LOG_ERR("Could not find delim %d, val: %s", i,
				log_strdup(value));
			goto done;
		}
		/* Start next search after current delim location */
		search_start = delims[i] + 1;
	}
	/* the first value in the message is the RSRP */
	ictx.mdm_rssi = strtol(value, NULL, 10);
	/* the 4th ',' (last in the msg) is the start of the SINR */
	ictx.mdm_sinr = strtol(delims[3] + 1, NULL, 10);
	if ((delims[1] - delims[0]) == 1) {
		/* there is no value between the first and second
		 *  delimiter, signal is unknown
		 */
		LOG_INF("RSSI (RSRP): UNKNOWN");
	} else {
		LOG_INF("RSSI (RSRP): %d SINR: %d", ictx.mdm_rssi,
			ictx.mdm_sinr);
		event_handler(HL7800_EVENT_RSSI, &ictx.mdm_rssi);
		event_handler(HL7800_EVENT_SINR, &ictx.mdm_sinr);
	}
done:
	return true;
}

/* Handle the "OK" response from an AT command or a socket call */
static bool on_cmd_sockok(struct net_buf **buf, uint16_t len)
{
	struct hl7800_socket *sock = NULL;

	ictx.last_error = 0;
	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		k_sem_give(&ictx.response_sem);
	} else {
		k_sem_give(&sock->sock_send_sem);
	}
	return true;
}

/* Handler: +KTCP_IND/+KUDP_IND */
static bool on_cmd_sock_ind(struct net_buf **buf, uint16_t len, const char *const type)
{
	struct hl7800_socket *sock = NULL;
	char *delim;
	char value[MDM_MAX_RESP_SIZE];
	size_t out_len;
	int id;

	ictx.last_error = 0;

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	/* find ',' because this is the format we expect */
	delim = strchr(value, ',');
	if (!delim) {
		LOG_ERR("%s could not find ','", type);
		goto done;
	}

	id = strtol(value, NULL, 10);
	LOG_DBG("%s ID: %d", type, id);
	sock = socket_from_id(id);
	if (sock) {
		k_sem_give(&sock->sock_send_sem);
	} else {
		LOG_ERR("Could not find socket id (%d)", id);
	}

done:
	return true;
}

static bool on_cmd_ktcp_ind(struct net_buf **buf, uint16_t len)
{
	return on_cmd_sock_ind(buf, len, "+KTCP_IND");
}

static bool on_cmd_kudp_ind(struct net_buf **buf, uint16_t len)
{
	return on_cmd_sock_ind(buf, len, "+KUDP_IND");
}


/* Handler: ERROR */
static bool on_cmd_sockerror(struct net_buf **buf, uint16_t len)
{
	struct hl7800_socket *sock = NULL;
	char string[MDM_MAX_RESP_SIZE];

	if (len > 0) {
		memset(string, 0, sizeof(string));
		net_buf_linearize(string, sizeof(string), *buf, 0, len);
		LOG_ERR("'%s'", string);
	}

	ictx.last_error = -EIO;
	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		k_sem_give(&ictx.response_sem);
	} else {
		k_sem_give(&sock->sock_send_sem);
	}

	return true;
}

/* Handler: CME/CMS Error */
static bool on_cmd_sock_error_code(struct net_buf **buf, uint16_t len)
{
	struct hl7800_socket *sock = NULL;
	char value[MDM_MAX_RESP_SIZE];
	size_t out_len;

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	LOG_ERR("Error code: %s", log_strdup(value));

	ictx.last_error = -EIO;
	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		k_sem_give(&ictx.response_sem);
	} else {
		k_sem_give(&sock->sock_send_sem);
	}

	return true;
}

static void sock_notif_cb_work(struct k_work *work)
{
	struct hl7800_socket *sock = NULL;
	struct k_work_delayable *dwork;

	dwork = k_work_delayable_from_work(work);
	sock = CONTAINER_OF(dwork, struct hl7800_socket, notif_work);

	hl7800_lock();
	/* send null packet */
	if (sock->recv_pkt != NULL) {
		/* we are in the middle of RX,
		 * requeue this and try again
		 */
		k_work_reschedule_for_queue(&hl7800_workq, &sock->notif_work,
					    MDM_SOCK_NOTIF_DELAY);
	} else {
		if (sock->type == SOCK_STREAM) {
			LOG_DBG("Sock %d trigger NULL packet", sock->socket_id);
			sock->state = SOCK_SERVER_CLOSED;
			k_work_submit_to_queue(&hl7800_workq, &sock->recv_cb_work);
			sock->error = false;
		}
	}
	hl7800_unlock();
}

/* Handler: +KTCP_NOTIF/+KUDP_NOTIF */
static bool on_cmd_sock_notif(struct net_buf **buf, uint16_t len)
{
	struct hl7800_socket *sock = NULL;
	char *delim;
	char value[MDM_MAX_RESP_SIZE];
	size_t out_len;
	uint8_t notif_val;
	bool err = false;
	bool trigger_sem = true;
	int id;

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;

	/* find ',' because this is the format we expect */
	delim = strchr(value, ',');
	if (!delim) {
		LOG_ERR("+K**P_NOTIF could not find ','");
		goto done;
	}

	notif_val = strtol(delim + 1, NULL, 10);
	switch (notif_val) {
	case HL7800_TCP_DATA_SND:
		err = false;
		ictx.last_error = 0;
		break;
	case HL7800_TCP_DISCON:
		trigger_sem = false;
		err = true;
		ictx.last_error = -EIO;
		break;
	default:
		err = true;
		ictx.last_error = -EIO;
		break;
	}

	id = strtol(value, NULL, 10);
	LOG_WRN("+K**P_NOTIF: %d,%d", id, notif_val);

	sock = socket_from_id(id);
	if (err) {
		if (sock) {
			/* Send NULL packet to callback to notify upper stack layers
			 * that the peer closed the connection or there was an error.
			 * This is so an app will not get stuck in recv() forever.
			 * Let's do the callback processing in a different work queue
			 * so RX is not delayed.
			 */
			sock->error = true;
			sock->error_val = notif_val;
			k_work_reschedule_for_queue(&hl7800_workq,
						    &sock->notif_work,
						    MDM_SOCK_NOTIF_DELAY);
			if (trigger_sem) {
				k_sem_give(&sock->sock_send_sem);
			}
		} else {
			LOG_ERR("Could not find socket id (%d)", id);
		}
	}
done:
	return true;
}

static int delete_socket(struct hl7800_socket *sock, enum net_sock_type type, uint8_t id)
{
	char cmd[sizeof("AT+KUDPCLOSE=###")];

	if (type == SOCK_STREAM) {
		snprintk(cmd, sizeof(cmd), "AT+KTCPDEL=%d", id);
	} else if (type == SOCK_DGRAM) {
		snprintk(cmd, sizeof(cmd), "AT+KUDPCLOSE=%d", id);
	}

	return send_at_cmd(sock, cmd, MDM_CMD_SEND_TIMEOUT, 0, false);
}

static void delete_untracked_socket_work_cb(struct k_work *item)
{
	struct stale_socket *sock = NULL;

	do {
		sock = dequeue_stale_socket();
		if (sock != NULL) {
			LOG_DBG("Delete untracked socket [%d]", sock->id);
			delete_socket(NULL, sock->type, sock->id);
			free_stale_socket(sock);
		}
	} while (sock != NULL);
}

static bool on_cmd_sockcreate(enum net_sock_type type, struct net_buf **buf, uint16_t len)
{
	size_t out_len;
	char value[MDM_MAX_RESP_SIZE];
	struct hl7800_socket *sock = NULL;

	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	value[out_len] = 0;
	ictx.last_socket_id = strtol(value, NULL, 10);
	if (type == SOCK_STREAM) {
		LOG_DBG("+KTCPCFG: %d", ictx.last_socket_id);
	} else if (type == SOCK_DGRAM) {
		LOG_DBG("+KUDPCFG: %d", ictx.last_socket_id);
	}

	/* check if the socket has been created already */
	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		LOG_DBG("look up new socket by creation id");
		sock = socket_from_id(MDM_CREATE_SOCKET_ID);
		if (!sock) {
			if (queue_stale_socket(type, ictx.last_socket_id) == 0) {
				/* delay some time before socket cleanup in case there
				 * are multiple sockets to cleanup
				 */
				k_work_reschedule_for_queue(&hl7800_workq,
							    &ictx.delete_untracked_socket_work,
							    SOCKET_CLEANUP_WORK_DELAY);
			}
			goto done;
		}
	}

	sock->socket_id = ictx.last_socket_id;
	sock->created = true;
	sock->reconfig = false;
	/* don't give back semaphore -- OK to follow */
done:
	return true;
}

/* Handler: +KTCPCFG: <session_id> */
static bool on_cmd_sock_tcp_create(struct net_buf **buf, uint16_t len)
{
	return on_cmd_sockcreate(SOCK_STREAM, buf, len);
}

/* Handler: +KUDPCFG: <session_id> */
static bool on_cmd_sock_udp_create(struct net_buf **buf, uint16_t len)
{
	return on_cmd_sockcreate(SOCK_DGRAM, buf, len);
}

static void sockreadrecv_cb_work(struct k_work *work)
{
	struct hl7800_socket *sock = NULL;
	struct net_pkt *pkt;

	sock = CONTAINER_OF(work, struct hl7800_socket, recv_cb_work);

	LOG_DBG("Sock %d RX CB (size: %zd)", sock->socket_id,
		(sock->recv_pkt != NULL) ? net_pkt_get_len(sock->recv_pkt) : 0);
	/* return data */
	pkt = sock->recv_pkt;
	sock->recv_pkt = NULL;
	if (sock->recv_cb) {
		sock->recv_cb(sock->context, pkt, NULL, NULL, 0,
			      sock->recv_user_data);
	} else {
		net_pkt_unref(pkt);
	}
}

static void sock_read(struct net_buf **buf, uint16_t len)
{
	struct hl7800_socket *sock = NULL;
	struct net_buf *frag;
	uint8_t c = 0U;
	int i, hdr_len;
	char ok_resp[sizeof(OK_STRING)];
	char eof[sizeof(EOF_PATTERN)];
	size_t out_len;

	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", ictx.last_socket_id);
		goto exit;
	}

	if (sock->error) {
		/* cancel notif work and restart */
		k_work_reschedule_for_queue(&hl7800_workq, &sock->notif_work,
					    MDM_SOCK_NOTIF_DELAY);
	}

	LOG_DBG("Socket %d RX %u bytes", sock->socket_id, sock->rx_size);

	/* remove ending \r\n from last CONNECT */
	if (net_buf_frags_len(*buf) < 2) {
		/* wait for \n to be RXd.  \r was already RXd. */
		wait_for_modem_data(buf, 0, 1);
	}
	/* remove \r\n */
	net_buf_remove(buf, 2);
	if (!*buf) {
		wait_for_modem_data(buf, 0, sock->rx_size);
	}

	LOG_DBG("Processing RX, buf len: %zd", net_buf_frags_len(*buf));

	/* allocate an RX pkt */
	sock->recv_pkt = net_pkt_rx_alloc_with_buffer(
		net_context_get_iface(sock->context), sock->rx_size,
		sock->family, sock->ip_proto, BUF_ALLOC_TIMEOUT);
	if (!sock->recv_pkt) {
		LOG_ERR("Failed net_pkt_get_reserve_rx!");
		goto done;
	}

	/* set pkt data */
	net_pkt_set_context(sock->recv_pkt, sock->context);

	/* add IP / protocol headers */
	hdr_len = pkt_setup_ip_data(sock->recv_pkt, sock);

	/* receive data */
	for (i = 0; i < sock->rx_size; i++) {
		/* pull data from buf and advance to the next frag if needed */
		c = net_buf_get_u8(buf);
		/* write data to packet */
		if (net_pkt_write_u8(sock->recv_pkt, c)) {
			LOG_ERR("Unable to add data! Aborting! Bytes RXd:%d",
				i);
			goto rx_err;
		}

		if (!*buf && i < sock->rx_size) {
			LOG_DBG("RX more data, bytes RXd:%d", i + 1);
			/* wait for at least one more byte */
			wait_for_modem_data(buf, 0, 1);
			if (!*buf) {
				LOG_ERR("No data in buf!");
				break;
			}
		}
	}

	LOG_DBG("Got all data, get EOF and OK (buf len:%zd)",
		net_buf_frags_len(*buf));

	if (!*buf || (net_buf_frags_len(*buf) < strlen(EOF_PATTERN))) {
		wait_for_modem_data(buf, net_buf_frags_len(*buf),
				    strlen(EOF_PATTERN));
		if (!*buf) {
			LOG_WRN("No EOF present");
			goto all_rx_data;
		}
	}

	out_len = net_buf_linearize(eof, sizeof(eof), *buf, 0,
				    strlen(EOF_PATTERN));
	eof[out_len] = 0;
	/* remove EOF pattern from buffer */
	net_buf_remove(buf, strlen(EOF_PATTERN));
	if (strcmp(eof, EOF_PATTERN)) {
		LOG_WRN("Could not find EOF [%s]", log_strdup(eof));
	}

	/* Make sure we have \r\nOK\r\n length in the buffer */
	if (!*buf || (net_buf_frags_len(*buf) < strlen(OK_STRING) + 4)) {
		wait_for_modem_data(buf, net_buf_frags_len(*buf),
				    strlen(OK_STRING) + 4);
		if (!*buf) {
			LOG_WRN("No OK present");
			goto all_rx_data;
		}
	}

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag);
	if (!frag) {
		LOG_WRN("Unable to find OK start");
		goto all_rx_data;
	}
	/* remove \r\n before OK */
	net_buf_skipcrlf(buf);

	out_len = net_buf_linearize(ok_resp, sizeof(ok_resp), *buf, 0,
				    strlen(OK_STRING));
	ok_resp[out_len] = 0;
	/* remove the message from the buffer */
	net_buf_remove(buf, strlen(OK_STRING));
	if (strcmp(ok_resp, OK_STRING)) {
		LOG_WRN("Could not find OK [%s]", log_strdup(ok_resp));
	}

	/* remove \r\n after OK */
	net_buf_skipcrlf(buf);

all_rx_data:
	net_pkt_cursor_init(sock->recv_pkt);
	net_pkt_set_overwrite(sock->recv_pkt, true);

	if (hdr_len > 0) {
		net_pkt_skip(sock->recv_pkt, hdr_len);
	}

	/* Let's do the callback processing in a different work queue in
	 * case the app takes a long time.
	 */
	k_work_submit_to_queue(&hl7800_workq, &sock->recv_cb_work);
	LOG_DBG("Sock %d RX done", sock->socket_id);
	goto done;
rx_err:
	net_pkt_unref(sock->recv_pkt);
	sock->recv_pkt = NULL;
done:
	if (sock->type == SOCK_STREAM) {
		sock->state = SOCK_CONNECTED;
	} else {
		sock->state = SOCK_IDLE;
	}
exit:
	allow_sleep(true);
	hl7800_TX_unlock();
}

static bool on_cmd_connect(struct net_buf **buf, uint16_t len)
{
	bool remove_data_from_buffer = true;
	struct hl7800_socket *sock = NULL;

	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		LOG_ERR("Sock (%d) not found", ictx.last_socket_id);
		goto done;
	}

	if (sock->state == SOCK_RX) {
		remove_data_from_buffer = false;
		sock_read(buf, len);
	} else {
		k_sem_give(&sock->sock_send_sem);
	}

done:
	return remove_data_from_buffer;
}

static int start_socket_rx(struct hl7800_socket *sock, uint16_t rx_size)
{
	char sendbuf[sizeof("AT+KTCPRCV=+#########,#####")];

	if ((sock->socket_id <= 0) || (sock->rx_size <= 0)) {
		LOG_WRN("Cannot start socket RX, ID: %d rx size: %d",
			sock->socket_id, sock->rx_size);
		return -1;
	}

	LOG_DBG("Start socket RX ID:%d size:%d", sock->socket_id, rx_size);
	sock->state = SOCK_RX;
	if (sock->type == SOCK_DGRAM) {
#if defined(CONFIG_NET_IPV4)
		if (rx_size > (net_if_get_mtu(ictx.iface) - NET_IPV4UDPH_LEN)) {
			sock->rx_size =
				net_if_get_mtu(ictx.iface) - NET_IPV4UDPH_LEN;
		}
#endif
#if defined(CONFIG_NET_IPV6)
		if (rx_size > (net_if_get_mtu(ictx.iface) - NET_IPV6UDPH_LEN)) {
			sock->rx_size =
				net_if_get_mtu(ictx.iface) - NET_IPV6UDPH_LEN;
		}
#endif
		snprintk(sendbuf, sizeof(sendbuf), "AT+KUDPRCV=%d,%u",
			 sock->socket_id, rx_size);
	} else {
#if defined(CONFIG_NET_IPV4)
		if (rx_size > (net_if_get_mtu(ictx.iface) - NET_IPV4TCPH_LEN)) {
			sock->rx_size =
				net_if_get_mtu(ictx.iface) - NET_IPV4TCPH_LEN;
		}
#endif
#if defined(CONFIG_NET_IPV6)
		if (rx_size > (net_if_get_mtu(ictx.iface) - NET_IPV6TCPH_LEN)) {
			sock->rx_size =
				net_if_get_mtu(ictx.iface) - NET_IPV6TCPH_LEN;
		}
#endif
		snprintk(sendbuf, sizeof(sendbuf), "AT+KTCPRCV=%d,%u",
			 sock->socket_id, sock->rx_size);
	}

	/* Send AT+K**PRCV, The modem
	 * will respond with "CONNECT" and the data requested
	 * and then "OK" or "ERROR".
	 * The rest of the data processing will be handled
	 * once CONNECT is RXd.
	 */
	send_at_cmd(sock, sendbuf, K_NO_WAIT, 0, false);
	return 0;
}

static void sock_rx_data_cb_work(struct k_work *work)
{
	struct hl7800_socket *sock = NULL;
	int rc;

	sock = CONTAINER_OF(work, struct hl7800_socket, rx_data_work);

	hl7800_lock();
	wakeup_hl7800();

	/* start RX */
	rc = start_socket_rx(sock, sock->rx_size);

	/* Only unlock the RX because we just locked it above.
	 *  At the end of socket RX, the TX will be unlocked.
	 */
	hl7800_RX_unlock();
	if (rc < 0) {
		/* we didn't start socket RX so unlock TX now. */
		hl7800_TX_unlock();
	}
}

/* Handler: +KTCP_DATA/+KUDP_DATA: <socket_id>,<left_bytes> */
static bool on_cmd_sockdataind(struct net_buf **buf, uint16_t len)
{
	int socket_id, left_bytes, rc;
	size_t out_len;
	char *delim;
	char value[sizeof("##,####")];
	struct hl7800_socket *sock = NULL;
	bool unlock = false;
	bool defer_rx = false;

	if (!hl7800_TX_locked()) {
		hl7800_TX_lock();
		unlock = true;
	} else {
		defer_rx = true;
	}

	out_len = net_buf_linearize(value, sizeof(value) - 1, *buf, 0, len);
	value[out_len] = 0;

	/* First comma separator marks the end of socket_id */
	delim = strchr(value, ',');
	if (!delim) {
		LOG_ERR("Missing comma");
		goto error;
	}

	/* replace comma with null */
	*delim++ = '\0';
	socket_id = strtol(value, NULL, 0);

	/* second param is for left_bytes */
	left_bytes = strtol(delim, NULL, 0);

	sock = socket_from_id(socket_id);
	if (!sock) {
		LOG_ERR("Unable to find socket_id:%d", socket_id);
		goto error;
	}

	sock->rx_size = left_bytes;
	if (defer_rx) {
		LOG_DBG("Defer socket RX -> ID: %d bytes: %u", socket_id,
			left_bytes);
		k_work_submit_to_queue(&hl7800_workq, &sock->rx_data_work);
	} else {
		if (left_bytes > 0) {
			rc = start_socket_rx(sock, left_bytes);
			if (rc < 0) {
				goto error;
			}
			goto done;
		}
	}
error:
	if (unlock) {
		hl7800_TX_unlock();
	}
done:
	return true;
}

/* Handler: +WDSI: ## */
static bool on_cmd_device_service_ind(struct net_buf **buf, uint16_t len)
{
	char value[MDM_MAX_RESP_SIZE];
	size_t out_len;

	memset(value, 0, sizeof(value));
	out_len = net_buf_linearize(value, sizeof(value), *buf, 0, len);
	if (out_len > 0) {
		ictx.device_services_ind = strtol(value, NULL, 10);
	}
	LOG_INF("+WDSI: %d", ictx.device_services_ind);

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	if (ictx.device_services_ind == WDSI_PKG_DOWNLOADED) {
		k_work_submit_to_queue(&hl7800_workq,
				       &ictx.finish_fw_update_work);
	}
#endif

	return true;
}

static inline struct net_buf *read_rx_allocator(k_timeout_t timeout,
						void *user_data)
{
	return net_buf_alloc((struct net_buf_pool *)user_data, timeout);
}

static size_t hl7800_read_rx(struct net_buf **buf)
{
	uint8_t uart_buffer[CONFIG_MODEM_HL7800_RECV_BUF_SIZE];
	size_t bytes_read, total_read;
	int ret;
	uint16_t rx_len;

	bytes_read = 0, total_read = 0;

	/* read all of the data from mdm_receiver */
	while (true) {
		ret = mdm_receiver_recv(&ictx.mdm_ctx, uart_buffer,
					sizeof(uart_buffer), &bytes_read);
		if (ret < 0 || bytes_read == 0) {
			/* mdm_receiver buffer is empty */
			break;
		}

		if (IS_ENABLED(HL7800_ENABLE_VERBOSE_MODEM_RECV_HEXDUMP)) {
			LOG_HEXDUMP_DBG((const uint8_t *)&uart_buffer,
					bytes_read, "HL7800 RX");
		}
		/* make sure we have storage */
		if (!*buf) {
			*buf = net_buf_alloc(&mdm_recv_pool, BUF_ALLOC_TIMEOUT);
			if (!*buf) {
				LOG_ERR("Can't allocate RX data! "
					"Skipping data!");
				break;
			}
		}

		rx_len =
			net_buf_append_bytes(*buf, bytes_read, uart_buffer,
					     BUF_ALLOC_TIMEOUT,
					     read_rx_allocator, &mdm_recv_pool);
		if (rx_len < bytes_read) {
			LOG_ERR("Data was lost! read %u of %zu!", rx_len,
				bytes_read);
		}
		total_read += bytes_read;
	}

	return total_read;
}

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
static void finish_fw_update_work_callback(struct k_work *item)
{
	ARG_UNUSED(item);

	send_at_cmd(NULL, "AT+WDSR=4", MDM_CMD_SEND_TIMEOUT, 0, false);
	ictx.fw_updated = true;
	set_fota_state(HL7800_FOTA_INSTALL);
	hl7800_unlock();
}

static uint8_t calc_fw_update_crc(uint8_t *ptr, int count)
{
	uint8_t crc = 0;
	unsigned char l;
	uint16_t i = 0;

	while (i < count) {
		l = *ptr;
		crc += l;
		++ptr;
		++i;
	}

	return crc;
}

static int send_fw_update_packet(struct xmodem_packet *pkt)
{
	generate_fota_count_event();
	LOG_DBG("Send FW update packet %d,%d", pkt->id, ictx.fw_packet_count);
	return mdm_receiver_send(&ictx.mdm_ctx, (const uint8_t *)pkt,
				 XMODEM_PACKET_SIZE);
}

static int prepare_and_send_fw_packet(void)
{
	int ret = 0;
	int read_res;

	ictx.fw_packet.id_complement = 0xFF - ictx.fw_packet.id;

	ret = fs_seek(&ictx.fw_update_file, ictx.file_pos, FS_SEEK_SET);
	if (ret < 0) {
		set_fota_state(HL7800_FOTA_FILE_ERROR);
		LOG_ERR("Could not seek to offset %d of file", ictx.file_pos);
		return ret;
	}

	read_res = fs_read(&ictx.fw_update_file, ictx.fw_packet.data,
			   XMODEM_DATA_SIZE);
	if (read_res < 0) {
		set_fota_state(HL7800_FOTA_FILE_ERROR);
		LOG_ERR("Failed to read fw update file [%d]", read_res);
		return ret;
	} else if (read_res < XMODEM_DATA_SIZE) {
		set_fota_state(HL7800_FOTA_PAD);
		fs_close(&ictx.fw_update_file);
		/* pad rest of data */
		for (int i = read_res; i < XMODEM_DATA_SIZE; i++) {
			ictx.fw_packet.data[i] = XMODEM_PAD_VALUE;
		}
	}

	ictx.fw_packet.crc =
		calc_fw_update_crc(ictx.fw_packet.data, XMODEM_DATA_SIZE);

	send_fw_update_packet(&ictx.fw_packet);

	ictx.file_pos += read_res;
	ictx.fw_packet_count++;
	ictx.fw_packet.id++;

	return ret;
}

static void process_fw_update_rx(struct net_buf **rx_buf)
{
	static uint8_t xm_msg;
	uint8_t eot = XM_EOT;

	xm_msg = net_buf_get_u8(rx_buf);

	if (xm_msg == XM_NACK) {
		if (ictx.fw_update_state == HL7800_FOTA_START) {
			/* send first FW update packet */
			set_fota_state(HL7800_FOTA_WIP);
			ictx.file_pos = 0;
			ictx.fw_packet_count = 1;
			ictx.fw_packet.id = 1;
			ictx.fw_packet.preamble = XM_SOH_1K;

			prepare_and_send_fw_packet();
		} else if (ictx.fw_update_state == HL7800_FOTA_WIP) {
			LOG_DBG("RX FW update NACK");
			/* resend last packet */
			send_fw_update_packet(&ictx.fw_packet);
		}
	} else if (xm_msg == XM_ACK) {
		LOG_DBG("RX FW update ACK");
		if (ictx.fw_update_state == HL7800_FOTA_WIP) {
			/* send next FW update packet */
			prepare_and_send_fw_packet();
		} else if (ictx.fw_update_state == HL7800_FOTA_PAD) {
			set_fota_state(HL7800_FOTA_SEND_EOT);
			mdm_receiver_send(&ictx.mdm_ctx, &eot, sizeof(eot));
		}
	} else {
		LOG_WRN("RX unhandled FW update value: %02x", xm_msg);
	}
}

#endif /* CONFIG_MODEM_HL7800_FW_UPDATE */

/* RX thread */
static void hl7800_rx(void)
{
	struct net_buf *rx_buf = NULL;
	struct net_buf *frag = NULL;
	int i, cmp_res;
	uint16_t len;
	size_t out_len;
	bool cmd_handled = false;
	static char rx_msg[MDM_HANDLER_MATCH_MAX_LEN];
	bool unlock = false;
	bool remove_line_from_buf = true;
#ifdef HL7800_LOG_UNHANDLED_RX_MSGS
	char msg[MDM_MAX_RESP_SIZE];
#endif

	static const struct cmd_handler handlers[] = {
		/* MODEM Information */
		CMD_HANDLER("AT+CGMI", atcmdinfo_manufacturer),
		CMD_HANDLER("AT+CGMM", atcmdinfo_model),
		CMD_HANDLER("AT+CGMR", atcmdinfo_revision),
		CMD_HANDLER("AT+CGSN", atcmdinfo_imei),
		CMD_HANDLER("AT+KGSN=3", atcmdinfo_serial_number),
		CMD_HANDLER("+KCELLMEAS: ", atcmdinfo_rssi),
		CMD_HANDLER("+CGCONTRDP: ", atcmdinfo_ipaddr),
		CMD_HANDLER("+COPS: ", atcmdinfo_operator_status),
		CMD_HANDLER("+KSRAT: ", radio_tech_status),
		CMD_HANDLER("+KBNDCFG: ", radio_band_configuration),
		CMD_HANDLER("+KBND: ", radio_active_bands),
		CMD_HANDLER("+CCID: ", atcmdinfo_iccid),
		CMD_HANDLER("ACTIVE PROFILE:", atcmdinfo_active_profile),
		CMD_HANDLER("STORED PROFILE 0:", atcmdinfo_stored_profile0),
		CMD_HANDLER("STORED PROFILE 1:", atcmdinfo_stored_profile1),
		CMD_HANDLER("+WPPP: 1,1,", atcmdinfo_pdp_authentication_cfg),
		CMD_HANDLER("+CGDCONT: 1", atcmdinfo_pdp_context),
		CMD_HANDLER("AT+CEREG?", network_report_query),
		CMD_HANDLER("+KCARRIERCFG: ", operator_index_query),
		CMD_HANDLER("AT+CIMI", atcmdinfo_imsi),
		CMD_HANDLER("+CFUN: ", modem_functionality),
		CMD_HANDLER("%MEAS: ", survey_status),
		CMD_HANDLER("+CCLK: ", rtc_query),

		/* UNSOLICITED modem information */
		/* mobile startup report */
		CMD_HANDLER("+KSUP: ", startup_report),
		/* network status */
		CMD_HANDLER("+CEREG: ", network_report),

		/* SOLICITED CMD AND SOCKET RESPONSES */
		CMD_HANDLER("OK", sockok),
		CMD_HANDLER("ERROR", sockerror),

		/* SOLICITED SOCKET RESPONSES */
		CMD_HANDLER("+CME ERROR: ", sock_error_code),
		CMD_HANDLER("+CMS ERROR: ", sock_error_code),
		CMD_HANDLER("+CEER: ", sockerror),
		CMD_HANDLER("+KTCPCFG: ", sock_tcp_create),
		CMD_HANDLER("+KUDPCFG: ", sock_udp_create),
		CMD_HANDLER(CONNECT_STRING, connect),
		CMD_HANDLER("NO CARRIER", sockerror),

		/* UNSOLICITED SOCKET RESPONSES */
		CMD_HANDLER("+KTCP_IND: ", ktcp_ind),
		CMD_HANDLER("+KUDP_IND: ", kudp_ind),
		CMD_HANDLER("+KTCP_NOTIF: ", sock_notif),
		CMD_HANDLER("+KUDP_NOTIF: ", sock_notif),
		CMD_HANDLER("+KTCP_DATA: ", sockdataind),
		CMD_HANDLER("+KUDP_DATA: ", sockdataind),

		/* FIRMWARE UPDATE RESPONSES */
		CMD_HANDLER("+WDSI: ", device_service_ind),

#ifdef CONFIG_MODEM_HL7800_GPS
		CMD_HANDLER("+GNSSEV: ", gps_event),
		CMD_HANDLER("Latitude: ", latitude),
		CMD_HANDLER("Longitude: ", longitude),
		CMD_HANDLER("GpsTime: ", gps_time),
		CMD_HANDLER("FixType: ", fix_type),
		CMD_HANDLER("HEPE: ", hepe),
		CMD_HANDLER("Altitude: ", altitude),
		CMD_HANDLER("AltUnc: ", alt_unc),
		CMD_HANDLER("Direction: ", direction),
		CMD_HANDLER("HorSpeed: ", hor_speed),
		CMD_HANDLER("VerSpeed: ", ver_speed),
#endif

#ifdef CONFIG_MODEM_HL7800_POLTE
		CMD_HANDLER("%POLTEEVU: \"REGISTER\",0,", polte_registration),
		CMD_HANDLER("%POLTECMD: \"LOCATE\",", polte_locate_cmd_rsp),
		CMD_HANDLER("%POLTEEVU: \"LOCATION\",", polte_location),
#endif
	};

	while (true) {
		/* wait for incoming data */
		(void)k_sem_take(&ictx.mdm_ctx.rx_sem, K_FOREVER);

		hl7800_read_rx(&rx_buf);
		/* If an external module hasn't locked the command processor,
		 * then do so now.
		 */
		if (!hl7800_RX_locked()) {
			hl7800_RX_lock();
			unlock = true;
		} else {
			unlock = false;
		}

		while (rx_buf) {
			remove_line_from_buf = true;
			cmd_handled = false;

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
			if ((ictx.fw_update_state == HL7800_FOTA_START) ||
			    (ictx.fw_update_state == HL7800_FOTA_WIP) ||
			    (ictx.fw_update_state == HL7800_FOTA_PAD)) {
				process_fw_update_rx(&rx_buf);
				if (!rx_buf) {
					break;
				}
			}
#endif

			net_buf_skipcrlf(&rx_buf);
			if (!rx_buf) {
				break;
			}

			frag = NULL;
			len = net_buf_findcrlf(rx_buf, &frag);
			if (!frag) {
				break;
			}

			out_len = net_buf_linearize(rx_msg, sizeof(rx_msg),
						    rx_buf, 0, len);

			/* look for matching data handlers */
			i = -1;
			for (i = 0; i < ARRAY_SIZE(handlers); i++) {
				if (ictx.search_no_id_resp) {
					cmp_res = strncmp(ictx.no_id_resp_cmd,
							  handlers[i].cmd,
							  handlers[i].cmd_len);
				} else {
					cmp_res =
						strncmp(rx_msg, handlers[i].cmd,
							handlers[i].cmd_len);
				}

				if (cmp_res == 0) {
					/* found a matching handler */

					/* skip cmd_len */
					if (!ictx.search_no_id_resp) {
						rx_buf = net_buf_skip(
							rx_buf,
							handlers[i].cmd_len);
					}

					/* locate next cr/lf */
					frag = NULL;
					len = net_buf_findcrlf(rx_buf, &frag);
					if (!frag) {
						break;
					}

					LOG_DBG("HANDLE %s (len:%u)",
						handlers[i].cmd, len);
					/* call handler */
					if (handlers[i].func) {
						remove_line_from_buf =
							handlers[i].func(
								&rx_buf, len);
					}
					cmd_handled = true;
					ictx.search_no_id_resp = false;
					frag = NULL;
					/* make sure buf still has data */
					if (!rx_buf) {
						break;
					}

					/* We've handled the current line
					 * and need to exit the "search for
					 * handler loop".  Let's skip any
					 * "extra" data and look for the next
					 * CR/LF, leaving us ready for the
					 * next handler search.
					 */
					len = net_buf_findcrlf(rx_buf, &frag);
					break;
				}
			}

			/* Handle unhandled commands */
			if (IS_ENABLED(HL7800_LOG_UNHANDLED_RX_MSGS) &&
			    !cmd_handled && frag && len > 1) {
				out_len = net_buf_linearize(msg, sizeof(msg),
							    rx_buf, 0, len);
				msg[out_len] = 0;
				LOG_HEXDUMP_DBG((const uint8_t *)&msg, len,
						"UNHANDLED RX");
			}
			if (remove_line_from_buf && frag && rx_buf) {
				/* clear out processed line (buffers) */
				net_buf_remove(&rx_buf, len);
			}
		}

		if (unlock) {
			hl7800_RX_unlock();
		}

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

static void shutdown_uart(void)
{
#ifdef CONFIG_PM_DEVICE
	int rc;

	if (ictx.uart_on) {
		HL7800_IO_DBG_LOG("Power OFF the UART");
		uart_irq_rx_disable(ictx.mdm_ctx.uart_dev);
		rc = pm_device_action_run(ictx.mdm_ctx.uart_dev,
					 PM_DEVICE_ACTION_SUSPEND);
		if (rc) {
			LOG_ERR("Error disabling UART peripheral (%d)", rc);
		}
		ictx.uart_on = false;
	}
#endif
}

static void power_on_uart(void)
{
#ifdef CONFIG_PM_DEVICE
	int rc;

	if (!ictx.uart_on) {
		HL7800_IO_DBG_LOG("Power ON the UART");
		rc = pm_device_action_run(ictx.mdm_ctx.uart_dev,
					 PM_DEVICE_ACTION_RESUME);
		if (rc) {
			LOG_ERR("Error enabling UART peripheral (%d)", rc);
		}
		uart_irq_rx_enable(ictx.mdm_ctx.uart_dev);
		ictx.uart_on = true;
	}
#endif
}

/* Make sure all IO voltages are removed for proper reset. */
static void prepare_io_for_reset(void)
{
	HL7800_IO_DBG_LOG("Preparing IO for reset/sleep");
	shutdown_uart();
	modem_assert_wake(false);
	modem_assert_pwr_on(false);
	modem_assert_fast_shutd(false);
	ictx.wait_for_KSUP = true;
	ictx.wait_for_KSUP_tries = 0;
}

static void mdm_vgpio_work_cb(struct k_work *item)
{
	ARG_UNUSED(item);

	hl7800_lock();
	if (!ictx.vgpio_state) {
		if (ictx.desired_sleep_level == HL7800_SLEEP_HIBERNATE ||
		    ictx.desired_sleep_level == HL7800_SLEEP_LITE_HIBERNATE) {
			if (ictx.sleep_state != ictx.desired_sleep_level) {
				set_sleep_state(ictx.desired_sleep_level);
			}
		}
		if (ictx.iface && ictx.initialized && net_if_is_up(ictx.iface) &&
		    ictx.low_power_mode != HL7800_LPM_PSM) {
			net_if_down(ictx.iface);
		}
	}
	hl7800_unlock();
}

void mdm_vgpio_callback_isr(const struct device *port, struct gpio_callback *cb,
			    uint32_t pins)
{
	ictx.vgpio_state = read_pin(1, ictx.gpio_port_dev[MDM_VGPIO], pinconfig[MDM_VGPIO].pin);
	HL7800_IO_DBG_LOG("VGPIO:%d", ictx.vgpio_state);
	if (!ictx.vgpio_state) {
		prepare_io_for_reset();
		if (!ictx.restarting && ictx.initialized) {
			ictx.reconfig_IP_connection = true;
		}
		check_hl7800_awake();
	} else {
		/* The peripheral must be enabled in ISR context
		 * because the driver may be
		 * waiting for +KSUP or waiting to send commands.
		 * This can occur, for example, during a modem reset.
		 */
		power_on_uart();
		allow_sleep(false);
	}

	/* When the network state changes a semaphore must be taken.
	 * This can't be done in interrupt context because the wait time != 0.
	 */
	k_work_submit_to_queue(&hl7800_workq, &ictx.mdm_vgpio_work);
}

void mdm_uart_dsr_callback_isr(const struct device *port,
			       struct gpio_callback *cb, uint32_t pins)
{
	ictx.dsr_state = read_pin(1, ictx.gpio_port_dev[MDM_UART_DSR], pinconfig[MDM_UART_DSR].pin);
	HL7800_IO_DBG_LOG("MDM_UART_DSR:%d", ictx.dsr_state);
}

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
static void mark_sockets_for_reconfig(void)
{
	int i;
	struct hl7800_socket *sock = NULL;

	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		sock = &ictx.sockets[i];
		if ((sock->context != NULL) && (sock->created)) {
			/* mark socket as possibly needing re-configuration */
			sock->reconfig = true;
		}
	}
}
#endif

void mdm_gpio6_callback_isr(const struct device *port, struct gpio_callback *cb,
			    uint32_t pins)
{
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	ictx.gpio6_state = read_pin(1, ictx.gpio_port_dev[MDM_GPIO6], pinconfig[MDM_GPIO6].pin);
	HL7800_IO_DBG_LOG("MDM_GPIO6:%d", ictx.gpio6_state);
	if (!ictx.gpio6_state) {
		/* HL7800 is not awake, shut down UART to save power */
		shutdown_uart();
		ictx.wait_for_KSUP = true;
		ictx.wait_for_KSUP_tries = 0;
		ictx.reconfig_IP_connection = true;
		mark_sockets_for_reconfig();
		/* TODO: may need to indicate all TCP connections lost here */
	} else {
		power_on_uart();
	}

	if ((ictx.gpio6_callback != NULL) &&
	    ((ictx.desired_sleep_level == HL7800_SLEEP_HIBERNATE) ||
	     (ictx.desired_sleep_level == HL7800_SLEEP_LITE_HIBERNATE))) {
		ictx.gpio6_callback(ictx.gpio6_state);
	}

	check_hl7800_awake();
#else
	HL7800_IO_DBG_LOG("Spurious gpio6 interrupt from the modem");
#endif
}

/**
 * @brief Short spikes in CTS can be removed in the signal used by the application
 */
static int glitch_filter(int default_state, const struct device *port, gpio_pin_t pin,
			 uint32_t usec_to_wait, uint32_t max_iterations)
{
	int i = 0;
	int state1;
	int state2;

	do {
		state1 = read_pin(-1, port, pin);
		k_busy_wait(usec_to_wait);
		state2 = read_pin(-1, port, pin);
		i += 1;
	} while (((state1 != state2) || (state1 < 0) || (state2 < 0)) && (i < max_iterations));

	if (i >= max_iterations) {
		LOG_WRN("glitch filter max iterations exceeded %d", i);
		if (state1 < 0) {
			if (state2 < 0) {
				state1 = read_pin(default_state, port, pin);
			} else {
				state1 = state2;
			}
		}
	}

	return state1;
}

void mdm_uart_cts_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	ictx.cts_state =
		glitch_filter(0, ictx.gpio_port_dev[MDM_UART_CTS], pinconfig[MDM_UART_CTS].pin,
			      CONFIG_MODEM_HL7800_CTS_FILTER_US,
			      CONFIG_MODEM_HL7800_CTS_FILTER_MAX_ITERATIONS);

	/* CTS toggles A LOT,
	 * comment out the debug print unless we really need it.
	 */
	/* HL7800_IO_DBG_LOG("MDM_UART_CTS:%d", ictx.cts_state); */

	if ((ictx.cts_callback != NULL) && (ictx.desired_sleep_level == HL7800_SLEEP_SLEEP)) {
		ictx.cts_callback(ictx.cts_state);
	}

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	if (ictx.cts_state) {
		/* HL7800 is not awake, shut down UART to save power */
		if (ictx.allow_sleep) {
			shutdown_uart();
		}
	} else {
		power_on_uart();
		if (ictx.sleep_state == HL7800_SLEEP_SLEEP) {
			allow_sleep(false);
		}
	}
#endif

	check_hl7800_awake();
}

static void modem_reset(void)
{
	prepare_io_for_reset();

	LOG_INF("Modem Reset");
	/* Hard reset the modem */
	gpio_pin_set(ictx.gpio_port_dev[MDM_RESET], pinconfig[MDM_RESET].pin,
		     MDM_RESET_ASSERTED);
	/* >20 milliseconds required for reset low */
	k_sleep(MDM_RESET_LOW_TIME);

	ictx.mdm_startup_reporting_on = false;
	set_sleep_state(HL7800_SLEEP_UNINITIALIZED);
	check_hl7800_awake();
	set_network_state(HL7800_NOT_REGISTERED);
	set_startup_state(HL7800_STARTUP_STATE_UNKNOWN);
#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	if (ictx.fw_update_state == HL7800_FOTA_REBOOT_AND_RECONFIGURE) {
		set_fota_state(HL7800_FOTA_COMPLETE);
	} else {
		set_fota_state(HL7800_FOTA_IDLE);
	}
#endif
	k_sem_reset(&ictx.mdm_awake);
}

static void modem_run(void)
{
	LOG_INF("Modem Run");
	gpio_pin_set(ictx.gpio_port_dev[MDM_RESET], pinconfig[MDM_RESET].pin,
		     MDM_RESET_NOT_ASSERTED);
	k_sleep(MDM_RESET_HIGH_TIME);
	allow_sleep(false);
}

static int modem_boot_handler(char *reason)
{
	int ret;

	LOG_DBG("%s", reason);
	ret = k_sem_take(&ictx.mdm_awake, MDM_BOOT_TIME);
	if (ret) {
		LOG_ERR("Err waiting for boot: %d, DSR: %u", ret,
			ictx.dsr_state);
		return -1;
	} else {
		LOG_INF("Modem booted!");
	}

	/* Turn OFF EPS network registration status reporting because
	 * it isn't needed until after initialization is complete.
	 */
	SEND_AT_CMD_EXPECT_OK("AT+CEREG=0");

	/* Determine if echo is on/off by reading the profile
	 * note: It wasn't clear how to read the
	 * active profile so all 3 are read.
	 */
	ictx.mdm_echo_is_on = true;
	SEND_AT_CMD_EXPECT_OK("AT&V");

	if (ictx.mdm_echo_is_on) {
		/* Turn OFF echo (after boot/reset) because a profile
		 * hasn't been saved yet
		 */
		SEND_AT_CMD_EXPECT_OK("ATE0");

		/* Save profile 0 */
		SEND_AT_CMD_EXPECT_OK("AT&W");

		/* Reread profiles so echo state can be checked again. */
		SEND_AT_CMD_EXPECT_OK("AT&V");
	}

	__ASSERT(!ictx.mdm_echo_is_on, "Echo should be off");

	return 0;

error:
	return ret;
}

/**
 * @brief  compares two version strings with any delimiter
 *
 * @param  *v1: version string 1
 * @param  *v2: version string 2
 *
 * @retval 0 if equal, < 0 if v1 < v2, > 0 if v1 > v2.
 */
static int compare_versions(char *v1, const char *v2)
{
	int result = 0;
	char *tail1;
	char *tail2;
	unsigned long ver1, ver2;

	/* loop through each level of the version string */
	while (result == 0) {
		/* extract leading version numbers */
		ver1 = strtoul(v1, &tail1, 10);
		ver2 = strtoul(v2, &tail2, 10);

		/* if numbers differ, then set the result */
		if (ver1 < ver2)
			result = -1;
		else if (ver1 > ver2)
			result = 1;
		else {
			/* if numbers are the same, go to next level */
			v1 = tail1;
			v2 = tail2;
			/* if we reach the end of both, then they are identical */
			if (*v1 == '\0' && *v2 == '\0')
				break;
			/* if we reach the end of one only, it is the smaller */
			else if (*v1 == '\0')
				result = -1;
			else if (*v2 == '\0')
				result = 1;
			/*  not at end ... so far they match so keep going */
			else {
				v1++;
				v2++;
			}
		}
	}
	return result;
}

static int setup_gprs_connection(char *access_point_name)
{
	char cmd_string[sizeof("AT+KCNXCFG=1,\"GPRS\",\"\",,,"
			       "\"IPV4V6\"") +
			MDM_HL7800_APN_MAX_SIZE];
	int cmd_max_len = sizeof(cmd_string) - 1;

	memset(cmd_string, 0, cmd_max_len);
	strncat(cmd_string, "AT+KCNXCFG=1,\"GPRS\",\"", cmd_max_len);
	strncat(cmd_string, access_point_name, cmd_max_len);
	strncat(cmd_string, "\",,,\"", cmd_max_len);
	strncat(cmd_string, MODEM_HL7800_ADDRESS_FAMILY "\"", cmd_max_len);
	return send_at_cmd(NULL, cmd_string, MDM_CMD_SEND_TIMEOUT, 0, false);
}

static int set_bands(const char *bands, bool full_reboot)
{
	int ret;
	char cmd[sizeof("AT+KBNDCFG=#,####################")];

	snprintk(cmd, sizeof(cmd), "AT+KBNDCFG=%d,%s", ictx.mdm_rat, bands);
	ret = send_at_cmd(NULL, cmd, MDM_CMD_SEND_TIMEOUT, MDM_DEFAULT_AT_CMD_RETRIES, false);
	if (ret < 0) {
		return ret;
	}

	if (!full_reboot) {
		ret = send_at_cmd(NULL, "AT+CFUN=1,1", MDM_CMD_SEND_TIMEOUT,
				  MDM_DEFAULT_AT_CMD_RETRIES, false);
		if (ret < 0) {
			return ret;
		}

		ret = modem_boot_handler("LTE bands were just set");
	} else {
		ret = modem_reset_and_configure();
	}
	return ret;
}

int32_t mdm_hl7800_set_bands(const char *bands)
{
	int ret, i;
	char temp_bands[MDM_BAND_BITMAP_STR_LENGTH_MAX + 1];
	int num_leading_zeros;

	if ((bands == NULL) || (strlen(bands) > MDM_BAND_BITMAP_STR_LENGTH_MAX) ||
	    (strlen(bands) < MDM_BAND_BITMAP_STR_LENGTH_MIN)) {
		return -EINVAL;
	}

	if (strlen(bands) < MDM_BAND_BITMAP_STR_LENGTH_MAX) {
		num_leading_zeros = MDM_BAND_BITMAP_STR_LENGTH_MAX - strlen(bands);
		for (i = 0; i < num_leading_zeros; i++) {
			temp_bands[i] = '0';
			if (i == (num_leading_zeros - 1)) {
				strncpy(temp_bands + (i + 1), bands, sizeof(temp_bands) - (i + 1));
			}
		}
	} else {
		memcpy(temp_bands, bands, sizeof(temp_bands));
	}

	/* no need to set bands if settings match */
	if (strncmp(temp_bands, ictx.mdm_bands_string, sizeof(temp_bands)) == 0) {
		return 0;
	}

	hl7800_lock();

	ret = set_bands(temp_bands, true);

	hl7800_unlock();

	return ret;
}

static int modem_reset_and_configure(void)
{
	int ret = 0;
	bool sleep = false;
#ifdef CONFIG_MODEM_HL7800_EDRX
	int edrx_act_type;
	char set_edrx_msg[sizeof("AT+CEDRXS=2,4,\"0000\"")];
#endif
#if CONFIG_MODEM_HL7800_CONFIGURE_BANDS
	uint16_t bands_top = 0;
	uint32_t bands_middle = 0, bands_bottom = 0;
	char new_bands[MDM_BAND_BITMAP_STR_LENGTH_MAX + 1];
#endif
#if CONFIG_MODEM_HL7800_PSM
	const char TURN_ON_PSM[] =
		"AT+CPSMS=1,,,\"" CONFIG_MODEM_HL7800_PSM_PERIODIC_TAU
		"\",\"" CONFIG_MODEM_HL7800_PSM_ACTIVE_TIME "\"";
#endif

	ictx.restarting = true;
	if (ictx.iface && net_if_is_up(ictx.iface)) {
		net_if_down(ictx.iface);
	}

	hl7800_stop_rssi_work();

reboot:
	modem_reset();
	modem_run();
	ret = modem_boot_handler("Initialization");
	if (!ictx.mdm_startup_reporting_on) {
		/* Turn on mobile start-up reporting for next reset.
		 * It will indicate if SIM is present.
		 * Its value is saved in non-volatile memory on the HL7800.
		 */
		SEND_AT_CMD_EXPECT_OK("AT+KSREP=1");
		goto reboot;
	} else if (ret < 0) {
		goto error;
	}

	/* turn on numeric error codes */
	SEND_AT_CMD_EXPECT_OK("AT+CMEE=1");

	/* modem revision */
	SEND_COMPLEX_AT_CMD("AT+CGMR");

	/* determine RAT command support */
	ret = compare_versions(ictx.mdm_revision, NEW_RAT_CMD_MIN_VERSION);
	if (ret < 0) {
		ictx.new_rat_cmd_support = false;
	} else {
		ictx.new_rat_cmd_support = true;
	}

	/* Query current Radio Access Technology (RAT) */
	SEND_AT_CMD_EXPECT_OK("AT+KSRAT?");

	/* If CONFIG_MODEM_HL7800_RAT_M1 or CONFIG_MODEM_HL7800_RAT_NB1, then
	 * set the radio mode. This is only done here if the driver has not been
	 * initialized (!ictx.configured) yet because the public API also
	 * allows the RAT to be changed (and will reset the modem).
	 */
#ifndef CONFIG_MODEM_HL7800_RAT_NO_CHANGE
	if (!ictx.configured) {
#if CONFIG_MODEM_HL7800_RAT_M1
		if (ictx.mdm_rat != MDM_RAT_CAT_M1) {
			if (ictx.new_rat_cmd_support) {
				SEND_AT_CMD_ONCE_EXPECT_OK(SET_RAT_M1_CMD);
			} else {
				SEND_AT_CMD_ONCE_EXPECT_OK(
					SET_RAT_M1_CMD_LEGACY);
			}
			if (ret >= 0) {
				goto reboot;
			}
		}
#elif CONFIG_MODEM_HL7800_RAT_NB1
		if (ictx.mdm_rat != MDM_RAT_CAT_NB1) {
			if (ictx.new_rat_cmd_support) {
				SEND_AT_CMD_ONCE_EXPECT_OK(SET_RAT_NB1_CMD);
			} else {
				SEND_AT_CMD_ONCE_EXPECT_OK(
					SET_RAT_NB1_CMD_LEGACY);
			}

			if (ret >= 0) {
				goto reboot;
			}
		}
#endif
	}
#endif

	/* If this isn't defined, then keep the current state.
	 * If the bands are being reconfigured, this is overridden.
	 */
#ifdef CONFIG_MODEM_HL7800_BOOT_IN_AIRPLANE_MODE
	SEND_AT_CMD_EXPECT_OK("AT+CFUN=4,0");
#endif

	SEND_AT_CMD_EXPECT_OK("AT+KBNDCFG?");

	/* Configure LTE bands */
#if CONFIG_MODEM_HL7800_CONFIGURE_BANDS
#if CONFIG_MODEM_HL7800_BAND_1
	bands_bottom |= 1 << 0;
#endif
#if CONFIG_MODEM_HL7800_BAND_2
	bands_bottom |= 1 << 1;
#endif
#if CONFIG_MODEM_HL7800_BAND_3
	bands_bottom |= 1 << 2;
#endif
#if CONFIG_MODEM_HL7800_BAND_4
	bands_bottom |= 1 << 3;
#endif
#if CONFIG_MODEM_HL7800_BAND_5
	bands_bottom |= 1 << 4;
#endif
#if CONFIG_MODEM_HL7800_BAND_8
	bands_bottom |= 1 << 7;
#endif
#if CONFIG_MODEM_HL7800_BAND_9
	bands_bottom |= 1 << 8;
#endif
#if CONFIG_MODEM_HL7800_BAND_10
	bands_bottom |= 1 << 9;
#endif
#if CONFIG_MODEM_HL7800_BAND_12
	bands_bottom |= 1 << 11;
#endif
#if CONFIG_MODEM_HL7800_BAND_13
	bands_bottom |= 1 << 12;
#endif
#if CONFIG_MODEM_HL7800_BAND_14
	bands_bottom |= 1 << 13;
#endif
#if CONFIG_MODEM_HL7800_BAND_17
	bands_bottom |= 1 << 16;
#endif
#if CONFIG_MODEM_HL7800_BAND_18
	bands_bottom |= 1 << 17;
#endif
#if CONFIG_MODEM_HL7800_BAND_19
	bands_bottom |= 1 << 18;
#endif
#if CONFIG_MODEM_HL7800_BAND_20
	bands_bottom |= 1 << 19;
#endif
#if CONFIG_MODEM_HL7800_BAND_25
	bands_bottom |= 1 << 24;
#endif
#if CONFIG_MODEM_HL7800_BAND_26
	bands_bottom |= 1 << 25;
#endif
#if CONFIG_MODEM_HL7800_BAND_27
	bands_bottom |= 1 << 26;
#endif
#if CONFIG_MODEM_HL7800_BAND_28
	bands_bottom |= 1 << 27;
#endif
#if CONFIG_MODEM_HL7800_BAND_66
	bands_top |= 1 << 1;
#endif

	/* Check if bands are configured correctly */
	if (ictx.mdm_bands_top != bands_top ||
	    ictx.mdm_bands_middle != bands_middle ||
	    ictx.mdm_bands_bottom != bands_bottom) {
		if (ictx.mdm_bands_top != bands_top) {
			LOG_INF("Top band mismatch, want %04x got %04x",
				bands_top, ictx.mdm_bands_top);
		}
		if (ictx.mdm_bands_middle != bands_middle) {
			LOG_INF("Middle band mismatch, want %08x got %08x",
				bands_middle, ictx.mdm_bands_middle);
		}
		if (ictx.mdm_bands_bottom != bands_bottom) {
			LOG_INF("Bottom band mismatch, want %08x got %08x",
				bands_bottom, ictx.mdm_bands_bottom);
		}

		snprintk(new_bands, sizeof(new_bands),
			 "%0" STRINGIFY(MDM_TOP_BAND_SIZE) "x%0" STRINGIFY(
				 MDM_MIDDLE_BAND_SIZE) "x%0" STRINGIFY(MDM_BOTTOM_BAND_SIZE) "x",
			 bands_top, bands_middle, bands_bottom);

		ret = set_bands(new_bands, false);
		if (ret < 0) {
			goto error;
		}
	}
#endif

	ictx.low_power_mode = HL7800_LPM_NONE;
#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	/* enable GPIO6 low power monitoring */
	SEND_AT_CMD_EXPECT_OK("AT+KHWIOCFG=3,1,6");

	initialize_sleep_level();
	ret = set_sleep_level();
	if (ret < 0) {
		goto error;
	}

#if CONFIG_MODEM_HL7800_PSM
	ictx.low_power_mode = HL7800_LPM_PSM;
	/* Turn off eDRX */
	SEND_AT_CMD_EXPECT_OK("AT+CEDRXS=0");

	SEND_AT_CMD_EXPECT_OK(TURN_ON_PSM);
#elif CONFIG_MODEM_HL7800_EDRX
	ictx.low_power_mode = HL7800_LPM_EDRX;
	/* Turn off PSM */
	SEND_AT_CMD_EXPECT_OK("AT+CPSMS=0");

	/* turn on eDRX */
	if (ictx.mdm_rat == MDM_RAT_CAT_NB1) {
		edrx_act_type = 5;
	} else {
		edrx_act_type = 4;
	}
	snprintk(set_edrx_msg, sizeof(set_edrx_msg), "AT+CEDRXS=1,%d,\"%s\"",
		 edrx_act_type, CONFIG_MODEM_HL7800_EDRX_VALUE);
	SEND_AT_CMD_EXPECT_OK(set_edrx_msg);
#endif
	sleep = true;
#else
	/* Turn off sleep mode */
	SEND_AT_CMD_EXPECT_OK("AT+KSLEEP=2");

	/* Turn off PSM */
	SEND_AT_CMD_EXPECT_OK("AT+CPSMS=0");

	/* Turn off eDRX */
	SEND_AT_CMD_EXPECT_OK("AT+CEDRXS=0");
#endif

	/* modem manufacturer */
	SEND_COMPLEX_AT_CMD("AT+CGMI");

	/* modem model */
	SEND_COMPLEX_AT_CMD("AT+CGMM");

	/* query modem IMEI */
	SEND_COMPLEX_AT_CMD("AT+CGSN");

	/* query modem serial number */
	SEND_COMPLEX_AT_CMD("AT+KGSN=3");

	if (ictx.mdm_startup_state != HL7800_STARTUP_STATE_SIM_NOT_PRESENT) {
		/* query SIM ICCID */
		SEND_AT_CMD_IGNORE_ERROR("AT+CCID?");

		/* query SIM IMSI */
		(void)send_at_cmd(NULL, "AT+CIMI", MDM_CMD_SEND_TIMEOUT, MDM_DEFAULT_AT_CMD_RETRIES,
				  true);
	}

	/* Query PDP context to get APN */
	SEND_AT_CMD_EXPECT_OK("AT+CGDCONT?");
	if (strcmp(ictx.mdm_pdp_addr_fam, MODEM_HL7800_ADDRESS_FAMILY)) {
		/* set PDP context address family along with current APN */
		ret = write_apn(ictx.mdm_apn.value);
		if (ret < 0) {
			goto error;
		}
	}

	ret = setup_gprs_connection(ictx.mdm_apn.value);
	if (ret < 0) {
		goto error;
	}

	/* Query PDP authentication context to get APN username/password.
	 * Temporary Workaround - Ignore error
	 * On some modules this is returning an error and the response data.
	 */
	SEND_AT_CMD_IGNORE_ERROR("AT+WPPP?");

#if CONFIG_MODEM_HL7800_SET_APN_NAME_ON_STARTUP
	if (!ictx.configured) {
		if (strncmp(ictx.mdm_apn.value, CONFIG_MODEM_HL7800_APN_NAME,
			    MDM_HL7800_APN_MAX_STRLEN) != 0) {
			ret = write_apn(CONFIG_MODEM_HL7800_APN_NAME);
			if (ret < 0) {
				goto error;
			} else {
				goto reboot;
			}
		}
	}
#endif

	/* query the network status in case we already registered */
	SEND_COMPLEX_AT_CMD("AT+CEREG?");

	/* Turn on EPS network registration status reporting */
	SEND_AT_CMD_EXPECT_OK("AT+CEREG=4");

	/* query all socket configs to cleanup any sockets that are not
	 * tracked by the driver
	 */
	SEND_AT_CMD_EXPECT_OK("AT+KTCPCFG?");
	SEND_AT_CMD_EXPECT_OK("AT+KUDPCFG?");

	/* The modem has been initialized and now the network interface can be
	 * started in the CEREG message handler.
	 */
	LOG_INF("Modem ready!");
	ictx.restarting = false;
	ictx.configured = true;
	allow_sleep(sleep);
	/* trigger APN update event */
	event_handler(HL7800_EVENT_APN_UPDATE, &ictx.mdm_apn);

#ifdef CONFIG_MODEM_HL7800_BOOT_DELAY
	if (!ictx.initialized) {
		if (ictx.iface != NULL) {
			hl7800_build_mac(&ictx);
			net_if_set_link_addr(ictx.iface, ictx.mac_addr,
					     sizeof(ictx.mac_addr),
					     NET_LINK_ETHERNET);
			ictx.initialized = true;
		}
	}
#endif

	return 0;

error:
	LOG_ERR("Unable to configure modem");
	ictx.configured = false;
	set_network_state(HL7800_UNABLE_TO_CONFIGURE);
	/* Kernel will fault with non-zero return value.
	 * Allow other parts of application to run when modem cannot be configured.
	 */
	return 0;
}

static int write_apn(char *access_point_name)
{
	char cmd_string[MDM_HL7800_APN_CMD_MAX_SIZE];

	/* PDP Context */
	memset(cmd_string, 0, MDM_HL7800_APN_CMD_MAX_SIZE);
	if (strcmp(MODEM_HL7800_ADDRESS_FAMILY, ADDRESS_FAMILY_IPV4)) {
		strncat(cmd_string, "AT+CGDCONT=1,\"" MODEM_HL7800_ADDRESS_FAMILY "\",\"",
			MDM_HL7800_APN_CMD_MAX_STRLEN);
	} else {
		strncat(cmd_string, "AT+CGDCONT=1,\"IP\",\"", MDM_HL7800_APN_CMD_MAX_STRLEN);
	}
	strncat(cmd_string, access_point_name, MDM_HL7800_APN_CMD_MAX_STRLEN);
	strncat(cmd_string, "\"", MDM_HL7800_APN_CMD_MAX_STRLEN);
	return send_at_cmd(NULL, cmd_string, MDM_CMD_SEND_TIMEOUT, 0, false);
}

static void mdm_reset_work_callback(struct k_work *item)
{
	ARG_UNUSED(item);

	mdm_hl7800_reset();
}

int32_t mdm_hl7800_reset(void)
{
	int ret;

	hl7800_lock();

	ret = modem_reset_and_configure();

	hl7800_unlock();

	return ret;
}

static int hl7800_power_off(void)
{
	int ret = 0;

	LOG_INF("Powering off modem");
	wakeup_hl7800();
	hl7800_stop_rssi_work();

	/* use the restarting flag to prevent +CEREG updates */
	ictx.restarting = true;

	ret = send_at_cmd(NULL, "AT+CPOF", MDM_CMD_SEND_TIMEOUT, 1, false);
	if (ret) {
		LOG_ERR("AT+CPOF ret:%d", ret);
		return ret;
	}
	/* bring the iface down */
	if (ictx.iface && net_if_is_up(ictx.iface)) {
		net_if_down(ictx.iface);
	}
	LOG_INF("Modem powered off");
	return ret;
}

int32_t mdm_hl7800_power_off(void)
{
	int rc;

	hl7800_lock();
	rc = hl7800_power_off();
	hl7800_unlock();

	return rc;
}

void mdm_hl7800_register_event_callback(struct mdm_hl7800_callback_agent *agent)
{
	k_mutex_lock(&cb_lock, K_FOREVER);
	if (!agent->event_callback) {
		LOG_WRN("event_callback is NULL");
	}
	sys_slist_append(&hl7800_event_callback_list, &agent->node);
	k_mutex_unlock(&cb_lock);
}

void mdm_hl7800_unregister_event_callback(struct mdm_hl7800_callback_agent *agent)
{
	k_mutex_lock(&cb_lock, K_FOREVER);
	(void)sys_slist_find_and_remove(&hl7800_event_callback_list, &agent->node);
	k_mutex_unlock(&cb_lock);
}

/*** OFFLOAD FUNCTIONS ***/

static int connect_TCP_socket(struct hl7800_socket *sock)
{
	int ret;
	char cmd_con[sizeof("AT+KTCPCNX=##")];

	snprintk(cmd_con, sizeof(cmd_con), "AT+KTCPCNX=%d", sock->socket_id);
	ret = send_at_cmd(sock, cmd_con, MDM_CMD_SEND_TIMEOUT, 0, false);
	if (ret < 0) {
		LOG_ERR("AT+KTCPCNX ret:%d", ret);
		ret = -EIO;
		goto done;
	}
	/* Now wait for +KTCP_IND or +KTCP_NOTIF to ensure
	 * the connection succeeded or failed.
	 */
	ret = k_sem_take(&sock->sock_send_sem, MDM_CMD_CONN_TIMEOUT);
	if (ret == 0) {
		ret = ictx.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}
	if (ret < 0) {
		LOG_ERR("+KTCP_IND/NOTIF ret:%d", ret);
		goto done;
	} else {
		sock->state = SOCK_CONNECTED;
		net_context_set_state(sock->context, NET_CONTEXT_CONNECTED);
	}
done:
	return ret;
}

static int configure_TCP_socket(struct hl7800_socket *sock)
{
	int ret;
	char cmd_cfg[sizeof("AT+KTCPCFG=#,#,\"" IPV6_ADDR_FORMAT "\",#####,,,,#,,#")];
	int dst_port = -1;
	int af;
	bool restore_on_boot = false;

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	restore_on_boot = true;
#endif

	if (sock->dst.sa_family == AF_INET6) {
		af = MDM_HL7800_SOCKET_AF_IPV6;
		dst_port = net_sin6(&sock->dst)->sin6_port;
	} else if (sock->dst.sa_family == AF_INET) {
		af = MDM_HL7800_SOCKET_AF_IPV4;
		dst_port = net_sin(&sock->dst)->sin_port;
	} else {
		return -EINVAL;
	}

	sock->socket_id = MDM_CREATE_SOCKET_ID;

	snprintk(cmd_cfg, sizeof(cmd_cfg), "AT+KTCPCFG=%d,%d,\"%s\",%u,,,,%d,,%d", 1, 0,
		 hl7800_sprint_ip_addr(&sock->dst), dst_port, af, restore_on_boot);
	ret = send_at_cmd(sock, cmd_cfg, MDM_CMD_SEND_TIMEOUT, 0, false);
	if (ret < 0) {
		LOG_ERR("AT+KTCPCFG ret:%d", ret);
		ret = -EIO;
		goto done;
	}

done:
	return ret;
}

static int configure_UDP_socket(struct hl7800_socket *sock)
{
	int ret = 0;
	char cmd[sizeof("AT+KUDPCFG=1,0,,,,,0,#")];
	int af;
	bool restore_on_boot = false;

#ifdef CONFIG_MODEM_HL7800_LOW_POWER_MODE
	restore_on_boot = true;
#endif

	sock->socket_id = MDM_CREATE_SOCKET_ID;

	if (sock->family == AF_INET) {
		af = MDM_HL7800_SOCKET_AF_IPV4;
	} else if (sock->family == AF_INET6) {
		af = MDM_HL7800_SOCKET_AF_IPV6;
	} else {
		return -EINVAL;
	}

	snprintk(cmd, sizeof(cmd), "AT+KUDPCFG=1,0,,,,,%d,%d", af, restore_on_boot);
	ret = send_at_cmd(sock, cmd, MDM_CMD_SEND_TIMEOUT, 0, false);
	if (ret < 0) {
		LOG_ERR("AT+KUDPCFG ret:%d", ret);
		goto done;
	}

	/* Now wait for +KUDP_IND or +KUDP_NOTIF to ensure
	 * the socket was created.
	 */
	ret = k_sem_take(&sock->sock_send_sem, MDM_CMD_CONN_TIMEOUT);
	if (ret == 0) {
		ret = ictx.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}
	if (ret < 0) {
		LOG_ERR("+KUDP_IND/NOTIF ret:%d", ret);
		goto done;
	}
done:
	return ret;
}

static int reconfigure_IP_connection(void)
{
	int ret = 0;

	if (ictx.reconfig_IP_connection) {
		ictx.reconfig_IP_connection = false;

		/* reconfigure GPRS connection so sockets can be used */
		ret = setup_gprs_connection(ictx.mdm_apn.value);
		if (ret < 0) {
			LOG_ERR("AT+KCNXCFG= ret:%d", ret);
			goto done;
		}

		/* query all TCP socket configs */
		ret = send_at_cmd(NULL, "AT+KTCPCFG?", MDM_CMD_SEND_TIMEOUT, 0,
				  false);

		/* query all UDP socket configs */
		ret = send_at_cmd(NULL, "AT+KUDPCFG?", MDM_CMD_SEND_TIMEOUT, 0,
				  false);
	}

done:
	return ret;
}

static int offload_get(sa_family_t family, enum net_sock_type type,
		       enum net_ip_protocol ip_proto,
		       struct net_context **context)
{
	int ret = 0;
	struct hl7800_socket *sock = NULL;

	hl7800_lock();
	/* new socket */
	sock = socket_get();
	if (!sock) {
		ret = -ENOMEM;
		goto done;
	}

	(*context)->offload_context = sock;
	/* set the context iface index to our iface */
	(*context)->iface = net_if_get_by_iface(ictx.iface);
	sock->family = family;
	sock->type = type;
	sock->ip_proto = ip_proto;
	sock->context = *context;
	sock->reconfig = false;
	sock->created = false;
	sock->socket_id = MDM_CREATE_SOCKET_ID;

	/* If UDP, create UDP socket now.
	 * TCP socket needs to be created later once the
	 * connection IP address is known.
	 */
	if (type == SOCK_DGRAM) {
		wakeup_hl7800();

		/* reconfig IP connection if necessary */
		if (reconfigure_IP_connection() < 0) {
			socket_put(sock);
			goto done;
		}

		ret = configure_UDP_socket(sock);
		if (ret < 0) {
			socket_put(sock);
			goto done;
		}
	}
done:
	allow_sleep(true);
	hl7800_unlock();
	return ret;
}

static int offload_bind(struct net_context *context,
			const struct sockaddr *addr, socklen_t addr_len)
{
	struct hl7800_socket *sock = NULL;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct hl7800_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

	/* save bind address information */
	sock->src.sa_family = addr->sa_family;
#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&sock->src)->sin6_addr,
				&net_sin6(addr)->sin6_addr);
		net_sin6(&sock->src)->sin6_port = net_sin6(addr)->sin6_port;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
		if (addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->src)->sin_addr,
				&net_sin(addr)->sin_addr);
		net_sin(&sock->src)->sin_port = net_sin(addr)->sin_port;
	} else
#endif
	{
		return -EPFNOSUPPORT;
	}

	return 0;
}

static int offload_listen(struct net_context *context, int backlog)
{
	/* NOT IMPLEMENTED */
	return -ENOTSUP;
}

static int offload_connect(struct net_context *context,
			   const struct sockaddr *addr, socklen_t addr_len,
			   net_context_connect_cb_t cb, int32_t timeout,
			   void *user_data)
{
	int ret = 0;
	int dst_port = -1;
	struct hl7800_socket *sock;

	if (!context || !addr) {
		return -EINVAL;
	}

	sock = (struct hl7800_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

	if (sock->socket_id < 1) {
		LOG_ERR("Invalid socket_id(%d) for net_ctx:%p!",
			sock->socket_id, context);
		return -EINVAL;
	}

	sock->dst.sa_family = addr->sa_family;

#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&sock->dst)->sin6_addr,
				&net_sin6(addr)->sin6_addr);
		dst_port = ntohs(net_sin6(addr)->sin6_port);
		net_sin6(&sock->dst)->sin6_port = dst_port;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
		if (addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->dst)->sin_addr,
				&net_sin(addr)->sin_addr);
		dst_port = ntohs(net_sin(addr)->sin_port);
		net_sin(&sock->dst)->sin_port = dst_port;
	} else
#endif
	{
		return -EINVAL;
	}

	if (dst_port < 0) {
		LOG_ERR("Invalid port: %d", dst_port);
		return -EINVAL;
	}

	hl7800_lock();

	if (sock->type == SOCK_STREAM) {
		wakeup_hl7800();

		reconfigure_IP_connection();

		/* Configure/create TCP connection */
		if (!sock->created) {
			ret = configure_TCP_socket(sock);
			if (ret < 0) {
				goto done;
			}
		}

		/* Connect to TCP */
		ret = connect_TCP_socket(sock);
		if (ret < 0) {
			goto done;
		}
	}

done:
	allow_sleep(true);
	hl7800_unlock();

	if (cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int offload_accept(struct net_context *context, net_tcp_accept_cb_t cb,
			  int32_t timeout, void *user_data)
{
	/* NOT IMPLEMENTED */
	return -ENOTSUP;
}

static int offload_sendto(struct net_pkt *pkt, const struct sockaddr *dst_addr,
			  socklen_t addr_len, net_context_send_cb_t cb,
			  int32_t timeout, void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	struct hl7800_socket *sock;
	int ret, dst_port = 0;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct hl7800_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

#if defined(CONFIG_NET_IPV6)
	if (dst_addr->sa_family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&sock->dst)->sin6_addr,
				&net_sin6(dst_addr)->sin6_addr);
		dst_port = ntohs(net_sin6(dst_addr)->sin6_port);
		net_sin6(&sock->dst)->sin6_port = dst_port;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
		if (dst_addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->dst)->sin_addr,
				&net_sin(dst_addr)->sin_addr);
		dst_port = ntohs(net_sin(dst_addr)->sin_port);
		net_sin(&sock->dst)->sin_port = dst_port;
	} else
#endif
	{
		return -EINVAL;
	}

	hl7800_lock();

	wakeup_hl7800();

	reconfigure_IP_connection();

	ret = send_data(sock, pkt);

	allow_sleep(true);
	hl7800_unlock();

	if (ret >= 0) {
		net_pkt_unref(pkt);
	}

	if (cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int offload_send(struct net_pkt *pkt, net_context_send_cb_t cb,
			int32_t timeout, void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	socklen_t addr_len;

	addr_len = 0;
#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		addr_len = sizeof(struct sockaddr_in6);
	} else
#endif /* CONFIG_NET_IPV6 */
#if defined(CONFIG_NET_IPV4)
		if (net_pkt_family(pkt) == AF_INET) {
		addr_len = sizeof(struct sockaddr_in);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		return -EPFNOSUPPORT;
	}

	return offload_sendto(pkt, &context->remote, addr_len, cb, timeout,
			      user_data);
}

static int offload_recv(struct net_context *context, net_context_recv_cb_t cb,
			int32_t timeout, void *user_data)
{
	struct hl7800_socket *sock;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct hl7800_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

	sock->recv_cb = cb;
	sock->recv_user_data = user_data;

	return 0;
}

static int offload_put(struct net_context *context)
{
	struct hl7800_socket *sock;
	char cmd[sizeof("AT+KTCPCLOSE=##")];

	if (!context) {
		return -EINVAL;
	}

	sock = (struct hl7800_socket *)context->offload_context;
	if (!sock) {
		/* socket was already closed?  Exit quietly here. */
		return 0;
	}

	/* cancel notif work if queued */
	k_work_cancel_delayable(&sock->notif_work);

	hl7800_lock();

	/* close connection */
	if (sock->type == SOCK_STREAM) {
		snprintk(cmd, sizeof(cmd), "AT+KTCPCLOSE=%d",
			 sock->socket_id);
	} else {
		snprintk(cmd, sizeof(cmd), "AT+KUDPCLOSE=%d",
			 sock->socket_id);
	}

	wakeup_hl7800();

	send_at_cmd(sock, cmd, MDM_CMD_SEND_TIMEOUT, 0, false);

	if (sock->type == SOCK_STREAM) {
		/* delete session */
		delete_socket(sock, sock->type, sock->socket_id);
	}
	allow_sleep(true);

	socket_put(sock);
	net_context_unref(context);
	if (sock->type == SOCK_STREAM) {
		/* TCP contexts are referenced twice,
		 * once for the app and once for the stack.
		 * Since TCP stack is not used for offload,
		 * unref a second time.
		 */
		net_context_unref(context);
	}

	hl7800_unlock();

	return 0;
}

static struct net_offload offload_funcs = {
	.get = offload_get,
	.bind = offload_bind,
	.listen = offload_listen,
	.connect = offload_connect,
	.accept = offload_accept,
	.send = offload_send,
	.sendto = offload_sendto,
	.recv = offload_recv,
	.put = offload_put,
};

/* Use the last 6 digits of the IMEI as the mac address */
static void hl7800_build_mac(struct hl7800_iface_ctx *ictx)
{
	ictx->mac_addr[0] = ictx->mdm_imei[MDM_HL7800_IMEI_STRLEN - 6];
	ictx->mac_addr[1] = ictx->mdm_imei[MDM_HL7800_IMEI_STRLEN - 5];
	ictx->mac_addr[2] = ictx->mdm_imei[MDM_HL7800_IMEI_STRLEN - 4];
	ictx->mac_addr[3] = ictx->mdm_imei[MDM_HL7800_IMEI_STRLEN - 3];
	ictx->mac_addr[4] = ictx->mdm_imei[MDM_HL7800_IMEI_STRLEN - 2];
	ictx->mac_addr[5] = ictx->mdm_imei[MDM_HL7800_IMEI_STRLEN - 1];
}

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
int32_t mdm_hl7800_update_fw(char *file_path)
{
	int ret = 0;
	struct fs_dirent file_info;
	char cmd1[sizeof("AT+WDSD=24643584")];

	/* HL7800 will stay locked for the duration of the FW update */
	hl7800_lock();

	/* get file info */
	ret = fs_stat(file_path, &file_info);
	if (ret >= 0) {
		LOG_DBG("file '%s' size %zu", log_strdup(file_info.name),
			file_info.size);
	} else {
		LOG_ERR("Failed to get file [%s] info: %d",
			log_strdup(file_path), ret);
		goto err;
	}

	ret = fs_open(&ictx.fw_update_file, file_path, FS_O_READ);
	if (ret < 0) {
		LOG_ERR("%s open err: %d", log_strdup(file_path), ret);
		goto err;
	}

	/* turn on device service indications */
	ret = send_at_cmd(NULL, "AT+WDSI=2", MDM_CMD_SEND_TIMEOUT, 0, false);
	if (ret < 0) {
		goto err;
	}

	if (ictx.iface && net_if_is_up(ictx.iface)) {
		LOG_DBG("HL7800 iface DOWN");
		hl7800_stop_rssi_work();
		net_if_down(ictx.iface);
		notify_all_tcp_sockets_closed();
	}

	/* start firmware update process */
	LOG_INF("Initiate FW update, total packets: %zd",
		((file_info.size / XMODEM_DATA_SIZE) + 1));
	set_fota_state(HL7800_FOTA_START);
	snprintk(cmd1, sizeof(cmd1), "AT+WDSD=%zd", file_info.size);
	send_at_cmd(NULL, cmd1, K_NO_WAIT, 0, false);

	goto done;

err:
	hl7800_unlock();
done:
	return ret;
}
#endif

static int hl7800_init(const struct device *dev)
{
	int i, ret = 0;

	ARG_UNUSED(dev);

	LOG_DBG("HL7800 Init");

	/* check for valid pinconfig */
	__ASSERT(ARRAY_SIZE(pinconfig) == MAX_MDM_CONTROL_PINS,
		 "Incorrect modem pinconfig!");

	/* Prevent the network interface from starting until
	 * the modem has been initialized
	 * because the modem may not have a valid SIM card.
	 */
	ictx.iface = net_if_get_default();
	if (ictx.iface == NULL) {
		return -EIO;
	}
	net_if_flag_set(ictx.iface, NET_IF_NO_AUTO_START);

	/* init sockets */
	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		ictx.sockets[i].socket_id = -1;
		k_work_init(&ictx.sockets[i].recv_cb_work,
			    sockreadrecv_cb_work);
		k_work_init(&ictx.sockets[i].rx_data_work,
			    sock_rx_data_cb_work);
		k_work_init_delayable(&ictx.sockets[i].notif_work,
				      sock_notif_cb_work);
		k_sem_init(&ictx.sockets[i].sock_send_sem, 0, 1);
	}
	ictx.last_socket_id = 0;
	k_sem_init(&ictx.response_sem, 0, 1);
	k_sem_init(&ictx.mdm_awake, 0, 1);

	/* initialize the work queue */
	k_work_queue_start(&hl7800_workq, hl7800_workq_stack,
			   K_THREAD_STACK_SIZEOF(hl7800_workq_stack),
			   WORKQ_PRIORITY, NULL);

	/* init work tasks */
	k_work_init_delayable(&ictx.rssi_query_work, hl7800_rssi_query_work);
	k_work_init_delayable(&ictx.iface_status_work, iface_status_work_cb);
	k_work_init_delayable(&ictx.dns_work, dns_work_cb);
	k_work_init(&ictx.mdm_vgpio_work, mdm_vgpio_work_cb);
	k_work_init_delayable(&ictx.mdm_reset_work, mdm_reset_work_callback);
	k_work_init_delayable(&ictx.allow_sleep_work,
			      allow_sleep_work_callback);
	k_work_init_delayable(&ictx.delete_untracked_socket_work, delete_untracked_socket_work_cb);

#ifdef CONFIG_MODEM_HL7800_GPS
	k_work_init_delayable(&ictx.gps_work, gps_work_callback);
#endif

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	k_work_init(&ictx.finish_fw_update_work,
		    finish_fw_update_work_callback);
	ictx.fw_updated = false;
#endif

	/* setup port devices and pin directions */
	for (i = 0; i < MAX_MDM_CONTROL_PINS; i++) {
		ictx.gpio_port_dev[i] =
			device_get_binding(pinconfig[i].dev_name);
		if (!ictx.gpio_port_dev[i]) {
			LOG_ERR("gpio port (%s) not found!",
				pinconfig[i].dev_name);
			return -ENODEV;
		}

		ret = gpio_pin_configure(ictx.gpio_port_dev[i], pinconfig[i].pin,
					 pinconfig[i].config);
		if (ret) {
			LOG_ERR("Error configuring IO %s %d err: %d!", pinconfig[i].dev_name,
				pinconfig[i].pin, ret);
			return ret;
		}
	}

	/* when this driver starts, the UART peripheral is already enabled */
	ictx.uart_on = true;

	modem_assert_wake(false);
	modem_assert_pwr_on(false);
	modem_assert_fast_shutd(false);

	/* Allow modem to run so we are in a known state.
	 * This allows HL7800 VGPIO to be high, which is good because the UART
	 * IO are already configured.
	 */
	modem_run();

	/* setup input pin callbacks */
	/* VGPIO */
	gpio_init_callback(&ictx.mdm_vgpio_cb, mdm_vgpio_callback_isr,
			   BIT(pinconfig[MDM_VGPIO].pin));
	ret = gpio_add_callback(ictx.gpio_port_dev[MDM_VGPIO],
				&ictx.mdm_vgpio_cb);
	if (ret) {
		LOG_ERR("Cannot setup vgpio callback! (%d)", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure(ictx.gpio_port_dev[MDM_VGPIO],
					   pinconfig[MDM_VGPIO].pin,
					   pinconfig[MDM_VGPIO].irq_config);
	if (ret) {
		LOG_ERR("Error config vgpio interrupt! (%d)", ret);
		return ret;
	}

	/* UART DSR */
	gpio_init_callback(&ictx.mdm_uart_dsr_cb, mdm_uart_dsr_callback_isr,
			   BIT(pinconfig[MDM_UART_DSR].pin));
	ret = gpio_add_callback(ictx.gpio_port_dev[MDM_UART_DSR],
				&ictx.mdm_uart_dsr_cb);
	if (ret) {
		LOG_ERR("Cannot setup uart dsr callback! (%d)", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure(ictx.gpio_port_dev[MDM_UART_DSR],
					   pinconfig[MDM_UART_DSR].pin,
					   pinconfig[MDM_UART_DSR].irq_config);
	if (ret) {
		LOG_ERR("Error config uart dsr interrupt! (%d)", ret);
		return ret;
	}

	/* GPIO6 */
	gpio_init_callback(&ictx.mdm_gpio6_cb, mdm_gpio6_callback_isr,
			   BIT(pinconfig[MDM_GPIO6].pin));
	ret = gpio_add_callback(ictx.gpio_port_dev[MDM_GPIO6],
				&ictx.mdm_gpio6_cb);
	if (ret) {
		LOG_ERR("Cannot setup gpio6 callback! (%d)", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure(ictx.gpio_port_dev[MDM_GPIO6],
					   pinconfig[MDM_GPIO6].pin,
					   pinconfig[MDM_GPIO6].irq_config);
	if (ret) {
		LOG_ERR("Error config gpio6 interrupt! (%d)", ret);
		return ret;
	}

	/* UART CTS */
	gpio_init_callback(&ictx.mdm_uart_cts_cb, mdm_uart_cts_callback,
			   BIT(pinconfig[MDM_UART_CTS].pin));
	ret = gpio_add_callback(ictx.gpio_port_dev[MDM_UART_CTS],
				&ictx.mdm_uart_cts_cb);
	if (ret) {
		LOG_ERR("Cannot setup uart cts callback! (%d)", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure(ictx.gpio_port_dev[MDM_UART_CTS],
					   pinconfig[MDM_UART_CTS].pin,
					   pinconfig[MDM_UART_CTS].irq_config);
	if (ret) {
		LOG_ERR("Error config uart cts interrupt! (%d)", ret);
		return ret;
	}

	/* Set modem data storage */
	ictx.mdm_ctx.data_manufacturer = ictx.mdm_manufacturer;
	ictx.mdm_ctx.data_model = ictx.mdm_model;
	ictx.mdm_ctx.data_revision = ictx.mdm_revision;
#ifdef CONFIG_MODEM_SIM_NUMBERS
	ictx.mdm_ctx.data_imei = ictx.mdm_imei;
#endif
	ictx.mdm_ctx.data_rssi = &ictx.mdm_rssi;

	ret = mdm_receiver_register(&ictx.mdm_ctx, MDM_UART_DEV,
				    mdm_recv_buf, sizeof(mdm_recv_buf));
	if (ret < 0) {
		LOG_ERR("Error registering modem receiver (%d)!", ret);
		return ret;
	}

	k_queue_init(&ictx.stale_socket_queue);

	/* start RX thread */
	k_thread_name_set(
		k_thread_create(&hl7800_rx_thread, hl7800_rx_stack,
				K_THREAD_STACK_SIZEOF(hl7800_rx_stack),
				(k_thread_entry_t)hl7800_rx, NULL, NULL, NULL,
				RX_THREAD_PRIORITY, 0, K_NO_WAIT),
		"hl7800 rx");

#ifdef CONFIG_MODEM_HL7800_BOOT_DELAY
	modem_reset();
#else
	ret = modem_reset_and_configure();
#endif

	return ret;
}

static void offload_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct hl7800_iface_ctx *ctx = dev->data;

	iface->if_dev->offload = &offload_funcs;
	ctx->iface = iface;

	if (!IS_ENABLED(CONFIG_MODEM_HL7800_BOOT_DELAY)) {
		hl7800_build_mac(&ictx);
		net_if_set_link_addr(iface, ictx.mac_addr, sizeof(ictx.mac_addr),
				     NET_LINK_ETHERNET);
		ictx.initialized = true;
	}
}

static struct net_if_api api_funcs = {
	.init = offload_iface_init,
};

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, hl7800_init, NULL, &ictx,
				  NULL, CONFIG_MODEM_HL7800_INIT_PRIORITY,
				  &api_funcs, MDM_MTU);
