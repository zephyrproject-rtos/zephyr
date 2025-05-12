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

static inline int hl78xx_set_apn(const struct device *dev, const char *apn, uint16_t size)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->set_apn == NULL) {
		return -ENOSYS;
	}

	return api->set_apn(dev, apn, size);
}

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

static inline int hl78xx_get_phone_functionality(const struct device *dev,
						 enum hl78xx_phone_functionality *functionality)
{
	const struct hl78xx_driver_api *api = (const struct hl78xx_driver_api *)dev->api;

	if (api->get_phone_functionality == NULL) {
		return -ENOSYS;
	}

	return api->get_phone_functionality(dev, functionality);
}

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

/***************** Inline Public functions *******************************/
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

static inline int hl78xx_parse_rsrq(uint8_t rsrq, int16_t *value)
{
	if (rsrq == CESQ_RSRQ_UNKNOWN) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRQ_TO_DB(rsrq);
	return 0;
}
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HL78XX_APIS_H_ */
