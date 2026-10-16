/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define CTF_PACKET_MAGIC 0xC1FC1FC1U
#define CTF_PACKET_CONTEXT_SIZE 36U
#define CTF_SLOT_SIZE (CONFIG_RAM_TRACING_BUFFER_SIZE / \
			   CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT)

struct ctf_packet_header {
	uint32_t magic;
	uint64_t timestamp_begin;
	uint64_t timestamp_end;
	uint32_t content_size;
	uint32_t packet_size;
	uint64_t packet_seq_num;
} __packed;

BUILD_ASSERT(sizeof(struct ctf_packet_header) == CTF_PACKET_CONTEXT_SIZE);

extern uint8_t ram_tracing_circular[CONFIG_RAM_TRACING_BUFFER_SIZE];

static void generate_trace_events(void)
{
	struct k_sem sem;

	zassert_equal(k_sem_init(&sem, 0U, 1U), 0, "semaphore initialization failed");

	for (uint32_t i = 0U; i < (CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT * 8U); i++) {
		k_sem_give(&sem);
		zassert_equal(k_sem_take(&sem, K_NO_WAIT), 0, "semaphore take failed");
	}

	k_sleep(K_MSEC(1));
}

ZTEST(ctf_circular_ram, test_packet_headers_and_trace_data)
{
	uint32_t nonempty_packets = 0U;
	uint64_t highest_sequence = 0U;

	generate_trace_events();

	for (uint32_t slot = 0U;
	     slot < CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT; slot++) {
		const struct ctf_packet_header *header =
			(const struct ctf_packet_header *)&ram_tracing_circular[slot * CTF_SLOT_SIZE];

		zassert_equal(header->magic, CTF_PACKET_MAGIC,
			      "invalid packet magic in slot %u", slot);
		zassert_equal(header->packet_size, CTF_SLOT_SIZE * 8U,
			      "invalid packet size in slot %u", slot);
		zassert_between_inclusive(header->content_size,
					 CTF_PACKET_CONTEXT_SIZE * 8U,
					 header->packet_size,
					 "invalid content size in slot %u", slot);
		zassert_true(header->timestamp_end >= header->timestamp_begin,
			     "timestamps are not ordered in slot %u", slot);

		if (header->content_size > (CTF_PACKET_CONTEXT_SIZE * 8U)) {
			const uint8_t *event = (const uint8_t *)header + CTF_PACKET_CONTEXT_SIZE;
			uint64_t event_timestamp;

			memcpy(&event_timestamp, event, sizeof(event_timestamp));
			zassert_between_inclusive(event_timestamp, header->timestamp_begin,
					 header->timestamp_end,
					 "invalid event timestamp in slot %u", slot);
			nonempty_packets++;
		}

		if (header->packet_seq_num > highest_sequence) {
			highest_sequence = header->packet_seq_num;
		}
	}

	zassert_true(nonempty_packets > 0U, "no CTF trace events were captured");
	zassert_true(highest_sequence >= CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT,
		     "circular packet sequence did not advance: %" PRIu64,
		     highest_sequence);
}

ZTEST_SUITE(ctf_circular_ram, NULL, NULL, NULL, NULL, NULL);
