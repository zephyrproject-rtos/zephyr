/*
 * Copyright (c) 2025 Netfeasa
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

#define MDM_MAX_SOCKETS           6
#define MDM_BASE_SOCKET_NUM       1
#define MDM_BAND_BITMAP_LEN_BYTES 32
#define MDM_BAND_HEX_STR_LEN      (MDM_BAND_BITMAP_LEN_BYTES * 2 + 1)

#define MDM_KBND_BITMAP_MAX_ARRAY_SIZE 64

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
#define GET_FULLFUNCTIONAL_MODE_CMD        "AT+CFUN?"
/* PDP Context commands */
#define DEACTIVATE_PDP_CONTEXT             "AT+CGACT=0"
#define ACTIVATE_PDP_CONTEXT               "AT+CGACT=1"

/* Helper macros */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)

/* Enums */

enum hl78xx_gnss_event {
	HL78XX_GNSS_EVENT_INVALID = -1,
	HL78XX_GNSS_EVENT_INIT,
	HL78XX_GNSS_EVENT_START,
	HL78XX_GNSS_EVENT_STOP,
	HL78XX_GNSS_EVENT_POSITION,
};

enum hl78xx_gnss_status {
	HL78XX_GNSS_STATUS_INVALID = -1,
	HL78XX_GNSS_STATUS_FAILURE,
	HL78XX_GNSS_STATUS_SUCCESS,
};

enum hl78xx_gnss_position_event {
	HL78XX_GNSS_POSITION_EVENT_INVALID = -1,
	HL78XX_GNSS_POSITION_EVENT_LOST_OR_NOT_AVAILABLE_YET,
	HL78XX_GNSS_POSITION_EVENT_PREDICTION_AVAILABLE,
	HL78XX_GNSS_POSITION_EVENT_2D_AVAILABLE,
	HL78XX_GNSS_POSITION_EVENT_3D_AVAILABLE,
	HL78XX_GNSS_POSITION_EVENT_FIXED_TO_INVALID,
	HL78XX_GNSS_POSITION_EVENT_SATELLITE_TIMEOUT,
};

enum hl78xx_state {
	MODEM_HL78XX_STATE_IDLE = 0,
	MODEM_HL78XX_STATE_RESET_PULSE,
	MODEM_HL78XX_STATE_POWER_ON_PULSE,
	MODEM_HL78XX_STATE_AWAIT_POWER_ON,
	MODEM_HL78XX_STATE_SET_BAUDRATE,
	MODEM_HL78XX_STATE_RUN_INIT_SCRIPT,
	MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT,
	MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT,
	MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT,
	/* Full functionality, searching
	 * CFUN=1
	 */
	MODEM_HL78XX_STATE_AWAIT_REGISTERED,
	MODEM_HL78XX_STATE_CARRIER_ON,
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
	MODEM_HL78XX_STATE_INIT_POWER_OFF,
	MODEM_HL78XX_STATE_POWER_OFF_PULSE,
	MODEM_HL78XX_STATE_AWAIT_POWER_OFF,
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
	MODEM_HL78XX_EVENT_SOCKET_READY,
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

struct registration_status {
	bool is_registered;
	enum hl78xx_registration_status network_state;
	enum hl78xx_cell_rat_mode rat_mode;
};
/* driver data */
struct modem_buffers {
	uint8_t uart_rx[CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES];
	uint8_t uart_tx[CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES];
	uint8_t chat_rx[CONFIG_MODEM_HL78XX_CHAT_BUFFER_SIZES];
	uint8_t *delimiter;
	uint8_t *filter;
	uint8_t *argv[32];
	uint8_t *eof_pattern;
	uint8_t eof_pattern_size;
};

struct modem_identity {
	uint8_t imei[MDM_IMEI_LENGTH];
	uint8_t model_id[MDM_MODEL_LENGTH];
	uint8_t imsi[MDM_IMSI_LENGTH];
	uint8_t iccid[MDM_ICCID_LENGTH];
	uint8_t manufacturer[MDM_MANUFACTURER_LENGTH];
	uint8_t fw_version[MDM_REVISION_LENGTH];
	char apn[MDM_APN_MAX_LENGTH];
};

struct modem_status {
	struct registration_status registration;
	uint8_t rssi;
	uint8_t ksrep;
	uint8_t rsrp;
	uint8_t rsrq;
	uint16_t script_fail_counter;
	int variant;
	enum hl78xx_state state;
	struct kband_syntax kbndcfg[HL78XX_RAT_COUNT];
	enum hl78xx_phone_functionality phone_functionality;
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

	struct k_mutex tx_lock;
	struct k_sem script_stopped_sem_tx_int;
	struct k_sem script_stopped_sem_rx_int;
	struct k_sem suspended_sem;

	struct modem_buffers buffers;
	struct modem_identity identity;
	struct modem_status status;
	struct modem_gpio_callbacks gpio_cbs;
	struct modem_event_system events;

