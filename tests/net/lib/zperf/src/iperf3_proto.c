/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The iperf3 message encoding, checked against messages captured from
 * iperf 3.16 with --debug.
 */

#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "zperf_iperf3.h"

/* The parameters iperf 3.16 sends for "-t 1", "-u -b 1M -l 1000
 * --udp-counters-64bit" and "-R"
 */
static const char params_tcp[] =
	"{\"tcp\":true,\"omit\":0,\"time\":1,\"num\":0,\"blockcount\":0,\"parallel\":1,"
	"\"len\":131072,\"pacing_timer\":1000,\"client_version\":\"3.16\"}";
static const char params_udp64[] =
	"{\"udp\":true,\"omit\":0,\"time\":1,\"num\":0,\"blockcount\":0,\"parallel\":1,"
	"\"len\":1000,\"bandwidth\":1000000,\"pacing_timer\":1000,\"udp_counters_64bit\":1,"
	"\"client_version\":\"3.16\"}";
static const char params_reverse[] =
	"{\"tcp\":true,\"omit\":0,\"time\":1,\"num\":0,\"blockcount\":0,\"parallel\":1,"
	"\"reverse\":true,\"len\":131072,\"pacing_timer\":1000,\"client_version\":\"3.16\"}";

/* The results an iperf 3.16 server sent after a UDP and a TCP test */
static const char results_udp[] =
	"{\"cpu_util_total\":0.93776675176068669,\"cpu_util_user\":0,"
	"\"cpu_util_system\":0.93756684411801738,\"sender_has_retransmits\":-1,"
	"\"streams\":[{\"id\":1,\"bytes\":125000,\"retransmits\":-1,"
	"\"jitter\":4.1014147773721059e-05,\"errors\":0,\"omitted_errors\":0,"
	"\"packets\":125,\"omitted_packets\":0,\"start_time\":0,\"end_time\":1.000422}]}";
static const char results_tcp[] =
	"{\"cpu_util_total\":77.29023946953204,\"cpu_util_user\":2.6414547923865066,"
	"\"cpu_util_system\":74.648684814955359,\"sender_has_retransmits\":-1,"
	"\"congestion_used\":\"cubic\",\"streams\":[{\"id\":1,\"bytes\":9313976320,"
	"\"retransmits\":-1,\"jitter\":0,\"errors\":0,\"omitted_errors\":0,\"packets\":0,"
	"\"omitted_packets\":0,\"start_time\":0,\"end_time\":1.001358}]}";

/* The parser writes into its input, so each test works on a copy */
static char json_buf[512];

static char *copy(const char *json)
{
	zassert_true(strlen(json) < sizeof(json_buf));
	strcpy(json_buf, json);

	return json_buf;
}

ZTEST(zperf_iperf3_proto, test_params_decode_tcp)
{
	struct zperf_iperf3_params p;

	zassert_ok(zperf_iperf3_params_decode(copy(params_tcp), strlen(params_tcp), &p));

	zassert_true(p.tcp);
	zassert_false(p.udp);
	zassert_false(p.reverse);
	zassert_equal(p.time, 1);
	zassert_equal(p.parallel, 1);
	zassert_equal(p.omit, 0);
	zassert_equal(p.len, 131072);
	zassert_equal(p.client_version.length, 4);
	zassert_mem_equal(p.client_version.start, "3.16", 4);
}

ZTEST(zperf_iperf3_proto, test_params_decode_udp_64bit_counters)
{
	struct zperf_iperf3_params p;

	zassert_ok(zperf_iperf3_params_decode(copy(params_udp64), strlen(params_udp64), &p));

	zassert_true(p.udp);
	zassert_false(p.tcp);
	zassert_equal(p.len, 1000);
	zassert_equal(p.bandwidth, 1000000);
	zassert_equal(p.udp_counters_64bit, 1);
}

ZTEST(zperf_iperf3_proto, test_params_decode_reverse)
{
	struct zperf_iperf3_params p;

	zassert_ok(zperf_iperf3_params_decode(copy(params_reverse), strlen(params_reverse), &p));

	zassert_true(p.reverse);
}

