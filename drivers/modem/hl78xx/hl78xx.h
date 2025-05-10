#ifndef HL78XX_H
#define HL78XX_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/cellular.h>
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
#include "../modem_context.h"
#include "../modem_socket.h"

#define MDM_CMD_TIMEOUT                      (10)  /*K_SECONDS*/
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

#define MDM_MAX_DATA_LENGTH CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES

#define MDM_MAX_SOCKETS     6
#define MDM_BASE_SOCKET_NUM 1

#define MDM_MANUFACTURER_LENGTH 10
#define MDM_MODEL_LENGTH        16
#define MDM_REVISION_LENGTH     64
#define MDM_IMEI_LENGTH         16
#define MDM_IMSI_LENGTH         23
#define MDM_ICCID_LENGTH        22
#define MDM_APN_MAX_LENGTH      64

#define ADDRESS_FAMILY_IP         "IP"
#define ADDRESS_FAMILY_IP4        "IPV4"
#define ADDRESS_FAMILY_IPV6       "IPV6"
#define ADDRESS_FAMILY_IPV4V6     "IPV4V6"
#define MDM_HL78XX_SOCKET_AF_IPV4 0
#define MDM_HL78XX_SOCKET_AF_IPV6 1
#if defined(CONFIG_MODEM_HL78XX_ADDRESS_FAMILY_IPV4V6)
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
#define EOF_PATTERN      "--EOF--Pattern--"
#define EOF_PATTERN_GNSS "+++"
#define CONNECT_STRING   "CONNECT"

/* RAT (Radio Access Technology) commands */
#define SET_RAT_M1_CMD_LEGACY    "AT+KSRAT=0"
#define SET_RAT_NB1_CMD_LEGACY   "AT+KSRAT=1"
#define SET_RAT_GSM_CMD_LEGACY   "AT+KSRAT=2"
#define SET_RAT_NBNTN_CMD_LEGACY "AT+KSRAT=3"

#define KSRAT_QUERY      "AT+KSRAT?"
#define DISABLE_RAT_AUTO "AT+KSELACQ=0,0"

#define SET_RAT_M1_CMD    "AT+KSRAT=0,1"
#define SET_RAT_NB1_CMD   "AT+KSRAT=1,1"
#define SET_RAT_GMS_CMD   "AT+KSRAT=2,1"
#define SET_RAT_NBNTN_CMD "AT+KSRAT=3,1"

/* Power mode commands */
#define SET_AIRPLANE_MODE_CMD_LEGACY       "AT+CFUN=4,0"
#define SET_AIRPLANE_MODE_CMD              "AT+CFUN=4,1"
#define SET_FULLFUNCTIONAL_MODE_CMD_LEGACY "AT+CFUN=1,0"
#define SET_FULLFUNCTIONAL_MODE_CMD        "AT+CFUN=1,1"

/* Helper macros */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)

/* Enums */
enum mdm_hl78xx_rat_mode {
	MDM_RAT_CAT_M1 = 0,
	MDM_RAT_NB1,
	MDM_RAT_GSM,
	MDM_RAT_NBNTN,
	MDM_RAT_MODE_NONE,
};

enum mdm_hl78xx_gnss_event {
	HL78XX_GNSS_EVENT_INVALID = -1,
	HL78XX_GNSS_EVENT_INIT,
	HL78XX_GNSS_EVENT_START,
	HL78XX_GNSS_EVENT_STOP,
	HL78XX_GNSS_EVENT_POSITION,
};

enum mdm_hl78xx_gnss_status {
	HL78XX_GNSS_STATUS_INVALID = -1,
	HL78XX_GNSS_STATUS_FAILURE,
	HL78XX_GNSS_STATUS_SUCCESS,
};

enum mdm_hl78xx_gnss_position_event {
	HL78XX_GNSS_POSITION_EVENT_INVALID = -1,
	HL78XX_GNSS_POSITION_EVENT_LOST_OR_NOT_AVAILABLE_YET,
	HL78XX_GNSS_POSITION_EVENT_PREDICTION_AVAILABLE,
	HL78XX_GNSS_POSITION_EVENT_2D_AVAILABLE,
	HL78XX_GNSS_POSITION_EVENT_3D_AVAILABLE,
	HL78XX_GNSS_POSITION_EVENT_FIXED_TO_INVALID,
	HL78XX_GNSS_POSITION_EVENT_SATELLITE_TIMEOUT,
};

