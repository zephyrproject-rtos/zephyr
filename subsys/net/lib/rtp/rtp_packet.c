/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file rtp_packet.c
 *
 * @brief Internal functions to serialize and deserialize RTP packets.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtp_packet, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/net/net_log.h>
#include <zephyr/net/rtp.h>
#include <zephyr/sys/byteorder.h>

#include "rtp_packet.h"

int rtp_packet_deserialize(struct rtp_packet *packet, uint8_t *data, size_t len)
{
	uint8_t *cursor = data;
	uint8_t *end = cursor + len;

	if (len < RTP_MIN_HEADER_LEN) {
		NET_DBG("RTP packet too small (%zu)", len);
		return -EINVAL;
	}

	packet->header.vpxcc = *cursor;
	cursor += sizeof(uint8_t);
	packet->header.mpt = *cursor;
	cursor += sizeof(uint8_t);

	packet->header.seq = sys_get_be16(cursor);
	cursor += sizeof(uint16_t);

	packet->header.ts = sys_get_be32(cursor);
	cursor += sizeof(uint32_t);

	packet->header.ssrc = sys_get_be32(cursor);
	cursor += sizeof(uint32_t);

	if (rtp_header_get_v(&packet->header) != RTP_VERSION) {
		NET_DBG("Invalid RTP version (%d)", rtp_header_get_v(&packet->header));
		return -EINVAL;
	}

	if (rtp_header_get_cc(&packet->header) > 0) {
		size_t csrc_count = rtp_header_get_cc(&packet->header);
		size_t csrc_skip = 0;

		if (end - cursor < (ptrdiff_t)(csrc_count * sizeof(uint32_t))) {
			NET_DBG("Data too small for cc");
			return -EINVAL;
		}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
		if (csrc_count > ARRAY_SIZE(packet->header.csrc)) {
			NET_DBG("Size of csrc too small, ignoring following csrcs. Please increase "
				"CONFIG_RTP_MAX_CSRC_COUNT");
			csrc_skip = csrc_count - ARRAY_SIZE(packet->header.csrc);
			csrc_count = ARRAY_SIZE(packet->header.csrc);
			rtp_header_set_cc(&packet->header, csrc_count);
		}

		for (size_t i = 0; i < csrc_count; i++) {
			packet->header.csrc[i] = sys_get_be32(cursor);
			cursor += sizeof(uint32_t);
		}
#else
		NET_DBG("Received packet with cc > 0, but CONFIG_RTP_MAX_CSRC_COUNT = 0");

		csrc_skip = csrc_count;
		rtp_header_set_cc(&packet->header, 0);
#endif /* CONFIG_RTP_MAX_CSRC_COUNT > 0 */

		cursor += csrc_skip * sizeof(uint32_t);
	}

	if (rtp_header_get_x(&packet->header) == 1) {
		struct rtp_header_extension *hdr_x = &packet->header.header_extension;
		size_t x_data_len;

		if (end - cursor < (ptrdiff_t)(2 * sizeof(uint16_t))) {
			NET_DBG("Data too small for header extension");
			return -EINVAL;
		}

		hdr_x->definition = sys_get_be16(cursor);
		cursor += sizeof(uint16_t);

		hdr_x->length = sys_get_be16(cursor);
		cursor += sizeof(uint16_t);
		x_data_len = hdr_x->length * sizeof(uint32_t);

		if (end - cursor < (ptrdiff_t)x_data_len) {
			NET_DBG("RTP extension header length too large for pkt");
			return -EINVAL;
		}

		hdr_x->data = cursor;
		cursor += x_data_len;
	}

	packet->payload_len = end - cursor;
	packet->payload = cursor;

	if (rtp_header_get_p(&packet->header) == 1) {
		uint8_t padding;

		if (packet->payload_len == 0) {
			NET_DBG("Padding flag set but no payload");
			return -EINVAL;
		}

		padding = *(end - 1);

		if (padding == 0) {
			NET_DBG("Padding flag is set but padding is 0");
			return -EINVAL;
		}

		if (padding > packet->payload_len) {
			NET_DBG("Padding larger than payload");
			return -EINVAL;
		}

		packet->payload_len -= padding;
	}

	return 0;
}

