/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for DMX512-A protocol.
 * @defgroup dmx DMX512
 * @ingroup connectivity
 * @{
 */

#ifndef ZEPHYR_INCLUDE_DMX_DMX_H_
#define ZEPHYR_INCLUDE_DMX_DMX_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/dmx/dmx.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of data slots in a DMX512 packet. */
#define DMX_MAX_SLOTS 512

/** Minimum data slots for a text packet (ANSI E1.11 Annex D2). */
#define DMX_TEXT_MIN_SLOTS 3

/** Required data slots for a test packet (ANSI E1.11 Annex D3). */
#define DMX_TEST_SLOTS 512

/** Minimum data slots for a UTF-8 text packet (ANSI E1.11 Annex D4). */
#define DMX_UTF8_MIN_SLOTS 3

/** Minimum data slots for a manufacturer packet (ANSI E1.11 Annex D1). */
#define DMX_MANUFACTURER_MIN_SLOTS 2

/** Required data slots for a SIP packet (ANSI E1.11 Annex D5.2). */
#define DMX_SIP_SLOTS 24

/** @defgroup dmx_start_codes DMX512 Start Codes
 * @{
 */

/** NULL START code - dimmer/untyped data (ANSI E1.11 clause 8.5.1). */
#define DMX_SC_NULL 0x00

/** ASCII text packet (ANSI E1.11 Annex D2). */
#define DMX_SC_TEXT 0x17

/** Test packet - 512 slots of 0x55 (ANSI E1.11 Annex D3). */
#define DMX_SC_TEST 0x55

/** UTF-8 text packet (ANSI E1.11 Annex D4). */
#define DMX_SC_UTF8 0x90

/** Manufacturer/organisation specific (ANSI E1.11 Annex D1, Table D1). */
#define DMX_SC_MANUFACTURER 0x91

/** System Information Packet (ANSI E1.11 Annex D5). */
#define DMX_SC_SIP 0xCF

/** @} */

/** Operating mode of a DMX interface. */
enum dmx_mode {
	/** Receive DMX data. */
	DMX_MODE_INPUT,
	/** Transmit DMX data. */
	DMX_MODE_OUTPUT,
};

/**
 * @brief DMX receive filter.
 *
 * Controls which start code types are delivered to the application. All 512 data slots are always
 * received; the application reads only the slots it needs. Must be configured before enabling the
 * interface.
 */
struct dmx_filter {
	/** Accept NULL START code packets (0x00). */
	bool sc_null;
	/** Accept ASCII text packets (0x17). */
	bool sc_text;
	/** Accept test packets (0x55). */
	bool sc_test;
	/** Accept UTF-8 text packets (0x90). */
	bool sc_utf8;
	/** Accept manufacturer-specific packets (0x91). */
	bool sc_manufacturer;
	/** Accept System Information Packets (0xCF). */
	bool sc_sip;
	/** Accept all other start codes not listed above (as raw packets). */
	bool sc_other;
};

/* ---- Packet structures -------------------------------------------------- */

/**
 * @brief Common header for all received DMX packets.
 *
 * Every packet in the receive buffer starts with this minimal header. Filled during phase 1 (ISR
 * reads break + start code).
 */
struct dmx_rx_header {
	/** Uptime in milliseconds when the BREAK was detected. */
	uint32_t timestamp;
	/**
	 * 16-bit ones complement additive checksum of all received bytes (start code + slot data)
	 * per ANSI E1.11 Annex D5.5. Usually computed incrementally during reception.
	 */
	uint16_t checksum;
	/** DMX start code of the frame. */
	uint8_t start_code;
};

/**
 * @brief NULL START code packet (dimmer/untyped data).
 *
 * Contains up to 512 data slots. Short frames (fewer than 512 slots) are allowed per ANSI E1.11
 * clause 8.7; slot_count indicates how many were actually received. data[0] is always DMX slot 1.
 */
struct dmx_null_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** Number of data slots received (1-512). */
	uint16_t slot_count;
	/** Slot data (slot_count bytes, data[0] = DMX slot 1). */
	uint8_t data[];
};

/** Calculate the byte size of a dmx_null_packet with @p n data slots. */
#define DMX_NULL_PACKET_SIZE(n) (offsetof(struct dmx_null_packet, data) + (n))

/**
 * @brief Test packet (SC = 0x55).
 *
 * Per ANSI E1.11 Annex D3, a valid test packet has 512 data slots all carrying the value 0x55.
 * The slot_errors field is computed at receive time.
 */
struct dmx_test_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** Number of data slots received. */
	uint16_t slot_count;
	/** Number of incorrect slot values (0 = pass). */
	int16_t slot_errors;
	/** Whether there are no slot errors and the packet is the correct length */
	bool pass;
	/** Raw slot data (slot_count bytes). */
	uint8_t data[];
};

/** Calculate the byte size of a dmx_test_packet with @p n data slots. */
#define DMX_TEST_PACKET_SIZE(n) (offsetof(struct dmx_test_packet, data) + (n))

