/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_frontend_stmesp_demux.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stmesp_demux);

BUILD_ASSERT(sizeof(struct log_frontend_stmesp_demux_log_header) == sizeof(uint32_t),
		"Must fit in a word");

#define NUM_OF_ACTIVE CONFIG_LOG_FRONTEND_STMESP_DEMUX_ACTIVE_PACKETS
#define M_ID_OFF      16
#define M_ID_MASK     (BIT_MASK(16) << M_ID_OFF)
#define C_ID_MASK     BIT_MASK(16)

#define M_CH_HW_EVENT 0x00800000
#define M_CH_INVALID  0xFFFFFFFF

struct log_frontend_stmesp_demux_active_entry {
	sys_snode_t node;
	uint32_t m_ch;
	struct log_frontend_stmesp_demux_log *packet;
	int off;
};

struct log_frontend_stmesp_demux {
	/* Pool for active entries. */
	struct k_mem_slab mslab;

	/* List of currently active entries. */
	sys_slist_t active_entries;

	/* The most recently used entry. */
	struct log_frontend_stmesp_demux_active_entry *curr;

	struct mpsc_pbuf_buffer pbuf;

	uint32_t curr_m_ch;

	const uint16_t *m_ids;

	uint32_t m_ids_cnt;

	uint32_t dropped;
};

static uint32_t buffer[CONFIG_LOG_FRONTEND_STMESP_DEMUX_BUFFER_SIZE]
	__aligned(Z_LOG_MSG_ALIGNMENT);

static struct log_frontend_stmesp_demux demux;
static uint32_t slab_buf[NUM_OF_ACTIVE * sizeof(struct log_frontend_stmesp_demux_active_entry)];
static bool skip;

static void notify_drop(const struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic *packet)
{
	demux.dropped++;
}

static uint32_t calc_wlen(uint32_t total_len)
{
	return DIV_ROUND_UP(ROUND_UP(total_len, Z_LOG_MSG_ALIGNMENT), sizeof(uint32_t));
}

static uint32_t get_wlen(const union mpsc_pbuf_generic *packet)
{
	union log_frontend_stmesp_demux_packet p = {.rgeneric = packet};
	uint32_t len;

	switch (p.generic_packet->type) {
	case LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT:
		len = sizeof(struct log_frontend_stmesp_demux_trace_point) / sizeof(uint32_t);
		break;
	case LOG_FRONTEND_STMESP_DEMUX_TYPE_HW_EVENT:
		len = sizeof(struct log_frontend_stmesp_demux_hw_event) / sizeof(uint32_t);
		break;
	default:
		len = calc_wlen(p.log->hdr.total_len +
			offsetof(struct log_frontend_stmesp_demux_log, data));
		break;
	}

	return len;
}

int log_frontend_stmesp_demux_init(const struct log_frontend_stmesp_demux_config *config)
{
	int err;
	static const struct mpsc_pbuf_buffer_config pbuf_config = {
		.buf = buffer,
		.size = ARRAY_SIZE(buffer),
		.notify_drop = notify_drop,
		.get_wlen = get_wlen,
		.flags = MPSC_PBUF_MODE_OVERWRITE |
			 (IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_DEMUX_MAX_UTILIZATION) ?
				MPSC_PBUF_MAX_UTILIZATION : 0)};

	memset(buffer, 0, sizeof(buffer));
	mpsc_pbuf_init(&demux.pbuf, &pbuf_config);

	sys_slist_init(&demux.active_entries);

	if (config->m_ids_cnt > BIT(3)) {
		return -EINVAL;
	}

	demux.m_ids = config->m_ids;
	demux.m_ids_cnt = config->m_ids_cnt;
	demux.dropped = 0;
	demux.curr_m_ch = M_CH_INVALID;
	demux.curr = NULL;

	err = k_mem_slab_init(&demux.mslab, slab_buf,
			      sizeof(struct log_frontend_stmesp_demux_active_entry),
			      NUM_OF_ACTIVE);

	return err;
}

