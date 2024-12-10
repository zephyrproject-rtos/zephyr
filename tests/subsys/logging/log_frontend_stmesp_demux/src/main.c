/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/logging/log_frontend_stmesp_demux.h>
#include <zephyr/logging/log_frontend.h>

#define M_HW  0x80
#define M_ID0 0x30
#define M_ID1 0x3131
#define M_ID2 0x20

#define TOTAL_LEN(len)                                                                             \
	ROUND_UP(len + offsetof(struct log_frontend_stmesp_demux_log, data), 2 * sizeof(uint32_t))
#define TOTAL_WLEN(len) (TOTAL_LEN(len) / sizeof(uint32_t))

static const uint16_t ids[] = {M_ID0, M_ID1, M_ID2, M_HW};

void log_frontend_init(void)
{
	/* empty */
}

void log_frontend_msg(const void *source, const struct log_msg_desc desc, uint8_t *package,
		      const void *data)
{
	/* empty */
}

void log_frontend_panic(void)
{
	/* empty */
}

static void claim_packet(uint16_t exp_m_idx, uint64_t exp_ts, uint16_t exp_len, uint8_t exp_id,
			 int line)
{
	union log_frontend_stmesp_demux_packet packet = log_frontend_stmesp_demux_claim();

	if (packet.generic == NULL) {
		zassert_equal(exp_len, 0, "%d: Expected a packet", line);
		return;
	}

	zassert_equal(packet.generic_packet->type, LOG_FRONTEND_STMESP_DEMUX_TYPE_LOG);
	zassert_equal(exp_ts, packet.log->timestamp, "%d: Unexpected ts %llu/%x (exp:%llu/%x)",
		      line, packet.log->timestamp, packet.log->timestamp, exp_ts, exp_ts);
	zassert_equal(exp_m_idx, ids[packet.log->hdr.major], "%d: Unexpected major:%d (exp:%d)",
		      line, packet.log->hdr.major, exp_m_idx);
	zassert_equal(exp_len, packet.log->hdr.total_len, "%d: Unexpected len:%d (exp:%d)", line,
		      packet.log->hdr.total_len, exp_len);
	for (int i = 0; i < exp_len; i++) {
		zassert_equal(packet.log->data[i], i + exp_id,
			      "%d: Unexpected data(%d) at %d index (exp:%d)", line,
			      packet.log->data[i], i, i + exp_id);
	}

	log_frontend_stmesp_demux_free(packet);
}

#define CLAIM_PACKET(_exp_m_idx, _exp_ts, _exp_len, _exp_id)                                       \
	claim_packet(_exp_m_idx, _exp_ts, _exp_len, _exp_id, __LINE__)

static void claim_trace_point(uint16_t exp_m_idx, uint16_t exp_id, uint64_t exp_ts,
			      uint32_t *exp_data, int line)
{
	union log_frontend_stmesp_demux_packet packet = log_frontend_stmesp_demux_claim();

	zassert_equal(packet.generic_packet->type, LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT);
	zassert_equal(exp_ts, packet.trace_point->timestamp,
		      "%d: Unexpected ts %llu/%x (exp:%llu/%x)", line,
		      packet.trace_point->timestamp, packet.trace_point->timestamp, exp_ts, exp_ts);
	zassert_equal(exp_id, packet.trace_point->id, "%d: Unexpected id:%d (exp:%d)", line,
		      packet.trace_point->id, exp_id);
	zassert_equal(exp_m_idx, ids[packet.trace_point->major],
		      "%d: Unexpected major:%d (exp:%d)", line, packet.trace_point->major,
		      exp_m_idx);
	if (exp_data) {
		zassert_equal(1, packet.trace_point->has_data);
		zassert_equal(*exp_data, packet.trace_point->data,
			      "%d: Unexpected data:%d (exp:%d)", line, packet.trace_point->data,
			      *exp_data);
	} else {
		zassert_equal(0, packet.trace_point->has_data);
	}

	log_frontend_stmesp_demux_free(packet);
}

#define CLAIM_TRACE_POINT(_exp_m_idx, _exp_id, _exp_ts, _exp_data)                                 \
	claim_trace_point(_exp_m_idx, _exp_id, _exp_ts, _exp_data, __LINE__)

static void claim_hw_event(uint8_t exp_evt, uint64_t exp_ts, int line)
{
	union log_frontend_stmesp_demux_packet packet = log_frontend_stmesp_demux_claim();

	zassert_equal(packet.generic_packet->type, LOG_FRONTEND_STMESP_DEMUX_TYPE_HW_EVENT);
	zassert_equal(exp_ts, packet.hw_event->timestamp, "%d: Unexpected ts %llu/%x (exp:%llu/%x)",
		      line, packet.hw_event->timestamp, packet.hw_event->timestamp, exp_ts, exp_ts);
	zassert_equal(exp_evt, packet.hw_event->evt, "%d: Unexpected id:%d (exp:%d)", line,
		      packet.hw_event->evt, exp_evt);

	log_frontend_stmesp_demux_free(packet);
}

