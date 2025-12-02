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

/* Magic constants */
#define CSQ_RSSI_UNKNOWN        (99)
#define CESQ_RSRP_UNKNOWN       (255)
#define CESQ_RSRQ_UNKNOWN       (255)
/* Magic numbers to units conversions */
#define CSQ_RSSI_TO_DB(v)       (-113 + (2 * (v)))
#define CESQ_RSRP_TO_DB(v)      (-140 + (v))
#define CESQ_RSRQ_TO_DB(v)      (-20 + ((v) / 2))
/** Monitor is paused. */
#define PAUSED                  1
/** Monitor is active, default */
#define ACTIVE                  0
#define MDM_MANUFACTURER_LENGTH 20
#define MDM_MODEL_LENGTH        32
#define MDM_REVISION_LENGTH     64
#define MDM_IMEI_LENGTH         16
#define MDM_IMSI_LENGTH         23
#define MDM_ICCID_LENGTH        22
#define MDM_APN_MAX_LENGTH      64
#define MDM_MAX_CERT_LENGTH     4096
#define MDM_MAX_HOSTNAME_LEN    128
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

/** Cellular radio access technologies */
enum hl78xx_cell_rat_mode {
	HL78XX_RAT_CAT_M1 = 0,
	HL78XX_RAT_NB1,
#ifdef CONFIG_MODEM_HL78XX_12
	HL78XX_RAT_GSM,
#ifdef CONFIG_MODEM_HL78XX_12_FW_R6
	HL78XX_RAT_NBNTN,
#endif
#endif
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	HL78XX_RAT_MODE_AUTO,
#endif
	HL78XX_RAT_MODE_NONE,
	HL78XX_RAT_COUNT = HL78XX_RAT_MODE_NONE
};

/** Phone functionality modes */
enum hl78xx_phone_functionality {
	HL78XX_SIM_POWER_OFF,
	HL78XX_FULLY_FUNCTIONAL,
	HL78XX_AIRPLANE = 4,
};
/** Module status codes */
enum hl78xx_module_status {
	HL78XX_MODULE_READY = 0,
	HL78XX_MODULE_WAITING_FOR_ACCESS_CODE,
	HL78XX_MODULE_SIM_NOT_PRESENT,
	HL78XX_MODULE_SIMLOCK,
	HL78XX_MODULE_UNRECOVERABLE_ERROR,
	HL78XX_MODULE_UNKNOWN_STATE,
	HL78XX_MODULE_INACTIVE_SIM
};

/** Cellular modem info type */
enum hl78xx_modem_info_type {
	/* <APN> Access Point Name */
	HL78XX_MODEM_INFO_APN,
	/* <Current RAT> */
	HL78XX_MODEM_INFO_CURRENT_RAT,
	/* <Network Operator> */
	HL78XX_MODEM_INFO_NETWORK_OPERATOR,
};

/** Cellular network structure */
struct hl78xx_network {
	/** Cellular access technology */
	enum hl78xx_cell_rat_mode technology;
	/**
	 * List of bands, as defined by the specified cellular access technology,
	 * to enables. All supported bands are enabled if none are provided.
	 */
	uint16_t *bands;
	/** Size of bands */
	uint16_t size;
};

enum hl78xx_evt_type {
	HL78XX_LTE_RAT_UPDATE,
	HL78XX_LTE_REGISTRATION_STAT_UPDATE,
	HL78XX_LTE_SIM_REGISTRATION,
	HL78XX_LTE_MODEM_STARTUP,
};

struct hl78xx_evt {
	enum hl78xx_evt_type type;

	union {
		enum cellular_registration_status reg_status;
		enum hl78xx_cell_rat_mode rat_mode;
		bool status;
		int value;
	} content;
};
/** API for configuring networks */
typedef int (*hl78xx_api_configure_networks)(const struct device *dev,
					     const struct hl78xx_network *networks, uint8_t size);

/** API for getting supported networks */
typedef int (*hl78xx_api_get_supported_networks)(const struct device *dev,
						 const struct hl78xx_network **networks,
						 uint8_t *size);

