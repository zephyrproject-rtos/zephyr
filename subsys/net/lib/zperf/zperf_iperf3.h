/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The iperf3 protocol: control channel messages, JSON bodies and the UDP
 * stream format, shared by the zperf iperf3 client and server.
 */

#ifndef ZPERF_IPERF3_H_
#define ZPERF_IPERF3_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/data/json.h>

#define ZPERF_IPERF3_DEFAULT_PORT 5201

/* A client identifies its test with 36 characters and a NUL */
#define ZPERF_IPERF3_COOKIE_SIZE 37

/* Size of the length prefix in front of every JSON message */
#define ZPERF_IPERF3_JSON_LEN_SIZE 4

/* Control channel states, each sent as a single signed byte */
enum zperf_iperf3_state {
	ZPERF_IPERF3_TEST_START = 1,
	ZPERF_IPERF3_TEST_RUNNING = 2,
	ZPERF_IPERF3_TEST_END = 4,
	ZPERF_IPERF3_PARAM_EXCHANGE = 9,
	ZPERF_IPERF3_CREATE_STREAMS = 10,
	ZPERF_IPERF3_SERVER_TERMINATE = 11,
	ZPERF_IPERF3_CLIENT_TERMINATE = 12,
	ZPERF_IPERF3_EXCHANGE_RESULTS = 13,
	ZPERF_IPERF3_DISPLAY_RESULTS = 14,
	ZPERF_IPERF3_IPERF_DONE = 16,
	ZPERF_IPERF3_ACCESS_DENIED = -1,
	ZPERF_IPERF3_SERVER_ERROR = -2,
};

/* Error numbers a SERVER_ERROR carries, as iperf3 clients print them. The
 * state byte is followed by the error number and an errno, both 32 bit
 * big endian.
 */
#define ZPERF_IPERF3_IENUMSTREAMS 6
#define ZPERF_IPERF3_IEBLOCKSIZE  7
#define ZPERF_IPERF3_IEUNIMP      13
#define ZPERF_IPERF3_IERECVPARAMS 114
#define ZPERF_IPERF3_SERVER_ERROR_SIZE 9

/* A UDP stream is set up with one datagram each way. Older servers reply
 * with a numeric value instead, written in their own byte order.
 */
#define ZPERF_IPERF3_UDP_CONNECT_SIZE 4
extern const uint8_t zperf_iperf3_udp_connect_msg[ZPERF_IPERF3_UDP_CONNECT_SIZE];
extern const uint8_t zperf_iperf3_udp_connect_reply[ZPERF_IPERF3_UDP_CONNECT_SIZE];

/* UDP datagrams start with the send time and a sequence number counted
 * from 1, 32 bit unless the client asked for 64 bit counters.
 */
#define ZPERF_IPERF3_UDP_HDR_SIZE    12
#define ZPERF_IPERF3_UDP_HDR_SIZE_64 16

/* Test parameters, sent by the client after PARAM_EXCHANGE. Only the keys
 * zperf acts on are modelled; anything else a client sends is ignored.
 */
struct zperf_iperf3_params {
	bool tcp;
	bool udp;
	bool sctp;
	bool reverse;
	bool bidirectional;
	bool nodelay;
	int32_t omit;
	int32_t time;
	int32_t parallel;
	int32_t len;
	int32_t tos;
	int32_t pacing_timer;
	int32_t udp_counters_64bit;
	uint64_t bandwidth;
	uint64_t num;
	uint64_t blockcount;
	/* Raw text of the version string, not NUL terminated */
	struct json_obj_token client_version;
};

/* One side's view of a single stream, exchanged after TEST_END */
struct zperf_iperf3_stream_stats {
	uint64_t bytes;
	/* UDP only: highest sequence number seen by a receiver, or datagrams
	 * sent by a sender
	 */
	uint64_t packets;
	/* UDP receiver only: datagrams lost */
	uint64_t errors;
	uint64_t jitter_us;
	uint64_t duration_us;
};

/* Receive side accounting of a UDP stream, following the rules an iperf3
 * receiver applies so that loss and jitter mean the same on both ends.
 */
struct zperf_iperf3_udp_stats {
	uint64_t bytes;
	uint64_t datagrams;
	/* Highest sequence number seen */
	uint64_t packet_count;
	uint64_t errors;
	uint64_t outoforder;
	int64_t prev_transit_us;
	bool have_transit;
	/* Smoothed jitter in microseconds, times 16 */
	uint64_t jitter_q4_us;
};

void zperf_iperf3_make_cookie(char cookie[ZPERF_IPERF3_COOKIE_SIZE]);

int zperf_iperf3_params_decode(char *json, size_t len, struct zperf_iperf3_params *params);

/* The encoders write the length prefix followed by the JSON text into buf and
 * return the total number of bytes, or a negative errno.
 */
int zperf_iperf3_params_encode(const struct zperf_iperf3_params *params, uint8_t *buf,
			       size_t size);
int zperf_iperf3_results_encode(const struct zperf_iperf3_stream_stats *stats, bool sender,
				uint8_t *buf, size_t size);

/* Decodes the first stream of a results message */
int zperf_iperf3_results_decode(char *json, size_t len, struct zperf_iperf3_stream_stats *stats);

/* Seconds in the form a JSON number takes, and back. Parsing accepts an
 * exponent, since iperf3 writes small values such as jitter that way.
 */
int zperf_iperf3_fmt_seconds(char *buf, size_t size, uint64_t usec);
int zperf_iperf3_parse_seconds(const char *str, size_t len, uint64_t *usec);

void zperf_iperf3_udp_hdr_put(uint8_t *buf, uint64_t time_us, uint64_t seq, bool counters_64bit);
int zperf_iperf3_udp_hdr_get(const uint8_t *buf, size_t len, bool counters_64bit,
			     uint64_t *time_us, uint64_t *seq);

void zperf_iperf3_udp_account(struct zperf_iperf3_udp_stats *stats, uint64_t seq,
			      int64_t transit_us, size_t len);

static inline uint64_t zperf_iperf3_udp_jitter_us(const struct zperf_iperf3_udp_stats *stats)
{
	return stats->jitter_q4_us / 16U;
}

/* I/O on the control channel. With ZSOCK_MSG_DONTWAIT in flags, send_all
 * returns -EAGAIN when the socket cannot take the whole message, possibly
 * after sending part of it, which leaves the control channel unusable.
 */
int zperf_iperf3_send_all(int sock, const void *buf, size_t len, int flags);
int zperf_iperf3_recv_all(int sock, void *buf, size_t len);
int zperf_iperf3_send_state(int sock, int8_t state, int flags);

const char *zperf_iperf3_strerror(int32_t error);

#endif /* ZPERF_IPERF3_H_ */
