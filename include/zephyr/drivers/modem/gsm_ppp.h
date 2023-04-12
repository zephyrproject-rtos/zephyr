/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GSM_PPP_MDM_MANUFACTURER_LENGTH  10
#define GSM_PPP_MDM_MODEL_LENGTH         16
#define GSM_PPP_MDM_REVISION_LENGTH      64
#define GSM_PPP_MDM_IMEI_LENGTH          16
#define GSM_PPP_MDM_IMSI_LENGTH          16
#define GSM_PPP_MDM_ICCID_LENGTH         32
#define GSM_PPP_MDM_FIRMWARE_LENGTH      32

#if defined(CONFIG_MODEM_GMS_ENABLE_SMS)
#define GSM_PPP_SMS_STATUS_LENGTH        16
#define GSM_PPP_SMS_OA_LENGTH            16
#define GSM_PPP_SMS_OA_NAME_LENGTH       32
#define GSM_PPP_SMS_DATE_LENGTH          16
#define GSM_PPP_SMS_TIME_LENGTH          16
#define GSM_PPP_SMS_DATA_LENGTH         180
#endif

#if defined(CONFIG_MODEM_GSM_ENABLE_GNSS)
#define GSM_PPP_GNSS_DATA_UTC_LEN        64
#endif

#define GSM_RSSI_INVALID              -1000

struct gsm_ppp_modem_info {
	char mdm_manufacturer[GSM_PPP_MDM_MANUFACTURER_LENGTH];
	char mdm_model[GSM_PPP_MDM_MODEL_LENGTH];
	char mdm_revision[GSM_PPP_MDM_REVISION_LENGTH];
	char mdm_imei[GSM_PPP_MDM_IMEI_LENGTH];
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char mdm_imsi[GSM_PPP_MDM_IMSI_LENGTH];
	char mdm_iccid[GSM_PPP_MDM_ICCID_LENGTH];
#endif
#if defined (CONFIG_MODEM_FIRMWARE_VERSION)
	char mdm_firmware_version[GSM_PPP_MDM_FIRMWARE_LENGTH];
#endif
	int  mdm_rssi;
};

#if defined(CONFIG_MODEM_GMS_ENABLE_SMS)
struct gsm_ppp_sms_message {
	bool valid;
	bool has_data;
	int index;
	char status[GSM_PPP_SMS_STATUS_LENGTH];
	char origin_address[GSM_PPP_SMS_OA_LENGTH];
	char origin_name[GSM_PPP_SMS_OA_NAME_LENGTH];
	char date[GSM_PPP_SMS_DATE_LENGTH];
	char time[GSM_PPP_SMS_TIME_LENGTH];
	char data[GSM_PPP_SMS_DATA_LENGTH];
};

enum ring_indicator_behaviour {
	OFF,
	PULSE,
	ALWAYS,
};
#endif  /* CONFIG_MODEM_GMS_ENABLE_SMS */

#if defined(CONFIG_MODEM_GSM_ENABLE_GNSS)
struct gsm_ppp_gnss_data {
	/**
	 * UTC in format ddmmyyhhmmss.s
	 */
	char utc[GSM_PPP_GNSS_DATA_UTC_LEN];
	/**
	 * Latitude in 10^-5 degree.
	 */
	int32_t lat;
	/**
	 * Longitude in 10^-5 degree.
	 */
	int32_t lon;
	/**
	 * Altitude in mm.
	 */
	int32_t alt;
	/**
	 * Horizontal dilution of precision in 10^-1.
	 */
	uint16_t hdop;
	/**
	 * Course over ground in 10^-2 degree.
	 */
	uint16_t cog;
	/**
	 * Speed in 10^-1 km/h.
	 */
	uint16_t kmh;
	/**
	 * Number of satellites in use.
	 */
	uint16_t nsat;
};
#endif /* CONFIG_MODEM_GSM_ENABLE_GNSS */

/** @cond INTERNAL_HIDDEN */
struct device;
typedef void (*gsm_modem_power_cb)(const struct device *, void *);

