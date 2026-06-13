/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dmx, CONFIG_DMX_LOG_LEVEL);

#include <zephyr/sys/byteorder.h>
#include <zephyr/dmx/dmx.h>
#include "dmx_internal.h"

void dmx_data_init(struct dmx_data *data, void *buf, size_t buflen)
{
	data->rx_buf = spsc_pbuf_init(buf, buflen, 0);
	k_sem_init(&data->rx_sem, 0, K_SEM_MAX_LIMIT);
	data->mode_set = false;
	data->enabled = false;
}

/*
 * Compile-time safety checks: verify that every packet type's wire data starts at or before
 * DMX_RX_MAX_HEADER, and that the full allocation (DMX_RX_MAX_HEADER + DMX_MAX_SLOTS) is large
 * enough for every type.
 */
BUILD_ASSERT(offsetof(struct dmx_null_packet, data) <= DMX_RX_MAX_HEADER,
	     "null_packet data offset exceeds DMX_RX_MAX_HEADER");
BUILD_ASSERT(offsetof(struct dmx_test_packet, data) <= DMX_RX_MAX_HEADER,
	     "test_packet data offset exceeds DMX_RX_MAX_HEADER");
BUILD_ASSERT(offsetof(struct dmx_text_packet, page) <= DMX_RX_MAX_HEADER,
	     "text_packet wire data offset exceeds DMX_RX_MAX_HEADER");
BUILD_ASSERT(offsetof(struct dmx_utf8_packet, page) <= DMX_RX_MAX_HEADER,
	     "utf8_packet wire data offset exceeds DMX_RX_MAX_HEADER");
BUILD_ASSERT(offsetof(struct dmx_manufacturer_packet, manufacturer_id) <= DMX_RX_MAX_HEADER,
	     "manufacturer_packet wire data offset exceeds DMX_RX_MAX_HEADER");
BUILD_ASSERT(offsetof(struct dmx_sip_packet, sip_length) <= DMX_RX_MAX_HEADER,
	     "sip_packet wire data offset exceeds DMX_RX_MAX_HEADER");
BUILD_ASSERT(offsetof(struct dmx_raw_packet, data) <= DMX_RX_MAX_HEADER,
	     "raw_packet data offset exceeds DMX_RX_MAX_HEADER");

BUILD_ASSERT(sizeof(struct dmx_sip_packet) <= DMX_RX_MAX_HEADER + DMX_MAX_SLOTS,
	     "sip_packet exceeds max allocation size");

BUILD_ASSERT(DMX_SIP_SLOTS <= DMX_MAX_SLOTS, "SIP slot count exceeds DMX_MAX_SLOTS");

void dmx_rx_begin_frame(struct dmx_data *data, struct dmx_rx_state *state)
{
	uint16_t needed;
	char *buf;
	int alloc_len;

	/* Abandon any in-progress frame (don't commit). */
	state->buf = NULL;
	state->data = NULL;
	state->start_code = 0;
	state->rx_pos = 0;
	state->data_idx = 0;
	state->timestamp = k_uptime_get_32();
	state->cksum_acc = 0;

	/* Always allocate for a full universe. */
	needed = DMX_RX_MAX_HEADER + DMX_MAX_SLOTS;
	alloc_len = spsc_pbuf_alloc(data->rx_buf, needed, &buf);
	if (alloc_len < (int)needed) {
		k_spinlock_key_t key = k_spin_lock(&data->stats_lock);

		data->stats.buff_full_frames_dropped_count++;
		k_spin_unlock(&data->stats_lock, key);
		LOG_DBG("rx buffer full, dropped frame (total: %u)",
			data->stats.buff_full_frames_dropped_count);
		return;
	}

	state->buf = buf;
	/* state->data is set in dmx_rx_byte() after the start code is known, so that wire bytes
	 * land in the correct struct.
	 */
}

/**
 * @brief Compute test packet slot errors.
 */
static int16_t test_packet_count_errors(const uint8_t *data, uint16_t count)
{
	int16_t errors = 0;

	for (uint16_t i = 0; i < count; i++) {
		if (data[i] != DMX_SC_TEST) {
			errors++;
		}
	}
	return errors;
}

/**
 * @brief Build the final packet in the spsc_pbuf and return commit size.
 *
 * Wire data was written directly into the type-specific struct fields by the ISR (data pointer
 * set per start code in dmx_rx_byte). This function fills metadata fields and byte-swaps any big-
 * endian wire uint16_t fields to native order.
 */