/**
 * @brief ASCII text packet (SC = 0x17).
 *
 * Per ANSI E1.11 Annex D2: slot 1 = page number, slot 2 = characters per line (0 = ignore), slots
 * 3+ = ASCII text (null-terminated).
 */
struct dmx_text_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** Number of data slots received (1-512). */
	uint16_t slot_count;
	/** Page number (slot 1). */
	uint8_t page;
	/** Characters per line (slot 2, 0 = ignore). */
	uint8_t chars_per_line;
	/** Text data (null-terminated ASCII, slots 3+). */
	uint8_t text[];
};

/** Calculate the byte size of a dmx_text_packet with @p n text bytes. */
#define DMX_TEXT_PACKET_SIZE(n) (offsetof(struct dmx_text_packet, text) + (n))

/**
 * @brief UTF-8 text packet (SC = 0x90).
 *
 * Per ANSI E1.11 Annex D4: same layout as ASCII text but encoded as UTF-8 per Unicode 5.0.
 */
struct dmx_utf8_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** Number of data slots received (1-512). */
	uint16_t slot_count;
	/** Page number (slot 1). */
	uint8_t page;
	/** Characters per line (slot 2, 0 = ignore). */
	uint8_t chars_per_line;
	/** Text data (null-terminated UTF-8, slots 3+). */
	uint8_t text[];
};

/** Calculate the byte size of a dmx_utf8_packet with @p n text bytes. */
#define DMX_UTF8_PACKET_SIZE(n) (offsetof(struct dmx_utf8_packet, text) + (n))

/**
 * @brief Manufacturer-specific packet (SC = 0x91).
 *
 * Per ANSI E1.11 Annex D1: slots 1-2 = manufacturer ID (MSB, LSB), remaining slots are
 * manufacturer-defined.
 */
struct dmx_manufacturer_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** Number of data slots received (1-512). */
	uint16_t slot_count;
	/** Manufacturer ID (slots 1-2). */
	uint16_t manufacturer_id;
	/** Manufacturer-defined data (slots 3+). */
	uint8_t data[];
};

/** Calculate the byte size of a dmx_manufacturer_packet with @p n data bytes. */
#define DMX_MANUFACTURER_PACKET_SIZE(n) (offsetof(struct dmx_manufacturer_packet, data) + (n))

/**
 * @brief System Information Packet (SC = 0xCF).
 *
 * Per ANSI E1.11 Annex D5. Contains checksum data relating to the previous NULL SC packet and
 * other control information.
 *
 * @note Cross-packet checksum verification against the previous NULL SC packet is not yet
 *       implemented.
 */
struct dmx_sip_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** SIP checksum valid flag (computed at receive time). */
	bool checksum_valid;
	/** SIP byte count/checksum pointer (slot 1, currently 24). */
	uint8_t sip_length;
	/** Control bit field (slot 2). */
	uint8_t control;
	/** 16-bit checksum of previous packet (slots 3-4). */
	uint16_t prev_checksum;
	/** SIP sequence number (slot 5). */
	uint8_t sequence;
	/** DMX512 universe number (slot 6). */
	uint8_t universe;
	/** DMX512 processing level (slot 7). */
	uint8_t processing_level;
	/** Software version (slot 8). */
	uint8_t sw_version;
	/** Standard packet length (slots 9-10). */
	uint16_t std_packet_len;
	/** Packets since last SIP (slots 11-12). */
	uint16_t num_packets;
	/** Manufacturer ID history (slots 13-22, 5 entries). */
	uint16_t manufacturer_ids[5];
	/** Reserved (slot 23, transmit as 0). */
	uint8_t _reserved_slot23;
	/** 8-bit additive checksum of the SIP (slot 24, Annex D5.14). */
	uint8_t sip_checksum;
};

/** Calculate the byte size of a dmx_sip_packet (fixed size, no FAM). */
#define DMX_SIP_PACKET_SIZE (sizeof(struct dmx_sip_packet))

/**
 * @brief Raw/generic packet for unrecognised start codes.
 *
 * Used for start codes that don't have a dedicated struct.
 */
struct dmx_raw_packet {
	/** Common packet header. */
	struct dmx_rx_header hdr;
	/** Number of data slots received. */
	uint16_t slot_count;
	/** Raw slot data (slot_count bytes). */
	uint8_t data[];
};

/** Calculate the byte size of a dmx_raw_packet with @p n data slots. */
#define DMX_RAW_PACKET_SIZE(n) (offsetof(struct dmx_raw_packet, data) + (n))

/**
 * @brief Maximum packet size before wire data.
 *
 * Used internally to reserve space in the receive buffer. The ISR sets its data pointer per start
 * code so that wire bytes land directly in the correct struct fields. This macro gives the
 * largest possible offset to wire data across all packet types, ensuring the allocation is big
 * enough for any type.
 */
#define DMX_RX_MAX_HEADER                                                      \
	MAX(offsetof(struct dmx_null_packet, data),                             \
	MAX(offsetof(struct dmx_text_packet, page),                             \
	MAX(offsetof(struct dmx_test_packet, data),                             \
	MAX(offsetof(struct dmx_utf8_packet, page),                             \
	MAX(offsetof(struct dmx_manufacturer_packet, manufacturer_id),          \
	MAX(offsetof(struct dmx_sip_packet, sip_length),                        \
	    offsetof(struct dmx_raw_packet, data)))))))

