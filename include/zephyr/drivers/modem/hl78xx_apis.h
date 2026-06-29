/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hl78xx_apis.h
 * @brief Header file for extended cellular API of HL78xx modems
 * @ingroup hl78xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_
#define ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <zephyr/modem/chat.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/sys/util_macro.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup hl78xx_interface HL78xx
 * @brief Sierra Wireless HL78xx cellular modems
 * @ingroup cellular_interface_ext
 * @{
 */
/* clang-format off */
/** Unknown RSSI value returned by AT+CSQ command */
#define CSQ_RSSI_UNKNOWN  (99)
/** Maximum valid RSSI code returned by AT+CSQ command */
#define CSQ_RSSI_MAX      (31)
/** Unknown RSRP value returned by AT+CESQ command */
#define CESQ_RSRP_UNKNOWN (255)
/** Maximum valid RSRP code returned by AT+CESQ command */
#define CESQ_RSRP_MAX     (97)
/** Unknown RSRQ value returned by AT+CESQ command */
#define CESQ_RSRQ_UNKNOWN (255)
/** Maximum valid RSRQ code returned by AT+CESQ command */
#define CESQ_RSRQ_MAX     (34)

/** Modem response type for generic ERROR responses returned by public AT helpers. */
#define HL78XX_MODEM_AT_ERROR     1
/** Modem response type for +CME ERROR responses returned by public AT helpers. */
#define HL78XX_MODEM_AT_CME_ERROR 2
/** Modem response type for +CMS ERROR responses returned by public AT helpers. */
#define HL78XX_MODEM_AT_CMS_ERROR 3

/**
 * Convert CSQ RSSI value to dBm
 * @param v RSSI value (0-31)
 * @return Signal strength in dBm (-113 to -51)
 */
#define CSQ_RSSI_TO_DB(v)  (-113 + (2 * (v)))
/**
 * Convert CESQ RSRP value to dBm
 * @param v RSRP value (0-97)
 * @return Conservative lower-bound representative in dBm (-141 to -44)
 */
#define CESQ_RSRP_TO_DB(v) (-141 + (v))
/**
 * Convert CESQ RSRQ value to dB
 * @param v RSRQ value (0-34)
 * @return Conservative integer representative in dB (-21 to -3)
 */
#define CESQ_RSRQ_TO_DB(v) (((v) == 0U) ? -21 : (-20 + ((v) / 2)))

/** Monitor is paused */
#define PAUSED 1
/** Monitor is active, default */
#define ACTIVE 0
/** Maximum length of modem manufacturer string */
#define MDM_MANUFACTURER_LENGTH        20
/** Maximum length of modem model string */
#define MDM_MODEL_LENGTH               32
/** Maximum length of modem revision string */
#define MDM_REVISION_LENGTH            64
/** Maximum length of modem IMEI string */
#define MDM_IMEI_LENGTH                16
/** Maximum length of modem IMSI string */
#define MDM_IMSI_LENGTH                23
/** Maximum length of modem ICCID string */
#define MDM_ICCID_LENGTH               22
/** Maximum length of APN string */
#define MDM_APN_MAX_LENGTH             64
/** Maximum length of certificate */
#define MDM_MAX_CERT_LENGTH            4096
/** Maximum length of hostname */
#define MDM_MAX_HOSTNAME_LEN           128
/** Maximum length of serial number string */
#define MDM_SERIAL_NUMBER_LENGTH       32
/** Recommended buffer size for extracted +CTZEU universal time strings */
#define HL78XX_CTZEU_UTIME_MAX_LEN     32
/** Maximum length of CEREG timer string */
#define HL78XX_CEREG_TIMER_STR_LEN     9
/** Maximum length of network address string */
#define HL78XX_NETWORK_ADDRESS_MAX_LEN 46
/** Pattern used to indicate end-of-file or completion of data transmission */
#define MDM_HL78XX_EOF_PATTERN         "--EOF--Pattern--"
/** Escape/termination sequence used to exit data mode and return to command mode */
#define MDM_HL78XX_TERMINATION_PATTERN "+++"
/** Response string indicating a successful data connection has been established */
#define MDM_HL78XX_CONNECT_STRING      "CONNECT"
/** Response string indicating the connection has been lost or terminated */
#define MDM_HL78XX_NO_CARRIER_STRING   "NO CARRIER"
/** Prefix for CME (Mobile Equipment) error responses, typically followed by an error code */
#define MDM_HL78XX_CME_ERROR_STRING    "+CME ERROR: "
/** Prefix for CMS (Message Service) error responses, typically related to SMS operations */
#define MDM_HL78XX_CMS_ERROR_STRING    "+CMS ERROR: "
/** Generic error response string indicating command failure */
#define MDM_HL78XX_ERROR_STRING        "ERROR"
/** Standard response string indicating successful execution of an AT command */
#define MDM_HL78XX_OK_STRING           "OK"

/**
 * @brief Initial active state for HL78xx monitors.
 *
 * Use this as the optional state argument when defining a monitor that should
 * start in the active state.
 */
#define HL78XX_MONITOR_ACTIVE false

/**
 * @brief Initial paused state for HL78xx monitors.
 *
 * Use this as the optional state argument when defining a monitor that should
 * start paused.
 */
#define HL78XX_MONITOR_PAUSED true

/**
 * @brief Resolve the optional monitor initial state argument.
 *
 * If no optional state argument is provided, the monitor starts active.
 * Otherwise, the provided state value is used.
 *
 * @param ... Optional monitor initial state, either @c HL78XX_MONITOR_ACTIVE
 *        or @c HL78XX_MONITOR_PAUSED.
 */
#define HL78XX_MONITOR_INITIAL_PAUSED(...) \
	COND_CODE_1(IS_EMPTY(__VA_ARGS__), (false), (__VA_ARGS__))

/* clang-format on */

/**
 * @brief Define an Event monitor to receive notifications in the system workqueue thread.
 *
 * @param name The monitor name.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c HL78XX_MONITOR_ACTIVE or @c HL78XX_MONITOR_PAUSED).
 *	      The default initial state of a monitor is active.
 */
#define HL78XX_EVT_MONITOR(name, _handler, ...)                                                    \
	static STRUCT_SECTION_ITERABLE(hl78xx_evt_monitor_entry, name) = {                         \
		.handler = _handler,                                                               \
		.next = NULL,                                                                      \
		.flags = {                                                                         \
			.direct = false,                                                           \
			.paused = HL78XX_MONITOR_INITIAL_PAUSED(__VA_ARGS__),                      \
		}}

/** Monitor any parsed unsolicited AT notification */
#define HL78XX_AT_MONITOR_ANY NULL

/**
 * @brief Define an AT monitor to receive parsed unsolicited AT notifications in the
 * system workqueue thread.
 *
 * @param name The monitor name.
 * @param _filter Exact unsolicited AT match pattern to receive, or
 *        @c HL78XX_AT_MONITOR_ANY to receive all parsed notifications.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c HL78XX_MONITOR_ACTIVE or @c HL78XX_MONITOR_PAUSED).
 *        The default initial state of a monitor is active.
 */
#define HL78XX_AT_MONITOR(name, _filter, _handler, ...)                                            \
	static STRUCT_SECTION_ITERABLE(hl78xx_at_monitor_entry, name) = {                          \
		.filter = _filter,                                                                 \
		.handler = _handler,                                                               \
		.next = NULL,                                                                      \
		.flags = {                                                                         \
			.direct = false,                                                           \
			.paused = HL78XX_MONITOR_INITIAL_PAUSED(__VA_ARGS__),                      \
		}}
/**
 * @brief Define an AT monitor to receive parsed unsolicited AT notifications directly
 * in the HL78xx modem RX workqueue context.
 *
 * Direct monitor callbacks run in the HL78xx modem RX workqueue context.
 * Handlers should keep processing short and must avoid operations that could
 * block modem receive processing for a long time.
 *
 * @param name The monitor name.
 * @param _filter Exact unsolicited AT match pattern to receive, or
 *        @c HL78XX_AT_MONITOR_ANY to receive all parsed notifications.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c HL78XX_MONITOR_ACTIVE or @c HL78XX_MONITOR_PAUSED).
 *        The default initial state of a monitor is active.
 */
#define HL78XX_AT_MONITOR_DIRECT(name, _filter, _handler, ...)                                     \
	static STRUCT_SECTION_ITERABLE(hl78xx_at_monitor_entry, name) = {                          \
		.filter = _filter,                                                                 \
		.handler = _handler,                                                               \
		.next = NULL,                                                                      \
		.flags = {                                                                         \
			.direct = true,                                                            \
			.paused = HL78XX_MONITOR_INITIAL_PAUSED(__VA_ARGS__),                      \
		}}

/**
 * @brief Cellular radio access technologies
 *
 * Supported radio access technology modes for HL78xx modem
 */
enum hl78xx_cell_rat_mode {
	/** LTE Cat-M1 radio access technology */
	HL78XX_RAT_CAT_M1 = 0,
	/** NB-IoT radio access technology */
	HL78XX_RAT_NB1,
#ifdef CONFIG_MODEM_HL78XX_12
	/** GSM radio access technology (HL7812 only) */
	HL78XX_RAT_GSM,
#ifdef CONFIG_MODEM_HL78XX_12_FW_R6
	/** NB-IoT Non-Terrestrial Network (HL7812 FW R6+) */
	HL78XX_RAT_NBNTN,
#endif /* CONFIG_MODEM_HL78XX_12_FW_R6 */
#endif /* CONFIG_MODEM_HL78XX_12 */
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	/** Automatic RAT selection mode */
	HL78XX_RAT_MODE_AUTO,
#endif
	/** No RAT mode */
	HL78XX_RAT_MODE_NONE,
	/** Number of valid RAT modes */
	HL78XX_RAT_COUNT = HL78XX_RAT_MODE_NONE
};

/**
 * @brief Raw AT+KSELACQ PRL RAT entries.
 *
 * These values follow the modem command syntax directly and are not the same
 * numeric encoding as enum hl78xx_cell_rat_mode.
 */
enum hl78xx_kselacq_rat {
	/** Clear PRL and disable automatic RAT switching. */
	HL78XX_KSELACQ_RAT_CLEAR = 0,
	/** PRL entry for Cat-M1. */
	HL78XX_KSELACQ_RAT_CAT_M1 = 1,
	/** PRL entry for NB-IoT. */
	HL78XX_KSELACQ_RAT_NB1 = 2,
	/** PRL entry for GSM. Supported only on HL7812. */
	HL78XX_KSELACQ_RAT_GSM = 3,
};

/**
 * @brief KSELACQ RAT configuration syntax
 */
struct kselacq_syntax {
	/** 0 = configure PRL, 1 = reserved. */
	bool mode;
	/** Preferred RAT in PRL position 1, expressed as raw AT+KSELACQ value. */
	enum hl78xx_kselacq_rat rat1;
	/** Preferred RAT in PRL position 2, expressed as raw AT+KSELACQ value. */
	enum hl78xx_kselacq_rat rat2;
	/** Preferred RAT in PRL position 3, expressed as raw AT+KSELACQ value. */
	enum hl78xx_kselacq_rat rat3;
};

/**
 * @brief Callback used to supply an optional runtime band override for a RAT.
 *
 * The modem driver stays agnostic to how callers store or derive runtime band
 * choices. When a provider is registered, hl78xx_band_cfg() asks for an
 * override band for the requested RAT and falls back to Kconfig defaults when
 * none is supplied.
 *
 * @param dev Cellular network device instance.
 * @param rat RAT currently being configured.
 * @param band Output band number, written only when the callback returns true.
 * @param user_data Opaque caller-owned context passed at registration time.
 * @return true when a valid runtime band override exists for @p rat.
 */
typedef bool (*hl78xx_runtime_band_provider_t)(const struct device *dev,
					       enum hl78xx_cell_rat_mode rat, uint16_t *band,
					       void *user_data);

/**
 * @brief Phone functionality modes
 *
 * AT+CFUN command modes for controlling modem operational state
 */
enum hl78xx_phone_functionality {
	/** SIM and modem powered off (minimum functionality) */
	HL78XX_SIM_POWER_OFF,
	/** Full functionality, modem operational */
	HL78XX_FULLY_FUNCTIONAL,
	/** Airplane mode, RF transmitters disabled */
	HL78XX_AIRPLANE = 4,
};

/**
 * @brief External SIM slot selection driven by the optional SIM-switch GPIO.
 *
 * Slot 1 maps to a logical low on the SIM-switch GPIO, and slot 2 maps to a
 * logical high. Devicetree polarity flags still apply to the physical pin.
 */
enum hl78xx_sim_slot {
	/** Select SIM slot 1 (logical low). */
	HL78XX_SIM_SLOT_1 = 0,
	/** Select SIM slot 2 (logical high). */
	HL78XX_SIM_SLOT_2 = 1,
};

/**
 * @brief Driver-managed modem restart modes.
 */
enum hl78xx_modem_restart_mode {
	/** Restart the modem with the dedicated reset pin. */
	HL78XX_MODEM_RESTART_HARD = 0,
	/** Restart the modem by issuing `AT+CFUN=4,1`. */
	HL78XX_MODEM_RESTART_SOFT,
};

/**
 * @brief Module status codes
 *
 * Status codes returned by AT+CPIN? indicating SIM and module state
 */
enum hl78xx_module_status {
	/** Module is ready to receive commands for the TE. No access code is required. */
	HL78XX_MODULE_READY = 0,
	/** Module is waiting for an access code. Use AT+CPIN? to determine it. */
	HL78XX_MODULE_WAITING_FOR_ACCESS_CODE,
	/** SIM card is not present */
	HL78XX_MODULE_SIM_NOT_PRESENT,
	/** Module is in "SIMlock" state */
	HL78XX_MODULE_SIMLOCK,
	/** Unrecoverable error */
	HL78XX_MODULE_UNRECOVERABLE_ERROR,
	/** Unknown state */
	HL78XX_MODULE_UNKNOWN_STATE,
	/** Inactive SIM */
	HL78XX_MODULE_INACTIVE_SIM
};

/**
 * @brief Cellular modem info type
 *
 * Types of modem information that can be queried
 */
enum hl78xx_modem_info_type {
	/** Modem Serial Number */
	HL78XX_MODEM_INFO_SERIAL_NUMBER,
	/** Current Baud Rate */
	HL78XX_MODEM_INFO_CURRENT_BAUD_RATE,
};

/**
 * @brief Network operator format options
 */
enum hl78xx_operator_format {
	/** Long alphanumeric operator name format (AT+COPS format 0) */
	HL78XX_OPERATOR_FORMAT_LONG_ALPHANUMERIC = 0,
	/** Short alphanumeric operator name format (AT+COPS format 1) */
	HL78XX_OPERATOR_FORMAT_SHORT_ALPHANUMERIC,
	/** Numeric operator name format / MCC-MNC (AT+COPS format 2) */
	HL78XX_OPERATOR_FORMAT_NUMERIC,
};

/**
 * @brief Cached +CEREG/+CREG registration details.
 */
struct hl78xx_cxreg_status {
	/** Registration status from +CEREG/+CREG. */
	enum cellular_registration_status reg_status;
	/** Parsed tracking area code is present. */
	bool has_tac;
	/** Tracking area code from +CEREG/+CREG. */
	uint32_t tac;
	/** Parsed cell ID is present. */
	bool has_cell_id;
	/** Cell ID from +CEREG/+CREG. */
	uint32_t cell_id;
	/** Parsed RAT mode is present. */
	bool has_rat_mode;
	/** RAT mode derived from +CEREG/+CREG AcT. */
	enum hl78xx_cell_rat_mode rat_mode;
	/** Cause type is present. */
	bool has_cause_type;
	/** Registration reject cause type. */
	int cause_type;
	/** Reject cause is present. */
	bool has_reject_cause;
	/** Registration reject cause value. */
	int reject_cause;
	/** Active time field is present. */
	bool has_active_time;
	/** Raw active time timer string. */
	char active_time[HL78XX_CEREG_TIMER_STR_LEN];
	/** TAU field is present. */
	bool has_tau;
	/** Raw TAU timer string. */
	char tau[HL78XX_CEREG_TIMER_STR_LEN];
};

/**
 * @brief Cached network information types.
 */
enum hl78xx_network_info_type {
	/** Access Point Name */
	HL78XX_NETWORK_INFO_APN,
	/** Current Radio Access Technology */
	HL78XX_NETWORK_INFO_CURRENT_RAT,
	/** Network Operator name in long alphanumeric format */
	HL78XX_NETWORK_INFO_NETWORK_OPERATOR_LONG_ALPHA,
	/** Network Operator name in short alphanumeric format */
	HL78XX_NETWORK_INFO_NETWORK_OPERATOR_SHORT_ALPHA,
	/** Network Operator name in numeric format */
	HL78XX_NETWORK_INFO_NETWORK_OPERATOR_NUMERIC,
	/** Current operator format from +COPS */
	HL78XX_NETWORK_INFO_OPERATOR_FORMAT,
	/** Tracking area code from +CEREG */
	HL78XX_NETWORK_INFO_TAC,
	/** Mobile country code parsed from numeric operator */
	HL78XX_NETWORK_INFO_MCC,
	/** Mobile network code parsed from numeric operator */
	HL78XX_NETWORK_INFO_MNC,
	/** Cell ID from +CEREG */
	HL78XX_NETWORK_INFO_CELL_ID,
	/** PDP IP address. */
	HL78XX_NETWORK_INFO_IP_ADDRESS,
	/** Primary DNS address. */
	HL78XX_NETWORK_INFO_DNS_PRIMARY,
	/** Active band number from AT+KBND? decoded from the bitmap. */
	HL78XX_NETWORK_INFO_ACTIVE_BAND,
	/** Signal-to-Interference-plus-Noise Ratio from the last +KCELLMEAS URC. */
	HL78XX_NETWORK_INFO_SINR,
};

/**
 * @brief Cached network operator information.
 */
struct hl78xx_network_operator {
	/** Operator is available. */
	bool has_operator;
	/** Operator name in the currently selected +COPS format. */
	char operator_name[MDM_MODEL_LENGTH];
	/** MCC is available. */
	bool has_mcc;
	/** Mobile country code. */
	uint16_t mcc;
	/** MNC is available. */
	bool has_mnc;
	/** Mobile network code. */
	uint16_t mnc;
	/** Current +COPS operator format. */
	enum hl78xx_operator_format format;
};

/**
 * @brief Cached network information.
 */
struct hl78xx_network_info {
	/** Cached network operator information. */
	struct hl78xx_network_operator operator_info;
	/** IP address. */
	char ip_address[HL78XX_NETWORK_ADDRESS_MAX_LEN];
	/** Primary DNS server. */
	char dns_primary[HL78XX_NETWORK_ADDRESS_MAX_LEN];
};

/**
 * @brief NMEA output port options
 *
 * Port selection for GNSS NMEA sentence output from the modem
 */
enum nmea_output_port {
	/** 0x00 — NMEA frames are not output */
	NMEA_OUTPUT_NONE = 0x00,
	/** 0x01 — NMEA frames are output on dedicated NMEA port over USB */
	NMEA_OUTPUT_USB_NMEA_PORT = 0x01,
	/** 0x03 — NMEA frames are output on UART1 */
	NMEA_OUTPUT_UART1 = 0x03,
	/** 0x04 — NMEA frames are output on the same port the +GNSSNMEA was received on */
	NMEA_OUTPUT_SAME_PORT = 0x04,
	/** 0x05 — NMEA frames are output on CMUX DLC1 */
	NMEA_OUTPUT_CMUX_DLC1 = 0x05,
	/** 0x06 — NMEA frames are output on CMUX DLC2 */
	NMEA_OUTPUT_CMUX_DLC2 = 0x06,
	/** 0x07 — NMEA frames are output on CMUX DLC3 */
	NMEA_OUTPUT_CMUX_DLC3 = 0x07,
	/** 0x08 — NMEA frames are output on CMUX DLC4 */
	NMEA_OUTPUT_CMUX_DLC4 = 0x08
};

#if defined(CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE) || defined(__DOXYGEN__)
/**
 * @brief A-GNSS assistance data validity mode
 *
 * Mode value returned by AT+GNSSAD? read command or used by write command
 */
enum hl78xx_agnss_mode {
	/** Data is not valid (read) / Delete data (write) */
	HL78XX_AGNSS_MODE_INVALID = 0,
	/** Data is valid (read) / Download data (write) */
	HL78XX_AGNSS_MODE_VALID = 1,
};

/**
 * @brief Number of days of predicted assistance data to download (write command)
 * or number of days before it expires (read command)
 *
 * Only these values are accepted by the AT+GNSSAD=1,\<days\> command
 */
enum hl78xx_agnss_days {
	/** Request 1 day of A-GNSS assistance data */
	HL78XX_AGNSS_DAYS_1 = 1,
	/** Request 2 days of A-GNSS assistance data */
	HL78XX_AGNSS_DAYS_2 = 2,
	/** Request 3 days of A-GNSS assistance data */
	HL78XX_AGNSS_DAYS_3 = 3,
	/** Request 7 days of A-GNSS assistance data */
	HL78XX_AGNSS_DAYS_7 = 7,
	/** Request 14 days of A-GNSS assistance data */
	HL78XX_AGNSS_DAYS_14 = 14,
	/** Request 28 days of A-GNSS assistance data */
	HL78XX_AGNSS_DAYS_28 = 28,
};

/**
 * @brief A-GNSS assistance data status structure
 *
 * Contains the parsed response from AT+GNSSAD? command.
 * When mode is VALID, the expiry fields indicate time remaining.
 */
struct hl78xx_agnss_status {
	/** Validity mode: 0 = invalid/empty, 1 = valid */
	enum hl78xx_agnss_mode mode;
	/** Days remaining before expiry (1-28), valid only when mode=1 */
	uint8_t days;
	/** Hours remaining before expiry (0-23), valid only when mode=1 */
	uint8_t hours;
	/** Minutes remaining before expiry (0-59), valid only when mode=1 */
	uint8_t minutes;
};
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
/**
 * @brief Power down event types
 *
 * Types of power down events reported by the modem
 */
enum power_down_event {
	/** Power down event: Modem is entering power down mode */
	POWER_DOWN_EVENT_ENTER = 0,
	/** Power down event: Modem is exiting power down mode */
	POWER_DOWN_EVENT_EXIT,
	/** No power down event */
	POWER_DOWN_EVENT_NONE,
};

/**
 * @brief Application responses to a pending power-down request
 *
 * These values control how the driver handles the stage-2 shutdown work after
 * HL78XX_POWER_DOWN_UPDATE is dispatched with POWER_DOWN_EVENT_ENTER.
 */
enum hl78xx_power_down_response {
	/** Proceed with shutdown as soon as the driver workqueue can run it */
	HL78XX_POWER_DOWN_RESPONSE_IMMEDIATE = 0,
	/** Extend the shutdown grace period from now by a caller-supplied timeout */
	HL78XX_POWER_DOWN_RESPONSE_RESCHEDULE,
};
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX
/**
 * @brief eDRX event types
 *
 * Types of eDRX events reported by the modem
 */
enum hl78xx_edrx_event {
	/** Modem exited eDRX idle mode */
	HL78XX_EDRX_EVENT_IDLE_EXIT = 0,
	/** Modem entered eDRX idle mode */
	HL78XX_EDRX_EVENT_IDLE_ENTER,
	/** No eDRX event */
	HL78XX_EDRX_EVENT_IDLE_NONE,
};
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#ifdef CONFIG_MODEM_HL78XX_PSM
/**
 * @brief PSM event types
 *
 * Types of Power Saving Mode events reported by the modem
 */
enum hl78xx_psmev_event {
	/** Modem exited PSM mode */
	HL78XX_PSM_EVENT_EXIT = 0,
	/** Modem entered PSM mode */
	HL78XX_PSM_EVENT_ENTER,
	/** No PSM event */
	HL78XX_PSM_EVENT_NONE,
};
#endif /* CONFIG_MODEM_HL78XX_PSM */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

/**
 * @brief Cellular measurement signal information.
 *
 * Holds received signal power values reported by the modem during
 * cell measurement procedures.
 */
struct k_cellmeas_signal_info {
	/** Reference Signal Received Power (RSRP) in dBm. */
	int16_t rsrp;
};
/**
 * @brief Cellular network structure
 *
 * Configuration for cellular network technology and band selection
 */
struct hl78xx_network {
	/** Cellular access technology */
	enum hl78xx_cell_rat_mode technology;
	/**
	 * List of bands to enable.
	 * List of bands, as defined by the specified cellular access technology,
	 * to enables. All supported bands are enabled if none are provided.
	 */
	uint16_t *bands;
	/** Size of bands array */
	uint16_t size;
};

/**
 * @brief Parsed +CTZEU update payload.
 *
 * The @p tz field already includes any daylight saving adjustment, matching the
 * modem specification for +CTZEU.
 */
struct hl78xx_ctzeu_update {
	/** Local timezone offset from GMT in quarter-hours, including DST adjustment. */
	int tz;
	/** Daylight saving indicator reported by the modem (0, 1, or 2). */
	int dst;
	/** Universal time field is present in the notification. */
	bool has_utime;
	/** Parsed universal time in UTC Unix epoch milliseconds. */
	int64_t date_time_ms;
	/** Parsed universal time in broken-down UTC form. Valid when has_utime is true. */
	struct tm utc_time;
};

/**
 * @brief HL78xx event types
 *
 * Asynchronous event notifications from the HL78xx modem
 */
enum hl78xx_evt_type {
	/** LTE Radio Access Technology changed */
	HL78XX_LTE_RAT_UPDATE,
	/** LTE network registration status changed */
	HL78XX_LTE_REGISTRATION_STAT_UPDATE,
	/** SIM registration status changed */
	HL78XX_LTE_SIM_REGISTRATION,
	/** Modem startup completed */
	HL78XX_LTE_MODEM_STARTUP,
	/** FOTA update status changed */
	HL78XX_LTE_FOTA_UPDATE_STATUS,
	/** DNS resolution path completed and modem is ready for data transmission */
	HL78XX_LTE_DNS_READY,
	/** The AT command interface is ready for application use. */
	HL78XX_LTE_AT_CMD_READY,
	/** Phone functionality changed (+CFUN) */
	HL78XX_LTE_PHONE_FUNCTIONALITY_UPDATE,
	/** Extended timezone and universal time update (+CTZEU) */
	HL78XX_LTE_CTZEU_UPDATE,
#ifdef CONFIG_HL78XX_GNSS
	/** GNSS engine initialized and ready */
	HL78XX_GNSS_ENGINE_READY,
	/** GNSS engine initialization event */
	HL78XX_GNSS_EVENT_INIT,
	/** GNSS search started */
	HL78XX_GNSS_EVENT_START,
	/** GNSS search stopped */
	HL78XX_GNSS_EVENT_STOP,
	/** GNSS position fix obtained */
	HL78XX_GNSS_EVENT_POSITION,
	/** GNSS start failed because LTE is active (shared RF path) */
	HL78XX_GNSS_EVENT_START_BLOCKED,
	/** GNSS search timeout expired */
	HL78XX_GNSS_EVENT_SEARCH_TIMEOUT,
	/** GNSS mode exited - modem is now in airplane mode, user can decide next step */
	HL78XX_GNSS_EVENT_MODE_EXITED,
#endif /* CONFIG_HL78XX_GNSS */
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_EDRX
	/** eDRX idle mode entered */
	HL78XX_EDRX_IDLE_UPDATE,
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#ifdef CONFIG_MODEM_HL78XX_PSM
	/** Modem PSM event update */
	HL78XX_LTE_PSMEV_UPDATE,
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	/** Modem power-down event update */
	HL78XX_POWER_DOWN_UPDATE,
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
	/** VGPIO pin went LOW */
	HL78XX_VGPIO_LOW,
	/** VGPIO pin went HIGH */
	HL78XX_VGPIO_HIGH,
	/** GPIO6 pin went LOW indicating sleep or power-down entry */
	HL78XX_GPIO6_LOW,
	/** GPIO6 pin went HIGH indicating wake from sleep or power-down */
	HL78XX_GPIO6_HIGH,
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	/** Cellular measurement update */
	HL78XX_CELLMEAS_UPDATE,
	/** Event type count */
	HL78XX_EVT_TYPE_COUNT
};
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
/**
 * @brief Device Services Indications (+WDSI)
 *
 * Enum representing AirVantage Device Services Indications
 */
enum wdsi_indication {
	/** Raised at startup if credentials for Bootstrap Server are present */
	WDSI_BOOTSTRAP_CREDENTIALS_PRESENT = 0,
	/** Device requests user agreement to connect to AirVantage */
	WDSI_USER_AGREEMENT_REQUEST = 1,
	/** AirVantage requests device to download firmware package */
	WDSI_FIRMWARE_DOWNLOAD_REQUEST = 2,
	/** AirVantage requests device to install firmware package */
	WDSI_FIRMWARE_INSTALL_REQUEST = 3,
	/** Starting authentication with Bootstrap or DM Server */
	WDSI_AUTHENTICATION_START = 4,
	/** Authentication failed */
	WDSI_AUTHENTICATION_FAILED = 5,
	/** Authentication succeeded, starting session */
	WDSI_AUTHENTICATION_SUCCESS = 6,
	/** Connection denied by server */
	WDSI_CONNECTION_DENIED = 7,
	/** DM session closed */
	WDSI_DM_SESSION_CLOSED = 8,
	/** Firmware package available for download */
	WDSI_FIRMWARE_AVAILABLE = 9,
	/** Firmware package downloaded and stored */
	WDSI_FIRMWARE_DOWNLOADED = 10,
	/** Firmware download issue, reason indicated by subcode */
	WDSI_FIRMWARE_DOWNLOAD_ISSUE = 11,
	/** Package verified and certified */
	WDSI_PACKAGE_VERIFIED_CERTIFIED = 12,
	/** Package verified but not certified */
	WDSI_PACKAGE_VERIFIED_NOT_CERTIFIED = 13,
	/** Starting firmware update */
	WDSI_FIRMWARE_UPDATE_START = 14,
	/** Firmware update failed */
	WDSI_FIRMWARE_UPDATE_FAILED = 15,
	/** Firmware updated successfully */
	WDSI_FIRMWARE_UPDATE_SUCCESS = 16,
	/** Download in progress, percentage indicated */
	WDSI_DOWNLOAD_IN_PROGRESS = 18,
	/** Session started with Bootstrap server (+WDSI: 23,0) or DM server (+WDSI: 23,1) */
	WDSI_SESSION_STARTED = 23
};
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

#ifdef CONFIG_HL78XX_GNSS
/**
 * @brief GNSS event type
 *
 * Types of GNSS events reported by the modem
 */
enum hl78xx_gnss_event_type {
	/** GNSS engine initialization event */
	HL78XX_GNSSEV_INITIALISATION = 0,
	/** GNSS search start event */
	HL78XX_GNSSEV_START = 1,
	/** GNSS search stop event */
	HL78XX_GNSSEV_STOP = 2,
	/** GNSS position fix event */
	HL78XX_GNSSEV_POSITION = 3
};
/**
 * @brief GNSS event status
 *
 * Status codes for GNSS operations
 */
enum hl78xx_event_status {
	/** Operation failed */
	HL78XX_STATUS_FAILED = 0,
	/** Operation succeeded */
	HL78XX_STATUS_SUCCESS = 1
};
/**
 * @brief GNSS position events (eventType = 3)
 *
 * Position fix status reported by the modem
 */
enum gnss_position_events {
	/** 0 — The GNSS fix is lost or not available yet */
	GNSS_FIX_LOST_OR_UNAVAILABLE = 0,
	/** 1 — An estimated GNSS (predicted) position is available */
	GNSS_ESTIMATED_POSITION_AVAILABLE = 1,
	/** 2 — A 2-dimensional GNSS position is available */
	GNSS_2D_POSITION_AVAILABLE = 2,
	/** 3 — A 3-dimensional position is available */
	GNSS_3D_POSITION_AVAILABLE = 3,
	/** 4 — GNSS fix has been changed to invalid position */
	GNSS_FIX_CHANGED_TO_INVALID = 4
};
#endif /* CONFIG_HL78XX_GNSS */

/**
 * @brief HL78xx event structure
 *
 * Container for asynchronous events from the HL78xx modem.
 * The type field indicates which union member is valid.
 */
struct hl78xx_evt {
	/** Event type */
	enum hl78xx_evt_type type;

	/** Event content (depends on type) */
	union {
		/** Network registration status (for HL78XX_LTE_REGISTRATION_STAT_UPDATE) */
		enum cellular_registration_status reg_status;
		/** Radio access technology mode (for HL78XX_LTE_RAT_UPDATE) */
		enum hl78xx_cell_rat_mode rat_mode;
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
		/** AirVantage device service indication */
		enum wdsi_indication wdsi_indication;
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
#ifdef CONFIG_HL78XX_GNSS
		/** GNSS event status */
		enum hl78xx_event_status event_status;
		/** GNSS position event type */
		enum gnss_position_events position_event;
#endif /* CONFIG_HL78XX_GNSS */
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_PSM
		/* PSM event */
		enum hl78xx_psmev_event psm_event;
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_EDRX
		/* eDRX event */
		enum hl78xx_edrx_event edrx_event;
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		/* Power-down event */
		enum power_down_event power_down_event;
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
		/** Cellular measurement event content */
		struct k_cellmeas_signal_info cellmeas;
		/** Full +CTZEU update payload */
		struct hl78xx_ctzeu_update ctzeu;
		/** Boolean status value */
		bool status;
		/** Integer value */
		int value;
	} content;
};

#if defined(CONFIG_HL78XX_GNSS_AUX_DATA_PARSER) || defined(__DOXYGEN__)
/**
 * @brief Parsed GSA sentence data (GNSS DOP and Active Satellites)
 *
 * Contains dilution of precision values from the GSA NMEA sentence.
 * Values are stored as fixed-point integers (multiply by appropriate scale).
 */
struct nmea_match_gsa_data {
	/** Fix type: 1=no fix, 2=2D fix, 3=3D fix */
	int32_t fix_type;
	/** Position Dilution of Precision (scaled) */
	int64_t pdop;
	/** Horizontal Dilution of Precision (scaled) */
	int64_t hdop;
	/** Vertical Dilution of Precision (scaled) */
	int64_t vdop;
};

/**
 * @brief Parsed GST sentence data (GNSS Pseudorange Error Statistics)
 *
 * Contains position error estimates from the GST NMEA sentence.
 * Error values are in meters, stored as fixed-point integers.
 */
struct nmea_match_gst_data {
	/** Reserved for internal use */
	int32_t i32;
	/** UTC time of associated position fix (HHMMSS format) */
	uint32_t gst_utc;
	/** RMS value of standard deviation of range inputs */
	int64_t rms;
	/** Standard deviation of semi-major axis of error ellipse (meters) */
	int64_t sd_major;
	/** Standard deviation of semi-minor axis of error ellipse (meters) */
	int64_t sd_minor;
	/** Orientation of semi-major axis of error ellipse (degrees from true north) */
	int64_t orient;
	/** Standard deviation of latitude error (meters) */
	int64_t lat_err;
	/** Standard deviation of longitude error (meters) */
	int64_t lon_err;
	/** Standard deviation of altitude error (meters) */
	int64_t alt_err;
};

/**
 * @brief Parsed EPU sentence data (HL78xx proprietary position error)
 *
 * Contains estimated position and velocity uncertainty from the
 * HL78xx proprietary PEPU NMEA sentence. All values are in meters
 * or meters/second, stored as fixed-point integers.
 */
struct nmea_match_epu_data {
	/** 3D position error estimate (meters) */
	int64_t pos_3d;
	/** 2D (horizontal) position error estimate (meters) */
	int64_t pos_2d;
	/** Latitude position error estimate (meters) */
	int64_t pos_lat;
	/** Longitude position error estimate (meters) */
	int64_t pos_lon;
	/** Altitude position error estimate (meters) */
	int64_t pos_alt;
	/** 3D velocity error estimate (m/s) */
	int64_t vel_3d;
	/** 2D (horizontal) velocity error estimate (m/s) */
	int64_t vel_2d;
	/** Heading velocity error estimate (m/s) */
	int64_t vel_hdg;
	/** East velocity error estimate (m/s) */
	int64_t vel_east;
	/** North velocity error estimate (m/s) */
	int64_t vel_north;
	/** Up (vertical) velocity error estimate (m/s) */
	int64_t vel_up;
};

/**
 * @brief Container for all GNSS auxiliary NMEA data
 *
 * Aggregates parsed data from supplementary NMEA sentences
 * (GSA, GST, PEPU) that provide quality and accuracy metrics.
 */
struct hl78xx_gnss_nmea_aux_data {
	/** GSA sentence data (DOP and fix type) */
	struct nmea_match_gsa_data gsa;
	/** GST sentence data (pseudorange error statistics) */
	struct nmea_match_gst_data gst;
	/** EPU sentence data (HL78xx proprietary position/velocity error) */
	struct nmea_match_epu_data epu;
};

/**
 * @brief GNSS auxiliary data callback function type
 *
 * @param dev Pointer to the GNSS device
 * @param aux_data Pointer to auxiliary NMEA data structure
 * @param size Size of the auxiliary data
 */
typedef void (*hl78xx_gnss_aux_data_callback_t)(const struct device *dev,
						const struct hl78xx_gnss_nmea_aux_data *aux_data,
						uint16_t size);

/**
 * @brief GNSS auxiliary data callback structure
 *
 * Registers a callback to receive GNSS auxiliary data
 */
struct hl78xx_gnss_aux_data_callback {
	/** Filter callback to GNSS data from this device if not NULL */
	const struct device *dev;
	/** Callback called when GNSS auxiliary data is published */
	hl78xx_gnss_aux_data_callback_t callback;
};

/**
 * @brief Register a callback structure for GNSS auxiliary data published
 *
 * @param _dev Device pointer
 * @param _callback The callback function
 */
#define GNSS_AUX_DATA_CALLBACK_DEFINE(_dev, _callback)                                             \
	static const STRUCT_SECTION_ITERABLE(hl78xx_gnss_aux_data_callback,                        \
					     _hl78xx_gnss_aux_data_callback__##_callback) = {      \
		.dev = _dev,                                                                       \
		.callback = _callback,                                                             \
	}
/**
 * @brief Register a callback structure for GNSS auxiliary data published
 *
 * @param _node_id Device tree node identifier
 * @param _callback The callback function
 */
#define GNSS_DT_AUX_DATA_CALLBACK_DEFINE(_node_id, _callback)                                      \
	static const STRUCT_SECTION_ITERABLE(hl78xx_gnss_aux_data_callback,                        \
					     _CONCAT_4(_hl78xx_gnss_aux_data_callback_,            \
						       DT_DEP_ORD(_node_id), _, _callback)) = {    \
		.dev = DEVICE_DT_GET(_node_id),                                                    \
		.callback = _callback,                                                             \
	}
#else
/**
 * @brief Register a callback structure for GNSS auxiliary data published
 *
 * @param _dev Device pointer
 * @param _callback The callback function
 */
#define GNSS_AUX_DATA_CALLBACK_DEFINE(_dev, _callback)
/**
 * @brief Register a callback structure for GNSS auxiliary data published
 *
 * @param _node_id Device tree node identifier
 * @param _callback The callback function
 */
#define GNSS_DT_AUX_DATA_CALLBACK_DEFINE(_node_id, _callback)
#endif
/**
 * @brief API function pointer for configuring networks
 *
 * @param dev Cellular network device instance
 * @param networks Array of network configurations
 * @param size Number of networks in array
 * @return 0 on success, negative errno on failure
 */
typedef int (*hl78xx_api_configure_networks)(const struct device *dev,
					     const struct hl78xx_network *networks, uint8_t size);

/**
 * @brief API function pointer for getting supported networks
 *
 * @param dev Cellular network device instance
 * @param networks Pointer to receive network array
 * @param size Pointer to receive array size
 * @return 0 on success, negative errno on failure
 */
typedef int (*hl78xx_api_get_supported_networks)(const struct device *dev,
						 const struct hl78xx_network **networks,
						 uint8_t *size);

/** API for getting network signal strength */
typedef int (*hl78xx_api_get_signal)(const struct device *dev, const enum cellular_signal_type type,
				     int16_t *value);

/**
 * @brief API function pointer for getting modem information
 *
 * @param dev Cellular network device instance
 * @param type Type of modem information
 * @param info Buffer to store information string
 * @param size Size of info buffer
 * @return 0 on success, negative errno on failure
 */
typedef int (*hl78xx_api_get_modem_info)(const struct device *dev,
					 const enum cellular_modem_info_type type, char *info,
					 size_t size);

/** API for getting registration status */
typedef int (*hl78xx_api_get_registration_status)(const struct device *dev,
						  enum cellular_access_technology tech,
						  enum cellular_registration_status *status);

/**
 * @brief API function pointer for setting phone functionality
 *
 * @param dev Cellular network device instance
 * @param functionality Phone functionality mode
 * @param reset Whether to reset modem
 * @return 0 on success, negative errno on failure
 */
typedef int (*hl78xx_api_set_phone_functionality)(const struct device *dev,
						  enum hl78xx_phone_functionality functionality,
						  bool reset);

/** API for get phone functionality */
typedef int (*hl78xx_api_get_phone_functionality)(const struct device *dev,
						  enum hl78xx_phone_functionality *functionality);

/** API for get phone functionality */
typedef int (*hl78xx_api_send_at_cmd)(const struct device *dev, const char *cmd, uint16_t cmd_size,
				      const struct modem_chat_match *response_matches,
				      uint16_t matches_size);

/**
 * @brief Parsed unsolicited AT notification payload.
 *
 * The @p pattern field is the canonical unsolicited match pattern from the HL78xx
 * driver table, while @p argv[0] contains the exact matched token produced by
 * modem_chat.
 */
struct hl78xx_at_notification {
	/** Canonical unsolicited match pattern from the driver table */
	const char *pattern;
	/** Parsed arguments, where argv[0] is the exact matched token */
	const char *const *argv;
	/** Number of parsed arguments */
	uint16_t argc;
};

/**
 * @brief AT monitor entry structure (forward declaration)
 */
struct hl78xx_at_monitor_entry;

/**
 * @brief AT monitor handler function type.
 *
 * User callback for receiving parsed unsolicited AT notifications.
 *
 * @param notif Pointer to the parsed AT notification.
 * @param mon Pointer to the monitor entry that received the notification.
 */
typedef void (*hl78xx_at_monitor_handler_t)(const struct hl78xx_at_notification *notif,
					    struct hl78xx_at_monitor_entry *mon);

/**
 * @brief AT monitor release function type.
 *
 * Called when an AT monitor entry is released and no longer referenced.
 *
 * @param mon Pointer to the monitor entry being released.
 */
typedef void (*hl78xx_at_monitor_release_t)(struct hl78xx_at_monitor_entry *mon);

/**
 * @brief AT monitor entry structure.
 *
 * Represents a registered parsed unsolicited AT monitor with callback, filter,
 * and state.
 */
struct hl78xx_at_monitor_entry {
	/** Exact unsolicited AT match pattern to monitor, or NULL for all */
	const char *filter;
	/** Monitor callback function */
	const hl78xx_at_monitor_handler_t handler;
	/** Optional callback invoked when the final runtime reference is dropped */
	hl78xx_at_monitor_release_t release;
	/** Link for runtime list */
	struct hl78xx_at_monitor_entry *next;
	/** Reference count for list membership and in-flight dispatch snapshots */
	atomic_t refcnt;
	/** Work item used to defer release out of unregister/dispatch context */
	struct k_work release_work;
	/** Monitor flags */
	struct {
		/** Monitor is paused */
		uint8_t paused: 1;
		/** Dispatch directly in HL78xx modem RX workqueue context */
		uint8_t direct: 1;
	} flags;
};

/**
 * @brief Event monitor entry structure (forward declaration)
 */
struct hl78xx_evt_monitor_entry;

/**
 * @brief Event monitor dispatcher function type
 *
 * Called to dispatch events to registered monitors
 *
 * @param notif Pointer to the event notification
 */
typedef void (*hl78xx_evt_monitor_dispatcher_t)(struct hl78xx_evt *notif);

/**
 * @brief Event monitor handler function type
 *
 * User callback for receiving event notifications
 *
 * @param notif Pointer to the event notification
 * @param mon Pointer to the monitor entry that received the event
 */
typedef void (*hl78xx_evt_monitor_handler_t)(struct hl78xx_evt *notif,
					     struct hl78xx_evt_monitor_entry *mon);

/**
 * @brief Event monitor release function type
 *
 * Called when an event monitor entry is released and no longer referenced.
 *
 * @param mon Pointer to the monitor entry being released
 */
typedef void (*hl78xx_evt_monitor_release_t)(struct hl78xx_evt_monitor_entry *mon);

/**
 * @brief Event monitor entry structure
 *
 * Represents a registered event monitor with callback and state
 */
struct hl78xx_evt_monitor_entry {
	/** Monitor callback function */
	const hl78xx_evt_monitor_handler_t handler;
	/** Optional callback invoked when the final runtime reference is dropped */
	hl78xx_evt_monitor_release_t release;
	/** Link for runtime list */
	struct hl78xx_evt_monitor_entry *next;
	/** Reference count for list membership and in-flight dispatch snapshots */
	atomic_t refcnt;
	/** Work item used to defer release callback execution */
	struct k_work release_work;
	/** Monitor flags */
	struct {
		/** Monitor is paused */
		uint8_t paused: 1;
		/** Dispatch in ISR context */
		uint8_t direct: 1;
	} flags;
};

/**
 * @brief Set phone functionality mode (internal implementation)
 *
 * Internal function to set the modem's phone functionality mode.
 * Users should call hl78xx_set_phone_functionality() instead.
 *
 * @param dev Cellular network device instance
 * @param functionality Phone functionality mode to set
 * @param reset If true, the modem will be reset as part of applying the functionality change.
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_set_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality functionality,
					    bool reset);

/**
 * @brief Request a driver-managed modem restart.
 *
 * Hard restart enters the reset-pin state-machine path. Soft restart enters
 * a driver-owned `AT+CFUN=4,1` path and waits for the modem restart
 * indication before re-running initialization.
 *
 * Soft restart requires the modem chat path to be active. If the modem is in
 * sleep, idle, or power-off transition states, the call returns `-EAGAIN`.
 * If the soft-reset request cannot be issued or the modem never reports the
 * restart indication, the driver falls back to the full restart path when
 * hardware restart control is available.
 *
 * @param dev Cellular network device instance.
 * @param mode Requested restart mode.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p dev or @p mode is invalid.
 * @retval -ENOTSUP if hard restart is requested but no reset GPIO is configured.
 * @retval -EAGAIN if a soft restart is requested while the modem is not ready
 *         to accept AT commands.
 * @retval -EBUSY if another restart is already in progress.
 */
int hl78xx_api_func_restart(const struct device *dev, enum hl78xx_modem_restart_mode mode);

/**
 * @brief Get phone functionality mode (internal implementation)
 *
 * Internal function to query the modem's phone functionality mode.
 * Users should call hl78xx_get_phone_functionality() instead.
 *
 * @param dev Cellular network device instance
 * @param functionality Pointer to store the current phone functionality mode
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality);

/**
 * @brief Get network signal strength (internal implementation)
 *
 * Internal function to retrieve cellular signal strength metrics.
 * Users should use the standard cellular API instead.
 *
 * @param dev Cellular network device instance
 * @param type Type of the signal to retrieve (RSSI, RSRP, RSRQ, etc.)
 * @param value Pointer to store the signal value
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_get_signal(const struct device *dev, const enum cellular_signal_type type,
			       int16_t *value);

/**
 * @brief Get vendor-specific modem information (internal implementation)
 *
 * Internal function to retrieve HL78xx-specific modem information.
 * Users should call hl78xx_get_modem_info() instead.
 *
 * @param dev Cellular network device instance
 * @param type Type of the modem info to retrieve
 * @param info Pointer to store the modem info
 * @param size Size of the info buffer
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_get_modem_info(const struct device *dev, enum hl78xx_modem_info_type type,
				   void *info, size_t size);

/**
 * @brief Get standard (Zephyr cellular API) modem information from cache
 *
 * Reads identity fields cached by the driver during the init script.
 * Does NOT issue any AT command and is safe to call from any context.
 *
 * @param dev Cellular network device instance
 * @param type Zephyr cellular modem info type
 * @param info Buffer to store the info string
 * @param size Buffer size in bytes
 * @return 0 on success, -ENOTSUP if type not handled, negative errno on failure
 */
int hl78xx_api_func_get_modem_info_standard(const struct device *dev,
					    enum cellular_modem_info_type type, char *info,
					    size_t size);

/**
 * @brief Get cached vendor-specific network information.
 *
 * Internal function to retrieve HL78xx-specific network information.
 * Users should call hl78xx_get_network_info() instead.
 *
 * @param dev Cellular network device instance
 * @param type Type of the network info to retrieve
 * @param info Pointer to store the network info
 * @param size Size of the info buffer
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_get_network_info(const struct device *dev, enum hl78xx_network_info_type type,
				     void *info, size_t size);

/**
 * @brief Check whether the current RSRP meets the configured minimum threshold.
 *
 * Uses the driver's standard signal query path and compares the returned RSRP
 * in dBm against CONFIG_MODEM_MIN_ALLOWED_SIGNAL_STRENGTH.
 *
 * @param dev Cellular network device instance
 * @param is_valid Output flag set to true when the threshold is met
 * @return 0 on success, negative errno on failure
 */
int hl78xx_api_func_get_rsrp_validity(const struct device *dev, bool *is_valid);

/**
 * @brief Check whether the current RSRQ meets the configured minimum threshold.
 *
 * Uses the driver's standard signal query path and compares the returned RSRQ
 * in dB against CONFIG_MODEM_MIN_ALLOWED_SIGNAL_QUALITY.
 *
 * @param dev Cellular network device instance
 * @param is_valid Output flag set to true when the threshold is met
 * @return 0 on success, negative errno on failure
 */
int hl78xx_api_func_get_rsrq_validity(const struct device *dev, bool *is_valid);

/**
 * @brief Check whether the current SINR meets the configured minimum threshold.
 *
 * Uses the cached +KCELLMEAS-derived SINR value and compares it in dB against
 * CONFIG_MODEM_MIN_ALLOWED_SINR.
 *
 * @param dev Cellular network device instance
 * @param is_valid Output flag set to true when the threshold is met
 * @return 0 on success, negative errno on failure
 */
int hl78xx_api_func_get_sinr_validity(const struct device *dev, bool *is_valid);

/**
 * @brief Set the +COPS network operator format.
 *
 * @param dev Cellular network device instance
 * @param format Desired operator format to set
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_set_network_operator_format(const struct device *dev,
						enum hl78xx_operator_format format);

#ifdef CONFIG_MODEM_HL78XX_AUTORAT
/**
 * @brief Set a new Preferred RAT List through AT+KSELACQ.
 *
 * Sends `AT+KSELACQ=0,<rat1>,<rat2>,<rat3>` immediately, updates the driver's
 * cached PRL values, and does not force an immediate modem restart.
 *
 * Duplicate RAT entries are allowed to bias the modem's search order. To clear
 * the PRL and disable automatic RAT switching, pass rat1/rat2/rat3 as
 * HL78XX_KSELACQ_RAT_CLEAR; the driver emits `AT+KSELACQ=0,0` for that case.
 *
 * @param dev Cellular network device instance
 * @param kselacq_rats Raw `AT+KSELACQ` PRL entries to set
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_set_prl(const struct device *dev, const struct kselacq_syntax kselacq_rats);

/**
 * @brief Get the current Preferred RAT List.
 *
 * Retrieves the current raw `AT+KSELACQ` PRL values from the modem cache.
 *
 * @param dev Cellular network device instance
 * @param kselacq_rats Pointer to store the current PRL entries
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_get_prl(const struct device *dev, struct kselacq_syntax *kselacq_rats);
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

/**
 * @brief Register or clear a runtime band provider for the driver.
 *
 * When a provider is registered, hl78xx_band_cfg() may use its per-RAT band
 * override instead of the default Kconfig bitmap. Passing NULL clears the
 * provider.
 *
 * @param dev Cellular network device instance.
 * @param provider Callback invoked to obtain a runtime band override.
 * @param user_data Opaque context passed back to @p provider.
 * @return 0 on success, negative errno on failure.
 */
int hl78xx_set_runtime_band_provider(const struct device *dev,
				     hl78xx_runtime_band_provider_t provider, void *user_data);

/**
 * @brief Send dynamic AT command (internal implementation)
 *
 * Internal function to send arbitrary AT commands to the modem.
 * Users should call hl78xx_modem_cmd_send() instead.
 *
 * @param dev Cellular network device instance
 * @param cmd AT command to send
 * @param cmd_size Size of the AT command
 * @param response_matches Expected response patterns
 * @param matches_size Number of response patterns
 * @return 0 if successful, negative errno on failure
 */
int hl78xx_api_func_modem_dynamic_cmd_send(const struct device *dev, const char *cmd,
					   uint16_t cmd_size,
					   const struct modem_chat_match *response_matches,
					   uint16_t matches_size);

/**
 * @brief Send a pre-formatted AT command to the modem.
 *
 * Returns 0 on OK responses, a positive encoded modem error on
 * ERROR/+CME ERROR/+CMS ERROR responses, and a negative errno on
 * transport failure. Callers must format the command string first
 * with snprintf() when variable arguments are needed.
 *
 * @param dev Cellular network device instance
 * @param cmd AT command string to send
 * @return 0 on success, positive encoded modem error, or negative errno on failure
 */
int hl78xx_modem_at_send(const struct device *dev, const char *cmd);

/**
 * @brief Send a pre-formatted AT command and copy the whole modem response
 * into a caller-supplied buffer.
 *
 * Callers must format the command string first with snprintf() when
 * variable arguments are needed. The response buffer receives the full
 * modem response collected by the driver.
 *
 * @param dev Cellular network device instance
 * @param response Output buffer for the whole modem response
 * @param response_size Output buffer size
 * @param cmd AT command string to send
 * @return 0 on success, positive encoded modem error, or negative errno on failure
 */
int hl78xx_modem_at_cmd(const struct device *dev, char *response, size_t response_size,
			const char *cmd);

/**
 * @brief Get modem info for the device
 *
 * @param dev Cellular network device instance
 * @param type Type of the modem info requested
 * @param info Info string destination
 * @param size Info string size
 *
 * @retval 0 if successful.
 * @retval -ENOTSUP if API is not supported by cellular network device.
 * @retval -ENODATA if modem does not provide info requested
 * @retval Negative errno-code from chat module otherwise.
 */
static inline int hl78xx_get_modem_info(const struct device *dev,
					const enum hl78xx_modem_info_type type, void *info,
					size_t size)
{
	return hl78xx_api_func_get_modem_info(dev, type, info, size);
}

/**
 * @brief Get cached network info for the device.
 *
 * @param dev Cellular network device instance
 * @param type Type of the network info requested
 * @param info Info destination buffer
 * @param size Size of the destination buffer
 *
 * @retval 0 if successful.
 * @retval -ENOTSUP if the type is not supported.
 * @retval -ENODATA if the modem does not provide the requested info.
 * @retval Negative errno-code from chat module otherwise.
 */
static inline int hl78xx_get_network_info(const struct device *dev,
					  enum hl78xx_network_info_type type, void *info,
					  size_t size)
{
	return hl78xx_api_func_get_network_info(dev, type, info, size);
}

/**
 * @brief Check whether the current RSRP meets the configured threshold.
 *
 * @param dev Cellular network device instance
 * @param is_valid Output flag set to true when the threshold is met
 * @retval 0 on success.
 * @retval Negative errno-code on failure.
 */
static inline int hl78xx_get_rsrp_validity(const struct device *dev, bool *is_valid)
{
	return hl78xx_api_func_get_rsrp_validity(dev, is_valid);
}

/**
 * @brief Check whether the current RSRQ meets the configured threshold.
 *
 * @param dev Cellular network device instance
 * @param is_valid Output flag set to true when the threshold is met
 * @retval 0 on success.
 * @retval Negative errno-code on failure.
 */
static inline int hl78xx_get_rsrq_validity(const struct device *dev, bool *is_valid)
{
	return hl78xx_api_func_get_rsrq_validity(dev, is_valid);
}

/**
 * @brief Check whether the current SINR meets the configured threshold.
 *
 * @param dev Cellular network device instance
 * @param is_valid Output flag set to true when the threshold is met
 * @retval 0 on success.
 * @retval Negative errno-code on failure.
 */
static inline int hl78xx_get_sinr_validity(const struct device *dev, bool *is_valid)
{
	return hl78xx_api_func_get_sinr_validity(dev, is_valid);
}

#ifdef CONFIG_MODEM_HL78XX_AUTORAT
/**
 * @brief Set a new raw `AT+KSELACQ` Preferred RAT List.
 *
 * @param dev Pointer to the modem device instance.
 * @param prl Preferred RAT List expressed as raw `AT+KSELACQ` values.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the inputs are invalid.
 * @retval Negative errno on modem command failure.
 */
static inline int hl78xx_set_prl(const struct device *dev, struct kselacq_syntax prl)
{
	return hl78xx_api_func_set_prl(dev, prl);
}
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

/**
 * @brief Set the modem phone functionality mode.
 *
 * Configures the operational state of the modem (e.g., full, airplane, or minimum functionality).
 * Optionally, the modem can be reset during this transition.
 *
 * @param dev Pointer to the modem device instance.
 * @param functionality Desired phone functionality mode to be set.
 *                      (e.g., full, airplane, minimum – see enum hl78xx_phone_functionality)
 * @param reset If true, the modem will be reset as part of applying the functionality change.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an invalid parameter is passed.
 * @retval -EIO on communication or command failure with the modem.
 */
static inline int hl78xx_set_phone_functionality(const struct device *dev,
						 enum hl78xx_phone_functionality functionality,
						 bool reset)
{
	return hl78xx_api_func_set_phone_functionality(dev, functionality, reset);
}

/**
 * @brief Request a driver-managed modem restart.
 *
 * @param dev Pointer to the modem device instance.
 * @param mode Restart mode to execute.
 *
 * @retval 0 on success.
 * @retval Negative errno-code on failure.
 */
static inline int hl78xx_restart_modem(const struct device *dev,
				       enum hl78xx_modem_restart_mode mode)
{
	return hl78xx_api_func_restart(dev, mode);
}

/**
 * @brief Get the current phone functionality mode of the modem.
 *
 * Queries the modem to retrieve its current operational mode, such as
 * full functionality, airplane mode, or minimum functionality.
 *
 * @param dev Pointer to the modem device instance.
 * @param functionality Pointer to store the retrieved functionality mode.
 *                      (see enum hl78xx_phone_functionality)
 *
 * @retval 0 on success.
 * @retval -EINVAL if the input parameters are invalid.
 * @retval -EIO if the modem fails to respond or returns an error.
 */
static inline int hl78xx_get_phone_functionality(const struct device *dev,
						 enum hl78xx_phone_functionality *functionality)
{
	return hl78xx_api_func_get_phone_functionality(dev, functionality);
}
/**
 * @brief Send an AT command to the modem and wait for a matched response.
 *
 * Transmits the specified AT command to the modem and waits for a response that matches
 * one of the expected patterns defined in the response match table.
 *
 * @param dev Pointer to the modem device instance.
 * @param cmd Pointer to the AT command string to be sent.
 * @param cmd_size Length of the AT command string in bytes.
 * @param response_matches Pointer to an array of expected response patterns.
 *                         (see struct modem_chat_match)
 * @param matches_size Number of response patterns in the array.
 *
 * @retval 0 on successful command transmission and response match.
 * @retval -EINVAL if any parameter is invalid.
 * @retval -ETIMEDOUT if the modem did not respond in the expected time.
 * @retval -EIO on communication failure or if response did not match.
 */
static inline int hl78xx_modem_cmd_send(const struct device *dev, const char *cmd,
					uint16_t cmd_size,
					const struct modem_chat_match *response_matches,
					uint16_t matches_size)
{

	return hl78xx_api_func_modem_dynamic_cmd_send(dev, cmd, cmd_size, response_matches,
						      matches_size);
}

/**
 * @brief Return the modem error type encoded in hl78xx_modem_at_send/cmd return values.
 *
 * @param error Return value from hl78xx_modem_at_send() or hl78xx_modem_at_cmd().
 *
 * @retval HL78XX_MODEM_AT_ERROR for ERROR responses.
 * @retval HL78XX_MODEM_AT_CME_ERROR for +CME ERROR responses.
 * @retval HL78XX_MODEM_AT_CMS_ERROR for +CMS ERROR responses.
 */
static inline int hl78xx_modem_at_err_type(int error)
{
	return (error & 0x00ff0000U) >> 16;
}

/**
 * @brief Return the modem CME/CMS error code encoded in hl78xx_modem_at_send/cmd.
 *
 * @param error Return value from hl78xx_modem_at_send() or hl78xx_modem_at_cmd().
 *
 * @return Encoded modem error value, or 0 for generic ERROR responses.
 */
static inline int hl78xx_modem_at_err(int error)
{
	return (error & 0xff00ffffU);
}
/**
 * @brief Convert raw RSSI value from the modem to dBm.
 *
 * Parses the RSSI value reported by the modem (typically from an AT command response)
 * and converts it to a corresponding signal strength in dBm, as defined by 3GPP TS 27.007.
 *
 * @param rssi Raw RSSI value (0–31 or 99 for not known or not detectable).
 * @param value Pointer to store the converted RSSI in dBm.
 *
 * @retval 0 on successful conversion.
 * @retval -EINVAL if the RSSI value is out of valid range or unsupported.
 */
static inline int hl78xx_parse_rssi(uint8_t rssi, int16_t *value)
{
	/* AT+CSQ returns a response +CSQ: \<rssi\>,\<ber\> where:
	 * - rssi is a integer from 0 to 31 whose values describes a signal strength
	 *   between -113 dBm for 0 and -51dbM for 31 or unknown for 99
	 * - ber is an integer from 0 to 7 that describes the error rate, it can also
	 *   be 99 for an unknown error rate
	 */
	if ((rssi == CSQ_RSSI_UNKNOWN) || (rssi > CSQ_RSSI_MAX)) {
		return -EINVAL;
	}

	*value = (int16_t)CSQ_RSSI_TO_DB(rssi);
	return 0;
}
/**
 * @brief Convert raw RSRP value from the modem to dBm.
 *
 * Parses the Reference Signal Received Power (RSRP) value reported by the modem
 * and converts it into a corresponding signal strength in dBm, typically based on
 * 3GPP TS 36.133 specifications.
 *
 * @param rsrp Raw RSRP value (commonly in the range 0–97, or 255 if unknown).
 * @param value Pointer to store the converted RSRP in dBm.
 *
 * @retval 0 on successful conversion.
 * @retval -EINVAL if the RSRP value is out of range or represents an unknown value.
 */
static inline int hl78xx_parse_rsrp(uint8_t rsrp, int16_t *value)
{
	/* AT+CESQ returns a response
	 * +CESQ: \<rxlev\>,\<ber\>,\<rscp\>,\<ecn0\>,\<rsrq\>,\<rsrp\> where:
	 * rsrq is a integer from 0 to 34 whose values describes the Reference
	 * Signal Receive Quality between -20 dB for 0 and -3 dB for 34
	 * (0.5 dB steps), or unknown for 255
	 * rsrp is an integer from 0 to 97 that describes the Reference Signal
	 * Receive Power between below -140 dBm for 0 and -44 dBm for 97
	 * (1 dBm steps), or unknown for 255. Return a conservative lower-bound
	 * integer so threshold comparisons do not treat code 0 as valid for a
	 * -140 dBm minimum.
	 */
	if ((rsrp == CESQ_RSRP_UNKNOWN) || (rsrp > CESQ_RSRP_MAX)) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRP_TO_DB(rsrp);
	return 0;
}
/**
 * @brief Convert raw RSRQ value from the modem to dB.
 *
 * Parses the Reference Signal Received Quality (RSRQ) value provided by the modem
 * and converts it into a signal quality measurement in decibels (dB), as specified
 * by 3GPP TS 36.133.
 *
 * @param rsrq Raw RSRQ value (typically 0–34, or 255 if unknown).
 * @param value Pointer to store the converted RSRQ in dB.
 *
 * @retval 0 on successful conversion.
 * @retval -EINVAL if the RSRQ value is out of valid range or indicates unknown.
 */
static inline int hl78xx_parse_rsrq(uint8_t rsrq, int16_t *value)
{
	/* AT+CESQ encodes RSRQ in 0.5 dB steps. Return a conservative integer dB
	 * representative so code 0 stays below a -20 dB threshold.
	 */
	if ((rsrq == CESQ_RSRQ_UNKNOWN) || (rsrq > CESQ_RSRQ_MAX)) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRQ_TO_DB(rsrq);
	return 0;
}
/**
 * @brief Pause monitor.
 *
 * Pause monitor @p mon from receiving notifications.
 *
 * @param mon The monitor to pause.
 */
static inline void hl78xx_evt_monitor_pause(struct hl78xx_evt_monitor_entry *mon)
{
	mon->flags.paused = true;
}
/**
 * @brief Resume monitor.
 *
 * Resume forwarding notifications to monitor @p mon.
 *
 * @param mon The monitor to resume.
 */
static inline void hl78xx_evt_monitor_resume(struct hl78xx_evt_monitor_entry *mon)
{
	mon->flags.paused = false;
}

/**
 * @brief Pause AT monitor.
 *
 * Pause monitor @p mon from receiving parsed unsolicited AT notifications.
 *
 * @param mon The monitor to pause.
 */
static inline void hl78xx_at_monitor_pause(struct hl78xx_at_monitor_entry *mon)
{
	mon->flags.paused = true;
}

/**
 * @brief Resume AT monitor.
 *
 * Resume forwarding parsed unsolicited AT notifications to monitor @p mon.
 *
 * @param mon The monitor to resume.
 */
static inline void hl78xx_at_monitor_resume(struct hl78xx_at_monitor_entry *mon)
{
	mon->flags.paused = false;
}

/**
 * @brief Register an AT monitor.
 *
 * Adds a runtime monitor to the list of registered AT monitors. The monitor
 * entry must not already be registered and must not have outstanding runtime
 * references from a previous registration.
 *
 * Monitor entries should be zero-initialized before first registration.
 *
 * @param mon Pointer to the monitor entry to register.
 *
 * @return 0 on success.
 * @return -EINVAL if @p mon is NULL.
 * @return -EALREADY if @p mon is already registered.
 * @return -EBUSY if @p mon still has outstanding runtime references.
 * @return -ENOMEM if the maximum number of runtime monitors is reached.
 */
int hl78xx_at_monitor_register(struct hl78xx_at_monitor_entry *mon);

/**
 * @brief Unregister an AT monitor.
 *
 * Removes a runtime monitor from future dispatch snapshots. This function does
 * not wait for callbacks that are already in progress or for snapshots that
 * already captured the monitor entry.
 *
 * If the monitor entry is dynamically allocated, the caller must not free or
 * reuse it immediately after this function returns. Instead, provide a release
 * callback in the monitor entry and free the entry from that callback. The
 * release callback is invoked after the final runtime reference is dropped.
 *
 * This function may be called from a monitor callback.
 *
 * @param mon Pointer to the monitor entry to unregister.
 *
 * @return 0 on success.
 * @return -EINVAL if @p mon is NULL.
 * @return -ENOENT if @p mon is not registered.
 */
int hl78xx_at_monitor_unregister(struct hl78xx_at_monitor_entry *mon);

/**
 * @brief Set the event notification handler for HL78xx modem events.
 *
 * Registers a callback handler to receive asynchronous event notifications
 * from the HL78xx modem, such as network registration changes, GNSS updates,
 * or other modem-generated events.
 *
 * @param handler Function pointer to the event monitor callback.
 *                Pass NULL to clear the existing handler.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the handler parameter is invalid.
 */
int hl78xx_evt_notif_handler_set(hl78xx_evt_monitor_dispatcher_t handler);

/**
 * @brief Register an event monitor.
 *
 * Adds a runtime monitor to the list of registered event monitors. The monitor
 * entry must not already be registered and must not have outstanding runtime
 * references from a previous registration.
 *
 * Monitor entries should be zero-initialized before first registration.
 *
 * @param mon Pointer to the monitor entry to register.
 *
 * @return 0 on success.
 * @return -EINVAL if @p mon is NULL.
 * @return -EALREADY if @p mon is already registered.
 * @return -EBUSY if @p mon still has outstanding runtime references.
 * @return -ENOMEM if the maximum number of runtime monitors is reached.
 */
int hl78xx_evt_monitor_register(struct hl78xx_evt_monitor_entry *mon);

/**
 * @brief Unregister an event monitor.
 *
 * Removes a runtime monitor from future dispatch snapshots. This function does
 * not wait for callbacks that are already in progress or for snapshots that
 * already captured the monitor entry.
 *
 * If the monitor entry is dynamically allocated, the caller must not free or
 * reuse it immediately after this function returns. Instead, provide a release
 * callback in the monitor entry and free the entry from that callback. The
 * release callback is invoked after the final runtime reference is dropped.
 *
 * This function may be called from a monitor callback.
 *
 * @param mon Pointer to the monitor entry to unregister.
 *
 * @return 0 on success.
 * @return -EINVAL if @p mon is NULL.
 * @return -ENOENT if @p mon is not registered.
 */
int hl78xx_evt_monitor_unregister(struct hl78xx_evt_monitor_entry *mon);

/**
 * @brief Convert HL78xx RAT mode to standard cellular API
 *
 * Maps HL78xx-specific radio access technology enum to the
 * standard Zephyr cellular API access technology enum.
 *
 * @param rat_mode HL78xx RAT mode
 * @return Corresponding cellular_access_technology value
 */
enum cellular_access_technology hl78xx_rat_to_access_tech(enum hl78xx_cell_rat_mode rat_mode);

/**
 * @brief Convert standard cellular access technology to HL78xx RAT mode.
 *
 * @param access_tech Standard cellular access technology.
 * @return Corresponding HL78xx RAT mode.
 */
enum hl78xx_cell_rat_mode hl78xx_access_tech_to_rat(enum cellular_access_technology access_tech);
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
/**
 * @brief Start an AirVantage Device Management (DM) session
 *
 * Initiates a connection to the Sierra Wireless AirVantage server
 * for device management operations.
 *
 * @param dev Pointer to the modem device
 * @return 0 on success, negative errno on failure
 */
int hl78xx_start_airvantage_dm_session(const struct device *dev);

/**
 * @brief Stop an AirVantage Device Management (DM) session
 *
 * Terminates the active connection to the AirVantage server.
 *
 * @param dev Pointer to the modem device
 * @return 0 on success, negative errno on failure
 */
int hl78xx_stop_airvantage_dm_session(const struct device *dev);
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

/**
 * @brief Drive the modem WAKE pin low.
 *
 * This directly maps to `gpio_pin_set_dt(&config->mdm_gpio_wake, 0)` inside
 * the driver.
 *
 * @param dev Pointer to the modem device.
 * @return 0 on success.
 * @return -EINVAL if @p dev is invalid.
 * @return -ENOTSUP if the modem WAKE GPIO is not configured in devicetree.
 * @return Negative errno from the GPIO driver on failure.
 */
int hl78xx_set_wake_pin_low(const struct device *dev);

/**
 * @brief Drive the modem WAKE pin high.
 *
 * This directly maps to `gpio_pin_set_dt(&config->mdm_gpio_wake, 1)` inside
 * the driver.
 *
 * @param dev Pointer to the modem device.
 * @return 0 on success.
 * @return -EINVAL if @p dev is invalid.
 * @return -ENOTSUP if the modem WAKE GPIO is not configured in devicetree.
 * @return Negative errno from the GPIO driver on failure.
 */
int hl78xx_set_wake_pin_high(const struct device *dev);

/**
 * @brief Select the active external SIM slot.
 *
 * This only drives the optional `mdm-sim-switch-gpios` line exposed by the
 * modem devicetree node. It does not perform any modem state transition or
 * restart sequence, so callers should switch SIMs only when the modem is in a
 * safe state for the attached hardware.
 *
 * @param dev Pointer to the modem device.
 * @param sim_slot SIM slot to select. Only slot 1 and slot 2 are supported.
 * @return 0 on success.
 * @return -EINVAL if @p dev or @p sim_slot is invalid.
 * @return -ENOTSUP if the modem SIM-switch GPIO is not configured in
 *         devicetree.
 * @return Negative errno from the GPIO driver on failure.
 */
int hl78xx_set_active_sim(const struct device *dev, enum hl78xx_sim_slot sim_slot);

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
/**
 * @brief Wake the modem from PSM sleep
 *
 * Sends a wakeup signal to the modem, triggering the RESUME event in the
 * driver state machine. The modem will re-open the UART bus and proceed
 * to either GNSS (if a GNSS request is pending in post-PSM mode) or
 * LTE registration (AWAIT_REGISTERED).
 *
 * This is the user-facing API for the POST-PSM GNSS workflow:
 * 1. hl78xx_enter_gnss_mode() — queues GNSS, modem enters PSM
 * 2. (modem sleeps, waits for user trigger)
 * 3. hl78xx_wakeup_modem() — wakes the modem, GNSS runs before LTE
 * 4. GNSS completes, modem returns to LTE registration
 *
 * Can also be used independently of GNSS to wake the modem for any
 * purpose (e.g. sending data earlier than the next PSM cycle).
 *
 * @param dev Pointer to the modem device
 * @return 0 on success
 * @return -EALREADY if modem is not in sleep state
 * @return -EINVAL if dev is invalid
 */
int hl78xx_wakeup_modem(const struct device *dev);

#ifdef CONFIG_MODEM_HL78XX_EDRX
/**
 * @brief Get the remaining time for the modem to go in eDRX idle state
 *
 * @param dev Pointer to the modem device
 * @return Remaining time in milliseconds, or negative errno on failure
 */
int hl78xx_edrx_get_time_to_sleep(const struct device *dev);
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

#ifdef CONFIG_HL78XX_GNSS
/**
 * @brief Enter GNSS mode
 *
 * Switches modem from LTE mode to GNSS mode. The exact transition depends
 * on the modem's current power state:
 *
 * **Without low-power mode (default / airplane mode path):**
 * 1. Carrier off (notify app, close sockets)
 * 2. Put modem in airplane mode (AT+CFUN=4)
 * 3. Initialize GNSS engine
 * 4. Fire HL78XX_GNSS_ENGINE_READY event when complete
 *
 * **With low-power mode (PSM path):**
 * 1. Mark GNSS as pending
 * 2. Wait for modem to enter PSM sleep (PSMEV:1)
 * 3. Wake modem (UART only - LTE stays hibernating)
 * 4. Initialize GNSS engine (no CFUN=4 needed, RF path already free)
 * 5. Fire HL78XX_GNSS_ENGINE_READY event when complete
 *
 * Per HL78xx GNSS App Note 5.3: "GNSS can be used in PSM mode" because
 * the LTE modem is unavailable and the shared RF Rx path is free.
 *
 * After entering GNSS mode, call hl78xx_queue_gnss_search() to start fix acquisition.
 *
 * @param dev Pointer to the modem device
 * @return 0 on success, negative errno on failure
 */
int hl78xx_enter_gnss_mode(const struct device *dev);

/**
 * @brief Exit GNSS mode and return to LTE mode
 *
 * Switches modem from GNSS mode back to LTE mode. This will:
 * 1. Stop any active GNSS search
 * 2. Fire HL78XX_GNSS_EVENT_MODE_EXITED event
 *
 * After the event fires, the next step depends on the power mode:
 * - **Airplane mode**: User must call
 *   hl78xx_api_func_set_phone_functionality(dev, HL78XX_FULLY_FUNCTIONAL, ...)
 *   to restore LTE.
 * - **PSM mode**: The driver automatically transitions back to
 *   AWAIT_REGISTERED and the modem will re-register to LTE.
 *
 *  * - **LOW-POWER-PSM mode**: User does not need to do anything, the modem will automatically
 * transition back to LTE registration after the kcellmeasure or socket data transmission starts.
 *
 * @param dev Pointer to the modem device
 * @return 0 on success, -EALREADY if not in GNSS mode, negative errno on failure
 */
int hl78xx_exit_gnss_mode(const struct device *dev);

/**
 * @brief Queue a GNSS position search
 *
 * Starts GNSS satellite search to obtain a position fix.
 * Must be called after hl78xx_enter_gnss_mode().
 *
 * @param dev Pointer to the GNSS device
 * @return 0 on success, negative errno on failure
 */
int hl78xx_queue_gnss_search(const struct device *dev);

/**
 * @brief Set NMEA output port for GNSS
 *
 * @param dev Pointer to the GNSS device
 * @param port NMEA output port configuration
 * @return 0 on success, -EBUSY if GNSS search is active, negative errno on failure
 */
int hl78xx_gnss_set_nmea_output(const struct device *dev, enum nmea_output_port port);

/**
 * @brief Set GNSS search timeout
 *
 * @param dev Pointer to the GNSS device
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return 0 on success, -EBUSY if GNSS search is active, negative errno on failure
 */
int hl78xx_gnss_set_search_timeout(const struct device *dev, uint32_t timeout_ms);

/**
 * @brief Get the latest known GNSS fix from the modem
 *
 * Queries the modem for the last known position using AT+GNSSLOC?
 * Result is delivered via GNSS data callback.
 *
 * @param dev Pointer to the GNSS device
 * @return 0 on success, -EBUSY if another script is running, other negative errno on failure
 */
int hl78xx_gnss_get_latest_known_fix(const struct device *dev);

#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
/**
 * @brief Get A-GNSS assistance data status
 *
 * Queries the modem for A-GNSS assistance data validity and expiry
 * using AT+GNSSAD? command.
 *
 * @param dev Pointer to the GNSS device
 * @param status Pointer to structure to receive the status
 * @return 0 on success, negative errno on failure
 */
int hl78xx_gnss_assist_data_get_status(const struct device *dev,
				       struct hl78xx_agnss_status *status);

/**
 * @brief Download A-GNSS assistance data
 *
 * Initiates download of predicted assistance data from the network
 * using AT+GNSSAD=1,\<days\> command. Requires active network connection.
 *
 * @param dev Pointer to the GNSS device
 * @param days Number of days of prediction data to download.
 *             Valid values: 1, 2, 3, 7, 14, 28
 * @return 0 on success, -EINVAL for invalid days value, negative errno on failure
 */
int hl78xx_gnss_assist_data_download(const struct device *dev, enum hl78xx_agnss_days days);

/**
 * @brief Delete A-GNSS assistance data
 *
 * Deletes any stored assistance data from the modem
 * using AT+GNSSAD=0 command.
 *
 * @param dev Pointer to the GNSS device
 * @return 0 on success, negative errno on failure
 */
int hl78xx_gnss_assist_data_delete(const struct device *dev);
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */

#endif /* CONFIG_HL78XX_GNSS */

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
/**
 * @brief Trigger the HL78xx shutdown process after a caller-supplied delay.
 *
 * This overrides any currently pending stage-1 power-down delay. Pass
 * K_NO_WAIT to dispatch HL78XX_POWER_DOWN_UPDATE immediately, or a relative
 * delay such as K_SECONDS(5) to start the graceful shutdown flow later.
 *
 * @param dev Pointer to the HL78xx modem device.
 * @param delay Delay before stage-1 power-down notification runs.
 *
 * @return 0 on success or -EINVAL for invalid arguments.
 */
int hl78xx_power_down_trigger(const struct device *dev, k_timeout_t delay);

/**
 * @brief Cancel a pending stage-1 HL78xx shutdown timer.
 *
 * @param dev Pointer to the HL78xx modem device.
 *
 * @return 0 on success, -EINVAL for invalid arguments, or -ENOENT if no
 *         stage-1 shutdown timer is pending.
 */
int hl78xx_power_down_cancel(const struct device *dev);

/**
 * @brief Respond to a pending HL78XX power-down update.
 *
 * Call this after receiving HL78XX_POWER_DOWN_UPDATE with
 * POWER_DOWN_EVENT_ENTER. Use HL78XX_POWER_DOWN_RESPONSE_IMMEDIATE to allow
 * the modem to proceed with physical shutdown right away, or
 * HL78XX_POWER_DOWN_RESPONSE_RESCHEDULE to extend the shutdown grace period by
 * @p timeout_s seconds from the time of this call.
 *
 * @param dev Pointer to the HL78xx modem device.
 * @param response Requested handling of the pending shutdown.
 * @param timeout_s New grace period in seconds when @p response is
 *        HL78XX_POWER_DOWN_RESPONSE_RESCHEDULE. Ignored otherwise.
 *
 * @return 0 on success, -EINVAL for invalid arguments, or -ENOENT if no
 *         application-controlled shutdown is pending.
 */
int hl78xx_power_down_respond(const struct device *dev, enum hl78xx_power_down_response response,
			      uint32_t timeout_s);

/**
 * @brief Confirm that application cleanup is complete and the modem may
 *        proceed with physical shutdown.
 *
 * If no shutdown is pending this call is a no-op.
 *
 * @param dev Pointer to the HL78xx modem device.
 */
void hl78xx_power_down_confirm(const struct device *dev);
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_ */
