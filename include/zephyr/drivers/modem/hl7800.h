/** @file
 * @brief HL7800 modem public API header file.
 * @ingroup hl7800_interface
 *
 * Allows an application to control the HL7800 modem.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_HL7800_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_HL7800_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

#include <time.h>

/**
 * @defgroup hl7800_interface HL7800
 * @brief Sierra Wireless HL7800 cellular modems.
 * @ingroup cellular_interface_ext
 * @{
 */

/* The size includes the NUL character, the strlen doesn't */
/** Size of a firmware revision string, including the terminating NUL */
#define MDM_HL7800_REVISION_MAX_SIZE 29
/** Maximum length of a firmware revision string */
#define MDM_HL7800_REVISION_MAX_STRLEN (MDM_HL7800_REVISION_MAX_SIZE - 1)

/** Size of an IMEI string, including the terminating NUL */
#define MDM_HL7800_IMEI_SIZE 16
/** Length of an IMEI string */
#define MDM_HL7800_IMEI_STRLEN (MDM_HL7800_IMEI_SIZE - 1)

/** Size of an ICCID string, including the terminating NUL */
#define MDM_HL7800_ICCID_MAX_SIZE 21
/** Maximum length of an ICCID string */
#define MDM_HL7800_ICCID_MAX_STRLEN (MDM_HL7800_ICCID_MAX_SIZE - 1)

/** Size of a serial number string, including the terminating NUL */
#define MDM_HL7800_SERIAL_NUMBER_SIZE 15
/** Length of a serial number string */
#define MDM_HL7800_SERIAL_NUMBER_STRLEN (MDM_HL7800_SERIAL_NUMBER_SIZE - 1)

/** Size of an Access Point Name, including the terminating NUL */
#define MDM_HL7800_APN_MAX_SIZE 64
/** Size of an APN username, including the terminating NUL */
#define MDM_HL7800_APN_USERNAME_MAX_SIZE 65
/** Size of an APN password, including the terminating NUL */
#define MDM_HL7800_APN_PASSWORD_MAX_SIZE 65

/** Maximum length of an Access Point Name */
#define MDM_HL7800_APN_MAX_STRLEN (MDM_HL7800_APN_MAX_SIZE - 1)
/** Maximum length of an APN username */
#define MDM_HL7800_APN_USERNAME_MAX_STRLEN                                     \
	(MDM_HL7800_APN_USERNAME_MAX_SIZE - 1)
/** Maximum length of an APN password */
#define MDM_HL7800_APN_PASSWORD_MAX_STRLEN                                     \
	(MDM_HL7800_APN_PASSWORD_MAX_SIZE - 1)

/** @cond INTERNAL_HIDDEN */
#define MDM_HL7800_APN_CMD_MAX_SIZE                                            \
	(32 + MDM_HL7800_APN_USERNAME_MAX_STRLEN +                             \
	 MDM_HL7800_APN_PASSWORD_MAX_STRLEN)

#define MDM_HL7800_APN_CMD_MAX_STRLEN (MDM_HL7800_APN_CMD_MAX_SIZE - 1)
/** @endcond */

/** Access Point Name configuration */
struct mdm_hl7800_apn {
	char value[MDM_HL7800_APN_MAX_SIZE];               /**< APN */
	char username[MDM_HL7800_APN_USERNAME_MAX_SIZE];   /**< APN username */
	char password[MDM_HL7800_APN_PASSWORD_MAX_SIZE];   /**< APN password */
};

/** Size of an LTE band configuration string, including the terminating NUL */
#define MDM_HL7800_LTE_BAND_STR_SIZE 21
/** Maximum length of an LTE band configuration string */
#define MDM_HL7800_LTE_BAND_STRLEN (MDM_HL7800_LTE_BAND_STR_SIZE - 1)

/** @cond INTERNAL_HIDDEN */
#define MDM_HL7800_OPERATOR_INDEX_SIZE 3
#define MDM_HL7800_OPERATOR_INDEX_STRLEN (MDM_HL7800_OPERATOR_INDEX_SIZE - 1)
/** @endcond */