enum modem_hl78xx_state {
	MODEM_HL78XX_STATE_IDLE = 0,
	MODEM_HL78XX_STATE_RESET_PULSE,
	MODEM_HL78XX_STATE_POWER_ON_PULSE,
	MODEM_HL78XX_STATE_AWAIT_POWER_ON,
	MODEM_HL78XX_STATE_SET_BAUDRATE,
	MODEM_HL78XX_STATE_RUN_INIT_SCRIPT,
	MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT,
	MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT,
	MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT,
	MODEM_HL78XX_STATE_AWAIT_REGISTERED,
	MODEM_HL78XX_STATE_CARRIER_ON,
	MODEM_HL78XX_STATE_INIT_POWER_OFF,
	MODEM_HL78XX_STATE_POWER_OFF_PULSE,
	MODEM_HL78XX_STATE_AWAIT_POWER_OFF,
};

enum modem_hl78xx_event {
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
	MODEM_HL78XX_EVENT_SOCKET_READY,
};
struct kselacq_syntax {
	bool mode;
	enum mdm_hl78xx_rat_mode rat1;
	enum mdm_hl78xx_rat_mode rat2;
	enum mdm_hl78xx_rat_mode rat3;
};

struct registration_status {
	bool is_registered;
	enum cellular_registration_status network_state;
	enum mdm_hl78xx_rat_mode rat_mode;
};
/* driver data */
struct modem_hl78xx_data {
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES];
	uint8_t uart_backend_transmit_buf[CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES];

	/* Modem chat */
	struct k_sem script_stopped_sem_tx_int;
	struct k_sem script_stopped_sem_rx_int;
	struct modem_chat chat;
	uint8_t chat_receive_buf[CONFIG_MODEM_HL78XX_CHAT_BUFFER_SIZES];
	uint8_t *chat_delimiter;
	uint8_t *chat_filter;
	uint8_t *chat_argv[32];
	uint8_t *chat_eof_pattern;
	uint8_t chat_eof_pattern_size;
	struct k_mutex tx_lock;
	/* Status */
	struct registration_status mdm_registration_status;
	uint8_t rssi;
	uint8_t ksrep;
	uint8_t rsrp;
	uint8_t rsrq;
	uint8_t imei[MDM_IMEI_LENGTH];
	uint8_t model_id[MDM_MODEL_LENGTH];
	uint8_t imsi[MDM_IMSI_LENGTH];
	uint8_t iccid[MDM_ICCID_LENGTH];
	uint8_t manufacturer[MDM_MANUFACTURER_LENGTH];
	uint8_t fw_version[MDM_REVISION_LENGTH];
	uint16_t script_fail_counter;
	struct kselacq_syntax kselacq_data;

#if defined(CONFIG_MODEM_HL78XX_RSSI_WORK)
	/* RSSI work */
	struct k_work_delayable rssi_query_work;
#endif
	/* GPIO PORT devices */
	struct gpio_callback mdm_vgpio_cb;
	struct gpio_callback mdm_uart_dsr_cb;
	struct gpio_callback mdm_gpio6_cb;
	struct gpio_callback mdm_uart_cts_cb;
	/* modem variant */
	int mdm_variant;
	/* APN */
	char mdm_apn[MDM_APN_MAX_LENGTH];

	enum modem_hl78xx_state state;
	const struct device *dev;
	struct k_work_delayable timeout_work;
	/* Power management */
	struct k_sem suspended_sem;
	/* Event dispatcher */
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct ring_buf event_rb;
	struct k_mutex event_rb_lock;

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
};
struct modem_hl78xx_config {
	const struct device *uart;
	struct gpio_dt_spec mdm_gpio_reset;
	struct gpio_dt_spec mdm_gpio_wake;
	struct gpio_dt_spec mdm_gpio_pwr_on;
	struct gpio_dt_spec mdm_gpio_fast_shtdown;
	struct gpio_dt_spec mdm_gpio_uart_dtr;
	struct gpio_dt_spec mdm_gpio_uart_dsr;
	struct gpio_dt_spec mdm_gpio_uart_cts;
	struct gpio_dt_spec mdm_gpio_vgpio;
	struct gpio_dt_spec mdm_gpio_gpio6;
	struct gpio_dt_spec mdm_gpio_gpio8;
	struct gpio_dt_spec mdm_gpio_sim_switch;
	uint16_t power_pulse_duration_ms;
	uint16_t reset_pulse_duration_ms;
	uint16_t startup_time_ms;
	uint16_t shutdown_time_ms;

