/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_frontend_stmesp_demux.h>
#include <zephyr/logging/log_frontend_stmesp.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stmesp_demux);

BUILD_ASSERT(sizeof(struct log_frontend_stmesp_demux_log_header) == sizeof(uint32_t),
		"Must fit in a word");

#ifndef CONFIG_LOG_FRONTEND_STPESP_TURBO_SOURCE_PORT_ID
#define CONFIG_LOG_FRONTEND_STPESP_TURBO_SOURCE_PORT_ID 0
#endif

#ifndef CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG_BASE
#define CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG_BASE 0x8000
#endif

#define NUM_OF_ACTIVE CONFIG_LOG_FRONTEND_STMESP_DEMUX_ACTIVE_PACKETS
#define M_ID_OFF      16
#define M_ID_MASK     (BIT_MASK(16) << M_ID_OFF)
#define C_ID_MASK     BIT_MASK(16)

#define M_CH_HW_EVENT 0x00800000
#define M_CH_INVALID  0xFFFFFFFF

#define APP_M_ID  0x22
#define FLPR_M_ID 0x2D
#define PPR_M_ID  0x2E

struct log_frontend_stmesp_demux_active_entry {
	sys_snode_t node;
	uint32_t m_ch;
	uint32_t ts;
	struct log_frontend_stmesp_demux_log *packet;
	int off;
};

/* Coprocessors (FLPR, PPR) sends location where APP can find strings and logging
 * source names utilizing the fact that APP has access to FLPR/PPR memory if it is
 * an owner of that coprocessor. During the initialization FLPR/PPR sends 2 DMTS32
 * to the specific channel. First word is an address where logging source constant
 * data section is located and second is where a section with addresses to constant
 * strings used for logging is located.
 */
struct log_frontend_stmesp_coproc_sources {
	uint32_t m_id;
	uint32_t data_cnt;
	union {
		struct {
			const struct log_source_const_data *log_const;
			uintptr_t *log_str_section;
		} data;
		struct {
			uintptr_t data[2];
		} raw_data;
	};
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

	uint32_t *source_ids;

	uint16_t m_ids_cnt;

	uint16_t source_id_len;

	uint32_t dropped;

	struct log_frontend_stmesp_coproc_sources coproc_sources[2];
};