/** Minimum size of an IMSI string, including the terminating NUL */
#define MDM_HL7800_IMSI_MIN_STR_SIZE 15
/** Maximum size of an IMSI string, including the terminating NUL */
#define MDM_HL7800_IMSI_MAX_STR_SIZE 16
/** Maximum length of an IMSI string */
#define MDM_HL7800_IMSI_MAX_STRLEN (MDM_HL7800_IMSI_MAX_STR_SIZE - 1)

/** @cond INTERNAL_HIDDEN */
#define MDM_HL7800_MODEM_FUNCTIONALITY_SIZE 2
#define MDM_HL7800_MODEM_FUNCTIONALITY_STRLEN                                  \
	(MDM_HL7800_MODEM_FUNCTIONALITY_SIZE - 1)
/** @endcond */

/** Size of a GPS string, including the terminating NUL */
#define MDM_HL7800_MAX_GPS_STR_SIZE 33

/** Size of a PoLTE user ID string, including the terminating NUL */
#define MDM_HL7800_MAX_POLTE_USER_ID_SIZE 16
/** Size of a PoLTE password string, including the terminating NUL */
#define MDM_HL7800_MAX_POLTE_PASSWORD_SIZE 16
/** Size of a PoLTE location string, including the terminating NUL */
#define MDM_HL7800_MAX_POLTE_LOCATION_STR_SIZE 33

/**
 * Status reported by @ref HL7800_EVENT_POLTE when the PoLTE server could not
 * be reached.
 *
 * Assign the server error code (location response) to a value
 * that isn't used by locate response so that a single status
 * callback can be used.
 */
#define MDM_HL7800_POLTE_SERVER_ERROR 10

/** @cond INTERNAL_HIDDEN */
#define MDM_HL7800_SET_POLTE_USER_AND_PASSWORD_FMT_STR "AT%%POLTECMD=\"SERVERAUTH\",\"%s\",\"%s\""
/** @endcond */

/** Site survey result */
struct mdm_hl7800_site_survey {
	uint32_t earfcn; /**< EUTRA Absolute Radio Frequency Channel Number */
	uint32_t cell_id; /**< Cell identity */
	int rsrp; /**< Reference Signal Received Power */
	int rsrq; /**< Reference Signal Received Quality */
};

/** Radio Access Technology */
enum mdm_hl7800_radio_mode {
	MDM_RAT_CAT_M1 = 0, /**< LTE-M (LTE Cat M1) */
	MDM_RAT_CAT_NB1     /**< NB-IoT (LTE Cat NB1) */
};

/**
 * Event type reported to a @ref mdm_hl7800_event_callback_t.
 *
 * The documentation of each event lists the type that the @p event_data
 * argument of the callback points to when it is invoked with that event.
 */
