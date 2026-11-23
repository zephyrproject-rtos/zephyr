/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "zperf_iperf3.h"

/* The set-up datagrams are text so that they read the same in either byte
 * order.
 */
const uint8_t zperf_iperf3_udp_connect_msg[ZPERF_IPERF3_UDP_CONNECT_SIZE] = "9876";
const uint8_t zperf_iperf3_udp_connect_reply[ZPERF_IPERF3_UDP_CONNECT_SIZE] = "6789";

static const struct json_obj_descr params_decode_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, tcp, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, udp, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, sctp, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, reverse, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, bidirectional, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, nodelay, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, omit, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, time, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, parallel, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, len, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zperf_iperf3_params, "TOS", tos, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, pacing_timer, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, udp_counters_64bit, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, bandwidth, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, num, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, blockcount, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, client_version, JSON_TOK_OPAQUE),
};

/* What a client sends, in the order iperf3 sends it. Servers up to at
 * least 3.16 take a flag such as "udp" or "nodelay" to be set whenever the
 * key is present, whatever its value, and apply "bandwidth" and "TOS" when
 * present, so those keys are only sent when they have something to say.
 */
enum {
	PARAM_TCP,
	PARAM_UDP,
	PARAM_OMIT,
	PARAM_TIME,
	PARAM_NUM,
	PARAM_BLOCKCOUNT,
	PARAM_NODELAY,
	PARAM_PARALLEL,
	PARAM_LEN,
	PARAM_BANDWIDTH,
	PARAM_PACING_TIMER,
	PARAM_TOS,
	PARAM_CLIENT_VERSION,
	PARAM_COUNT,
};

static const struct json_obj_descr params_encode_descr[PARAM_COUNT] = {
	[PARAM_TCP] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, tcp, JSON_TOK_TRUE),
	[PARAM_UDP] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, udp, JSON_TOK_TRUE),
	[PARAM_OMIT] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, omit, JSON_TOK_NUMBER),
	[PARAM_TIME] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, time, JSON_TOK_NUMBER),
	[PARAM_NUM] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, num, JSON_TOK_UINT64),
	[PARAM_BLOCKCOUNT] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, blockcount,
						 JSON_TOK_UINT64),
	[PARAM_NODELAY] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, nodelay, JSON_TOK_TRUE),
	[PARAM_PARALLEL] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, parallel,
					       JSON_TOK_NUMBER),
	[PARAM_LEN] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, len, JSON_TOK_NUMBER),
	[PARAM_BANDWIDTH] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, bandwidth,
						JSON_TOK_UINT64),
	[PARAM_PACING_TIMER] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, pacing_timer,
						   JSON_TOK_NUMBER),
	[PARAM_TOS] = JSON_OBJ_DESCR_PRIM_NAMED(struct zperf_iperf3_params, "TOS", tos,
						JSON_TOK_NUMBER),
	[PARAM_CLIENT_VERSION] = JSON_OBJ_DESCR_PRIM(struct zperf_iperf3_params, client_version,
						     JSON_TOK_OPAQUE),
};

/* The results message. Fractional numbers are kept as raw number text, so
 * that neither side needs floating point support.
 */
struct results_stream {
	int32_t id;
	uint64_t bytes;
	int32_t retransmits;
	struct json_obj_token jitter;
	uint64_t errors;
	uint64_t packets;
	struct json_obj_token start_time;
	struct json_obj_token end_time;
};

struct results {
	struct json_obj_token cpu_util_total;
	struct json_obj_token cpu_util_user;
	struct json_obj_token cpu_util_system;
	int32_t sender_has_retransmits;
	struct results_stream streams[1];
	size_t streams_len;
};

static const struct json_obj_descr results_stream_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct results_stream, id, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct results_stream, bytes, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct results_stream, retransmits, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct results_stream, jitter, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct results_stream, errors, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct results_stream, packets, JSON_TOK_UINT64),
	JSON_OBJ_DESCR_PRIM(struct results_stream, start_time, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct results_stream, end_time, JSON_TOK_FLOAT),
};

static const struct json_obj_descr results_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct results, cpu_util_total, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct results, cpu_util_user, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct results, cpu_util_system, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct results, sender_has_retransmits, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct results, streams, 1, streams_len, results_stream_descr,
				 ARRAY_SIZE(results_stream_descr)),
};