ZTEST(zperf_iperf3_proto, test_params_decode_ignores_unknown_keys)
{
	/* Keys zperf does not model, including nested ones, and a bandwidth
	 * beyond 32 bits
	 */
	static const char json[] =
		"{\"tcp\":true,\"window\":131072,\"congestion\":\"cubic\","
		"\"extra_data\":\"x\",\"future\":{\"a\":[1,{\"b\":2}],\"c\":\"d\"},"
		"\"bandwidth\":10000000000,\"time\":10}";
	struct zperf_iperf3_params p;

	zassert_ok(zperf_iperf3_params_decode(copy(json), strlen(json), &p));

	zassert_true(p.tcp);
	zassert_equal(p.time, 10);
	zassert_equal(p.bandwidth, 10000000000ULL);
}

ZTEST(zperf_iperf3_proto, test_params_decode_rejects_garbage)
{
	static const char *const bad[] = {
		"",
		"not json",
		"{\"tcp\":tru}",
		/* A string where a number is expected */
		"{\"time\":\"10\"}",
	};
	struct zperf_iperf3_params p;

	ARRAY_FOR_EACH(bad, i) {
		zassert_true(zperf_iperf3_params_decode(copy(bad[i]), strlen(bad[i]), &p) < 0,
			     "accepted \"%s\"", bad[i]);
	}
}

static void check_params_encode(const struct zperf_iperf3_params *p, const char *expected)
{
	struct zperf_iperf3_params back;
	uint8_t buf[256];
	char *json = (char *)buf + ZPERF_IPERF3_JSON_LEN_SIZE;
	int len;

	len = zperf_iperf3_params_encode(p, buf, sizeof(buf));
	zassert_equal(len, ZPERF_IPERF3_JSON_LEN_SIZE + strlen(expected), "encode gave %d", len);
	zassert_equal(sys_get_be32(buf), strlen(expected));
	zassert_mem_equal(json, expected, strlen(expected), "got %.*s",
			  len - ZPERF_IPERF3_JSON_LEN_SIZE, json);

	zassert_ok(zperf_iperf3_params_decode(json, len - ZPERF_IPERF3_JSON_LEN_SIZE, &back));
	zassert_equal(back.tcp, p->tcp);
	zassert_equal(back.udp, p->udp);
	zassert_equal(back.bandwidth, p->bandwidth);
}

ZTEST(zperf_iperf3_proto, test_params_encode_matches_iperf3)
{
	/* The same test settings produce the very message iperf 3.16 sends */
	char version[] = "3.16";
	struct zperf_iperf3_params tcp = {
		.tcp = true,
		.time = 1,
		.parallel = 1,
		.len = 131072,
		.pacing_timer = 1000,
		.client_version = { .start = version, .length = sizeof(version) - 1 },
	};
	struct zperf_iperf3_params udp = {
		.udp = true,
		.time = 1,
		.parallel = 1,
		.len = 1000,
		.bandwidth = 1000000,
		.pacing_timer = 1000,
		.client_version = { .start = version, .length = sizeof(version) - 1 },
	};

	check_params_encode(&tcp, params_tcp);
	check_params_encode(&udp,
			    "{\"udp\":true,\"omit\":0,\"time\":1,\"num\":0,\"blockcount\":0,"
			    "\"parallel\":1,\"len\":1000,\"bandwidth\":1000000,"
			    "\"pacing_timer\":1000,\"client_version\":\"3.16\"}");
}