struct log_frontend_stmesp_entry_source_pair {
	uint16_t entry_id;
	uint16_t source_id;
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

static void store_source_id(uint16_t m_id, uint16_t entry_id, uint16_t source_id)
{
	uint32_t *source_ids_data = &demux.source_ids[m_id * (demux.source_id_len + 1)];
	uint32_t *wr_idx = source_ids_data;
	struct log_frontend_stmesp_entry_source_pair *source_ids =
		(struct log_frontend_stmesp_entry_source_pair *)&source_ids_data[1];

	source_ids[*wr_idx].entry_id = entry_id;
	source_ids[*wr_idx].source_id = source_id;
	*wr_idx = *wr_idx + 1;
	if (*wr_idx == (demux.source_id_len)) {
		*wr_idx = 0;
	}
}

static uint16_t get_source_id(uint16_t m_id, uint16_t entry_id)
{
	uint32_t *source_ids_data = &demux.source_ids[m_id * (demux.source_id_len + 1)];
	int32_t rd_idx = source_ids_data[0];
	uint32_t cnt = demux.source_id_len;
	struct log_frontend_stmesp_entry_source_pair *source_ids =
		(struct log_frontend_stmesp_entry_source_pair *)&source_ids_data[1];

	do {
		rd_idx = (rd_idx == 0) ? (demux.source_id_len - 1) : (rd_idx - 1);
		if (source_ids[rd_idx].entry_id == entry_id) {
			return source_ids[rd_idx].source_id;
		}
		cnt--;
	} while (cnt);

	return 0;
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
	demux.source_ids = config->source_id_buf;
	demux.source_id_len = config->source_id_buf_len / config->m_ids_cnt - 1;

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

static void store_turbo_log0(uint16_t m_id, uint16_t id, uint64_t *ts, uint16_t source_id)
{
	struct log_frontend_stmesp_demux_trace_point packet = {
		.valid = 1,
		.type = LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT,
		.content_invalid = 0,
		.has_data = 0,
		.timestamp = ts ? *ts : 0,
		.major = m_id,
		.source_id = source_id,
		.id = id,
		.data = 0};
	static const size_t wlen = sizeof(packet) / sizeof(uint32_t);

	mpsc_pbuf_put_data(&demux.pbuf, (const uint32_t *)&packet, wlen);
}

static void store_turbo_log1(uint16_t m_id, uint16_t id, uint64_t *ts, uint32_t data)
{
	struct log_frontend_stmesp_demux_trace_point packet = {
		.valid = 1,
		.type = LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT,
		.content_invalid = 0,
		.has_data = 0,
		.timestamp = ts ? *ts : 0,
		.major = m_id,
		.source_id = get_source_id(m_id, id),
		.id = id,
		.data = data};
	static const size_t wlen = sizeof(packet) / sizeof(uint32_t);

	mpsc_pbuf_put_data(&demux.pbuf, (const uint32_t *)&packet, wlen);
}

static void store_tracepoint(uint16_t m_id, uint16_t id, uint64_t *ts, uint32_t *data)
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

/* Check if there are any active messages which are not completed for a significant
 * amount of time. It may indicate that part of message was lost (due to reset,
 * fault in the core or fault on the bus). In that case message shall be closed as
 * incomplete to not block processing of other messages.
 */
static void garbage_collector(uint32_t now)
{
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&demux.active_entries, node) {
		struct log_frontend_stmesp_demux_active_entry *entry =
			CONTAINER_OF(node, struct log_frontend_stmesp_demux_active_entry, node);

		if ((now - entry->ts) > CONFIG_LOG_FRONTEND_STMESP_DEMUX_GC_TIMEOUT) {
			union log_frontend_stmesp_demux_packet p = {.log = entry->packet};

			sys_slist_find_and_remove(&demux.active_entries, node);
			entry->packet->content_invalid = 1;
			mpsc_pbuf_commit(&demux.pbuf, p.generic);
			demux.dropped++;
			k_mem_slab_free(&demux.mslab, entry);
			/* After removing one we need to stop as removing disrupts
			 * iterating over the list as current node is no longer in
			 * the list.
			 */
			break;
		}
	}
}

int log_frontend_stmesp_demux_log0(uint16_t source_id, uint64_t *ts)
{
	if (skip) {
		return 0;
	}

	if (demux.curr_m_ch == M_CH_INVALID) {
		return -EINVAL;
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

	if (ch < CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG_BASE) {
		return -EINVAL;
	}

	store_turbo_log0(m, ch, ts, source_id);

	return 1;
}

void log_frontend_stmesp_demux_source_id(uint16_t data)
{
	if (skip) {
		return;
	}

	if (demux.curr_m_ch == M_CH_INVALID) {
		return;
	}

	uint16_t ch = demux.curr_m_ch & C_ID_MASK;
	uint16_t m = get_major_id(demux.curr_m_ch >> M_ID_OFF);

	store_source_id(m, ch, data);
}

const char *log_frontend_stmesp_demux_sname_get(uint32_t m_id, uint16_t s_id)
{
	if (!IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG)) {
		return "";
	}

	if (demux.m_ids[m_id] == APP_M_ID) {
		return log_source_name_get(0, s_id);
	} else if (m_id == demux.coproc_sources[0].m_id) {
		return demux.coproc_sources[0].data.log_const[s_id].name;
	} else if (m_id == demux.coproc_sources[1].m_id) {
		return demux.coproc_sources[1].data.log_const[s_id].name;
	}

	return "unknown";
}

const char *log_frontend_stmesp_demux_str_get(uint32_t m_id, uint16_t s_id)
{
	if (!IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG)) {
		return "";
	}

	uintptr_t *log_str_start = NULL;

	if (demux.m_ids[m_id] == APP_M_ID) {
		log_str_start = (uintptr_t *)TYPE_SECTION_START(log_stmesp_ptr);
	} else if (m_id == demux.coproc_sources[0].m_id) {
		log_str_start = demux.coproc_sources[0].data.log_str_section;
	} else if (m_id == demux.coproc_sources[1].m_id) {
		log_str_start = demux.coproc_sources[1].data.log_str_section;
	}

	if (log_str_start) {
		return (const char *)log_str_start[s_id];
	}

	return "unknown";
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

	uint16_t ch = demux.curr_m_ch & C_ID_MASK;
	uint16_t m = get_major_id(demux.curr_m_ch >> M_ID_OFF);

	if (IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG) &&
	    (ch == CONFIG_LOG_FRONTEND_STPESP_TURBO_SOURCE_PORT_ID)) {
		struct log_frontend_stmesp_coproc_sources *src =
			&demux.coproc_sources[demux.m_ids[m] == FLPR_M_ID ? 0 : 1];

		if (src->data_cnt >= 2) {
			/* Unexpected packet. */
			return -EINVAL;
		}

		src->m_id = m;
		src->raw_data.data[src->data_cnt++] = (uintptr_t)*data;
		return 0;
	}

	if (demux.curr != NULL) {
		/* Previous package was incompleted. Finish it and potentially
		 * mark as incompleted if not all data is received.
		 */
		log_frontend_stmesp_demux_packet_end();
		return -EINVAL;
	}

	if (ch >= CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE) {
		/* Trace point */
		if (ch >= CONFIG_LOG_FRONTEND_STMESP_TURBO_LOG_BASE) {
			store_turbo_log1(m, ch, ts, *data);
		} else {
			store_tracepoint(m, ch, ts, data);
		}

		return 1;
	}

	union log_frontend_stmesp_demux_header hdr = {.raw = *data};
	uint32_t pkt_len = hdr.log.total_len + offsetof(struct log_frontend_stmesp_demux_log, data);
	uint32_t wlen = calc_wlen(pkt_len);
	uint32_t now = k_uptime_get_32();

	garbage_collector(now);
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
	entry->ts = now;
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
