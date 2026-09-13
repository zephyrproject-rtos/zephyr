/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <tracing_core.h>
#include <tracing_backend.h>
#include <zephyr/instrumentation/instrumentation.h>

#include "instr_transport.h"
#include "instr_timestamp.h"

#define INSTRUMENTATION_STREAM_ID 2

struct tracing_packet_header {
	uint16_t stream_id;
	uint16_t packet_size;
	uint64_t timestamp;
	uint16_t type;
} __packed;

struct tracing_callgraph_packet {
	struct tracing_packet_header header;
	void *callee;
	void *caller;
	struct instr_event_context context;
} __packed;


extern void instr_dump_deltas(void);

__no_instrumentation__
static void instr_format_raw_data(uint8_t *data, uint32_t length)
{
	unsigned int lock = irq_lock();

	tracing_buffer_handle(data, length);
	irq_unlock(lock);
}

__no_instrumentation__
void instr_transport_init(void)
{
	/* Tracing core initialzies itself during Zephyr boot */
}

__no_instrumentation__
void instr_transport_push_record(struct instr_record *record)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	struct tracing_callgraph_packet packet;

	packet.header.stream_id = INSTRUMENTATION_STREAM_ID;
	packet.header.packet_size = sizeof(struct tracing_callgraph_packet) * 8;
	packet.header.timestamp = record->timestamp;
	packet.header.type = (uint16_t)record->header.type;

	packet.callee = record->callee;
	packet.caller = record->caller;
	packet.context = record->context;

	instr_format_raw_data((uint8_t *)&packet, sizeof(packet));
#endif
}

__no_instrumentation__
void instr_transport_cmd_dump_trace(void)
{
	/* Streams continuously through tracing core */
}

__no_instrumentation__
void instr_transport_cmd_dump_profile(void)
{
	instr_dump_deltas();
}

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)

#define MAX_NUM_DISCO_FUNC CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_NUM_FUNC

static struct {
	struct tracing_packet_header header;
	uint32_t num_entries;
	struct {
		uint32_t callee;
		uint64_t delta_t;
	} __packed entries[MAX_NUM_DISCO_FUNC];
} __packed instr_profile_record;

__no_instrumentation__
void instr_transport_send_stats(struct disco_func_entry *stats, int count)
{
	uint32_t size = (sizeof(instr_profile_record) - sizeof(instr_profile_record.entries) +
			 sizeof(instr_profile_record.entries[0]) * count);

	instr_profile_record.header.stream_id = INSTRUMENTATION_STREAM_ID;
	instr_profile_record.header.packet_size = size * 8; /* CTF Size in bits */
	instr_profile_record.header.timestamp = instr_timestamp_ns();
	instr_profile_record.header.type = (uint16_t)INSTR_EVENT_PROFILE;

	instr_profile_record.num_entries = count;

	for (int i = 0; i < count; i++) {
		instr_profile_record.entries[i].callee = (uint32_t)(stats[i].addr);
		instr_profile_record.entries[i].delta_t = (uint64_t)(stats[i].delta_t);
	}

	instr_format_raw_data((uint8_t *)&instr_profile_record, size);
}
#endif