enum mdm_hl7800_event {
	/** Reserved, not generated */
	HL7800_EVENT_RESERVED = 0,
	/**
	 * Network registration change.
	 *
	 * The event data points to a struct mdm_hl7800_compound_event holding
	 * an enum mdm_hl7800_network_state code.
	 */
	HL7800_EVENT_NETWORK_STATE_CHANGE,
	/**
	 * Access Point Name change.
	 *
	 * The event data points to a struct mdm_hl7800_apn.
	 */
	HL7800_EVENT_APN_UPDATE,
	/**
	 * Reference Signal Received Power change.
	 *
	 * The event data points to an int holding the RSRP in dBm.
	 */
	HL7800_EVENT_RSSI,
	/**
	 * Signal to Interference plus Noise Ratio change.
	 *
	 * The event data points to an int holding the SINR in dB.
	 */
	HL7800_EVENT_SINR,
	/**
	 * Modem startup state change.
	 *
	 * The event data points to a struct mdm_hl7800_compound_event holding
	 * an enum mdm_hl7800_startup_state code.
	 */
	HL7800_EVENT_STARTUP_STATE_CHANGE,
	/**
	 * Sleep state change.
	 *
	 * The event data points to a struct mdm_hl7800_compound_event holding
	 * an enum mdm_hl7800_sleep code.
	 */
	HL7800_EVENT_SLEEP_STATE_CHANGE,
	/**
	 * Radio Access Technology change.
	 *
	 * The event data points to an enum mdm_hl7800_radio_mode value.
	 */
	HL7800_EVENT_RAT,
	/**
	 * Configured LTE bands changed.
	 *
	 * The event data points to a NUL terminated band configuration string.
	 */
	HL7800_EVENT_BANDS,
	/**
	 * Active LTE bands changed.
	 *
	 * The event data points to a NUL terminated band configuration string.
	 */
	HL7800_EVENT_ACTIVE_BANDS,
	/**
	 * Firmware update state change.
	 *
	 * The event data points to a struct mdm_hl7800_compound_event holding
	 * an enum mdm_hl7800_fota_state code.
	 */
	HL7800_EVENT_FOTA_STATE,
	/**
	 * Firmware update progress.
	 *
	 * The event data points to a uint32_t holding the number of bytes
	 * sent to the modem.
	 */
	HL7800_EVENT_FOTA_COUNT,
	/**
	 * Firmware revision change.
	 *
	 * The event data points to a NUL terminated revision string.
	 */
	HL7800_EVENT_REVISION,
	/**
	 * GPS data received.
	 *
	 * The event data points to a struct mdm_hl7800_compound_event holding
	 * an enum mdm_hl7800_gps_string_types code.
	 */
	HL7800_EVENT_GPS,
	/**
	 * Position fix status change.
	 *
	 * The event data points to an int holding an
	 * enum mdm_hl7800_gnss_position_event value.
	 */
	HL7800_EVENT_GPS_POSITION_STATUS,
	/**
	 * PoLTE registration result.
	 *
	 * The event data points to a
	 * struct mdm_hl7800_polte_registration_event_data.
	 */
	HL7800_EVENT_POLTE_REGISTRATION,
	/**
	 * Result of issuing the PoLTE locate command.
	 *
	 * The event data points to a struct mdm_hl7800_polte_location_data
	 * with only the status valid.
	 */
	HL7800_EVENT_POLTE_LOCATE_STATUS,
	/**
	 * PoLTE location response.
	 *
	 * The event data points to a struct mdm_hl7800_polte_location_data.
	 */
	HL7800_EVENT_POLTE,
	/**
	 * Site survey response.
	 *
	 * The event data points to a struct mdm_hl7800_site_survey. The event
	 * is generated once for each response received from the modem.
	 */
	HL7800_EVENT_SITE_SURVEY,
	/**
	 * Driver state change.
	 *
	 * The event data points to a struct mdm_hl7800_compound_event holding
	 * an enum mdm_hl7800_state code.
	 */
	HL7800_EVENT_STATE,
};

/** Driver state */
enum mdm_hl7800_state {
	HL7800_STATE_NOT_READY = 0, /**< Driver is not ready */
	HL7800_STATE_INITIALIZED,   /**< Driver has configured the modem */
};

/** Modem startup state */
enum mdm_hl7800_startup_state {
	HL7800_STARTUP_STATE_READY = 0,                /**< Ready */
	HL7800_STARTUP_STATE_WAITING_FOR_ACCESS_CODE,  /**< SIM waiting for PIN or PUK */
	HL7800_STARTUP_STATE_SIM_NOT_PRESENT,          /**< No SIM card present */
	HL7800_STARTUP_STATE_SIMLOCK,                  /**< SIM locked to another operator */
	HL7800_STARTUP_STATE_UNRECOVERABLE_ERROR,      /**< Unrecoverable error */
	HL7800_STARTUP_STATE_UNKNOWN,                  /**< Unknown state */
	HL7800_STARTUP_STATE_INACTIVE_SIM              /**< SIM card is inactive */
};

/**
 * Network registration state.
 *
 * Follows the +CEREG registration status values, with Laird defined
 * states above 0xef.
 */
enum mdm_hl7800_network_state {
	HL7800_NOT_REGISTERED = 0,   /**< Not registered, not searching */
	HL7800_HOME_NETWORK,         /**< Registered, home network */
	HL7800_SEARCHING,            /**< Not registered, searching for an operator */
	HL7800_REGISTRATION_DENIED,  /**< Registration denied */
	HL7800_OUT_OF_COVERAGE,      /**< Out of coverage */
	HL7800_ROAMING,              /**< Registered, roaming */
	HL7800_EMERGENCY = 8,        /**< Emergency services only */
	/* Laird defined states */
	HL7800_UNABLE_TO_CONFIGURE = 0xf0 /**< Modem could not be configured */
};