#define CLAIM_HW_EVENT(_exp_evt, _exp_ts) claim_hw_event(_exp_evt, _exp_ts, __LINE__)

#define DEMUX_EMPTY() CLAIM_PACKET(0, 0, 0, 0)

static int write_trace_point(uint16_t *m_id, uint16_t *c_id, uint32_t *data, uint64_t ts)
{
	if (m_id) {
		log_frontend_stmesp_demux_major(*m_id);
	}

	if (c_id) {
		log_frontend_stmesp_demux_channel(*c_id);
	}

	return log_frontend_stmesp_demux_packet_start(data, &ts);
}

static int write_hw_event(uint8_t evt, uint64_t ts)
{
	uint32_t data = (uint32_t)evt;

	log_frontend_stmesp_demux_major(M_HW);

	return log_frontend_stmesp_demux_packet_start(&data, &ts);
}

static void packet_start(uint16_t *m_id, uint16_t *c_id, uint32_t data, uint64_t ts, int exp_rv,
			 int line)
{
	int rv;

	if (m_id) {
		log_frontend_stmesp_demux_major(*m_id);
	}

	if (c_id) {
		log_frontend_stmesp_demux_channel(*c_id);
	}

	rv = log_frontend_stmesp_demux_packet_start(&data, &ts);
	zassert_equal(rv, exp_rv, "%d: Unexpected ret:%d (exp:%d)", line, rv, exp_rv);
}
#define PACKET_START(_m_id, _c_id, _data, _ts, _exp_rv)                                            \
	packet_start(_m_id, _c_id, _data, _ts, _exp_rv, __LINE__)

static void packet_data(uint16_t *m_id, uint16_t *c_id, uint8_t *data, size_t len)
{
	if (m_id) {
		log_frontend_stmesp_demux_major(*m_id);
	}

	if (c_id) {
		log_frontend_stmesp_demux_channel(*c_id);
	}

	log_frontend_stmesp_demux_data(data, len);
}

static void packet_end(uint16_t *m_id, uint16_t *c_id)
{
	if (m_id) {
		log_frontend_stmesp_demux_major(*m_id);
	}

	if (c_id) {
		log_frontend_stmesp_demux_channel(*c_id);
	}

	log_frontend_stmesp_demux_packet_end();
}

static void write_data(uint16_t len, uint8_t id)
{
	for (int i = 0; i < len; i++) {
		uint8_t d = i + id;

		log_frontend_stmesp_demux_data(&d, 1);
	}
}

static void write_packet(uint16_t m_id, uint16_t c_id, uint64_t ts, uint16_t len, uint8_t id)
{
	union log_frontend_stmesp_demux_header hdr = {.log = {.total_len = len}};

	log_frontend_stmesp_demux_major(m_id);
	log_frontend_stmesp_demux_channel(c_id);
	log_frontend_stmesp_demux_packet_start(&hdr.raw, NULL);
	log_frontend_stmesp_demux_timestamp(ts);
	write_data(len, id);
	log_frontend_stmesp_demux_packet_end();
}

static void demux_init(void)
{
	struct log_frontend_stmesp_demux_config config = {.m_ids = ids,
							  .m_ids_cnt = ARRAY_SIZE(ids)};
	int err;

	err = log_frontend_stmesp_demux_init(&config);
	zassert_equal(err, 0, NULL);
}