/* A single stream is always numbered 1 */
#define STREAM_ID 1

/* Longest seconds value: 10 integer digits, the point and 6 decimals */
#define SECONDS_STR_SIZE (10 + 1 + 6 + 1)

void zperf_iperf3_make_cookie(char cookie[ZPERF_IPERF3_COOKIE_SIZE])
{
	static const char chars[] = "abcdefghijklmnopqrstuvwxyz234567";
	uint8_t rnd[ZPERF_IPERF3_COOKIE_SIZE - 1];

	sys_rand_get(rnd, sizeof(rnd));

	/* 32 characters divide 256 evenly, so the modulo adds no bias */
	for (size_t i = 0; i < sizeof(rnd); i++) {
		cookie[i] = chars[rnd[i] % (sizeof(chars) - 1)];
	}

	cookie[ZPERF_IPERF3_COOKIE_SIZE - 1] = '\0';
}

int zperf_iperf3_params_decode(char *json, size_t len, struct zperf_iperf3_params *params)
{
	int64_t ret;

	memset(params, 0, sizeof(*params));

	ret = json_obj_parse(json, len, params_decode_descr, ARRAY_SIZE(params_decode_descr),
			     params);

	return (ret < 0) ? (int)ret : 0;
}

static int encode_with_prefix(const struct json_obj_descr *descr, size_t descr_len,
			      const void *val, uint8_t *buf, size_t size)
{
	ssize_t json_len;
	int ret;

	json_len = json_calc_encoded_len(descr, descr_len, val);
	if (json_len < 0) {
		return (int)json_len;
	}

	/* The encoder also writes a terminating NUL */
	if (size < ZPERF_IPERF3_JSON_LEN_SIZE + (size_t)json_len + 1U) {
		return -ENOSPC;
	}

	ret = json_obj_encode_buf(descr, descr_len, val, (char *)buf + ZPERF_IPERF3_JSON_LEN_SIZE,
				  size - ZPERF_IPERF3_JSON_LEN_SIZE);
	if (ret < 0) {
		return ret;
	}

	sys_put_be32((uint32_t)json_len, buf);

	return ZPERF_IPERF3_JSON_LEN_SIZE + (int)json_len;
}

int zperf_iperf3_params_encode(const struct zperf_iperf3_params *params, uint8_t *buf,
			       size_t size)
{
	struct json_obj_descr descr[PARAM_COUNT];
	size_t count = 0;

	for (size_t i = 0; i < PARAM_COUNT; i++) {
		bool send;

		switch (i) {
		case PARAM_TCP:
			send = params->tcp;
			break;
		case PARAM_UDP:
			send = params->udp;
			break;
		case PARAM_NODELAY:
			send = params->nodelay;
			break;
		case PARAM_BANDWIDTH:
			send = (params->bandwidth != 0U);
			break;
		case PARAM_TOS:
			send = (params->tos != 0);
			break;
		default:
			send = true;
			break;
		}

		if (send) {
			descr[count++] = params_encode_descr[i];
		}
	}

	return encode_with_prefix(descr, count, params, buf, size);
}

int zperf_iperf3_results_encode(const struct zperf_iperf3_stream_stats *stats, bool sender,
				uint8_t *buf, size_t size)
{
	/* No CPU utilisation is measured, and no retransmission count is
	 * known: a sender reports it has none to give, a receiver that the
	 * question does not apply.
	 */
	static const char zero_str[] = "0";
	/* The token is only read when encoding */
	char *zero = (char *)zero_str;
	char jitter[SECONDS_STR_SIZE];
	char end_time[SECONDS_STR_SIZE];
	struct results res = {
		.cpu_util_total = { .start = zero, .length = 1 },
		.cpu_util_user = { .start = zero, .length = 1 },
		.cpu_util_system = { .start = zero, .length = 1 },
		.sender_has_retransmits = sender ? 0 : -1,
		.streams_len = 1,
	};
	struct results_stream *stream = &res.streams[0];
	int len;

	stream->id = STREAM_ID;
	stream->bytes = stats->bytes;
	stream->retransmits = -1;
	stream->errors = stats->errors;
	stream->packets = stats->packets;
	stream->start_time = (struct json_obj_token){ .start = zero, .length = 1 };

	len = zperf_iperf3_fmt_seconds(jitter, sizeof(jitter), stats->jitter_us);
	if (len < 0) {
		return len;
	}

	stream->jitter = (struct json_obj_token){ .start = jitter, .length = len };

	len = zperf_iperf3_fmt_seconds(end_time, sizeof(end_time), stats->duration_us);
	if (len < 0) {
		return len;
	}

	stream->end_time = (struct json_obj_token){ .start = end_time, .length = len };

	return encode_with_prefix(results_descr, ARRAY_SIZE(results_descr), &res, buf, size);
}

