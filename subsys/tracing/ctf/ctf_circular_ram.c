/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <tracing_core.h>
#include "ctf_circular_ram.h"

/*
 * Circular RAM capture path for the upstream CTF format
 * (CONFIG_TRACING_CTF_CIRCULAR_RAM).
 *
 * Unlike the linear backend which drops every event once the buffer fills,
 * this backend models the buffer as N rotating, self-contained CTF packets
 * (LTTng flight-recorder style). When the active packet has no room for the
 * next event, the backend advances to the next slot - overwriting the oldest
 * whole packet - so the most recent traffic is always retained.
 *
 * Each slot carries a CTF packet header (magic 0xC1FC1FC1) and packet context
 * (timestamp_begin/end, content_size, packet_size, packet_seq_num), matching
 * ctf/tsdl/metadata_circular. The host-side sort_ctf_circular.py utility
 * orders the valid slots by sequence number before babeltrace2 decodes them.
 *
 * This capture path requires CONFIG_TRACING_SYNC (one complete CTF event per
 * write() call). The async path delivers arbitrary byte runs and is not
 * supported.
 *
 * The dumped symbol is still ram_tracing_circular[CONFIG_RAM_TRACING_BUFFER_SIZE], so
 * the existing T32 / gdb dump flow is unchanged.
 */

#define SLOT_COUNT       CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT
#define SLOT_SIZE        (CONFIG_RAM_TRACING_BUFFER_SIZE / SLOT_COUNT)
#define CTF_PACKET_MAGIC 0xC1FC1FC1U

BUILD_ASSERT((CONFIG_RAM_TRACING_BUFFER_SIZE % SLOT_COUNT) == 0,
	     "RAM_TRACING_BUFFER_SIZE must be a multiple of TRACING_BACKEND_RAM_PACKET_COUNT");

/*
 * On-wire CTF packet header + context. Field order and sizes MUST match the
 * trace.packet.header and stream.packet.context in ctf/tsdl/metadata_circular.
 * The stream is little-endian; this assumes a little-endian target.
 */
struct ctf_packet_hdr {
	uint32_t magic;           /* trace.packet.header.magic */
	uint64_t timestamp_begin; /* stream.packet.context ... */
	uint64_t timestamp_end;
	uint32_t content_size;    /* bits, includes this header */
	uint32_t packet_size;     /* bits, == SLOT_SIZE * 8 */
	uint64_t packet_seq_num;
} __packed;

#define CTX_BYTES (sizeof(struct ctf_packet_hdr))

BUILD_ASSERT(SLOT_SIZE >= (CTX_BYTES + CONFIG_TRACING_PACKET_MAX_SIZE),
	     "Circular CTF packet slot must fit its header and one tracing packet");

uint8_t ram_tracing_circular[CONFIG_RAM_TRACING_BUFFER_SIZE];

static uint32_t cur_slot;    /* slot currently being filled, 0..SLOT_COUNT-1 */
static uint32_t cur_off;     /* write cursor within the current slot (bytes)  */
static uint64_t seq;         /* monotonic packet sequence number              */
static uint64_t last_tstamp; /* timestamp of the most recent event            */

static inline uint8_t *slot_base(uint32_t s)
{
	return ram_tracing_circular + (s * SLOT_SIZE);
}

/*
 * Extract the event timestamp, which CTF_EVENT writes as the first field of
 * every event. Width depends on CONFIG_TRACING_CTF_TIMESTAMP_64; fall back
 * to the previous timestamp for any anomalously short packet.
 */
static inline uint64_t event_tstamp(const uint8_t *data, uint32_t length)
{
#ifdef CONFIG_TRACING_CTF_TIMESTAMP_64
	uint64_t t;

	if (length >= sizeof(t)) {
		memcpy(&t, data, sizeof(t));
		return t;
	}
#else
	uint32_t t;

	if (length >= sizeof(t)) {
		memcpy(&t, data, sizeof(t));
		return (uint64_t)t;
	}
#endif /* CONFIG_TRACING_CTF_TIMESTAMP_64 */
	return last_tstamp;
}

static void packet_open(uint32_t s, uint64_t tstamp)
{
	struct ctf_packet_hdr hdr = {
		.magic = CTF_PACKET_MAGIC,
		.timestamp_begin = tstamp,
		.timestamp_end = tstamp,
		.content_size = (uint32_t)(CTX_BYTES * 8U),
		.packet_size = (uint32_t)(SLOT_SIZE * 8U),
		.packet_seq_num = seq,
	};

	memcpy(slot_base(s), &hdr, sizeof(hdr));
	cur_off = CTX_BYTES;
}

static void packet_update(uint32_t s, uint64_t tstamp)
{
	struct ctf_packet_hdr *hdr = (struct ctf_packet_hdr *)slot_base(s);

	hdr->content_size = cur_off * 8U;
	hdr->timestamp_end = tstamp;
}

/* SYNC mode: data/length is exactly one whole CTF event. */
void ctf_circular_ram_write(const uint8_t *data, uint32_t length)
{
	uint64_t tstamp;

	if (!is_tracing_enabled()) {
		return;
	}

	if (length > (SLOT_SIZE - CTX_BYTES)) {
		return;
	}

	tstamp = event_tstamp(data, length);

	if ((cur_off + length) > SLOT_SIZE) {
		/* Close the current packet, then overwrite the oldest one. */
		packet_update(cur_slot, last_tstamp);
		cur_slot = (cur_slot + 1U) % SLOT_COUNT;
		seq++;
		packet_open(cur_slot, tstamp);
	}

	memcpy(slot_base(cur_slot) + cur_off, data, length);
	cur_off += length;
	last_tstamp = tstamp;

	/*
	 * Keep the packet self-consistent after every event so a dump taken
	 * mid-slot truncates cleanly at the last completed event rather than
	 * leaving a half-written event for the parser.
	 */
	packet_update(cur_slot, tstamp);
}

static int ctf_circular_ram_init(void)
{
	uint32_t s;

	memset(ram_tracing_circular, 0, CONFIG_RAM_TRACING_BUFFER_SIZE);

	/* Pre-format every slot as a valid empty packet. */
	seq = 0U;
	last_tstamp = 0U;
	for (s = 0U; s < SLOT_COUNT; s++) {
		packet_open(s, 0U);
		packet_update(s, 0U);
	}

	/* Open slot 0 for writing with the first live sequence number. */
	cur_slot = 0U;
	seq = 1U;
	packet_open(cur_slot, 0U);

	return 0;
}

SYS_INIT(ctf_circular_ram_init, APPLICATION, 0);
