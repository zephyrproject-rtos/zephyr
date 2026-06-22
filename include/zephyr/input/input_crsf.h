/*
 * Copyright (c) 2026 CogniPilot Foundation
 * Copyright (c) 2026 NXP Semiconductors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crossfire (CRSF) Protocol Definitions and Helpers
 *
 * This module provides constants, packet type definitions, telemetry payload
 * structures, and helper APIs for working with the CRSF (Crossfire) protocol
 * used by RC transmitters and receivers.
 *
 * It covers frame layout, telemetry payload formats, RC channel conversions,
 * and link statistics access.
 * @ingroup input_interface_ext
 * @{
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INPUT_INPUT_CRSF_H_
#define ZEPHYR_INCLUDE_DRIVERS_INPUT_INPUT_CRSF_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRSF packet (frame) types.
 */
enum crsf_packet_type {
	/** GPS telemetry data */
	CRSF_TYPE_GPS = 0x02,

	/** Battery telemetry data */
	CRSF_TYPE_BATTERY = 0x08,

	/** Transmitter link identifier */
	CRSF_TYPE_LINK_TX_ID = 0x0A,

	/** Link statistics */
	CRSF_TYPE_LINK_STATS = 0x14,

	/** RC channel data */
	CRSF_TYPE_RC_CHANNELS = 0x16,

	/** Attitude telemetry */
	CRSF_TYPE_ATTITUDE = 0x1E,

	/** Flight mode string */
	CRSF_TYPE_FLIGHT_MODE = 0x21,

	/** Device discovery / ping */
	CRSF_TYPE_PING_DEVICES = 0x28,

	/** Device information */
	CRSF_TYPE_DEVICE_INFO = 0x29,

	/** Parameter read/write */
	CRSF_TYPE_PARAMETER_SETTINGS = 0x2B,

	/** Command frame */
	CRSF_TYPE_COMMAND = 0x32,

	/** Radio identifier */
	CRSF_TYPE_RADIO_ID = 0x3A,

	/** ArduPilot custom telemetry */
	CRSF_TYPE_AP_CUSTOM_TELEM = 0x80
};

/**
 * @brief GPS telemetry payload (Type 0x02).
 *
 * Multi-byte values are transmitted in big-endian format.
 */
struct crsf_payload_gps {
	/** Latitude in degrees × 10,000,000 */
	int32_t lat;

	/** Longitude in degrees × 10,000,000 */
	int32_t lon;

	/** Ground speed in km/h × 10 */
	uint16_t speed;

	/** Heading in degrees × 100 */
	uint16_t heading;

	/** Altitude in meters with +1000 m offset */
	uint16_t altitude;

	/** Number of visible satellites */
	uint8_t satellites;
} __packed;

/**
 * @brief Battery telemetry payload (Type 0x08).
 */
struct crsf_payload_battery {
	/** Battery voltage in volts × 10 */
	uint16_t voltage_dV;

	/** Battery current in amps × 10 */
	uint16_t current_dA;

	/** Consumed capacity in milliamp-hours (24-bit, big-endian) */
	uint8_t capacity_mah[3];

	/** Remaining battery percentage (0–100) */
	uint8_t remaining_pct;
} __packed;

/**
 * @brief Link statistics payload (Type 0x14).
 */
struct crsf_link_stats {
	/** Uplink RSSI antenna 1 (dBm × −1) */
	uint8_t uplink_rssi_1;

	/** Uplink RSSI antenna 2 (dBm × −1) */
	uint8_t uplink_rssi_2;

	/** Uplink link quality (0–100 %) */
	uint8_t uplink_link_quality;

	/** Uplink signal-to-noise ratio (dB) */
	int8_t uplink_snr;

	/** Active antenna index */
	uint8_t active_antenna;

	/** RF mode (e.g. 4 Hz, 50 Hz, 150 Hz) */
	uint8_t rf_mode;

	/** Transmit power level (protocol-defined enum) */
	uint8_t tx_power;

	/** Downlink RSSI (dBm × −1) */
	uint8_t downlink_rssi;

	/** Downlink link quality (0–100 %) */
	uint8_t downlink_link_quality;

	/** Downlink signal-to-noise ratio (dB) */
	int8_t downlink_snr;
} __packed;

/**
 * @name RC Channel Conversion Helpers
 * @brief Conversion between CRSF ticks and microseconds.
 * @{
 */

/**
 * @brief Convert CRSF channel ticks to microseconds.
 *
 * @param x CRSF channel value in ticks
 * @return Pulse width in microseconds
 */
#define CRSF_TICKS_TO_US(x) (((x) - 992) * 5 / 8 + 1500)

/**
 * @brief Convert microseconds to CRSF channel ticks.
 *
 * @param x Pulse width in microseconds
 * @return CRSF channel value in ticks
 */
#define CRSF_US_TO_TICKS(x) (((x) - 1500) * 8 / 5 + 992)

/** @} */

/**
 * @brief Attitude telemetry payload (Type 0x1E).
 */
struct crsf_payload_attitude {
	/** Pitch angle in radians × 10,000 */
	int16_t pitch_rad;

	/** Roll angle in radians × 10,000 */
	int16_t roll_rad;

	/** Yaw angle in radians × 10,000 */
	int16_t yaw_rad;
} __packed;

/**
 * @brief Send a generic CRSF telemetry frame.
 *
 * @param dev Pointer to the CRSF input device
 * @param type Packet type (see @ref crsf_packet_type)
 * @param payload Pointer to payload buffer
 * @param payload_len Payload length in bytes
 *
 * @return 0 on success, negative errno on failure
 */
int input_crsf_send_telemetry(const struct device *dev, uint8_t type, uint8_t *payload,
			      size_t payload_len);

/**
 * @brief Retrieve the latest CRSF link statistics.
 *
 * @param dev Pointer to the CRSF input device
 *
 * @return Current link statistics structure
 */
struct crsf_link_stats input_crsf_get_link_stats(const struct device *dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INPUT_INPUT_CRSF_H_ */