void log_frontend_stmesp_demux_major(uint16_t id)
{
	for (int i = 0; i < demux.m_ids_cnt; i++) {
		if (id == demux.m_ids[i]) {
			sys_snode_t *node;

			demux.curr_m_ch = id << M_ID_OFF;
			demux.curr = NULL;

			SYS_SLIST_FOR_EACH_NODE(&demux.active_entries, node) {
				struct log_frontend_stmesp_demux_active_entry *entry =
					CONTAINER_OF(node,
						struct log_frontend_stmesp_demux_active_entry,
						node);

				if (entry->m_ch == demux.curr_m_ch) {
					demux.curr = entry;
					break;
				}
			}
			skip = false;
			return;
		}
	}

	skip = true;
}

void log_frontend_stmesp_demux_channel(uint16_t id)
{
	if (skip) {
		return;
	}

	if (id == CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID) {
		/* Flushing data that shall be discarded. */
		goto bail;
	}

	demux.curr_m_ch &= ~C_ID_MASK;
	demux.curr_m_ch |= id;

	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&demux.active_entries, node) {
		struct log_frontend_stmesp_demux_active_entry *entry =
			CONTAINER_OF(node, struct log_frontend_stmesp_demux_active_entry, node);

		if (entry->m_ch == demux.curr_m_ch) {
			demux.curr = entry;
			return;
		}
	}

bail:
	demux.curr = NULL;
}

static uint8_t get_major_id(uint16_t m_id)
{
	for (int i = 0; i < demux.m_ids_cnt; i++) {
		if (m_id == demux.m_ids[i]) {
			return i;
		}
	}

	__ASSERT_NO_MSG(0);
	return 0;
}

static void log_frontend_stmesp_demux_trace_point(uint16_t m_id, uint16_t id, uint64_t *ts,
						  uint32_t *data)
{
	struct log_frontend_stmesp_demux_trace_point packet = {.valid = 1,
					       .type = LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT,
					       .content_invalid = 0,
					       .has_data = data ? 1 : 0,
					       .timestamp = ts ? *ts : 0,
					       .major = m_id,
					       .id = id,
					       .data = data ? *data : 0};
	static const size_t wlen = sizeof(packet) / sizeof(uint32_t);

	mpsc_pbuf_put_data(&demux.pbuf, (const uint32_t *)&packet, wlen);
}

static void log_frontend_stmesp_demux_hw_event(uint64_t *ts, uint8_t data)
{
	struct log_frontend_stmesp_demux_hw_event packet = {.valid = 1,
					    .type = LOG_FRONTEND_STMESP_DEMUX_TYPE_HW_EVENT,
					    .content_invalid = 0,
					    .timestamp = ts ? *ts : 0,
					    .evt = data};
	static const size_t wlen = sizeof(packet) / sizeof(uint32_t);

	mpsc_pbuf_put_data(&demux.pbuf, (const uint32_t *)&packet, wlen);
}

int log_frontend_stmesp_demux_packet_start(uint32_t *data, uint64_t *ts)
{
	if (skip) {
		return 0;
	}

	struct log_frontend_stmesp_demux_active_entry *entry;
	union log_frontend_stmesp_demux_packet p;
	int err;

	if (demux.curr_m_ch == M_CH_INVALID) {
		return -EINVAL;
	}

	if (demux.curr_m_ch == M_CH_HW_EVENT) {
		/* HW event */
		log_frontend_stmesp_demux_hw_event(ts, (uint8_t)*data);

		return 1;
	}

	if (demux.curr != NULL) {
		/* Previous package was incompleted. Finish it and potentially
		 * mark as incompleted if not all data is received.
		 */
		log_frontend_stmesp_demux_packet_end();
		return -EINVAL;
	}

	uint16_t ch = demux.curr_m_ch & C_ID_MASK;
	uint16_t m = get_major_id(demux.curr_m_ch >> M_ID_OFF);

	if (ch >= CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE) {
		uint32_t id = (uint32_t)ch - CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE;

		/* Trace point */
		log_frontend_stmesp_demux_trace_point(m, id, ts, data);

		return 1;
	}

	union log_frontend_stmesp_demux_header hdr = {.raw = *data};
	uint32_t pkt_len = hdr.log.total_len + offsetof(struct log_frontend_stmesp_demux_log, data);
	uint32_t wlen = calc_wlen(pkt_len);

	err = k_mem_slab_alloc(&demux.mslab, (void **)&entry, K_NO_WAIT);

	if (err < 0) {
		goto on_nomem;
	}

	entry->m_ch = demux.curr_m_ch;
	entry->off = 0;
	p.generic = mpsc_pbuf_alloc(&demux.pbuf, wlen, K_NO_WAIT);
	if (p.generic == NULL) {
		k_mem_slab_free(&demux.mslab, entry);
		goto on_nomem;
	}

	entry->packet = p.log;
	entry->packet->type = LOG_FRONTEND_STMESP_DEMUX_TYPE_LOG;
	entry->packet->content_invalid = 0;
	if (ts) {
		entry->packet->timestamp = *ts;
	}
	entry->packet->hdr = hdr.log;
	entry->packet->hdr.major = m;
	demux.curr = entry;
	sys_slist_append(&demux.active_entries, &entry->node);

	return 0;

on_nomem:
	demux.curr = NULL;
	demux.dropped++;
	return -ENOMEM;
}