static uint16_t end_frame_finalize(struct dmx_data *data, struct dmx_rx_state *state)
{
	uint8_t sc = state->start_code;
	uint16_t received = MIN(state->data_idx, (uint16_t)DMX_MAX_SLOTS);
	struct dmx_rx_header *hdr = (struct dmx_rx_header *)state->buf;

	/* Common header fields. */
	hdr->timestamp = state->timestamp;
	hdr->checksum = dmx_cksum_fold(state->cksum_acc);
	hdr->start_code = sc;

	switch (sc) {
	case DMX_SC_NULL: {
		struct dmx_null_packet *pkt = (struct dmx_null_packet *)state->buf;

		pkt->slot_count = received;
		return DMX_NULL_PACKET_SIZE(received);
	}
	case DMX_SC_TEXT: {
		struct dmx_text_packet *pkt = (struct dmx_text_packet *)state->buf;

		if (received < DMX_TEXT_MIN_SLOTS) {
			return 0;
		}
		pkt->slot_count = received;
		/* text[] starts at slot 3; slots 1-2 are page + cpl. */
		return DMX_TEXT_PACKET_SIZE(received - (DMX_TEXT_MIN_SLOTS - 1));
	}
	case DMX_SC_TEST: {
		struct dmx_test_packet *pkt = (struct dmx_test_packet *)state->buf;
		int16_t errors;
		bool pass;

		pkt->slot_count = received;
		errors = test_packet_count_errors(state->data, received);
		pkt->slot_errors = errors;
		pass = (errors == 0 && received == DMX_MAX_SLOTS);
		pkt->pass = pass;
		if (!pass) {
			k_spinlock_key_t key = k_spin_lock(&data->stats_lock);

			data->stats.test_packets_with_errors_count++;
			k_spin_unlock(&data->stats_lock, key);
		}
		return DMX_TEST_PACKET_SIZE(received);
	}
	case DMX_SC_UTF8: {
		struct dmx_utf8_packet *pkt = (struct dmx_utf8_packet *)state->buf;

		if (received < DMX_UTF8_MIN_SLOTS) {
			return 0;
		}
		pkt->slot_count = received;
		/* text[] starts at slot 3; slots 1-2 are page + cpl. */
		return DMX_UTF8_PACKET_SIZE(received - (DMX_UTF8_MIN_SLOTS - 1));
	}
	case DMX_SC_MANUFACTURER: {
		struct dmx_manufacturer_packet *pkt = (struct dmx_manufacturer_packet *)state->buf;

		if (received < DMX_MANUFACTURER_MIN_SLOTS) {
			return 0;
		}
		pkt->slot_count = received;
		pkt->manufacturer_id = sys_be16_to_cpu(pkt->manufacturer_id);
		/* data[] starts at slot 3; slots 1-2 are manufacturer ID. */
		return DMX_MANUFACTURER_PACKET_SIZE(MAX(received - DMX_MANUFACTURER_MIN_SLOTS, 0));
	}
	case DMX_SC_SIP: {
		struct dmx_sip_packet *pkt = (struct dmx_sip_packet *)state->buf;
		uint8_t sum = DMX_SC_SIP;

		if (received < DMX_SIP_SLOTS) {
			return 0;
		}

		/* Only support the current SIP format (24 slots). Future spec revisions may extend
		 * this; drop unsupported lengths.
		 */
		if (pkt->sip_length != DMX_SIP_SLOTS) {
			return 0;
		}

		/* Validate SIP checksum BEFORE byte-swapping. 8-bit ones complement additive over
		 * raw wire bytes (SC + all 24 slots), Annex D5.14.
		 */
		for (uint16_t i = 0; i < DMX_SIP_SLOTS; i++) {
			sum += state->data[i];
		}
		pkt->checksum_valid = (sum == 0xFF);

		/* Byte-swap multi-byte fields to native order. */
		pkt->prev_checksum = sys_be16_to_cpu(pkt->prev_checksum);
		pkt->std_packet_len = sys_be16_to_cpu(pkt->std_packet_len);
		pkt->num_packets = sys_be16_to_cpu(pkt->num_packets);
		for (int i = 0; i < 5; i++) {
			pkt->manufacturer_ids[i] = sys_be16_to_cpu(pkt->manufacturer_ids[i]);
		}
		return DMX_SIP_PACKET_SIZE;
	}
	default: {
		struct dmx_raw_packet *pkt = (struct dmx_raw_packet *)state->buf;

		pkt->slot_count = received;
		return DMX_RAW_PACKET_SIZE(received);
	}
	}
}