/** API for getting network signal strength */
typedef int (*hl78xx_api_get_signal)(const struct device *dev, const enum cellular_signal_type type,
				     int16_t *value);

/** API for getting modem information */
typedef int (*hl78xx_api_get_modem_info)(const struct device *dev,
					 const enum cellular_modem_info_type type, char *info,
					 size_t size);

/** API for getting registration status */
typedef int (*hl78xx_api_get_registration_status)(const struct device *dev,
						  enum cellular_access_technology tech,
						  enum cellular_registration_status *status);

/** API for setting apn */
typedef int (*hl78xx_api_set_apn)(const struct device *dev, const char *apn, uint16_t size);

/** API for set phone functionality */
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

/**< Event monitor entry */
struct hl78xx_evt_monitor_entry; /* forward declaration */
/* Event monitor dispatcher */
typedef void (*hl78xx_evt_monitor_dispatcher_t)(struct hl78xx_evt *notif);
/* Event monitor handler */
typedef void (*hl78xx_evt_monitor_handler_t)(struct hl78xx_evt *notif,
					     struct hl78xx_evt_monitor_entry *mon);

struct hl78xx_evt_monitor_entry {
	/** Monitor callback. */
	const hl78xx_evt_monitor_handler_t handler;
	/* link for runtime list */
	struct hl78xx_evt_monitor_entry *next;
	struct {
		uint8_t paused: 1; /* Monitor is paused. */
		uint8_t direct: 1; /* Dispatch in ISR. */
	} flags;
};
/**
 * @brief hl78xx_api_func_set_phone_functionality
 * @param dev Cellular network device instance
 * @param functionality phone functionality mode to set
 * @param reset If true, the modem will be reset as part of applying the functionality change.
 * @return 0 if successful.
 */
int hl78xx_api_func_set_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality functionality,
					    bool reset);
/**
 * @brief hl78xx_api_func_get_phone_functionality
 * @param dev Cellular network device instance
 * @param functionality Pointer to store the current phone functionality mode
 * @return 0 if successful.
 */
int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality);
/**
 * @brief hl78xx_api_func_get_signal - Brief description of the function.
 * @param dev Cellular network device instance
 * @param type Type of the signal to retrieve
 * @param value Pointer to store the signal value
 * @return 0 if successful.
 */
int hl78xx_api_func_get_signal(const struct device *dev, const enum cellular_signal_type type,
			       int16_t *value);
/**
 * @brief hl78xx_api_func_get_modem_info_vendor - Brief description of the function.
 * @param dev Cellular network device instance
 * @param type Type of the modem info to retrieve
 * @param info Pointer to store the modem info
 * @param size Size of the info buffer
 * @return 0 if successful.
 */
int hl78xx_api_func_get_modem_info_vendor(const struct device *dev,
					  enum hl78xx_modem_info_type type, void *info,
					  size_t size);
/**
 * @brief hl78xx_api_func_modem_dynamic_cmd_send - Brief description of the function.
 * @param dev Cellular network device instance
 * @param cmd AT command to send
 * @param cmd_size Size of the AT command
 * @param response_matches Expected response patterns
 * @param matches_size Size of the response patterns
 * @return 0 if successful.
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
	/* AT+CSQ returns a response +CSQ: <rssi>,<ber> where:
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
	 * +CESQ: <rxlev>,<ber>,<rscp>,<ecn0>,<rsrq>,<rsrp> where:
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
 * @brief Register an event monitor to receive HL78xx modem event notifications.
 */
int hl78xx_evt_monitor_register(struct hl78xx_evt_monitor_entry *mon);
/**
 * @brief Unregister an event monitor from receiving HL78xx modem event notifications.
 */
int hl78xx_evt_monitor_unregister(struct hl78xx_evt_monitor_entry *mon);
/**
 * @brief Convert HL78xx RAT mode to standard cellular API.
 */
enum cellular_access_technology hl78xx_rat_to_access_tech(enum hl78xx_cell_rat_mode rat_mode);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_ */