/** Sleep state */
enum mdm_hl7800_sleep {
	HL7800_SLEEP_UNINITIALIZED = 0, /**< Sleep state not known yet */
	HL7800_SLEEP_HIBERNATE,         /**< Hibernate (lowest power consumption) */
	HL7800_SLEEP_AWAKE,             /**< Awake */
	HL7800_SLEEP_LITE_HIBERNATE,    /**< Lite hibernate (IO state retained) */
	HL7800_SLEEP_SLEEP,             /**< Sleep (UART can wake the modem) */
};

/** Firmware update state */
enum mdm_hl7800_fota_state {
	HL7800_FOTA_IDLE,                   /**< No update in progress */
	HL7800_FOTA_START,                  /**< Update file transfer is starting */
	HL7800_FOTA_WIP,                    /**< File transfer in progress */
	HL7800_FOTA_PAD,                    /**< Padding of the last XMODEM block */
	HL7800_FOTA_SEND_EOT,               /**< End of transmission being sent */
	HL7800_FOTA_FILE_ERROR,             /**< Update file could not be read */
	HL7800_FOTA_INSTALL,                /**< Modem is installing the update */
	HL7800_FOTA_REBOOT_AND_RECONFIGURE, /**< Modem is rebooting to complete the update */
	HL7800_FOTA_COMPLETE,               /**< Update complete */
};

/** Modem functionality level, follows the +CFUN values */
enum mdm_hl7800_functionality {
	HL7800_FUNCTIONALITY_MINIMUM = 0, /**< Minimum (reduced) functionality */
	HL7800_FUNCTIONALITY_FULL = 1,    /**< Full functionality */
	HL7800_FUNCTIONALITY_AIRPLANE = 4 /**< Airplane mode (radio disabled) */
};

/**
 * Event data made of a code and a string.
 *
 * The modem reports state values as an enumeration and a string.
 * GPS values are reported with a type of value and string.
 */
struct mdm_hl7800_compound_event {
	uint8_t code; /**< Value of the enumeration matching the event type */
	char *string; /**< NUL terminated string reported with the value */
};

/** GNSS event reported by the modem */
enum mdm_hl7800_gnss_event {
	HL7800_GNSS_EVENT_INVALID = -1, /**< Event could not be parsed */
	HL7800_GNSS_EVENT_INIT,         /**< GNSS initialized */
	HL7800_GNSS_EVENT_START,        /**< Position fix started */
	HL7800_GNSS_EVENT_STOP,         /**< Position fix stopped */
	HL7800_GNSS_EVENT_POSITION,     /**< Position fix status change */
};

/** Status of a GNSS event */
enum mdm_hl7800_gnss_status {
	HL7800_GNSS_STATUS_INVALID = -1, /**< Status could not be parsed */
	HL7800_GNSS_STATUS_FAILURE,      /**< Operation failed */
	HL7800_GNSS_STATUS_SUCCESS,      /**< Operation succeeded */
};

/** Position fix status reported by @ref HL7800_EVENT_GPS_POSITION_STATUS */
enum mdm_hl7800_gnss_position_event {
	HL7800_GNSS_POSITION_EVENT_INVALID = -1,              /**< Event could not be parsed */
	HL7800_GNSS_POSITION_EVENT_LOST_OR_NOT_AVAILABLE_YET, /**< No position fix */
	HL7800_GNSS_POSITION_EVENT_PREDICTION_AVAILABLE,      /**< Predicted position available */
	HL7800_GNSS_POSITION_EVENT_2D_AVAILABLE,              /**< 2D position fix available */
	HL7800_GNSS_POSITION_EVENT_3D_AVAILABLE,              /**< 3D position fix available */
	HL7800_GNSS_POSITION_EVENT_FIXED_TO_INVALID,          /**< Position fix was lost */
};

