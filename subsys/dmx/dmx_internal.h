/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_DMX_INTERNAL_H_
#define ZEPHYR_SUBSYS_DMX_INTERNAL_H_

#include <string.h>

#include <zephyr/dmx/dmx.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/spsc_pbuf.h>

/**
 * @brief Per-frame RX assembly state.
 *
 * Tracks in-progress frame reception. The backend fills this from ISR context using the dmx_rx_*
 * helpers below.
 *
 * Slot data is always written at offset DMX_RX_MAX_HEADER in the allocated spsc_pbuf region. At
 * finalize time, the type-specific header is written and the frame is committed.
 */
struct dmx_rx_state {
	/** Pointer to the allocated spsc_pbuf region, or NULL when no allocation is active (frame
	 *  dropped/idle).
	 */
	char *buf;
	/** Pointer to slot data write area (buf + DMX_RX_MAX_HEADER). */
	uint8_t *data;
	/** Start code byte (valid when rx_pos > 0). */
	uint8_t start_code;
	/** Current byte position in the frame.
	 *  0 = start code not yet received, >= 1 = start code received.
	 */
	uint16_t rx_pos;
	/** Write index into data[]. */
	uint16_t data_idx;
	/** k_uptime_get_32() when the BREAK was detected. */
	uint32_t timestamp;
	/** Running checksum accumulator (32-bit to defer folding). */
	uint32_t cksum_acc;
};

/**
 * @brief Common DMX interface data shared by all backends.
 *
 * Backend data structures must embed this as their first member so that generic code can access
 * it via a cast of dev->data.
 */
struct dmx_data {
	/** SPSC packet buffer for received frames. */
	struct spsc_pbuf *rx_buf;
	/** Signalled when a new packet is committed to rx_buf. */
	struct k_sem rx_sem;
	/** Protects concurrent access to filter (ISR/thread). */
	struct k_spinlock filter_lock;
	/** Start code filter. Must be set before enabling. */
	struct dmx_filter filter;
	/** Current operating mode. */
	enum dmx_mode mode;
	/** True while the interface is enabled. */
	bool enabled;
	/** True once a mode has been configured. */
	bool mode_set;
	/** Protects concurrent access to stats (ISR/thread). */
	struct k_spinlock stats_lock;
	/** Receive statistics. */
	struct dmx_rx_stats stats;
};

/** Initialise common DMX data. Called from backend init. */
void dmx_data_init(struct dmx_data *data, void *buf, size_t buflen);

/**
 * @brief Begin assembling a new frame (called on BREAK detection).
 *
 * Allocates a slot in the SPSC buffer and prepares the rx_state for byte-by-byte accumulation.
 * Must be called from ISR context.
 *
 * If a previous frame was in progress (rx_state->buf != NULL) it is silently abandoned (the
 * allocation is not committed).
 */
void dmx_rx_begin_frame(struct dmx_data *data, struct dmx_rx_state *state);

/**
 * @brief Return the expected maximum slot count for a given start code.
 */
static inline uint16_t dmx_expected_max_slots(uint8_t start_code)
{
	switch (start_code) {
	case DMX_SC_SIP:
		return DMX_SIP_SLOTS;
	case DMX_SC_NULL:
	case DMX_SC_TEXT:
	case DMX_SC_TEST:
	case DMX_SC_UTF8:
	case DMX_SC_MANUFACTURER:
	default:
		return DMX_MAX_SLOTS;
	}
}

/**
 * @brief Append a received byte to the in-progress frame.
 *
 * The first byte is stored as the start code; subsequent bytes are written sequentially
 * into the data area. Filtering is deferred to finalize time. ISR context.
 *
 * @return true if the frame has received all expected slots and is ready to be committed (call
 *          dmx_rx_end_frame).
 */