	struct k_work_delayable timeout_work;

#if defined(CONFIG_MODEM_HL78XX_RSSI_WORK)
	struct k_work_delayable rssi_query_work;
#endif

	const struct device *dev;
	struct kselacq_syntax kselacq_data;
};

struct hl78xx_config {
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
bool hl78xx_is_registered(struct hl78xx_data *data);

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

/**
 * @brief Callback to update and handle network interface status.
 *
 * This function is typically scheduled as work to check and respond to changes
 * in the modem's network interface state, such as registration, link readiness,
 * or disconnection events.
 *
 * @param data Pointer to the modem HL78xx driver data structure.
 */
void iface_status_work_cb(struct hl78xx_data *data,
			  modem_chat_script_callback script_user_callback);

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
void hl78xx_socket_init(struct hl78xx_data *data);

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
 * @brief Send a command to the modem and wait for matching response(s).
 *
 * This function sends a raw command to the modem and processes its response using
 * the provided match patterns. It supports asynchronous notification via callback.
 *
 * @param data Pointer to the modem HL78xx driver data structure.
 * @param script_user_callback Callback function invoked on matched responses or errors.
 * @param cmd Pointer to the command buffer to send.
 * @param cmd_len Length of the command in bytes.
 * @param response_matches Array of expected response match patterns.
 * @param matches_size Number of elements in the response_matches array.
 *
 * @return 0 on success, negative errno code on failure.
 */
int modem_cmd_send_int(struct hl78xx_data *data, modem_chat_script_callback script_user_callback,
		       const uint8_t *cmd, uint16_t cmd_len,
		       const struct modem_chat_match *response_matches, uint16_t matches_size,
		       bool user_cmd);

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
const void *c99_memmem(const void *haystack, size_t haystacklen, const void *needle,
		       size_t needlelen);

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
 * @brief Handle modem state update from +KSTATE URC (unsolicited result code).
 *
 * This function is called when a +KSTATE URC is received, indicating a change
 * in the modem's internal state. It updates the modem driver's state machine
 * accordingly.
 *
 * @param data Pointer to the HL78xx modem driver data structure.
 * @param state Integer value representing the new modem state as reported by the URC.
 */
void hl78xx_on_kstatev_parser(struct hl78xx_data *data, int state);

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
 * @brief Get the default band configuration in hexadecimal string format.
 *
 * Retrieves the modem's default band configuration as a hex string,
 * used for configuring or restoring band settings.
 *
 * @param hex_bndcfg Buffer to store the resulting hex band configuration string.
 * @param SizeInBytes Size of the buffer in bytes.
 *
 * @retval 0 on success.
 * @retval Negative errno code on failure.
 */
int hl78xx_get_band_default_config(char *hex_bndcfg, size_t SizeInBytes);
/**
 * @brief Trim leading zeros from a hexadecimal string.
 *
 * Removes any '0' characters from the beginning of the provided hex string,
 * returning a pointer to the first non-zero character.
 *
 * @param hex_str Null-terminated hexadecimal string.
 *
 * @return Pointer to the first non-zero digit in the string,
 *         or the last zero if the string is all zeros.
 */
const char *hl78xx_trim_leading_zeros(const char *hex_str);
/**
 * @brief Convert a binary bitmap to a trimmed hexadecimal string.
 *
 * Converts a bitmap into a hex string, removing leading zeros for a
 * compact representation. Useful for modem configuration commands.
 *
 * @param bitmap Pointer to the input binary bitmap.
 * @param hex_str Output buffer for the resulting hex string.
 * @param hex_str_len Size of the output buffer in bytes.
 */
void hl78xx_bitmap_to_hex_string_trimmed(const uint8_t *bitmap, char *hex_str, size_t hex_str_len);
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

int hl78xx_set_apn_internal(struct hl78xx_data *data, const char *apn, uint16_t size);

int hl78xx_api_func_set_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality functionality,
					    bool reset);

int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality);

int hl78xx_api_func_get_signal(const struct device *dev, const enum hl78xx_signal_type type,
			       int16_t *value);

int hl78xx_api_func_get_registration_status(const struct device *dev,
					    enum hl78xx_cell_rat_mode *tech,
					    enum hl78xx_registration_status *status);

int hl78xx_api_func_get_modem_info(const struct device *dev, enum hl78xx_modem_info_type type,
				   char *info, size_t size);

int hl78xx_api_func_set_apn(const struct device *dev, const char *apn, uint16_t size);

int hl78xx_api_func_modem_cmd_send_int(const struct device *dev, const char *cmd, uint16_t cmd_size,
				       const struct modem_chat_match *response_matches,
				       uint16_t matches_size);

void hl78xx_enter_state(struct hl78xx_data *data, enum hl78xx_state state);

void hl78xx_delegate_event(struct hl78xx_data *data, enum hl78xx_event evt);

void notif_carrier_off(void);
int check_if_any_socket_connected(void);

#endif /* HL78XX_H */