/** Type of GPS string reported by @ref HL7800_EVENT_GPS */
enum mdm_hl7800_gps_string_types {
	HL7800_GPS_STR_LATITUDE,  /**< Latitude */
	HL7800_GPS_STR_LONGITUDE, /**< Longitude */
	HL7800_GPS_STR_GPS_TIME,  /**< Time of the position fix */
	HL7800_GPS_STR_FIX_TYPE,  /**< Type of position fix */
	HL7800_GPS_STR_HEPE,      /**< Horizontal Estimated Position Error */
	HL7800_GPS_STR_ALTITUDE,  /**< Altitude */
	HL7800_GPS_STR_ALT_UNC,   /**< Altitude uncertainty */
	HL7800_GPS_STR_DIRECTION, /**< Direction of movement */
	HL7800_GPS_STR_HOR_SPEED, /**< Horizontal speed */
	HL7800_GPS_STR_VER_SPEED  /**< Vertical speed */
};

/** PoLTE registration result */
struct mdm_hl7800_polte_registration_event_data {
	int status;     /**< Negative errno, 0 on success */
	char *user;     /**< PoLTE user, not valid if status is non-zero */
	char *password; /**< PoLTE password, not valid if status is non-zero */
};

/** PoLTE location response */
struct mdm_hl7800_polte_location_data {
	/** Timestamp of the location response */
	uint32_t timestamp;
	/**
	 * Negative errno, 0 on success, positive error code from the server.
	 * The other members are not valid if status is non-zero.
	 */
	int status;
	/** Latitude */
	char latitude[MDM_HL7800_MAX_POLTE_LOCATION_STR_SIZE];
	/** Longitude */
	char longitude[MDM_HL7800_MAX_POLTE_LOCATION_STR_SIZE];
	/** Location confidence, in meters */
	char confidence_in_meters[MDM_HL7800_MAX_POLTE_LOCATION_STR_SIZE];
};

/**
 * @brief Event callback
 *
 * @param event The type of event
 * @param event_data Pointer to the event specific data structure, as listed
 *                   in the documentation of each @ref mdm_hl7800_event value
 */
typedef void (*mdm_hl7800_event_callback_t)(enum mdm_hl7800_event event,
					    void *event_data);

/** Event callback registration */
struct mdm_hl7800_callback_agent {
	/** Reserved for the driver, do not modify */
	sys_snode_t node;
	/** Callback invoked when a modem event occurs */
	mdm_hl7800_event_callback_t event_callback;
};

/**
 * @brief Power off the HL7800
 *
 * @return int32_t 0 for success
 */
int32_t mdm_hl7800_power_off(void);

/**
 * @brief Reset the HL7800 (and allow it to reconfigure).
 *
 * @return int32_t >= 0 for success, < 0 for failure
 */
int32_t mdm_hl7800_reset(void);

/**
 * @brief Control the wake signals to the HL7800.
 * @note this API should only be used for debug purposes.
 *
 * @param awake True to keep the HL7800 awake, False to allow sleep
 */
void mdm_hl7800_wakeup(bool awake);

/**
 * @brief Send an AT command to the HL7800.
 * @note This API should only be used for debug purposes.
 *   It is possible to break the driver using this API.
 *
 * @param data AT command string
 * @param resp_timeout Timeout in seconds to wait for the response
 * @param resp Pointer to the response buffer. This can be NULL to ignore the response.
 * @param resp_len Input: length of the response buffer, Output: length of the response.
 *   This can be NULL.
 * @return int32_t 0 for success
 */
int32_t mdm_hl7800_send_at_cmd(const uint8_t *data, uint8_t resp_timeout, char *resp,
			       uint16_t *resp_len);

/**
 * @brief Get the signal quality of the HL7800.
 * If @kconfig{CONFIG_MODEM_HL7800_RSSI_RATE_SECONDS} is non-zero, then
 * this function returns the value from the last periodic read.
 * If @kconfig{CONFIG_MODEM_HL7800_RSSI_RATE_SECONDS} is 0, then this
 * may cause the modem to be woken so that the values can be queried.
 *
 * @param rsrp Reference Signals Received Power (dBm)
 *             Range = -140 dBm to -44 dBm
 * @param sinr Signal to Interference plus Noise Ratio (dB)
 *             Range = -128 dB to 40 dB
 */