/* ---- Driver API --------------------------------------------------------- */

/**
 * @brief DMX backend driver API.
 *
 * Backends (UART, PIO, etc.) implement these operations. Generic functions like dmx_rx_claim()
 * and dmx_rx_free() operate on the common data structures and do not require backend involvement.
 */
__subsystem struct dmx_driver_api {
	/** Set the operating mode (input/output). */
	int (*set_mode)(const struct device *dev, enum dmx_mode mode);
	/** Enable the interface. */
	int (*enable)(const struct device *dev);
	/** Disable the interface. */
	int (*disable)(const struct device *dev);
	/** Check if DMX signal is currently being received. */
	bool (*is_receiving)(const struct device *dev);
};

/**
 * @brief Set the DMX interface operating mode.
 *
 * Must be called before dmx_enable(). Changing the mode while the interface is enabled returns
 * @c -EBUSY.
 *
 * @param dev DMX device.
 * @param mode Operating mode (input or output).
 * @retval 0 on success.
 * @retval -EBUSY if the interface is currently enabled.
 * @retval -ENOTSUP if the requested mode is not implemented.
 */
static inline int dmx_set_mode(const struct device *dev, enum dmx_mode mode)
{
	const struct dmx_driver_api *api = (const struct dmx_driver_api *)dev->api;

	return api->set_mode(dev, mode);
}

/**
 * @brief Enable the DMX interface.
 *
 * Starts reception or transmission depending on the configured mode. A mode must be set with
 * dmx_set_mode() before enabling.
 *
 * @param dev DMX device.
 * @retval 0 on success.
 * @retval -EALREADY if already enabled.
 * @retval -EINVAL if no mode was set.
 */
static inline int dmx_enable(const struct device *dev)
{
	const struct dmx_driver_api *api = (const struct dmx_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 * @brief Disable the DMX interface.
 *
 * Stops reception or transmission.
 *
 * @param dev DMX device.
 * @retval 0 on success.
 */
static inline int dmx_disable(const struct device *dev)
{
	const struct dmx_driver_api *api = (const struct dmx_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Set the receive filter for DMX input.
 *
 * Configures which start codes are accepted. Must be called before dmx_enable(). May also be
 * called while enabled; changes take effect on the next frame.
 *
 * @param dev    DMX device.
 * @param filter Filter to apply.
 * @retval 0 on success.
 * @retval -EINVAL if filter is NULL.
 */
int dmx_set_filter(const struct device *dev, const struct dmx_filter *filter);

/* ---- Claim/free --------------------------------------------------------- */

/**
 * @brief Claim the next received packet header (zero-copy).
 *
 * Returns a pointer to the common header of the oldest unread packet. Inspect hdr->start_code to
 * determine the packet type, then cast to the appropriate struct.
 *
 * The packet remains valid until released with dmx_rx_free().
 *
 * @param[in]  dev     DMX device.
 * @param[out] hdr     Set to point at the packet header on success.
 * @param[in]  timeout How long to wait for a packet.
 * @retval 0 on success.
 * @retval -EAGAIN if no packet is available within the timeout.
 * @retval -EINVAL if @p hdr is NULL.
 */
int dmx_rx_claim(const struct device *dev, const struct dmx_rx_header **hdr, k_timeout_t timeout);

/**
 * @brief Free a previously claimed DMX packet.
 *
 * Accepts a pointer to any packet type (header, null, test, or raw). Releases the buffer space so
 * it can be reused for future frames.
 *
 * @param dev DMX device.
 * @param hdr Packet header previously obtained via any claim function.
 */
void dmx_rx_free(const struct device *dev, const struct dmx_rx_header *hdr);

/* ---- Signal status ------------------------------------------------------ */

/**
 * @brief Check whether a valid DMX signal is being received.
 *
 * Returns true after sync locks (two consecutive breaks with valid timing) and false when the
 * signal is lost (no break within 1.25 s).
 *
 * @param dev DMX device.
 * @return true if receiving, false if idle or signal lost.
 */
bool dmx_rx_is_receiving(const struct device *dev);

/* ---- Statistics --------------------------------------------------------- */

/**
 * @brief DMX receive statistics.
 */
struct dmx_rx_stats {
	/** Frames dropped because the receive buffer was full. */
	uint32_t buff_full_frames_dropped_count;
	/** Frames discarded due to framing errors during reception. */
	uint32_t rx_frame_errors;
	/** Test packets received with one or more slot errors. */
	uint32_t test_packets_with_errors_count;
};

/**
 * @brief Get receive statistics.
 *
 * @param dev   DMX device.
 * @param stats Destination for the current statistics snapshot.
 */
void dmx_rx_stats_get(const struct device *dev, struct dmx_rx_stats *stats);

/**
 * @brief Reset all receive statistics to zero.
 *
 * @param dev DMX device.
 */
void dmx_rx_stats_reset(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DMX_DMX_H_ */