	bool autostarts;

	const struct modem_chat_script *init_chat_script;
};
/* socket read callback data */
struct socket_read_data {
	char *recv_buf;
	size_t recv_buf_len;
	struct sockaddr *recv_addr;
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
bool modem_cellular_is_registered(struct modem_hl78xx_data *data);

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
uint32_t hash32(const char *str, int len);

/**
 * @brief DNS resolution work callback.
 *
 * Should be used internally to handle DNS resolution events.
 */
void dns_work_cb(void);

void iface_status_work_cb(struct modem_hl78xx_data *data);
/**
 * @brief Convert a string to an integer with error handling.
 *
 * Similar to atoi, but allows specifying an error fallback and logs errors.
 *
 * @param s Input string to convert.
 * @param err_value Value to return on failure.
 * @param desc Description of the value for logging purposes.
 * @param func Function name for logging purposes.
 *
 * @return Converted integer on success, or err_value on failure.
 */
int modem_atoi(const char *s, const int err_value, const char *desc, const char *func);
/**
 * @brief Initialize sockets for the modem.
 *
 * Sets up the socket table and configuration for socket communication.
 *
 * @param data Pointer to the modem HL78xx driver data structure.
 */
void modem_hl78xx_socket_init(struct modem_hl78xx_data *data);
/**
 * @brief Notify the system of socket data changes.
 *
 * Typically used when data has been received or transmitted on a socket.
 *
 * @param socket_id ID of the affected socket.
 * @param new_total New data count or buffer level associated with the socket.
 */
void socknotifydata(int socket_id, int new_total);
/**
 * @brief Find a memory block inside another block (C99-compatible version).
 *
 * Searches for the first occurrence of the byte string needle of length needlelen
 * in the memory area haystack of length haystacklen.
 *
 * @param haystack Pointer to the memory block to search within.
 * @param haystacklen Length of the haystack memory block.
 * @param needle Pointer to the memory block to search for.
 * @param needlelen Length of the needle memory block.
 *
 * @return Pointer to the beginning of the found needle, or NULL if not found.
 */
void *c99_memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);

/**
 * @brief Generate a pseudo-MAC address using a hash of a dummy IMEI.
 *
 * The MAC address is constructed using a fixed prefix and a hash of the IMEI value.
 *
 * @param mac_addr Pointer to a buffer where the generated MAC address will be stored.
 *
 * @return Pointer to the provided mac_addr buffer containing the generated MAC address.
 */
static inline uint8_t *modem_get_mac(uint8_t *mac_addr)
{
	uint32_t hash_value;
	const char *dummy_imei = "111111111111111";

	/* Define MAC address prefix */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x10;

	/* Generate MAC address based on IMEI */
	hash_value = hash32(dummy_imei, strlen(dummy_imei));
	UNALIGNED_PUT(hash_value, (uint32_t *)(mac_addr + 2));

	return mac_addr;
}

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
/**
 * @brief Find the appropriate APN from a list of profiles based on an associated number.
 *
 * Parses a string of APN profiles and attempts to match the correct APN using the
 * provided associated number (e.g., IMSI, MCC/MNC).
 *
 * @param apn Output buffer where the selected APN will be stored.
 * @param apnlen Length of the output buffer.
 * @param profiles String containing a list of APN profiles to search through.
 * @param associated_number Identifier used to match the correct APN (e.g., MCCMNC).
 *
 * @return 0 on success, -1 on failure or if no match is found.
 */
int find_apn(char *apn, int apnlen, const char *profiles, const char *associated_number);
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
int modem_detect_apn(struct modem_hl78xx_data *data, const char *associated_number);

#endif
#endif /* HL78XX_H */