void mdm_hl7800_get_signal_quality(int *rsrp, int *sinr);

/**
 * @brief Get the SIM card ICCID
 *
 * @return NUL terminated ICCID string
 */
char *mdm_hl7800_get_iccid(void);

/**
 * @brief Get the HL7800 serial number
 *
 * @return NUL terminated serial number string
 */
char *mdm_hl7800_get_sn(void);

/**
 * @brief Get the HL7800 IMEI
 *
 * @return NUL terminated IMEI string
 */
char *mdm_hl7800_get_imei(void);

/**
 * @brief Get the HL7800 firmware version
 *
 * @return NUL terminated firmware version string
 */
char *mdm_hl7800_get_fw_version(void);

/**
 * @brief Get the IMSI
 *
 * @return NUL terminated IMSI string
 */
char *mdm_hl7800_get_imsi(void);

/**
 * @brief Update the Access Point Name in the modem.
 *
 * @param access_point_name NUL terminated APN string
 *
 * @retval 0 on success, negative on failure.
 */
int32_t mdm_hl7800_update_apn(char *access_point_name);

/**
 * @brief Update the Radio Access Technology (mode).
 *
 * @param value new Radio Access Technology
 *
 * @retval 0 on success, negative on failure.
 */
int32_t mdm_hl7800_update_rat(enum mdm_hl7800_radio_mode value);

/**
 * @brief Check if a Radio Access Technology value is valid.
 *
 * @param value RAT value to check
 *
 * @retval true if RAT value is valid
 */
bool mdm_hl7800_valid_rat(uint8_t value);

/**
 * @brief Register a function that is called when a modem event occurs.
 * Multiple users registering for callbacks is supported.
 *
 * @param agent event callback agent
 *
 * @retval 0 on success, negative on failure
 */
int mdm_hl7800_register_event_callback(struct mdm_hl7800_callback_agent *agent);

/**
 * @brief Unregister a callback event function
 *
 * @param agent event callback agent
 *
 * @retval 0 on success, negative on failure
 */
int mdm_hl7800_unregister_event_callback(struct mdm_hl7800_callback_agent *agent);

/**
 * @brief Force modem module to generate status events.
 *
 * @note This can be used to get the current state when a module initializes
 * later than the modem.
 */
void mdm_hl7800_generate_status_events(void);

/**
 * @brief Get the local time from the modem's real time clock.
 *
 * @param tm time structure
 * @param offset The amount the local time is offset from GMT/UTC in seconds.
 * @return int32_t 0 if successful
 */
int32_t mdm_hl7800_get_local_time(struct tm *tm, int32_t *offset);

#if defined(CONFIG_MODEM_HL7800_FW_UPDATE) || defined(__DOXYGEN__)
/**
 * @brief Update the HL7800 via XMODEM protocol.  During the firmware update
 * no other modem functions will be available.
 *
 * Only available if @kconfig{CONFIG_MODEM_HL7800_FW_UPDATE} is enabled.
 *
 * @param file_path Absolute path of the update file
 *
 * @return 0 if successful
 */
int32_t mdm_hl7800_update_fw(char *file_path);
#endif

/**
 * @brief Read the operator index from the modem.
 *
 * @retval negative error code, 0 on success
 */
int32_t mdm_hl7800_get_operator_index(void);

/**
 * @brief Get modem functionality
 *
 * @return int32_t negative errno on failure, else mdm_hl7800_functionality
 */
int32_t mdm_hl7800_get_functionality(void);

/**
 * @brief Set airplane, normal, or reduced functionality mode.
 * Airplane mode persists when reset.
 *
 * @note Boot functionality is also controlled by
 * @kconfig{CONFIG_MODEM_HL7800_BOOT_IN_AIRPLANE_MODE}.
 *
 * @param mode new functionality level
 * @return int32_t negative errno, 0 on success
 */
int32_t mdm_hl7800_set_functionality(enum mdm_hl7800_functionality mode);