void log_frontend_stmesp_demux_timestamp(uint64_t ts)
{
	if (demux.curr == NULL) {
		return;
	}

	demux.curr->packet->timestamp = ts;
}

void log_frontend_stmesp_demux_data(uint8_t *data, size_t len)
{
	if (demux.curr == NULL) {
		return;
	}

	if (demux.curr->off + len <= demux.curr->packet->hdr.total_len) {
		memcpy(&demux.curr->packet->data[demux.curr->off], data, len);
		demux.curr->off += len;
	}
}

void log_frontend_stmesp_demux_packet_end(void)
{
	if (demux.curr == NULL) {
		return;
	}

	union log_frontend_stmesp_demux_packet p = {.log = demux.curr->packet};

	if (demux.curr->off != demux.curr->packet->hdr.total_len) {
		demux.curr->packet->content_invalid = 1;
		demux.dropped++;
	}

	mpsc_pbuf_commit(&demux.pbuf, p.generic);

	sys_slist_find_and_remove(&demux.active_entries, &demux.curr->node);
	k_mem_slab_free(&demux.mslab, demux.curr);
	demux.curr = NULL;
}

uint32_t log_frontend_stmesp_demux_get_dropped(void)
{
	uint32_t rv = demux.dropped;

	demux.dropped = 0;

	return rv;
}

union log_frontend_stmesp_demux_packet log_frontend_stmesp_demux_claim(void)
{
	union log_frontend_stmesp_demux_packet p;

	/* Discard any invalid packets. */
	while ((p.rgeneric = mpsc_pbuf_claim(&demux.pbuf)) != NULL) {
		if (p.generic_packet->content_invalid) {
			mpsc_pbuf_free(&demux.pbuf, p.rgeneric);
		} else {
			break;
		}
	}

	return p;
}

void log_frontend_stmesp_demux_free(union log_frontend_stmesp_demux_packet packet)
{
	mpsc_pbuf_free(&demux.pbuf, packet.rgeneric);
}

void log_frontend_stmesp_demux_reset(void)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&demux.active_entries)) != NULL) {
		struct log_frontend_stmesp_demux_active_entry *entry =
			CONTAINER_OF(node, struct log_frontend_stmesp_demux_active_entry, node);
		union log_frontend_stmesp_demux_packet p = {.log = entry->packet};

		entry->packet->content_invalid = 1;
		mpsc_pbuf_commit(&demux.pbuf, p.generic);
		demux.dropped++;
		demux.curr_m_ch = M_CH_INVALID;

		k_mem_slab_free(&demux.mslab, entry);
	}
}

bool log_frontend_stmesp_demux_is_idle(void)
{
	return sys_slist_is_empty(&demux.active_entries);
}

int log_frontend_stmesp_demux_max_utilization(void)
{
	uint32_t max;
	int rv = mpsc_pbuf_get_max_utilization(&demux.pbuf, &max);

	return rv == 0 ? max : rv;
}