ZTEST(zperf_iperf3_proto, test_params_encode_leaves_out_unset_flags)
{
	/* A server takes a flag to be set whenever its key is present, so
	 * one that is not set must not be sent at all, not even as false.
	 * Options with a value are sent only when they have one.
	 */
	char version[] = "zperf";
	struct zperf_iperf3_params p = {
		.tcp = true,
		.nodelay = true,
		.tos = 16,
		.time = 10,
		.parallel = 1,
		.len = 1024,
		.client_version = { .start = version, .length = sizeof(version) - 1 },
	};

	check_params_encode(&p, "{\"tcp\":true,\"omit\":0,\"time\":10,\"num\":0,"
				"\"blockcount\":0,\"nodelay\":true,\"parallel\":1,"
				"\"len\":1024,\"pacing_timer\":0,\"TOS\":16,"
				"\"client_version\":\"zperf\"}");

	/* And a bandwidth beyond 32 bits survives */
	p = (struct zperf_iperf3_params){ .udp = true, .bandwidth = 10000000000ULL };
	check_params_encode(&p, "{\"udp\":true,\"omit\":0,\"time\":0,\"num\":0,"
				"\"blockcount\":0,\"parallel\":0,\"len\":0,"
				"\"bandwidth\":10000000000,\"pacing_timer\":0,"
				"\"client_version\":\"\"}");
}

ZTEST(zperf_iperf3_proto, test_params_encode_too_small)
{
	struct zperf_iperf3_params p = { .tcp = true };
	uint8_t buf[16];

	zassert_equal(zperf_iperf3_params_encode(&p, buf, sizeof(buf)), -ENOSPC);
}

ZTEST(zperf_iperf3_proto, test_results_encode_receiver)
{
	static const char expected[] =
		"{\"cpu_util_total\":0,\"cpu_util_user\":0,\"cpu_util_system\":0,"
		"\"sender_has_retransmits\":-1,\"streams\":[{\"id\":1,\"bytes\":125000,"
		"\"retransmits\":-1,\"jitter\":0.000041,\"errors\":3,\"packets\":125,"
		"\"start_time\":0,\"end_time\":1.000422}]}";
	struct zperf_iperf3_stream_stats stats = {
		.bytes = 125000,
		.packets = 125,
		.errors = 3,
		.jitter_us = 41,
		.duration_us = 1000422,
	};
	uint8_t buf[512];
	int len;

	len = zperf_iperf3_results_encode(&stats, false, buf, sizeof(buf));
	zassert_equal(len, ZPERF_IPERF3_JSON_LEN_SIZE + sizeof(expected) - 1,
		      "unexpected length %d", len);
	zassert_equal(sys_get_be32(buf), sizeof(expected) - 1);
	zassert_mem_equal(buf + ZPERF_IPERF3_JSON_LEN_SIZE, expected, sizeof(expected) - 1,
			  "got %s", (char *)buf + ZPERF_IPERF3_JSON_LEN_SIZE);
}

ZTEST(zperf_iperf3_proto, test_results_encode_sender)
{
	struct zperf_iperf3_stream_stats stats = { .bytes = 1, .duration_us = 1 };
	uint8_t buf[512];

	zassert_true(zperf_iperf3_results_encode(&stats, true, buf, sizeof(buf)) > 0);
	zassert_not_null(strstr((char *)buf + ZPERF_IPERF3_JSON_LEN_SIZE,
				"\"sender_has_retransmits\":0"));
}

ZTEST(zperf_iperf3_proto, test_results_decode_udp)
{
	struct zperf_iperf3_stream_stats stats;

	zassert_ok(zperf_iperf3_results_decode(copy(results_udp), strlen(results_udp), &stats));

	zassert_equal(stats.bytes, 125000);
	zassert_equal(stats.packets, 125);
	zassert_equal(stats.errors, 0);
	/* 4.1014e-05 s */
	zassert_equal(stats.jitter_us, 41);
	zassert_equal(stats.duration_us, 1000422);
}

ZTEST(zperf_iperf3_proto, test_results_decode_tcp)
{
	struct zperf_iperf3_stream_stats stats;

	zassert_ok(zperf_iperf3_results_decode(copy(results_tcp), strlen(results_tcp), &stats));

	/* More than 32 bits */
	zassert_equal(stats.bytes, 9313976320ULL);
	zassert_equal(stats.jitter_us, 0);
	zassert_equal(stats.duration_us, 1001358);
}