/**
 * @brief When rate is non-zero: Put modem into Airplane mode. Enable GPS and
 * generate HL7800_EVENT_GPS events.
 * When zero: Disable GPS and put modem into normal mode.
 *
 * @note Airplane mode isn't cleared when the modem is reset.
 *
 * @param rate in seconds to query location
 * @return int32_t negative errno, 0 on success
 */
int32_t mdm_hl7800_set_gps_rate(uint32_t rate);

/**
 * @brief Register modem/SIM with polte.io
 *
 * @note It takes around 30 seconds for HL7800_EVENT_POLTE_REGISTRATION to
 * be generated.  If the applications saves the user and password
 * information into non-volatile memory, then this command
 * only needs to be run once.
 *
 * @return int32_t negative errno, 0 on success
 */
int32_t mdm_hl7800_polte_register(void);

/**
 * @brief Enable PoLTE.
 *
 * @param user from polte.io or register command callback
 * @param password from polte.io register command callback
 * @return int32_t negative errno, 0 on success
 */
int32_t mdm_hl7800_polte_enable(char *user, char *password);

/**
 * @brief Locate device using PoLTE.
 *
 * @note The first HL7800_EVENT_POLTE_LOCATE_STATUS event indicates
 * the status of issuing the locate command. The second event
 * requires 20-120 seconds to be generated and it contains the
 * location information (or indicates server failure).
 *
 * @return int32_t negative errno, 0 on success
 */
int32_t mdm_hl7800_polte_locate(void);

/**
 * @brief Perform a site survey.  This command may return different values
 * each time it is run (depending on what is in range).
 *
 * HL7800_EVENT_SITE_SURVEY is generated for each response received from modem.
 *
 * @retval negative error code, 0 on success
 */
int32_t mdm_hl7800_perform_site_survey(void);

/**
 * @brief Set desired sleep level. Requires
 * @kconfig{CONFIG_MODEM_HL7800_LOW_POWER_MODE}.
 *
 * @param level (sleep, lite hibernate, or hibernate)
 * @return int negative errno, 0 on success
 */
int mdm_hl7800_set_desired_sleep_level(enum mdm_hl7800_sleep level);

/**
 * @brief Allows mapping of WAKE_UP signal
 * to a user accessible test point on the development board.
 *
 * @param func to be called when application requests modem wake/sleep.
 * The state parameter of the callback is 1 when modem should stay awake,
 * 0 when modem can sleep
 */
void mdm_hl7800_register_wake_test_point_callback(void (*func)(int state));

/**
 * @brief Allows mapping of P1.12_GPIO6 signal
 * to a user accessible test point on the development board.
 *
 * @param func to be called when modem wakes/sleeps is sleep level is
 * hibernate or lite hibernate.
 * The state parameter of the callback follows gpio_pin_get definitions,
 * but will default high if there is an error reading pin
 */
void mdm_hl7800_register_gpio6_callback(void (*func)(int state));

/**
 * @brief Allows mapping of UART1_CTS signal
 * to a user accessible test point on the development board.
 *
 * @param func to be called when CTS state changes if sleep level is sleep.
 * The state parameter of the callback follows gpio_pin_get definitions,
 * but will default low if there is an error reading pin
 */
void mdm_hl7800_register_cts_callback(void (*func)(int state));

/**
 * @brief Set the bands available for the LTE connection.
 * NOTE: This will cause the modem to reboot. This call returns before the reboot.
 *
 * @param bands Band bitmap in hexadecimal format without the 0x prefix.
 * Leading 0's for the value can be omitted.
 *
 * @return int32_t negative errno, 0 on success
 */
int32_t mdm_hl7800_set_bands(const char *bands);

/**
 * @brief Set the log level for the modem.
 *
 * @note It cannot be set higher than @kconfig{CONFIG_MODEM_LOG_LEVEL}.
 * If debug level is desired, then it must be compiled with that level.
 *
 * @param level 0 (None) - 4 (Debug)
 *
 * @retval new log level
 */
uint32_t mdm_hl7800_log_filter_set(uint32_t level);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_HL7800_H_ */
