/*
 * Copyright (c) 2025 Netfeasa
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

#ifdef __cplusplus
extern "C" {
#endif

/* Magic constants */
#define CSQ_RSSI_UNKNOWN   (99)
#define CESQ_RSRP_UNKNOWN  (255)
#define CESQ_RSRQ_UNKNOWN  (255)
/* Magic numbers to units conversions */
#define CSQ_RSSI_TO_DB(v)  (-113 + (2 * (rssi)))
#define CESQ_RSRP_TO_DB(v) (-140 + (v))
#define CESQ_RSRQ_TO_DB(v) (-20 + ((v) / 2))
/** Monitor is paused. */
#define PAUSED             1
/** Monitor is active, default */
#define ACTIVE             0

/**
 * @brief Define an Event monitor to receive notifications in the system workqueue thread.
 *
 * @param name The monitor name.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c PAUSED or @c ACTIVE).
 *	      The default initial state of a monitor is active.
 */
#define HL78XX_EVT_MONITOR(name, _handler, ...)                                                    \
	static void _handler(struct hl78xx_evt *);                                                 \
	static STRUCT_SECTION_ITERABLE(hl78xx_evt_monitor_entry, name) = {                         \
		.handler = _handler,                                                               \
		.flags.direct = false,                                                             \
		COND_CODE_1(__VA_ARGS__, (.flags.paused = __VA_ARGS__,), ()) }

/** Cellular radio access technologies */
enum hl78xx_cell_rat_mode {
	HL78XX_RAT_CAT_M1 = 0,
	HL78XX_RAT_NB1,
#ifdef CONFIG_MODEM_HL7812
	HL78XX_RAT_GSM,
	HL78XX_RAT_NBNTN,
#endif
	HL78XX_RAT_MODE_NONE,
	HL78XX_RAT_COUNT = HL78XX_RAT_MODE_NONE
};
/*  */
enum hl78xx_phone_functionality {
	HL78XX_SIM_POWER_OFF,
	HL78XX_FULLY_FUNCTIONAL,
	HL78XX_AIRPLANE = 4,
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

/** Cellular signal type */
enum hl78xx_signal_type {
	HL78XX_SIGNAL_RSSI,
	HL78XX_SIGNAL_RSRP,
	HL78XX_SIGNAL_RSRQ,
};

/** Cellular modem info type */
enum hl78xx_modem_info_type {
	/** International Mobile Equipment Identity */
	HL78XX_MODEM_INFO_IMEI,
	/** Modem model ID */
	HL78XX_MODEM_INFO_MODEL_ID,
	/** Modem manufacturer */
	HL78XX_MODEM_INFO_MANUFACTURER,
	/** Modem fw version */
	HL78XX_MODEM_INFO_FW_VERSION,
	/** International Mobile Subscriber Identity */
	HL78XX_MODEM_INFO_SIM_IMSI,
	/** Integrated Circuit Card Identification Number (SIM) */
	HL78XX_MODEM_INFO_SIM_ICCID,
	/* <APN> Access Point Name */
	HL78XX_MODEM_INFO_APN,
};

enum hl78xx_registration_status {
	HL78XX_REGISTRATION_NOT_REGISTERED = 0,
	HL78XX_REGISTRATION_REGISTERED_HOME,
	HL78XX_REGISTRATION_SEARCHING,
	HL78XX_REGISTRATION_DENIED,
	HL78XX_REGISTRATION_UNKNOWN,
	HL78XX_REGISTRATION_REGISTERED_ROAMING,
};

enum hl78xx_evt_type {
	HL78XX_RAT_UPDATE,
	HL78XX_LTE_REGISTRATION_STAT_UPDATE,
	HL78XX_LTE_SIM_REGISTRATIION,
	HL78XX_LTE_PSMEV,
};

struct hl78xx_evt {
	enum hl78xx_evt_type type;