ZTEST(zperf_iperf3_proto, test_results_roundtrip)
{
	struct zperf_iperf3_stream_stats in = {
		.bytes = 9313976320ULL,
		.packets = 7,
		.errors = 2,
		.jitter_us = 1234,
		.duration_us = 10000001,
	};
	struct zperf_iperf3_stream_stats out;
	uint8_t buf[512];
	int len;

	len = zperf_iperf3_results_encode(&in, false, buf, sizeof(buf));
	zassert_true(len > 0);
	zassert_ok(zperf_iperf3_results_decode((char *)buf + ZPERF_IPERF3_JSON_LEN_SIZE,
					       len - ZPERF_IPERF3_JSON_LEN_SIZE, &out));
	zassert_mem_equal(&in, &out, sizeof(in));
}

ZTEST(zperf_iperf3_proto, test_results_decode_without_streams)
{
	static const char json[] = "{\"cpu_util_total\":0,\"cpu_util_user\":0,"
				   "\"cpu_util_system\":0,\"sender_has_retransmits\":-1}";
	struct zperf_iperf3_stream_stats stats;

	zassert_true(zperf_iperf3_results_decode(copy(json), strlen(json), &stats) < 0);
}

ZTEST(zperf_iperf3_proto, test_parse_seconds)
{
	static const struct {
		const char *str;
		uint64_t usec;
	} good[] = {
		{ "0", 0 },
		{ "12", 12000000 },
		{ "1.000422", 1000422 },
		{ "0.0000005", 1 },
		{ "0.0000004", 0 },
		{ "4.1014147773721059e-05", 41 },
		{ "1e-6", 1 },
		{ "1E+1", 10000000 },
		{ "2.5e3", 2500000000ULL },
		{ "1e-400", 0 },
		{ "0e400", 0 },
		/* More digits than fit, beyond the precision kept */
		{ "123456789012345678901e-12", 123456789012346ULL },
	};
	static const char *const bad[] = {
		"", "-1", "abc", ".", "1.2.3", "1e", "1e+", "1x", "1e400",
	};
	uint64_t usec;

	ARRAY_FOR_EACH(good, i) {
		zassert_ok(zperf_iperf3_parse_seconds(good[i].str, strlen(good[i].str), &usec),
			   "rejected \"%s\"", good[i].str);
		zassert_equal(usec, good[i].usec, "\"%s\" gave %llu", good[i].str, usec);
	}

	ARRAY_FOR_EACH(bad, i) {
		zassert_true(zperf_iperf3_parse_seconds(bad[i], strlen(bad[i]), &usec) < 0,
			     "accepted \"%s\"", bad[i]);
	}

	/* The length bounds the number: it need not be NUL terminated */
	zassert_ok(zperf_iperf3_parse_seconds("1.5,\"x\"", 3, &usec));
	zassert_equal(usec, 1500000);
}

ZTEST(zperf_iperf3_proto, test_fmt_seconds)
{
	char buf[32];

	zassert_equal(zperf_iperf3_fmt_seconds(buf, sizeof(buf), 0), 8);
	zassert_str_equal(buf, "0.000000");
	zassert_equal(zperf_iperf3_fmt_seconds(buf, sizeof(buf), 1000422), 8);
	zassert_str_equal(buf, "1.000422");
	zassert_true(zperf_iperf3_fmt_seconds(buf, sizeof(buf), 12345678901ULL) > 0);
	zassert_str_equal(buf, "12345.678901");
	zassert_equal(zperf_iperf3_fmt_seconds(buf, 8, 0), -ENOSPC);
}