void dmx_rx_end_frame(struct dmx_data *data, struct dmx_rx_state *state)
{
	uint16_t commit_size;

	if (state->buf == NULL) {
		return;
	}

	/* Need at least the start code to consider the frame valid. */
	if (state->rx_pos == 0) {
		state->buf = NULL;
		return;
	}

	/* Check start code against the filter. */
	{
		k_spinlock_key_t key = k_spin_lock(&data->filter_lock);
		bool accepted = dmx_filter_sc_accepted(&data->filter, state->start_code);

		k_spin_unlock(&data->filter_lock, key);
		if (!accepted) {
			state->buf = NULL;
			state->data = NULL;
			return;
		}
	}

	commit_size = end_frame_finalize(data, state);
	if (commit_size == 0) {
		/* Frame too short or was invalid for this start code type. Discard. */
		state->buf = NULL;
		state->data = NULL;
		return;
	}
	spsc_pbuf_commit(data->rx_buf, commit_size);
	k_sem_give(&data->rx_sem);

	state->buf = NULL;
	state->data = NULL;
}

/* ---- Public generic API (not dispatched through driver_api) ------------- */

int dmx_set_filter(const struct device *dev, const struct dmx_filter *filter)
{
	struct dmx_data *data = dev->data;
	k_spinlock_key_t key;

	if (filter == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->filter_lock);
	data->filter = *filter;
	k_spin_unlock(&data->filter_lock, key);

	return 0;
}

/* ---- Internal claim helper ---------------------------------------------- */

/**
 * @brief Get the total stored size of a claimed packet.
 *
 * Each packet type stores its size differently, so we dispatch on start_code to compute the
 * correct size for spsc_pbuf_free().
 */
static uint16_t packet_stored_size(const struct dmx_rx_header *hdr)
{
	switch (hdr->start_code) {
	case DMX_SC_NULL: {
		const struct dmx_null_packet *pkt = (const struct dmx_null_packet *)hdr;

		return DMX_NULL_PACKET_SIZE(pkt->slot_count);
	}
	case DMX_SC_TEXT: {
		const struct dmx_text_packet *pkt = (const struct dmx_text_packet *)hdr;

		return DMX_TEXT_PACKET_SIZE(pkt->slot_count - (DMX_TEXT_MIN_SLOTS - 1));
	}
	case DMX_SC_TEST: {
		const struct dmx_test_packet *pkt = (const struct dmx_test_packet *)hdr;

		return DMX_TEST_PACKET_SIZE(pkt->slot_count);
	}
	case DMX_SC_UTF8: {
		const struct dmx_utf8_packet *pkt = (const struct dmx_utf8_packet *)hdr;

		return DMX_UTF8_PACKET_SIZE(pkt->slot_count - (DMX_UTF8_MIN_SLOTS - 1));
	}
	case DMX_SC_MANUFACTURER: {
		const struct dmx_manufacturer_packet *pkt = (
			const struct dmx_manufacturer_packet *)hdr;

		return DMX_MANUFACTURER_PACKET_SIZE(
			MAX(pkt->slot_count - DMX_MANUFACTURER_MIN_SLOTS, 0));
	}
	case DMX_SC_SIP:
		return DMX_SIP_PACKET_SIZE;
	default: {
		const struct dmx_raw_packet *pkt = (const struct dmx_raw_packet *)hdr;

		return DMX_RAW_PACKET_SIZE(pkt->slot_count);
	}
	}
}

/* ---- Claim/free --------------------------------------------------------- */

int dmx_rx_claim(const struct device *dev, const struct dmx_rx_header **hdr, k_timeout_t timeout)
{
	struct dmx_data *data = dev->data;
	char *buf;
	uint16_t len;

	if (hdr == NULL) {
		return -EINVAL;
	}

	if (k_sem_take(&data->rx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	len = spsc_pbuf_claim(data->rx_buf, &buf);
	if (len == 0) {
		return -EAGAIN;
	}

	*hdr = (const struct dmx_rx_header *)buf;
	return 0;
}

/* ---- Free --------------------------------------------------------------- */

void dmx_rx_free(const struct device *dev, const struct dmx_rx_header *hdr)
{
	struct dmx_data *data = dev->data;

	spsc_pbuf_free(data->rx_buf, packet_stored_size(hdr));
}

/* ---- Signal status ------------------------------------------------------ */

bool dmx_rx_is_receiving(const struct device *dev)
{
	const struct dmx_driver_api *api = (const struct dmx_driver_api *)dev->api;

	return api->is_receiving(dev);
}

/* ---- Stats -------------------------------------------------------------- */

void dmx_rx_stats_get(const struct device *dev, struct dmx_rx_stats *stats)
{
	struct dmx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->stats_lock);

	*stats = data->stats;
	k_spin_unlock(&data->stats_lock, key);
}

void dmx_rx_stats_reset(const struct device *dev)
{
	struct dmx_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->stats_lock);

	memset(&data->stats, 0, sizeof(data->stats));
	k_spin_unlock(&data->stats_lock, key);
}
