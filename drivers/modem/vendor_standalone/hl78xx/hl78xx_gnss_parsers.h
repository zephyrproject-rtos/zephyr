/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_GNSS_PARSERS_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_GNSS_PARSERS_H_

#include <zephyr/modem/chat.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>

#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER

/**
 * @brief Match callback for NMEA0183 GSA message (GNSS DOP and Active Satellites)
 *
 * @details Parses GSA message to extract fix type and dilution of precision values.
 * Format: $xxGSA,<mode>,<fix_type>,<sat_ids>...,<pdop>,<hdop>,<vdop>*<checksum>
 *
 * @param chat Modem chat instance
 * @param argv Array of parsed fields from NMEA sentence
 * @param argc Number of fields in argv
 * @param user_data Pointer to hl78xx_gnss_data structure
 */
void gnss_nmea0183_match_gsa_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

/**
 * @brief Match callback for NMEA0183 GST message (GNSS Pseudorange Error Statistics)
 *
 * @details Parses GST message to extract position error estimates.
 * Format: $xxGST,<time>,<rms>,<smajor>,<sminor>,<orient>,<lat_err>,<lon_err>,<alt_err>*<checksum>
 *
 * @param chat Modem chat instance
 * @param argv Array of parsed fields from NMEA sentence
 * @param argc Number of fields in argv
 * @param user_data Pointer to hl78xx_gnss_data structure
 */
void gnss_nmea0183_match_gst_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

/**
 * @brief Match callback for PSEPU message (Position Velocity Accuracy Index)
 *
 * @details Parses HL78xx-specific PSEPU proprietary sentence for position and velocity uncertainty.
 * Format:
 * $PSEPU,<pos_3d>,<pos_2d>,<pos_lat>,<pos_lon>,<pos_alt>,<vel_3d>,
 * <vel_2d>,<vel_hdg>,<vel_east>,<vel_north>,<vel_up>*<checksum>
 *
 * @param chat Modem chat instance
 * @param argv Array of parsed fields from NMEA sentence
 * @param argc Number of fields in argv
 * @param user_data Pointer to hl78xx_gnss_data structure
 */
void gnss_nmea0183_match_epu_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

void gnss_publish_aux_data(const struct device *dev,
			   const struct hl78xx_gnss_nmea_aux_data *aux_data, uint16_t size);
#endif /* CONFIG_HL78XX_GNSS_AUX_DATA_PARSER */

/**
 * @brief Parse latitude/longitude in DMS format to nanodegrees
 *
 * @details Converts GNSSLOC DMS format (e.g., "52 Deg 4 Min 14.43 Sec N") to nanodegrees.
 * Used for parsing AT+GNSSLOC? response latitude/longitude fields.
 *
 * @param str Input string in DMS format
 * @param ndeg Output latitude/longitude in nanodegrees (0 to ±180E9)
 * @return 0 on success, negative error code on failure
 */
int gnssloc_dms_to_ndeg(const char *str, int64_t *ndeg);

/**
 * @brief Parse value with unit suffix to milli-units
 *
 * @details Parses values like "-12.800 m" or "4.30 m" to milli-units.
 * Used for parsing altitude, HEPE, and other metric values from GNSSLOC.
 *
 * @param str Input string with value and unit (e.g., "-12.800 m")
 * @param milli Output value in milli-units (millimeters for "m")
 * @return 0 on success, negative error code on failure
 */
int gnssloc_parse_value_with_unit(const char *str, int64_t *milli);

/**
 * @brief Parse speed value (m/s) to mm/s
 *
 * @details Converts speed string from GNSSLOC to millimeters per second.
 *
 * @param str Input speed string (e.g., "1.5" for 1.5 m/s)
 * @param mms Output speed in millimeters per second
 * @return 0 on success, negative error code on failure
 */
int gnssloc_parse_speed_to_mms(const char *str, uint32_t *mms);

/**
 * @brief Parse GNSSLOC GPS time string to gnss_time structure
 *
 * @details Parses time format "YYYY M D HH:MM:SS" from GNSSLOC response.
 *
 * @param str Input time string
 * @param utc Output gnss_time structure
 * @return 0 on success, negative error code on failure
 */
int gnssloc_parse_gpstime(const char *str, struct gnss_time *utc);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_GNSS_PARSERS_H_ */