void gsm_ppp_start(const struct device *dev);
void gsm_ppp_recover_cmux(const struct device *dev);
void gsm_ppp_stop(const struct device *dev, bool keep_AT_channel);
/** @endcond */

/**
 * @brief Register functions callbacks for power modem on/off.
 *
 * @param dev: gsm modem device
 * @param modem_on: callback function to
 *		execute during gsm ppp configuring.
 * @param modem_apn: callback function to
 * 		provide an APN from application.
 *   	callback is expected to call gsm_ppp_configure_apn.
 * @param modem_configured: callback function to
 *		execute when modem is configured.
 * @param modem_off: callback function to
 *		execute during gsm ppp stopping.
 * @param user_data: user specified data
 */
void gsm_ppp_register_modem_power_callback(const struct device *dev,
					   gsm_modem_power_cb modem_on,
					   gsm_modem_power_cb modem_apn,
					   gsm_modem_power_cb modem_configured,
					   gsm_modem_power_cb modem_off,
					   void *user_data);

/**
 * @brief Get GSM modem information.
 *
 * @param dev: GSM modem device.
 * @param update_rssi: Whether to update the rssi before obtaining the modem info.
 *
 * @retval struct gsm_ppp_modem_info * pointer to modem information structure.
 */
const struct gsm_ppp_modem_info *gsm_ppp_modem_info(const struct device *dev, bool update_rssi);

#if defined(CONFIG_MODEM_GMS_ENABLE_SMS)
/**
 * @brief Set modem ring indicator behaviour.
 *
 * @param dev: GSM modem device.
 * @param ring: Ring indicator behaviour when ring is presented.
 * @param ring_pulse_duration: Ring indicator pulse duration when ring behavior set to pulse.
 * @param sms: Ring indicator behaviour when SMS is presented.
 * @param sms_pulse_duration: SMS indicator pulse duration when SMS behavior set to pulse.
 * @param other: Ring indicator behaviour when other is presented.
 * @param other_pulse_duration: Other indicator pulse duration when other behavior set to pulse.
 */
void gsm_ppp_set_ring_indicator(const struct device *dev,
				enum ring_indicator_behaviour ring,
				uint16_t ring_pulse_duration,
				enum ring_indicator_behaviour sms,
				uint16_t sms_pulse_duration,
				enum ring_indicator_behaviour other,
				uint16_t other_pulse_duration);

/**
 * @brief Configure modem for receiving SMS messages.
 *
 * @param dev: GSM modem device.
 */
void gsm_ppp_configure_sms_reception(const struct device *dev);

/**
 * @brief Read a SMS message from the modem.
 *
 * @param dev: GSM modem device.
 *
 * @retval struct gsm_ppp_sms_message * pointer to the received SMS information
 */
struct gsm_ppp_sms_message * gsm_ppp_read_sms(const struct device *dev);

/**
 * @brief Delete all SMS messages from the modem storage.
 *
 * @param dev: GSM modem device.
 */
void gsm_ppp_delete_all_sms(const struct device *dev);

/**
 * @brief Clear the ring indicator of the modem.
 *
 * @param dev: GSM modem device.
 */
void gsm_ppp_clear_ring_indicator(const struct device *dev);
#endif /* CONFIG_MODEM_GMS_ENABLE_SMS */

#if defined(CONFIG_MODEM_GSM_ENABLE_GNSS)
/**
 * @brief Starts the modem in gnss operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int gsm_ppp_start_gnss(const struct device *dev);

/**
 * @brief Query gnss position form the modem.
 *
 * @return 0 on success. If no fix is acquired yet -EAGAIN is returned.
 *         Otherwise <0 is returned.
 */
int gsm_ppp_query_gnss(const struct device *dev, struct gsm_ppp_gnss_data *data);

/**
 * @brief Stops the gnss operation mode of the modem.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int gsm_ppp_stop_gnss(const struct device *dev);

/**
 * @brief Configure the APN.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int gsm_ppp_configure_apn(const struct device *dev, const char* apn);

#endif /* CONFIG_MODEM_GSM_ENABLE_GNSS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_ */
