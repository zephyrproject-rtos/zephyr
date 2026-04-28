/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The GNSS NMEA0183 match is a set of modem_chat match handlers and a context to be
 * passed to said handlers, to parse the NMEA0183 messages received from a NMEA0183
 * based GNSS device.
 *
 * The context struct gnss_nmea0183_match_data *data is placed as the first member
 * of the data structure which is passed to the modem_chat instance through the
 * user_data member.
 *
 *   struct my_gnss_nmea0183_driver {
 *           gnss_nmea0183_match_data match_data;
 *           ...
 *   };
 *
 * The struct gnss_nmea0183_match_data context must be initialized using
 * gnss_nmea0183_match_init().
 *
 * When initializing the modem_chat instance, the three match callbacks must be added
 * as part of the unsolicited matches.
 *
 *   MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
 *           MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
 *           MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
 *   #if CONFIG_GNSS_SATELLITES
 *           MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
 *   #endif
 *
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_NMEA0183_MATCH_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_NMEA0183_MATCH_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/modem/chat.h>

struct gnss_nmea0183_match_data {
	const struct device *gnss;
	struct gnss_data data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite *satellites;
	uint16_t satellites_size;
	uint16_t satellites_length;
#endif
	uint32_t gga_utc;
	uint32_t rmc_utc;
	uint8_t gsv_message_number;
};

/** GNSS NMEA0183 match configuration structure */
struct gnss_nmea0183_match_config {
	/** The GNSS device from which the data is published */
	const struct device *gnss;
#if CONFIG_GNSS_SATELLITES
	/** Buffer for parsed satellites */
	struct gnss_satellite *satellites;
	/** Number of elements in buffer for parsed satellites */
	uint16_t satellites_size;
#endif
};

/**
 * @brief Match callback for the NMEA GGA NMEA0183 message
 *
 * @details Should be used as the callback of a modem_chat match which matches "$??GGA,"
 */
void gnss_nmea0183_match_gga_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

/**
 * @brief Match callback for the NMEA RMC NMEA0183 message
 *
 * @details Should be used as the callback of a modem_chat match which matches "$??RMC,"
 */
void gnss_nmea0183_match_rmc_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

/**
 * @brief Match callback for the NMEA GSV NMEA0183 message
 *
 * @details Should be used as the callback of a modem_chat match which matches "$??GSV,"
 */
void gnss_nmea0183_match_gsv_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

/**
 * @brief Match callback for the NMEA GST NMEA0183 message
 *
 * @details Should be used as the callback of a modem_chat match which matches
 * "$??GST,". Parses the sentence into a struct gnss_accuracy and publishes it
 * via gnss_publish_accuracy() on every successful parse — no pairing with
 * GGA/RMC. Malformed sentences (parser returns -EINVAL) are silently dropped.
 *
 * @param chat Pointer to the chat instance that received the sentence
 * @param argv Array of comma-split arguments, including message id and checksum
 * @param argc Number of arguments in @p argv
 * @param user_data User data set during modem_chat_init(); must point to a
 *                  struct gnss_nmea0183_match_data (driver-private context —
 *                  typically the first member of the driver data struct so
 *                  the cast works through structural aliasing)
 */
void gnss_nmea0183_match_gst_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data);

/**
 * @brief modem_chat line-tap dispatcher that publishes raw NMEA sentences
 *
 * @details Should be used as @ref modem_chat_config.line_tap when the driver
 * wires raw NMEA forwarding. Forwards every complete delimited line to
 * gnss_publish_raw_nmea() — does not parse, validate, or filter; the receiver
 * sees every sentence the chip emits, parseable or not.
 *
 * @param chat Pointer to the chat instance that received the line
 * @param line Pointer to the line bytes (delimiter stripped, NOT null-terminated)
 * @param len Length of @p line in bytes
 * @param user_data User data set in @ref modem_chat_config.user_data; must
 *                  point to a struct gnss_nmea0183_match_data (driver-private
 *                  context — typically the first member of the driver data
 *                  struct so the cast works through structural aliasing)
 */
void gnss_nmea0183_match_raw_tap(struct modem_chat *chat,
				 const char *line, size_t len, void *user_data);

/**
 * @brief Initialize a GNSS NMEA0183 match instance
 *
 * @param data GNSS NMEA0183 match instance to initialize
 * @param config Configuration to apply to GNSS NMEA0183 match instance
 */
int gnss_nmea0183_match_init(struct gnss_nmea0183_match_data *data,
			     const struct gnss_nmea0183_match_config *config);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_NMEA0183_MATCH_H_ */
