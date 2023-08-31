/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_NMEA0183_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_NMEA0183_H_

#include <zephyr/drivers/gnss.h>

/**
 * @brief Compute NMEA0183 checksum
 *
 * @example "PAIR002" -> 0x38
 *
 * @param str String from which checksum is computed
 *
 * @retval checksum
 */
uint8_t gnss_nmea0183_checksum(const char *str);

/**
 * @brief Encapsulate str in NMEA0183 message format
 *
 * @example "PAIR%03u", 2 -> "$PAIR002*38"
 *
 * @param str Destination for encapsulated string
 * @param size Size of destination for encapsulated string
 * @param fmt Format of string to encapsulate
 * @param ... Arguments
 *
 * @retval checksum
 */
int gnss_nmea0183_snprintk(char *str, size_t size, const char *fmt, ...);

/**
 * @brief Computes and validates checksum
 *
 * @param argv Array of arguments split by ',' including message id and checksum
 * @param argc Number of arguments in argv
 *
 * @retval true if message is intact
 * @retval false if message is corrupted
 */
bool gnss_nmea0183_validate_message(char **argv, uint16_t argc);

/**
 * @brief Parse a ddmm.mmmm formatted angle to nano degrees
 *
 * @example "5610.9928" -> 56183214000
 *
 * @param ddmm_mmmm String representation of angle in ddmm.mmmm format
 * @param ndeg Result in nano degrees
 *
 * @retval -EINVAL if ddmm_mmmm argument is invalid
 * @retval 0 if parsed successfully
 */
int gnss_nmea0183_ddmm_mmmm_to_ndeg(const char *ddmm_mmmm, int64_t *ndeg);

/**
 * @brief Parse knots to millimeters pr second
 *
 * @example "15.231" -> 7835
 *
 * @param str String representation of speed in knots
 * @param mms Destination for speed in millimeters pr second
 *
 * @retval -EINVAL if str could not be parsed or if speed is negative
 * @retval 0 if parsed successfully
 */
int gnss_nmea0183_knots_to_mms(const char *str, int64_t *mms);

/**
 * @brief Parse hhmmss.sss to struct gnss_time
 *
 * @example "133243.012" -> { .hour = 13, .minute = 32, .ms = 43012 }
 * @example "133243" -> { .hour = 13, .minute = 32, .ms = 43000 }
 *
 * @param str String representation of hours, minutes, seconds and subseconds
 * @param utc Destination for parsed time
 *
 * @retval -EINVAL if str could not be parsed
 * @retval 0 if parsed successfully
 */
int gnss_nmea0183_parse_hhmmss(const char *hhmmss, struct gnss_time *utc);

/**
 * @brief Parse ddmmyy to unsigned integers
 *
 * @example "041122" -> { .mday = 4, .month = 11, .year = 22 }
 *
 * @param str String representation of speed in knots
 * @param utc Destination for parsed time
 *
 * @retval -EINVAL if str could not be parsed
 * @retval 0 if parsed successfully
 */
int gnss_nmea0183_parse_ddmmyy(const char *ddmmyy, struct gnss_time *utc);

/**
 * @brief Parses NMEA0183 RMC message
 *
 * @details Parses the time, date, latitude, longitude, speed, and bearing
 * from the NMEA0183 RMC message provided as an array of strings split by ','
 *
 * @param argv Array of arguments split by ',' including message id and checksum
 * @param argc Number of arguments in argv'
 * @param data Destination for data parsed from NMEA0183 RMC message
 *
 * @retval 0 if successful
 * @retval -EINVAL if input is invalid
 */
int gnss_nmea0183_parse_rmc(const char **argv, uint16_t argc, struct gnss_data *data);

/**
 * @brief Parses NMEA0183 GGA message
 *
 * @details Parses the GNSS fix quality and status, number of satellites used for
 * fix, HDOP, and altitude (geoid separation) from the NMEA0183 GGA message provided
 * as an array of strings split by ','
 *
 * @param argv Array of arguments split by ',' including message id and checksum
 * @param argc Number of arguments in argv'
 * @param data Destination for data parsed from NMEA0183 GGA message
 *
 * @retval 0 if successful
 * @retval -EINVAL if input is invalid
 */
int gnss_nmea0183_parse_gga(const char **argv, uint16_t argc, struct gnss_data *data);

/** GSV header structure */
struct gnss_nmea0183_gsv_header {
	/** Indicates the system of the space-vehicles contained in the message */
	enum gnss_system system;
	/** Number of GSV messages in total */
	uint16_t number_of_messages;
	/** Number of this GSV message */
	uint16_t message_number;
	/** Number of visible space-vehicles */
	uint16_t number_of_svs;
};

/**
 * @brief Parses header of NMEA0183 GSV message
 *
 * @details The GSV messages are part of a list of messages sent in ascending
 * order, split by GNSS system.
 *
 * @param argv Array of arguments split by ',' including message id and checksum
 * @param argc Number of arguments in argv
 * @param header Destination for parsed NMEA0183 GGA message header
 *
 * @retval 0 if successful
 * @retval -EINVAL if input is invalid
 */
int gnss_nmea0183_parse_gsv_header(const char **argv, uint16_t argc,
				   struct gnss_nmea0183_gsv_header *header);

/**
 * @brief Parses space-vehicles in NMEA0183 GSV message
 *
 * @details The NMEA0183 GSV message contains up to 4 space-vehicles which follow
 * the header.
 *
 * @param argv Array of arguments split by ',' including message id and checksum
 * @param argc Number of arguments in argv
 * @param satellites Destination for parsed satellites from NMEA0183 GGA message
 * @param size Size of destination for parsed satellites from NMEA0183 GGA message
 *
 * @retval Number of parsed space-vehicles stored at destination if successful
 * @retval -ENOMEM if all space-vehicles in message could not be stored at destination
 * @retval -EINVAL if input is invalid
 */
int gnss_nmea0183_parse_gsv_svs(const char **argv, uint16_t argc,
				struct gnss_satellite *satellites, uint16_t size);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_NMEA0183_H_ */