ZTEST(log_frontend_stmesp_demux_test, test_init)
{
	/* Ids limit is 8 */
	static const uint16_t m_ids[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	struct log_frontend_stmesp_demux_config config = {.m_ids = m_ids,
							  .m_ids_cnt = ARRAY_SIZE(m_ids)};
	int err;

	err = log_frontend_stmesp_demux_init(&config);
	zassert_equal(err, -EINVAL, NULL);

	config.m_ids_cnt = 8;
	err = log_frontend_stmesp_demux_init(&config);
	zassert_equal(err, 0, NULL);
}

ZTEST(log_frontend_stmesp_demux_test, test_basic)
{
	uint16_t m = M_ID0;
	uint16_t c = 0;
	uint8_t data = 1;

	demux_init();

	/* Writing to packet that was not started has no effect. */
	packet_data(&m, &c, &data, 1);
	packet_end(&m, &c);

	write_packet(M_ID0, 1, 1, 10, 1);
	write_packet(M_ID0, 2, 2, 10, 2);
	write_packet(M_ID1, 1, 3, 10, 3);

	CLAIM_PACKET(M_ID0, 1, 10, 1);
	CLAIM_PACKET(M_ID0, 2, 10, 2);
	CLAIM_PACKET(M_ID1, 3, 10, 3);

	DEMUX_EMPTY();

	zassert_equal(log_frontend_stmesp_demux_get_dropped(), 0, NULL);
}

ZTEST(log_frontend_stmesp_demux_test, test_overwrite)
{
	uint64_t ts = 0;
	uint32_t len = 10;
	uint32_t total_wlen = TOTAL_WLEN(len);
	int i;

	demux_init();

	for (i = 0; i < (CONFIG_LOG_FRONTEND_STMESP_DEMUX_BUFFER_SIZE / total_wlen); i++) {
		write_packet(M_ID0, 1, ts + i, len, i);
	}
	zassert_equal(log_frontend_stmesp_demux_get_dropped(), 0, NULL);

	write_packet(M_ID0, 1, ts + i, len, i);

	uint32_t dropped = log_frontend_stmesp_demux_get_dropped();

	zassert_true(dropped >= 1, NULL);

	for (i = dropped; i < (CONFIG_LOG_FRONTEND_STMESP_DEMUX_BUFFER_SIZE / total_wlen) + 1;
	     i++) {
		CLAIM_PACKET(M_ID0, ts + i, len, i);
	}

	DEMUX_EMPTY();
}

ZTEST(log_frontend_stmesp_demux_test, test_mix)
{
	uint16_t m_id = M_ID0;
	uint16_t c_id0 = 2;
	uint16_t c_id1 = 1;
	uint64_t ts0 = 0x1234567890;
	uint64_t ts1 = 0x3434343445;
	int len0 = 12;
	int len1 = 14;
	union log_frontend_stmesp_demux_header hdr0 = {.log = {.total_len = len0}};
	union log_frontend_stmesp_demux_header hdr1 = {.log = {.total_len = len1}};

	zassert_true(c_id0 != CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID);
	zassert_true(c_id1 != CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID);

	demux_init();

	/* Write 2 packets mixing them. */
	PACKET_START(&m_id, &c_id0, hdr0.raw, ts0, 0);

	PACKET_START(&m_id, &c_id1, hdr1.raw, ts1, 0);
	packet_data(&m_id, &c_id0, NULL, 0);
	write_data(len0, 0);
	packet_data(&m_id, &c_id1, NULL, 0);
	write_data(len1, 1);
	packet_end(&m_id, &c_id0);
	packet_end(&m_id, &c_id1);

	/* Expect demuxed packets. */
	CLAIM_PACKET(M_ID0, ts0, len0, 0);
	CLAIM_PACKET(M_ID0, ts1, len1, 1);

	DEMUX_EMPTY();
}

ZTEST(log_frontend_stmesp_demux_test, test_drop_too_many_active)
{
	BUILD_ASSERT(CONFIG_LOG_FRONTEND_STMESP_DEMUX_ACTIVE_PACKETS == 3,
		     "Test assumes certain configuration");

	uint16_t m_id0 = M_ID0;
	uint16_t m_id1 = M_ID1;
	uint16_t c_id0 = 2;
	uint16_t c_id1 = 1;
	int len = 4;
	uint64_t ts = 0;
	uint8_t data[] = {1, 2, 3, 4};
	union log_frontend_stmesp_demux_header hdr = {.log = {.total_len = len}};

	zassert_true(c_id0 != CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID);
	zassert_true(c_id1 != CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID);

	demux_init();

	PACKET_START(NULL, NULL, hdr.raw, ts, -EINVAL);

	/* Start writing to 3 packets */
	PACKET_START(&m_id0, &c_id0, hdr.raw, ts, 0);
	packet_data(NULL, NULL, data, 1);
	PACKET_START(&m_id0, &c_id1, hdr.raw, ts + 1, 0);
	PACKET_START(&m_id1, &c_id0, hdr.raw, ts + 2, 0);
	packet_data(NULL, NULL, data, 1);

	zassert_equal(log_frontend_stmesp_demux_get_dropped(), 0, NULL);
	/* Starting forth packet results in dropping. */
	PACKET_START(&m_id1, &c_id1, hdr.raw, ts + 3, -ENOMEM);
	zassert_equal(log_frontend_stmesp_demux_get_dropped(), 1, NULL);

	/* Complete first packet. */
	packet_data(&m_id0, &c_id0, &data[1], 3);
	packet_end(NULL, NULL);

	PACKET_START(&m_id1, &c_id1, hdr.raw, ts + 3, 0);
	zassert_equal(log_frontend_stmesp_demux_get_dropped(), 0, NULL);
}

ZTEST(log_frontend_stmesp_demux_test, test_max_utilization)
{
	int utilization;
	uint32_t len = 10;

	if (!IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_DEMUX_MAX_UTILIZATION)) {
		utilization = log_frontend_stmesp_demux_max_utilization();
		zassert_equal(utilization, -ENOTSUP, NULL);
		return;
	}

	demux_init();
	utilization = log_frontend_stmesp_demux_max_utilization();
	zassert_equal(utilization, 0, NULL);

	write_packet(M_ID0, 0, 1, len, 1);
	utilization = log_frontend_stmesp_demux_max_utilization();

	int exp_utilization = TOTAL_LEN(len);

	zassert_equal(utilization, exp_utilization, NULL);
}