static inline bool dmx_rx_byte(struct dmx_rx_state *state, uint8_t byte)
{
	if (state->buf == NULL) {
		return false;
	}

	/* Accumulate checksum over every received byte (SC + all slots). */
	state->cksum_acc += byte;

	if (state->rx_pos == 0) {
		/* Start code: stored separately, not in data[]. Set the data write pointer to the
		 * first wire-data field in the type-specific struct so that subsequent bytes land
		 * directly in the right place.
		 */
		state->start_code = byte;
		state->rx_pos = 1;

		switch (byte) {
		case DMX_SC_NULL:
			state->data = (uint8_t *)state->buf +
				offsetof(struct dmx_null_packet, data);
			break;
		case DMX_SC_TEXT:
			state->data = (uint8_t *)state->buf +
				offsetof(struct dmx_text_packet, page);
			break;
		case DMX_SC_TEST:
			state->data = (uint8_t *)state->buf +
				offsetof(struct dmx_test_packet, data);
			break;
		case DMX_SC_UTF8:
			state->data = (uint8_t *)state->buf +
				offsetof(struct dmx_utf8_packet, page);
			break;
		case DMX_SC_MANUFACTURER:
			state->data = (uint8_t *)state->buf +
				offsetof(struct dmx_manufacturer_packet, manufacturer_id);
			break;
		case DMX_SC_SIP:
			state->data = (uint8_t *)state->buf +
				offsetof(struct dmx_sip_packet, sip_length);
			break;
		default:
			state->data = (uint8_t *)state->buf + offsetof(struct dmx_raw_packet, data);
			break;
		}
		return false;
	}

	/* Write all bytes up to DMX_MAX_SLOTS into the wire data area. */
	if (state->data_idx < DMX_MAX_SLOTS) {
		state->data[state->data_idx] = byte;
	}
	state->data_idx++;
	state->rx_pos++;

	return (state->data_idx >= dmx_expected_max_slots(state->start_code));
}

/**
 * @brief Compute checksum over a buffer of bytes.
 *
 * Adds each byte to the running accumulator. Uses 4x loop unrolling for throughput.
 *
 * @param acc  Running accumulator from previous calls.
 * @param data Pointer to data bytes.
 * @param len  Number of bytes.
 * @return Updated accumulator (fold with dmx_cksum_fold() when done).
 */
static inline uint32_t dmx_cksum_buf(uint32_t acc, const uint8_t *data, size_t len)
{
	while (len >= 4) {
		acc += data[0] + data[1] + data[2] + data[3];
		data += 4;
		len -= 4;
	}
	while (len--) {
		acc += *data++;
	}
	return acc;
}

/**
 * @brief Account for a chunk of slot data already written in place by DMA.
 *
 * Unlike dmx_rx_byte(), this does NOT write data; it assumes the data is already at
 * state->data[state->data_idx]. It updates the checksum, counters, and checks for early-commit.
 *
 * @pre The start code must have been processed via dmx_rx_byte() first (state->rx_pos > 0
 *      and state->data is set).
 *
 * @param state Frame assembly state.
 * @param len   Number of bytes written by DMA.
 * @return true if the frame is ready to commit (call dmx_rx_end_frame).
 */
static inline bool dmx_rx_bytes(struct dmx_rx_state *state, size_t len)
{
	if (state->buf == NULL) {
		return false;
	}

	__ASSERT_NO_MSG(state->rx_pos > 0);

	uint16_t expected = dmx_expected_max_slots(state->start_code);
	size_t actual = MIN(len, expected - state->data_idx);

	state->cksum_acc = dmx_cksum_buf(state->cksum_acc, &state->data[state->data_idx], actual);
	state->data_idx += actual;
	state->rx_pos += actual;

	return (state->data_idx >= expected);
}

/**
 * @brief Fold a 32-bit accumulator into a 16-bit ones complement checksum.
 */
static inline uint16_t dmx_cksum_fold(uint32_t acc)
{
	while (acc > 0xFFFF) {
		acc = (acc & 0xFFFF) + (acc >> 16);
	}
	return (uint16_t)~acc;
}

/**
 * @brief Check whether a start code is accepted by the filter.
 */
static inline bool dmx_filter_sc_accepted(const struct dmx_filter *f, uint8_t sc)
{
	switch (sc) {
	case DMX_SC_NULL:         return f->sc_null;
	case DMX_SC_TEXT:         return f->sc_text;
	case DMX_SC_TEST:        return f->sc_test;
	case DMX_SC_UTF8:        return f->sc_utf8;
	case DMX_SC_MANUFACTURER: return f->sc_manufacturer;
	case DMX_SC_SIP:         return f->sc_sip;
	default:                 return f->sc_other;
	}
}

/**
 * @brief Finalize and commit a completed frame.
 *
 * Builds the type-specific packet struct, applies filtering for NULL SC, and commits the data to
 * the SPSC buffer so that dmx_rx_claim() can return it. Signals the RX semaphore.
 * ISR context.
 */
void dmx_rx_end_frame(struct dmx_data *data, struct dmx_rx_state *state);

#endif /* ZEPHYR_SUBSYS_DMX_INTERNAL_H_ */