int rtp_packet_serialize(const struct rtp_packet *packet, uint8_t padding, uint8_t *buf,
			 size_t buf_size)
{
	const uint8_t *end = buf + buf_size;
	uint8_t *cursor = buf;

	if (buf_size < RTP_MIN_HEADER_LEN) {
		NET_DBG("Buffer too small for RTP header");
		return -EINVAL;
	}

	*cursor = packet->header.vpxcc;
	cursor += sizeof(uint8_t);
	*cursor = packet->header.mpt;
	cursor += sizeof(uint8_t);

	sys_put_be16(packet->header.seq, cursor);
	cursor += sizeof(uint16_t);

	sys_put_be32(packet->header.ts, cursor);
	cursor += sizeof(uint32_t);

	sys_put_be32(packet->header.ssrc, cursor);
	cursor += sizeof(uint32_t);

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	if (rtp_header_get_cc(&packet->header) > CONFIG_RTP_MAX_CSRC_COUNT) {
		NET_DBG("CSRC count %u exceeds maximum %d", rtp_header_get_cc(&packet->header),
			CONFIG_RTP_MAX_CSRC_COUNT);
		return -EINVAL;
	}

	if (end - cursor <
	    (ptrdiff_t)((size_t)rtp_header_get_cc(&packet->header) * sizeof(uint32_t))) {
		NET_DBG("Not enough buffer space for CSRC list");
		return -EINVAL;
	}

	for (size_t i = 0; i < rtp_header_get_cc(&packet->header); i++) {
		sys_put_be32(packet->header.csrc[i], cursor);
		cursor += sizeof(uint32_t);
	}
#endif

	if (rtp_header_get_x(&packet->header) == 1) {
		const struct rtp_header_extension *hdr_x = &packet->header.header_extension;
		size_t x_data_len = (size_t)hdr_x->length * sizeof(uint32_t);

		if (hdr_x->length > 0 && hdr_x->data == NULL) {
			NET_DBG("Extension header data pointer is NULL");
			return -EINVAL;
		}

		if (end - cursor < (ptrdiff_t)(2 * sizeof(uint16_t) + x_data_len)) {
			NET_DBG("Extension header length too large");
			return -EINVAL;
		}

		sys_put_be16(hdr_x->definition, cursor);
		cursor += sizeof(uint16_t);

		sys_put_be16(hdr_x->length, cursor);
		cursor += sizeof(uint16_t);

		memcpy(cursor, hdr_x->data, x_data_len);
		cursor += x_data_len;
	}

	if (packet->payload_len > 0) {
		if (packet->payload == NULL) {
			NET_DBG("Payload pointer is NULL with non-zero length");
			return -EINVAL;
		}

		if (end - cursor < (ptrdiff_t)packet->payload_len) {
			NET_DBG("Payload of len %zu too large", packet->payload_len);
			return -EINVAL;
		}

		memcpy(cursor, packet->payload, packet->payload_len);
		cursor += packet->payload_len;
	}

	if (padding > 0) {
		if (end - cursor < (ptrdiff_t)padding) {
			NET_DBG("Padding of size %u too large", padding);
			return -EINVAL;
		}

		memset(cursor, 0, padding - 1);
		cursor += padding - 1;
		*cursor++ = padding;
	}

	return cursor - buf;
}

size_t rtp_packet_hdr_len(const struct rtp_packet *packet)
{
	size_t len = RTP_MIN_HEADER_LEN;

	len += (size_t)rtp_header_get_cc(&packet->header) * sizeof(uint32_t);

	if (rtp_header_get_x(&packet->header) == 1) {
		len += 2 * sizeof(uint16_t);
		len += (size_t)packet->header.header_extension.length * sizeof(uint32_t);
	}

	return len;
}