	union {
		enum hl78xx_registration_status reg_status;
		enum hl78xx_cell_rat_mode rat_mode;
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
typedef int (*hl78xx_api_get_signal)(const struct device *dev, const enum hl78xx_signal_type type,
				     int16_t *value);

/** API for getting modem information */
typedef int (*hl78xx_api_get_modem_info)(const struct device *dev,
					 const enum hl78xx_modem_info_type type, char *info,
					 size_t size);

/** API for getting registration status */
typedef int (*hl78xx_api_get_registration_status)(const struct device *dev,
						  enum hl78xx_cell_rat_mode *tech,
						  enum hl78xx_registration_status *status);

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

typedef void (*hl78xx_evt_monitor_handler_t)(struct hl78xx_evt *notif);

struct hl78xx_evt_monitor_entry {
	/** Monitor callback. */
	const hl78xx_evt_monitor_handler_t handler;
	struct {
		uint8_t paused: 1; /* Monitor is paused. */
		uint8_t direct: 1; /* Dispatch in ISR. */
	} flags;
};

/** Cellular driver API */
__subsystem struct hl78xx_driver_api {
	hl78xx_api_configure_networks configure_networks;
	hl78xx_api_get_supported_networks get_supported_networks;
	hl78xx_api_get_signal get_signal;
	hl78xx_api_get_modem_info get_modem_info;
	hl78xx_api_get_registration_status get_registration_status;
	hl78xx_api_set_apn set_apn;
	hl78xx_api_set_phone_functionality set_phone_functionality;
	hl78xx_api_get_phone_functionality get_phone_functionality;
	hl78xx_api_send_at_cmd send_at_cmd;
};

/**
 * @brief Configure cellular networks for the device
 *
 * @details Cellular network devices support at least one cellular access technology.
 * Each cellular access technology defines a set of bands, of which the cellular device
 * will support all or a subset of.
 *
 * The cellular device can only use one cellular network technology at a time. It must
 * exclusively use the cellular network configurations provided, and will prioritize
 * the cellular network configurations in the order they are provided in case there are
 * multiple (the first cellular network configuration has the highest priority).
 *
 * @param dev Cellular network device instance.
 * @param networks List of cellular network configurations to apply.
 * @param size Size of list of cellular network configurations.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if any provided cellular network configuration is invalid or unsupported.
 * @retval -ENOTSUP if API is not supported by cellular network device.
 * @retval Negative errno-code otherwise.
 */
static inline int hl78xx_configure_networks(const struct device *dev,
					    const struct hl78xx_network *networks, uint8_t size)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->configure_networks == NULL) {
		return -ENOSYS;
	}

	return api->configure_networks(dev, networks, size);
}

/**
 * @brief Get supported cellular networks for the device
 *
 * @param dev Cellular network device instance
 * @param networks Pointer to list of supported cellular network configurations.
 * @param size Size of list of cellular network configurations.
 *
 * @retval 0 if successful.
 * @retval -ENOTSUP if API is not supported by cellular network device.
 * @retval Negative errno-code otherwise.
 */
static inline int hl78xx_get_supported_networks(const struct device *dev,
						const struct hl78xx_network **networks,
						uint8_t *size)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->get_supported_networks == NULL) {
		return -ENOSYS;
	}

	return api->get_supported_networks(dev, networks, size);
}

/**
 * @brief Get signal for the device
 *
 * @param dev Cellular network device instance
 * @param type Type of the signal information requested
 * @param value Signal strength destination (one of RSSI, RSRP, RSRQ)
 *
 * @retval 0 if successful.
 * @retval -ENOTSUP if API is not supported by cellular network device.
 * @retval -ENODATA if device is not in a state where signal can be polled
 * @retval Negative errno-code otherwise.
 */
static inline int hl78xx_get_signal(const struct device *dev, const enum hl78xx_signal_type type,
				    int16_t *value)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->get_signal == NULL) {
		return -ENOSYS;
	}

	return api->get_signal(dev, type, value);
}

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
					const enum hl78xx_modem_info_type type, char *info,
					size_t size)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->get_modem_info == NULL) {
		return -ENOSYS;
	}

	return api->get_modem_info(dev, type, info, size);
}

/**
 * @brief Get network registration status for the device
 *
 * @param dev Cellular network device instance
 * @param tech Which access technology to get status for
 * @param status Registration status for given access technology
 *
 * @retval 0 if successful.
 * @retval -ENOSYS if API is not supported by cellular network device.
 * @retval -ENODATA if modem does not provide info requested
 * @retval Negative errno-code from chat module otherwise.
 */
static inline int hl78xx_get_registration_status(const struct device *dev,
						 enum hl78xx_cell_rat_mode *tech,
						 enum hl78xx_registration_status *status)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->get_registration_status == NULL) {
		return -ENOSYS;
	}

	return api->get_registration_status(dev, tech, status);
}

/**
 * @brief Set the Access Point Name (APN) for the modem.
 *
 * Stores the specified APN string in the modem driver context to be used
 * during PDP context activation or network registration.
 *
 * @param dev Pointer to the modem device instance.
 * @param apn Null-terminated string representing the APN to be set.
 * @param size Length of the APN string, including the null terminator.
 *
 * @retval 0 on success.
 * @retval -EINVAL if input parameters are invalid.
 */
static inline int hl78xx_set_apn(const struct device *dev, const char *apn, uint16_t size)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->set_apn == NULL) {
		return -ENOSYS;
	}

	return api->set_apn(dev, apn, size);
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
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->set_phone_functionality == NULL) {
		return -ENOSYS;
	}

	return api->set_phone_functionality(dev, functionality, reset);
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
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->get_phone_functionality == NULL) {
		return -ENOSYS;
	}

	return api->get_phone_functionality(dev, functionality);
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
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->send_at_cmd == NULL) {
		return -ENOSYS;
	}

	return api->send_at_cmd(dev, cmd, cmd_size, response_matches, matches_size);
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
int hl78xx_evt_notif_handler_set(hl78xx_evt_monitor_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_ */