static int token_seconds(const struct json_obj_token *token, uint64_t *usec)
{
	if (token->start == NULL) {
		*usec = 0U;
		return 0;
	}

	return zperf_iperf3_parse_seconds(token->start, token->length, usec);
}

int zperf_iperf3_results_decode(char *json, size_t len, struct zperf_iperf3_stream_stats *stats)
{
	struct results res = { 0 };
	const struct results_stream *stream = &res.streams[0];
	uint64_t start_us;
	uint64_t end_us;
	int64_t ret;

	ret = json_obj_parse(json, len, results_descr, ARRAY_SIZE(results_descr), &res);
	if (ret < 0) {
		return (int)ret;
	}

	if (res.streams_len == 0U) {
		return -EINVAL;
	}

	memset(stats, 0, sizeof(*stats));
	stats->bytes = stream->bytes;
	stats->packets = stream->packets;
	stats->errors = stream->errors;

	ret = token_seconds(&stream->jitter, &stats->jitter_us);
	if (ret < 0) {
		return (int)ret;
	}

	ret = token_seconds(&stream->start_time, &start_us);
	if (ret < 0) {
		return (int)ret;
	}

	ret = token_seconds(&stream->end_time, &end_us);
	if (ret < 0) {
		return (int)ret;
	}

	stats->duration_us = (end_us > start_us) ? (end_us - start_us) : 0U;

	return 0;
}

int zperf_iperf3_fmt_seconds(char *buf, size_t size, uint64_t usec)
{
	uint64_t secs = usec / USEC_PER_SEC;
	int len;

	/* 32 bit seconds keep this printable with a minimal cbprintf */
	if (secs > UINT32_MAX) {
		return -ERANGE;
	}

	len = snprintk(buf, size, "%u.%06u", (unsigned int)secs,
		       (unsigned int)(usec % USEC_PER_SEC));
	if (len < 0 || (size_t)len >= size) {
		return -ENOSPC;
	}

	return len;
}

/* Digits beyond this are dropped: they are far below a microsecond for any
 * value that fits
 */
#define PARSE_MANTISSA_MAX 100000000000000000ULL

int zperf_iperf3_parse_seconds(const char *str, size_t len, uint64_t *usec)
{
	uint64_t divisor = 1U;
	uint64_t mantissa = 0U;
	bool any_digit = false;
	int exp10 = 6; /* result is mantissa * 10^exp10 microseconds */
	size_t i = 0;

	/* Integer part */
	for (; i < len && isdigit((unsigned char)str[i]) != 0; i++) {
		any_digit = true;
		if (mantissa < PARSE_MANTISSA_MAX) {
			mantissa = mantissa * 10U + (uint64_t)(str[i] - '0');
		} else {
			exp10++;
		}
	}

	/* Fraction */
	if (i < len && str[i] == '.') {
		for (i++; i < len && isdigit((unsigned char)str[i]) != 0; i++) {
			any_digit = true;
			if (mantissa < PARSE_MANTISSA_MAX) {
				mantissa = mantissa * 10U + (uint64_t)(str[i] - '0');
				exp10--;
			}
		}
	}

	if (!any_digit) {
		return -EINVAL;
	}

	/* Exponent */
	if (i < len && (str[i] == 'e' || str[i] == 'E')) {
		bool negative = false;
		bool exp_digit = false;
		int exponent = 0;

		i++;
		if (i < len && (str[i] == '+' || str[i] == '-')) {
			negative = (str[i] == '-');
			i++;
		}

		for (; i < len && isdigit((unsigned char)str[i]) != 0; i++) {
			exp_digit = true;
			if (exponent < 1000) {
				exponent = exponent * 10 + (str[i] - '0');
			}
		}

		if (!exp_digit) {
			return -EINVAL;
		}

		exp10 += negative ? -exponent : exponent;
	}

	if (i != len) {
		return -EINVAL;
	}

	if (mantissa == 0U) {
		*usec = 0U;
		return 0;
	}

	if (exp10 >= 0) {
		for (; exp10 > 0; exp10--) {
			if (mantissa > UINT64_MAX / 10U) {
				return -ERANGE;
			}
			mantissa *= 10U;
		}
		*usec = mantissa;
		return 0;
	}

	if (exp10 < -19) {
		*usec = 0U;
		return 0;
	}

	for (; exp10 < 0; exp10++) {
		divisor *= 10U;
	}

	/* Round to the nearest microsecond */
	*usec = mantissa / divisor + (((mantissa % divisor) * 2U >= divisor) ? 1U : 0U);

	return 0;
}