ZTEST(zperf_iperf3_proto, test_udp_header)
{
	uint8_t buf[ZPERF_IPERF3_UDP_HDR_SIZE_64];
	uint64_t time_us;
	uint64_t seq;

	zperf_iperf3_udp_hdr_put(buf, 3000004ULL, 0x01020304, false);
	zassert_mem_equal(buf, ((uint8_t[]){ 0, 0, 0, 3, 0, 0, 0, 4, 1, 2, 3, 4 }),
			  ZPERF_IPERF3_UDP_HDR_SIZE);
	zassert_ok(zperf_iperf3_udp_hdr_get(buf, ZPERF_IPERF3_UDP_HDR_SIZE, false, &time_us,
					    &seq));
	zassert_equal(time_us, 3000004ULL);
	zassert_equal(seq, 0x01020304);

	zperf_iperf3_udp_hdr_put(buf, 5, 0x0102030405060708ULL, true);
	zassert_mem_equal(buf + 8, ((uint8_t[]){ 1, 2, 3, 4, 5, 6, 7, 8 }), 8);
	zassert_ok(zperf_iperf3_udp_hdr_get(buf, sizeof(buf), true, &time_us, &seq));
	zassert_equal(seq, 0x0102030405060708ULL);

	zassert_equal(zperf_iperf3_udp_hdr_get(buf, ZPERF_IPERF3_UDP_HDR_SIZE, true, &time_us,
					       &seq), -EINVAL);
	zassert_equal(zperf_iperf3_udp_hdr_get(buf, 11, false, &time_us, &seq), -EINVAL);
}

ZTEST(zperf_iperf3_proto, test_udp_accounting)
{
	static const uint64_t seqs[] = { 1, 2, 4, 3, 6 };
	struct zperf_iperf3_udp_stats stats = { 0 };

	ARRAY_FOR_EACH(seqs, i) {
		zperf_iperf3_udp_account(&stats, seqs[i], 100, 1000);
	}

	/* 4 opens a gap of one, 3 closes it, 6 opens another */
	zassert_equal(stats.packet_count, 6);
	zassert_equal(stats.errors, 1);
	zassert_equal(stats.outoforder, 1);
	zassert_equal(stats.datagrams, 5);
	zassert_equal(stats.bytes, 5000);
	/* Constant transit time: no jitter */
	zassert_equal(zperf_iperf3_udp_jitter_us(&stats), 0);
}

ZTEST(zperf_iperf3_proto, test_udp_jitter)
{
	struct zperf_iperf3_udp_stats stats = { 0 };

	/* The first transit time only sets the reference, however large the
	 * clock offset it contains
	 */
	zperf_iperf3_udp_account(&stats, 1, 1000000, 100);
	zassert_equal(zperf_iperf3_udp_jitter_us(&stats), 0);

	/* A transit change of 1600 us moves the jitter by 1/16 of it */
	zperf_iperf3_udp_account(&stats, 2, 1001600, 100);
	zassert_equal(zperf_iperf3_udp_jitter_us(&stats), 100);

	/* Repeated changes converge towards their size */
	for (uint64_t seq = 3; seq < 200; seq++) {
		zperf_iperf3_udp_account(&stats, seq, (seq % 2 == 0) ? 1001600 : 1000000, 100);
	}
	zassert_within(zperf_iperf3_udp_jitter_us(&stats), 1600, 16);
}

ZTEST(zperf_iperf3_proto, test_cookie)
{
	char a[ZPERF_IPERF3_COOKIE_SIZE];
	char b[ZPERF_IPERF3_COOKIE_SIZE];

	zperf_iperf3_make_cookie(a);
	zperf_iperf3_make_cookie(b);

	zassert_equal(strlen(a), ZPERF_IPERF3_COOKIE_SIZE - 1);
	zassert_equal(strspn(a, "abcdefghijklmnopqrstuvwxyz234567"),
		      ZPERF_IPERF3_COOKIE_SIZE - 1);
	zassert_true(memcmp(a, b, sizeof(a)) != 0);
}

ZTEST(zperf_iperf3_proto, test_udp_connect_messages)
{
	/* Text, so the same bytes whatever the byte order of either end */
	zassert_mem_equal(zperf_iperf3_udp_connect_msg, "9876", ZPERF_IPERF3_UDP_CONNECT_SIZE);
	zassert_mem_equal(zperf_iperf3_udp_connect_reply, "6789",
			  ZPERF_IPERF3_UDP_CONNECT_SIZE);
}

ZTEST_SUITE(zperf_iperf3_proto, NULL, NULL, NULL, NULL, NULL);
