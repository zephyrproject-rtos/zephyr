/** @file
 * @brief HL7800 modem public API header file.
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

#ifdef CONFIG_NEWLIB_LIBC
#include <time.h>
#endif

/* The size includes the NUL character, the strlen doesn't */
#define MDM_HL7800_REVISION_MAX_SIZE 29
#define MDM_HL7800_REVISION_MAX_STRLEN (MDM_HL7800_REVISION_MAX_SIZE - 1)

#define MDM_HL7800_IMEI_SIZE 16
#define MDM_HL7800_IMEI_STRLEN (MDM_HL7800_IMEI_SIZE - 1)

#define MDM_HL7800_ICCID_SIZE 21
#define MDM_HL7800_ICCID_STRLEN (MDM_HL7800_ICCID_SIZE - 1)

#define MDM_HL7800_SERIAL_NUMBER_SIZE 15
#define MDM_HL7800_SERIAL_NUMBER_STRLEN (MDM_HL7800_SERIAL_NUMBER_SIZE - 1)

#define MDM_HL7800_APN_MAX_SIZE 64
#define MDM_HL7800_APN_USERNAME_MAX_SIZE 65
#define MDM_HL7800_APN_PASSWORD_MAX_SIZE 65

#define MDM_HL7800_APN_MAX_STRLEN (MDM_HL7800_APN_MAX_SIZE - 1)
#define MDM_HL7800_APN_USERNAME_MAX_STRLEN                                     \
	(MDM_HL7800_APN_USERNAME_MAX_SIZE - 1)
#define MDM_HL7800_APN_PASSWORD_MAX_STRLEN                                     \
	(MDM_HL7800_APN_PASSWORD_MAX_SIZE - 1)

#define MDM_HL7800_APN_CMD_MAX_SIZE                                            \
	(32 + MDM_HL7800_APN_USERNAME_MAX_STRLEN +                             \
	 MDM_HL7800_APN_PASSWORD_MAX_STRLEN)

#define MDM_HL7800_APN_CMD_MAX_STRLEN (MDM_HL7800_APN_CMD_MAX_SIZE - 1)

struct mdm_hl7800_apn {
	char value[MDM_HL7800_APN_MAX_SIZE];
	char username[MDM_HL7800_APN_USERNAME_MAX_SIZE];
	char password[MDM_HL7800_APN_PASSWORD_MAX_SIZE];
};

#define MDM_HL7800_LTE_BAND_STR_SIZE 21
#define MDM_HL7800_LTE_BAND_STRLEN (MDM_HL7800_LTE_BAND_STR_SIZE - 1)

enum mdm_hl7800_radio_mode { MDM_RAT_CAT_M1 = 0, MDM_RAT_CAT_NB1 };

enum mdm_hl7800_event {
	HL7800_EVENT_RESERVED = 0,
	HL7800_EVENT_NETWORK_STATE_CHANGE,
	HL7800_EVENT_APN_UPDATE,
	HL7800_EVENT_RSSI,
	HL7800_EVENT_SINR,
	HL7800_EVENT_STARTUP_STATE_CHANGE,
	HL7800_EVENT_SLEEP_STATE_CHANGE,
	HL7800_EVENT_RAT,
	HL7800_EVENT_BANDS,
	HL7800_EVENT_ACTIVE_BANDS,
	HL7800_EVENT_FOTA_STATE,
	HL7800_EVENT_FOTA_COUNT,
	HL7800_EVENT_REVISION
};

enum mdm_hl7800_startup_state {
	HL7800_STARTUP_STATE_READY = 0,
	HL7800_STARTUP_STATE_WAITING_FOR_ACCESS_CODE,
	HL7800_STARTUP_STATE_SIM_NOT_PRESENT,
	HL7800_STARTUP_STATE_SIMLOCK,
	HL7800_STARTUP_STATE_UNRECOVERABLE_ERROR,
	HL7800_STARTUP_STATE_UNKNOWN,
	HL7800_STARTUP_STATE_INACTIVE_SIM
};

enum mdm_hl7800_network_state {
	HL7800_NOT_REGISTERED = 0,
	HL7800_HOME_NETWORK,
	HL7800_SEARCHING,
	HL7800_REGISTRATION_DENIED,
	HL7800_OUT_OF_COVERAGE,
	HL7800_ROAMING,
	HL7800_EMERGENCY = 8,
	/* Laird defined states */
	HL7800_UNABLE_TO_CONFIGURE = 0xf0
};