void zperf_iperf3_udp_hdr_put(uint8_t *buf, uint64_t time_us, uint64_t seq, bool counters_64bit)
{
	sys_put_be32((uint32_t)(time_us / USEC_PER_SEC), buf);
	sys_put_be32((uint32_t)(time_us % USEC_PER_SEC), buf + 4);

	if (counters_64bit) {
		sys_put_be64(seq, buf + 8);
	} else {
		sys_put_be32((uint32_t)seq, buf + 8);
	}
}

int zperf_iperf3_udp_hdr_get(const uint8_t *buf, size_t len, bool counters_64bit,
			     uint64_t *time_us, uint64_t *seq)
{
	size_t hdr_size = counters_64bit ? ZPERF_IPERF3_UDP_HDR_SIZE_64 : ZPERF_IPERF3_UDP_HDR_SIZE;

	if (len < hdr_size) {
		return -EINVAL;
	}

	*time_us = (uint64_t)sys_get_be32(buf) * USEC_PER_SEC + sys_get_be32(buf + 4);
	*seq = counters_64bit ? sys_get_be64(buf + 8) : sys_get_be32(buf + 8);

	return 0;
}

void zperf_iperf3_udp_account(struct zperf_iperf3_udp_stats *stats, uint64_t seq,
			      int64_t transit_us, size_t len)
{
	int64_t delta;

	stats->bytes += len;
	stats->datagrams++;

	/* A sequence number beyond the next expected one counts the ones
	 * skipped as lost. One behind it is out of order, and makes up for
	 * a loss counted when the gap opened.
	 */
	if (seq >= stats->packet_count + 1U) {
		if (seq > stats->packet_count + 1U) {
			stats->errors += seq - 1U - stats->packet_count;
		}
		stats->packet_count = seq;
	} else {
		stats->outoforder++;
		if (stats->errors > 0U) {
			stats->errors--;
		}
	}

	/* RFC 1889 jitter. The first datagram only sets the reference: its
	 * transit time includes the offset between the two clocks.
	 */
	if (!stats->have_transit) {
		stats->prev_transit_us = transit_us;
		stats->have_transit = true;
	}

	delta = transit_us - stats->prev_transit_us;
	if (delta < 0) {
		delta = -delta;
	}

	stats->prev_transit_us = transit_us;
	stats->jitter_q4_us += (uint64_t)delta - stats->jitter_q4_us / 16U;
}

int zperf_iperf3_send_all(int sock, const void *buf, size_t len, int flags)
{
	const uint8_t *pos = buf;

	while (len > 0U) {
		ssize_t ret = zsock_send(sock, pos, len, flags);

		if (ret < 0) {
			return -errno;
		}

		pos += ret;
		len -= (size_t)ret;
	}

	return 0;
}

int zperf_iperf3_recv_all(int sock, void *buf, size_t len)
{
	uint8_t *pos = buf;

	while (len > 0U) {
		ssize_t ret = zsock_recv(sock, pos, len, 0);

		if (ret < 0) {
			return -errno;
		}

		if (ret == 0) {
			return -ECONNRESET;
		}

		pos += ret;
		len -= (size_t)ret;
	}

	return 0;
}

int zperf_iperf3_send_state(int sock, int8_t state, int flags)
{
	return zperf_iperf3_send_all(sock, &state, sizeof(state), flags);
}

const char *zperf_iperf3_strerror(int32_t error)
{
	switch (error) {
	case ZPERF_IPERF3_IENUMSTREAMS:
		return "too many parallel streams";
	case ZPERF_IPERF3_IEBLOCKSIZE:
		return "block size too large";
	case ZPERF_IPERF3_IEUNIMP:
		return "not implemented";
	case ZPERF_IPERF3_IERECVPARAMS:
		return "unable to receive parameters";
	default:
		return "server error";
	}
}