ZTEST(log_frontend_stmesp_demux_test, test_trace_point)
{
	uint16_t m_id0 = M_ID0;
	uint16_t m_id1 = M_ID1;
	uint16_t id0 = 2;
	uint16_t id1 = 0;
	uint16_t c_id0 = CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE + id0;
	uint16_t c_id1 = CONFIG_LOG_FRONTEND_STMESP_TP_CHAN_BASE + id1;
	uint32_t data = 0x11223344;
	uint64_t t0 = 0x1122334455;
	uint64_t t1 = 0x5522334455;
	int err;

	demux_init();

	err = write_trace_point(&m_id0, &c_id0, NULL, t0);
	zassert_equal(err, 1);

	err = write_trace_point(NULL, &c_id0, NULL, t0);
	zassert_equal(err, 1);

	err = write_trace_point(NULL, &c_id0, &data, t0);
	zassert_equal(err, 1);

	err = write_trace_point(NULL, &c_id1, &data, t1);
	zassert_equal(err, 1);

	err = write_trace_point(&m_id1, &c_id0, NULL, t0);
	zassert_equal(err, 1);

	err = write_trace_point(&m_id1, &c_id1, NULL, t1);
	zassert_equal(err, 1);

	CLAIM_TRACE_POINT(m_id0, id0, t0, NULL);
	CLAIM_TRACE_POINT(m_id0, id0, t0, NULL);
	CLAIM_TRACE_POINT(m_id0, id0, t0, &data);
	CLAIM_TRACE_POINT(m_id0, id1, t1, &data);
	CLAIM_TRACE_POINT(m_id1, id0, t0, NULL);
	CLAIM_TRACE_POINT(m_id1, id1, t1, NULL);

	DEMUX_EMPTY();
}

ZTEST(log_frontend_stmesp_demux_test, test_hw_event)
{
	uint64_t t0 = 0x1122334455;
	uint64_t t1 = 0x5522334455;
	int err;

	demux_init();

	err = write_hw_event(0, t0);
	zassert_equal(err, 1);

	err = write_hw_event(1, t1);
	zassert_equal(err, 1);

	CLAIM_HW_EVENT(0, t0);
	CLAIM_HW_EVENT(1, t1);

	DEMUX_EMPTY();
}

ZTEST(log_frontend_stmesp_demux_test, test_reset)
{
	uint16_t m_id0 = M_ID0;
	uint16_t m_id1 = M_ID1;
	uint16_t c_id0 = 2;
	uint16_t c_id1 = 1;
	int len = 4;
	uint64_t ts = 0;
	uint8_t data[] = {1, 2, 3, 4};
	union log_frontend_stmesp_demux_header hdr = {.log = {.total_len = len}};

	zassert_true(c_id0 != CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID);
	zassert_true(c_id1 != CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID);

	demux_init();

	PACKET_START(NULL, NULL, hdr.raw, ts, -EINVAL);

	/* Start writing to 3 packets */
	PACKET_START(&m_id0, &c_id0, hdr.raw, ts, 0);
	packet_data(NULL, NULL, data, 1);
	PACKET_START(&m_id0, &c_id1, hdr.raw, ts + 1, 0);
	PACKET_START(&m_id1, &c_id0, hdr.raw, ts + 2, 0);
	packet_data(NULL, NULL, data, 4);
	packet_end(NULL, NULL);

	log_frontend_stmesp_demux_reset();
	zassert_equal(log_frontend_stmesp_demux_get_dropped(), 2, NULL);

	CLAIM_PACKET(M_ID1, ts + 2, len, 1);
	DEMUX_EMPTY();
}

ZTEST_SUITE(log_frontend_stmesp_demux_test, NULL, NULL, NULL, NULL, NULL);