enum mdm_hl7800_sleep_state {
	HL7800_SLEEP_STATE_UNINITIALIZED = 0,
	HL7800_SLEEP_STATE_ASLEEP,
	HL7800_SLEEP_STATE_AWAKE
};

enum mdm_hl7800_fota_state {
	HL7800_FOTA_IDLE,
	HL7800_FOTA_START,
	HL7800_FOTA_WIP,
	HL7800_FOTA_PAD,
	HL7800_FOTA_SEND_EOT,
	HL7800_FOTA_FILE_ERROR,
	HL7800_FOTA_INSTALL,
	HL7800_FOTA_REBOOT_AND_RECONFIGURE,
	HL7800_FOTA_COMPLETE,
};

/* The modem reports state values as an enumeration and a string */
struct mdm_hl7800_compound_event {
	uint8_t code;
	char *string;
};

/**
 * event - The type of event
 * event_data - Pointer to event specific data structure
 * HL7800_EVENT_NETWORK_STATE_CHANGE - compound event
 * HL7800_EVENT_APN_UPDATE - struct mdm_hl7800_apn
 * HL7800_EVENT_RSSI - int
 * HL7800_EVENT_SINR - int
 * HL7800_EVENT_STARTUP_STATE_CHANGE - compound event
 * HL7800_EVENT_SLEEP_STATE_CHANGE - compound event
 * HL7800_EVENT_RAT - int
 * HL7800_EVENT_BANDS - string
 * HL7800_EVENT_ACTIVE_BANDS - string
 * HL7800_EVENT_FOTA_STATE - compound event
 * HL7800_EVENT_FOTA_COUNT - uint32_t
 * HL7800_EVENT_REVISION - string
 */
typedef void (*mdm_hl7800_event_callback_t)(enum mdm_hl7800_event event,
					    void *event_data);

/**
 * @brief Power off the HL7800
 *
 * @return int32_t 0 for success
 */
int32_t mdm_hl7800_power_off(void);

/**
 * @brief Reset the HL7800
 *
 * @return int32_t 0 for success
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
 * @note this API should only be used for debug purposes.
 *
 * @param data AT command string
 * @return int32_t 0 for success
 */
int32_t mdm_hl7800_send_at_cmd(const uint8_t *data);

/**
 * @brief Get the signal quality of the HL7800
 *
 * @param rsrp Reference Signals Received Power (dBm)
 *             Range = -140 dBm to -44 dBm
 * @param sinr Signal to Interference plus Noise Ratio (dBm)
 *             Range = -128 dBm to 40dBm
 */
void mdm_hl7800_get_signal_quality(int *rsrp, int *sinr);

/**
 * @brief Get the SIM card ICCID
 *
 */
char *mdm_hl7800_get_iccid(void);

/**
 * @brief Get the HL7800 serial number
 *
 */
char *mdm_hl7800_get_sn(void);

/**
 * @brief Get the HL7800 IMEI
 *
 */
char *mdm_hl7800_get_imei(void);

/**
 * @brief Get the HL7800 firmware version
 *
 */
char *mdm_hl7800_get_fw_version(void);

/**
 * @brief Update the Access Point Name in the modem.
 *
 * @retval 0 on success, negative on failure.
 */
int32_t mdm_hl7800_update_apn(char *access_point_name);

/**
 * @brief Update the Radio Access Technology (mode).
 *
 * @retval 0 on success, negative on failure.
 */
int32_t mdm_hl7800_update_rat(enum mdm_hl7800_radio_mode value);

/**
 * @retval true if RAT value is valid
 */
bool mdm_hl7800_valid_rat(uint8_t value);

/**
 * @brief Register a function that is called when a modem event occurs.
 *
 * @param cb event callback
 */
void mdm_hl7800_register_event_callback(mdm_hl7800_event_callback_t cb);

/**
 * @brief Force modem module to generate status events.
 *
 * @note This can be used to get the current state when a module initializes
 * later than the modem.
 */
void mdm_hl7800_generate_status_events(void);

#ifdef CONFIG_NEWLIB_LIBC
/**
 * @brief Get the local time from the modem's real time clock.
 *
 * @param tm time structure
 * @param offset The amount the local time is offset from GMT/UTC in seconds.
 *
 * @param 0 if successful
 */
int32_t mdm_hl7800_get_local_time(struct tm *tm, int32_t *offset);
#endif

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
/**
 * @brief Update the HL7800 via XMODEM protocol.  During the firmware update
 * no other modem fuctions will be available.
 *
 * @param file_path Absolute path of the update file
 *
 * @param 0 if successful
 */
int32_t mdm_hl7800_update_fw(char *file_path);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_HL7800_H_ */
