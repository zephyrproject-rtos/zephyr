/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_
#define ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>
#include <stddef.h>
#include <zephyr/modem/chat.h>
#include <zephyr/drivers/cellular.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup hl78xx_constants HL78xx Constants and Macros
 * @{
 */

/** Unknown RSSI value returned by AT+CSQ command */
#define CSQ_RSSI_UNKNOWN         (99)
/** Unknown RSRP value returned by AT+CESQ command */
#define CESQ_RSRP_UNKNOWN        (255)
/** Unknown RSRQ value returned by AT+CESQ command */
#define CESQ_RSRQ_UNKNOWN        (255)

/**
 * Convert CSQ RSSI value to dBm
 * @param v RSSI value (0-31)
 * @return Signal strength in dBm (-113 to -51)
 */
#define CSQ_RSSI_TO_DB(v)        (-113 + (2 * (v)))
/**
 * Convert CESQ RSRP value to dBm
 * @param v RSRP value (0-97)
 * @return Reference signal received power in dBm (-140 to -44)
 */
#define CESQ_RSRP_TO_DB(v)       (-140 + (v))
/**
 * Convert CESQ RSRQ value to dB
 * @param v RSRQ value (0-34)
 * @return Reference signal received quality in dB (-20 to -3)
 */
#define CESQ_RSRQ_TO_DB(v)       (-20 + ((v) / 2))

/** Monitor is paused */
#define PAUSED                   1
/** Monitor is active, default */
#define ACTIVE                   0

/** Maximum length of modem manufacturer string */
#define MDM_MANUFACTURER_LENGTH  20
/** Maximum length of modem model string */
#define MDM_MODEL_LENGTH         32
/** Maximum length of modem revision string */
#define MDM_REVISION_LENGTH      64
/** Maximum length of modem IMEI string */
#define MDM_IMEI_LENGTH          16
/** Maximum length of modem IMSI string */
#define MDM_IMSI_LENGTH          23
/** Maximum length of modem ICCID string */
#define MDM_ICCID_LENGTH         22
/** Maximum length of APN string */
#define MDM_APN_MAX_LENGTH       64
/** Maximum length of certificate */
#define MDM_MAX_CERT_LENGTH      4096
/** Maximum length of hostname */
#define MDM_MAX_HOSTNAME_LEN     128
/** Maximum length of serial number string */
#define MDM_SERIAL_NUMBER_LENGTH 32

/** @} */

/**
 * @brief Define an Event monitor to receive notifications in the system workqueue thread.
 *
 * @param name The monitor name.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c PAUSED or @c ACTIVE).
 *	      The default initial state of a monitor is active.
 */
#define HL78XX_EVT_MONITOR(name, _handler, ...)                                                    \
	static STRUCT_SECTION_ITERABLE(hl78xx_evt_monitor_entry, name) = {                         \
		.handler = _handler,                                                               \
		.next = NULL,                                                                      \
		.flags.direct = false,                                                             \
		COND_CODE_1(__VA_ARGS__, (.flags.paused = __VA_ARGS__,), ()) }

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
#endif
#endif
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
	/** Access Point Name */
	HL78XX_MODEM_INFO_APN,
	/** Current Radio Access Technology */
	HL78XX_MODEM_INFO_CURRENT_RAT,
	/** Network Operator name */
	HL78XX_MODEM_INFO_NETWORK_OPERATOR,
	/** Modem Serial Number */
	HL78XX_MODEM_INFO_SERIAL_NUMBER,
	/** Current Baud Rate */
	HL78XX_MODEM_INFO_CURRENT_BAUD_RATE,
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
 * @brief Event monitor entry structure
 *
 * Represents a registered event monitor with callback and state
 */
struct hl78xx_evt_monitor_entry {
	/** Monitor callback function */
	const hl78xx_evt_monitor_handler_t handler;
	/** Link for runtime list */
	struct hl78xx_evt_monitor_entry *next;
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
int hl78xx_api_func_get_modem_info_vendor(const struct device *dev,
					  enum hl78xx_modem_info_type type, void *info,
					  size_t size);

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
	return hl78xx_api_func_get_modem_info_vendor(dev, type, info, size);
}
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
	if (rssi == CSQ_RSSI_UNKNOWN) {
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
	 * Receive Power between -140 dBm for 0 and -44 dBm for 97 (1 dBm steps),
	 * or unknown for 255
	 */
	if (rsrp == CESQ_RSRP_UNKNOWN) {
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
	if (rsrq == CESQ_RSRQ_UNKNOWN) {
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
 * @brief Register an event monitor to receive HL78xx modem event notifications
 *
 * Adds a monitor to the list of registered event monitors. Once registered,
 * the monitor will receive all modem events unless paused.
 *
 * @param mon Pointer to the monitor entry to register
 * @return 0 on success, negative errno on failure
 */
int hl78xx_evt_monitor_register(struct hl78xx_evt_monitor_entry *mon);

/**
 * @brief Unregister an event monitor from receiving HL78xx modem event notifications
 *
 * Removes a monitor from the list of registered event monitors.
 *
 * @param mon Pointer to the monitor entry to unregister
 * @return 0 on success, negative errno on failure
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

#ifdef CONFIG_HL78XX_GNSS
/**
 * @brief Enter GNSS mode
 *
 * Switches modem from LTE mode to GNSS mode. This will:
 * 1. Put modem in airplane mode (AT+CFUN=4)
 * 2. Initialize GNSS engine
 * 3. Fire HL78XX_GNSS_ENGINE_READY event when complete
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
 * 2. Restore full phone functionality (AT+CFUN=1)
 * 3. Modem will re-register to the network
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_ */
