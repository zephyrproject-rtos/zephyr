/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "signaling_tests.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <peer_connection.h>

#include "libpeer_endpoint.h"
#include "libpeer_offerer.h"
#include "platform.h"
#include "signaling_client_zephyr.h"
#include "signaling_test_server.h"

#define TEST_SDP                                                                                   \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=sendrecv\r\na=rtpmap:0 PCMU/8000\r\n"
#define TEST_SDP_NO_FINAL_CRLF                                                                     \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=sendrecv\r\na=rtpmap:0 PCMU/8000"
#define TEST_SDP_VIDEO                                                                             \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=video 9 UDP/TLS/RTP/SAVPF 96\r\na=rtpmap:96 H264/90000\r\n"
#define TEST_SDP_H264_PCMU                                                                         \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=video 9 UDP/TLS/RTP/SAVPF 96\r\na=sendrecv\r\na=rtpmap:96 H264/90000\r\n"               \
	"a=fmtp:96 packetization-mode=1;profile-level-id=42e01f\r\n"                               \
	"m=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=sendrecv\r\na=rtpmap:0 PCMU/8000\r\n"
#define TEST_SDP_H264_PCMU_BAD_PROFILE                                                             \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=video 9 UDP/TLS/RTP/SAVPF 96\r\na=sendrecv\r\na=rtpmap:96 H264/90000\r\n"               \
	"a=fmtp:96 packetization-mode=1;profile-level-id=64001f\r\n"                               \
	"m=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=sendrecv\r\na=rtpmap:0 PCMU/8000\r\n"
#define TEST_SDP_PORT_ZERO                                                                         \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=audio 0 UDP/TLS/RTP/SAVPF 0\r\na=sendrecv\r\na=rtpmap:0 PCMU/8000\r\n"
#define TEST_SDP_RECVONLY                                                                          \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"m=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=recvonly\r\na=rtpmap:0 PCMU/8000\r\n"
#define TEST_SDP_RTPMAP_BEFORE_MEDIA                                                               \
	"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"                                      \
	"a=rtpmap:0 PCMU/8000\r\nm=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=sendrecv\r\n"
#define TEST_SDP_PCMU_AND_VIDEO TEST_SDP "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
#define TEST_SDP_BARE_LF        "v=0\nm=audio 9 UDP/TLS/RTP/SAVPF 0\n"
/* Contains a NUL, so it needs an explicit length. */
#define TEST_SDP_NUL            "v=0\r\nm=audio 9 UDP/TLS/RTP/SAVPF 0\r\nx=a\0b\r\n"

#define RAW(literal)          .raw = literal, .raw_length = sizeof(literal) - 1U
#define STATUS_OK             "200 OK"
#define TYPE_SDP              "application/sdp"
#define OMIT                  ""
#define LENGTH_AUTO           NULL
#define STALL_TIMEOUT         300U
#define CASE_TIMEOUT          3000U
#define WIRE_BYTES            8448U
#define OVERSIZE_HEADER_BYTES (SIGNALING_HEADER_MAX_BYTES + 128U)

struct matrix_case {
	const char *name;
	enum signaling_result expected;
	/* Response fields. NULL selects the well-formed default, OMIT drops the header. */
	const char *status;
	const char *type;
	const char *length;
	const char *extra;
	bool oversize_header;
	const char *body;
	size_t body_length;
	const char *trailer;
	/* Complete raw response replacing every field above. */
	const char *raw;
	size_t raw_length;
	size_t chunk_bytes;
	bool hold_open;
};

static const struct matrix_case matrix_cases[] = {
	{.name = "ok_fragmented", .expected = SIGNALING_OK, .chunk_bytes = 7U},
	{.name = "ok_type_case_and_space", .expected = SIGNALING_OK, .type = "Application/SDP "},
	{.name = "status_500", .expected = SIGNALING_BAD_STATUS, .status = "500 Internal Error"},
	{.name = "status_302_redirect",
	 .expected = SIGNALING_BAD_STATUS,
	 .status = "302 Found",
	 .extra = "Location: http://127.0.0.1/\r\n"},
	{.name = "status_http2",
	 .expected = SIGNALING_BAD_STATUS,
	 RAW("HTTP/2 200 OK\r\nContent-Type: application/sdp\r\nContent-Length: 10\r\n\r\n")},
	{.name = "status_garbage", .expected = SIGNALING_BAD_STATUS, RAW("garbage\r\n\r\n")},
	{.name = "type_text_plain", .expected = SIGNALING_BAD_CONTENT_TYPE, .type = "text/plain"},
	{.name = "type_missing", .expected = SIGNALING_BAD_CONTENT_TYPE, .type = OMIT},
	{.name = "type_duplicate",
	 .expected = SIGNALING_BAD_CONTENT_TYPE,
	 .extra = "Content-Type: application/sdp\r\n"},
	{.name = "type_parameters",
	 .expected = SIGNALING_BAD_CONTENT_TYPE,
	 .type = "application/sdp; charset=utf-8"},
	{.name = "length_zero", .expected = SIGNALING_BAD_CONTENT_LENGTH, .length = "0"},
	{.name = "length_missing", .expected = SIGNALING_BAD_CONTENT_LENGTH, .length = OMIT},
	{.name = "length_empty", .expected = SIGNALING_BAD_CONTENT_LENGTH, .length = " "},
	{.name = "length_non_numeric", .expected = SIGNALING_BAD_CONTENT_LENGTH, .length = "12x"},
	{.name = "length_negative", .expected = SIGNALING_BAD_CONTENT_LENGTH, .length = "-1"},
	{.name = "length_duplicate",
	 .expected = SIGNALING_BAD_CONTENT_LENGTH,
	 .extra = "Content-Length: 51\r\n"},
	{.name = "length_oversize", .expected = SIGNALING_OVERSIZE, .length = "9000"},
	{.name = "length_overflow",
	 .expected = SIGNALING_OVERSIZE,
	 .length = "99999999999999999999"},
	{.name = "header_transfer_encoding",
	 .expected = SIGNALING_BAD_HEADER,
	 .extra = "Transfer-Encoding: chunked\r\n"},
	{.name = "header_without_colon",
	 .expected = SIGNALING_BAD_HEADER,
	 .extra = "Broken header\r\n"},
	{.name = "header_space_in_name",
	 .expected = SIGNALING_BAD_HEADER,
	 .extra = "X Y: value\r\n"},
	{.name = "header_obsolete_fold",
	 .expected = SIGNALING_BAD_HEADER,
	 .extra = " folded: value\r\n"},
	{.name = "header_bare_lf", .expected = SIGNALING_BAD_HEADER, .extra = "X: a\nb\r\n"},
	{.name = "header_nul",
	 .expected = SIGNALING_BAD_HEADER,
	 RAW("HTTP/1.1 200 OK\r\nX:\0bad\r\n\r\n")},
	{.name = "header_block_oversize", .expected = SIGNALING_OVERSIZE, .oversize_header = true},
	{.name = "body_truncated",
	 .expected = SIGNALING_TRUNCATED,
	 .length = "100",
	 .body = "v=0\r\nm=audio 9 UDP/TLS/RTP/SAVPF 0\r\n"},
	{.name = "body_nul",
	 .expected = SIGNALING_BAD_BODY,
	 .body = TEST_SDP_NUL,
	 .body_length = sizeof(TEST_SDP_NUL) - 1U},
	{.name = "body_garbage", .expected = SIGNALING_BAD_BODY, .body = "garbage\r\n"},
	{.name = "body_no_final_crlf",
	 .expected = SIGNALING_BAD_BODY,
	 .body = TEST_SDP_NO_FINAL_CRLF},
	{.name = "body_bare_lf", .expected = SIGNALING_BAD_BODY, .body = TEST_SDP_BARE_LF},
	{.name = "body_trailing_bytes", .expected = SIGNALING_BAD_BODY, .trailer = "unexpected"},
	{.name = "peer_closes_without_response", .expected = SIGNALING_TRUNCATED, RAW("")},
	{.name = "stall_silent", .expected = SIGNALING_TIMEOUT, RAW(""), .hold_open = true},
	{.name = "stall_partial_header",
	 .expected = SIGNALING_TIMEOUT,
	 RAW("HTTP/1.1 200 OK\r\nContent-Ty"),
	 .hold_open = true},
	{.name = "stall_partial_body",
	 .expected = SIGNALING_TIMEOUT,
	 .length = "200",
	 .hold_open = true},
};

static const char *const bad_urls[] = {
	"https://127.0.0.1/offer",
	"http://127.0.0.1",
	"http:///offer",
	"http://127.0.0.1:0/offer",
	"http://127.0.0.1:65536/offer",
	"http://127.0.0.1:/offer",
	"http://127.0.0.1:80x/offer",
	"http://user@127.0.0.1/offer",
	"http://127.0.0.1/offer#fragment",
	"http://127.0.0.1/of fer",
	"http://127.0.0.1/offer\r\nInjected: value",
	"http://example.com/offer",
	"http://256.0.0.1/offer",
	"http://[::1]/offer",
	"ftp://127.0.0.1/offer",
	"",
};

static char wire_buffer[WIRE_BYTES];
static char sdp_scratch[SIGNALING_SDP_MAX_BYTES + 2U];
static char answer_buffer[SIGNALING_ANSWER_BUFFER_BYTES];
static int matrix_cases_run;
static int matrix_failures;

static bool cancel_when_due(void *user_data)
{
	return k_uptime_get() >= *(const int64_t *)user_data;
}

/* A complete one-media SDP of exactly @p length bytes, padded with an attribute line. */
static void fill_sdp(char *buffer, size_t length)
{
	static const char head[] = "v=0\r\nm=audio 9 UDP/TLS/RTP/SAVPF 0\r\na=";

	memset(buffer, 'x', length);
	memcpy(buffer, head, sizeof(head) - 1U);
	memcpy(buffer + length - 2U, "\r\n", 2U);
	buffer[length] = '\0';
}

static size_t build_wire(const struct matrix_case *test)
{
	const char *body = test->body != NULL ? test->body : TEST_SDP;
	size_t body_length = test->body_length != 0U ? test->body_length : strlen(body);
	size_t used;

	if (test->raw != NULL) {
		memcpy(wire_buffer, test->raw, test->raw_length);
		return test->raw_length;
	}
	used = (size_t)snprintk(wire_buffer, sizeof(wire_buffer), "HTTP/1.1 %s\r\n",
				test->status != NULL ? test->status : STATUS_OK);
	if (test->type == NULL || test->type[0] != '\0') {
		used += (size_t)snprintk(wire_buffer + used, sizeof(wire_buffer) - used,
					 "Content-Type: %s\r\n",
					 test->type != NULL ? test->type : TYPE_SDP);
	}
	if (test->length == NULL) {
		used += (size_t)snprintk(wire_buffer + used, sizeof(wire_buffer) - used,
					 "Content-Length: %zu\r\n", body_length);
	} else if (test->length[0] != '\0') {
		used += (size_t)snprintk(wire_buffer + used, sizeof(wire_buffer) - used,
					 "Content-Length: %s\r\n", test->length);
	}
	if (test->oversize_header) {
		memcpy(wire_buffer + used, "X: ", 3U);
		memset(wire_buffer + used + 3U, 'x', OVERSIZE_HEADER_BYTES);
		memcpy(wire_buffer + used + 3U + OVERSIZE_HEADER_BYTES, "\r\n", 3U);
		used += 3U + OVERSIZE_HEADER_BYTES + 2U;
	} else if (test->extra != NULL) {
		used += (size_t)snprintk(wire_buffer + used, sizeof(wire_buffer) - used, "%s",
					 test->extra);
	}
	used += (size_t)snprintk(wire_buffer + used, sizeof(wire_buffer) - used,
				 "Connection: close\r\n\r\n");
	if (used + body_length + 16U >= sizeof(wire_buffer)) {
		return 0U;
	}
	memcpy(wire_buffer + used, body, body_length);
	used += body_length;
	if (test->trailer != NULL) {
		memcpy(wire_buffer + used, test->trailer, strlen(test->trailer));
		used += strlen(test->trailer);
	}
	return used;
}

static void report_case(const char *name, const char *expected, const char *got, bool pass)
{
	matrix_cases_run++;
	if (!pass) {
		matrix_failures++;
	}
	platform_log(pass ? "info" : "error", "signaling_case",
		     "name=%s expected=%s got=%s status=%s", name, expected, got,
		     pass ? "pass" : "fail");
}

static void run_matrix_case(const struct matrix_case *test)
{
	struct signaling_test_script script = {
		.expected_offer = TEST_SDP,
		.wire = wire_buffer,
		.chunk_bytes = test->chunk_bytes,
		.chunk_delay_ms = test->chunk_bytes != 0U ? 1U : 0U,
		.hold_open = test->hold_open,
		.hold_ms = 3000U,
	};
	struct signaling_request request = {.offer = TEST_SDP};
	enum signaling_result result;
	char url[48];
	size_t answer_length = 0U;
	uint16_t port;
	int server;

	script.wire_length = build_wire(test);
	if (signaling_test_server_start(&script, &port) != 0) {
		report_case(test->name, "server_start", "failed", false);
		return;
	}
	(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
	request.url = url;
	request.timeout_ms = test->expected == SIGNALING_TIMEOUT ? STALL_TIMEOUT : CASE_TIMEOUT;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	server = signaling_test_server_finish();

	if (result == SIGNALING_OK &&
	    (answer_length != strlen(TEST_SDP) || strcmp(answer_buffer, TEST_SDP) != 0)) {
		report_case(test->name, "answer_equals_sdp", "answer_differs", false);
	} else if (server != 0) {
		report_case(test->name, "valid_request", "server_rejected_request", false);
	} else {
		report_case(test->name, signaling_result_str(test->expected),
			    signaling_result_str(result), result == test->expected);
	}
}

static void expect_result(const char *name, enum signaling_result expected,
			  enum signaling_result got)
{
	report_case(name, signaling_result_str(expected), signaling_result_str(got),
		    got == expected);
}

static void run_cancellation_case(void)
{
	struct signaling_test_script script = {
		.expected_offer = TEST_SDP,
		.hold_open = true,
		.hold_ms = 3000U,
	};
	struct signaling_request request = {.offer = TEST_SDP, .timeout_ms = 10000U};
	enum signaling_result result;
	size_t answer_length = 0U;
	int64_t cancel_at;
	int64_t start;
	char url[48];
	uint16_t port;
	bool prompt;

	if (signaling_test_server_start(&script, &port) != 0) {
		report_case("cancel_while_stalled", "server_start", "failed", false);
		return;
	}
	(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
	start = k_uptime_get();
	cancel_at = start + 100;
	request.url = url;
	request.cancelled = cancel_when_due;
	request.user_data = &cancel_at;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	prompt = k_uptime_get() - start < 1500;
	(void)signaling_test_server_finish();
	report_case("cancel_while_stalled", "cancelled_promptly",
		    prompt ? signaling_result_str(result) : "late",
		    prompt && result == SIGNALING_CANCELLED);
}

static void run_unreachable_cases(void)
{
	struct signaling_request request = {.offer = TEST_SDP, .timeout_ms = CASE_TIMEOUT};
	enum signaling_result result;
	size_t answer_length = 0U;
	char url[48];
	uint16_t port;

	if (signaling_test_closed_port(&port) != 0) {
		report_case("unreachable_refused_port", "closed_port", "failed", false);
		return;
	}
	(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
	request.url = url;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("unreachable_refused_port", SIGNALING_UNREACHABLE, result);

	/* TEST-NET-1 has no route on the loopback-only simulator. */
	request.url = "http://192.0.2.1:8080/offer";
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("unreachable_no_route", SIGNALING_UNREACHABLE, result);
}

static void run_argument_cases(void)
{
	struct signaling_request request = {.url = "http://127.0.0.1:9/offer", .offer = TEST_SDP};
	enum signaling_result result;
	size_t answer_length = 0U;
	char name[40];

	for (size_t i = 0U; i < ARRAY_SIZE(bad_urls); i++) {
		request.url = bad_urls[i];
		result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer),
					    &answer_length);
		(void)snprintk(name, sizeof(name), "bad_url_%zu", i);
		expect_result(name, SIGNALING_BAD_URL, result);
	}

	request.url = "http://127.0.0.1:9/offer";
	result = signaling_exchange(NULL, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("null_request", SIGNALING_BAD_ARGUMENT, result);
	result = signaling_exchange(&request, answer_buffer, SIGNALING_ANSWER_BUFFER_BYTES - 1U,
				    &answer_length);
	expect_result("small_answer_buffer", SIGNALING_BAD_ARGUMENT, result);
	result = signaling_exchange(&request, NULL, sizeof(answer_buffer), &answer_length);
	expect_result("null_answer_buffer", SIGNALING_BAD_ARGUMENT, result);

	request.offer = NULL;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("null_offer", SIGNALING_BAD_ARGUMENT, result);
	request.offer = "";
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("empty_offer", SIGNALING_BAD_OFFER, result);
	request.offer = TEST_SDP_NO_FINAL_CRLF;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("offer_without_final_crlf", SIGNALING_BAD_OFFER, result);
	request.offer = "o=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nm=audio 9 UDP/TLS/RTP/SAVPF 0\r\n";
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("offer_without_version", SIGNALING_BAD_OFFER, result);
	request.offer = "v=0\r\ns=-\r\nt=0 0\r\n";
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("offer_without_media", SIGNALING_BAD_OFFER, result);
	fill_sdp(sdp_scratch, SIGNALING_SDP_MAX_BYTES + 1U);
	request.offer = sdp_scratch;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	expect_result("offer_oversize_by_one", SIGNALING_BAD_OFFER, result);
}

/* The largest accepted SDP crosses the wire in both directions. */
static void run_max_size_case(void)
{
	struct signaling_test_script script = {.hold_ms = 3000U};
	struct signaling_request request = {.timeout_ms = CASE_TIMEOUT};
	enum signaling_result result;
	size_t answer_length = 0U;
	size_t used;
	char url[48];
	uint16_t port;
	bool pass;
	int server;

	fill_sdp(sdp_scratch, SIGNALING_SDP_MAX_BYTES);
	script.expected_offer = sdp_scratch;
	used = (size_t)snprintk(wire_buffer, sizeof(wire_buffer),
				"HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
				"Content-Length: %zu\r\nConnection: close\r\n\r\n",
				(size_t)SIGNALING_SDP_MAX_BYTES);
	memcpy(wire_buffer + used, sdp_scratch, SIGNALING_SDP_MAX_BYTES);
	script.wire = wire_buffer;
	script.wire_length = used + SIGNALING_SDP_MAX_BYTES;
	if (signaling_test_server_start(&script, &port) != 0) {
		report_case("max_size_round_trip", "server_start", "failed", false);
		return;
	}
	(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
	request.url = url;
	request.offer = sdp_scratch;
	result = signaling_exchange(&request, answer_buffer, sizeof(answer_buffer), &answer_length);
	server = signaling_test_server_finish();
	pass = result == SIGNALING_OK && server == 0 && answer_length == SIGNALING_SDP_MAX_BYTES &&
	       strcmp(answer_buffer, sdp_scratch) == 0;
	report_case("max_size_round_trip", "ok", pass ? "ok" : signaling_result_str(result), pass);
}

static int run_matrix(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(matrix_cases); i++) {
		run_matrix_case(&matrix_cases[i]);
	}
	run_max_size_case();
	run_cancellation_case();
	run_unreachable_cases();
	run_argument_cases();
	platform_log(matrix_failures == 0 ? "info" : "error", "signaling_matrix",
		     "status=%s cases=%d failures=%d", matrix_failures == 0 ? "pass" : "fail",
		     matrix_cases_run, matrix_failures);
	return matrix_failures == 0 ? 0 : -EIO;
}

/* Mock-answerer state: one libpeer endpoint answering the offer that the server received. */
static struct libpeer_endpoint answerer;
static struct libpeer_endpoint offerer;

K_THREAD_STACK_DEFINE(offerer_stack, ZEPHYRTC_ENDPOINT_STACK_SIZE);
K_THREAD_STACK_DEFINE(answerer_stack, ZEPHYRTC_ENDPOINT_STACK_SIZE);

static int answer_with_libpeer(const char *offer, char *answer, size_t capacity, void *user_data)
{
	struct libpeer_endpoint *peer = user_data;
	const char *description;
	size_t length;

	peer_connection_set_remote_description(peer->connection, offer, SDP_TYPE_OFFER);
	if (peer_connection_get_state(peer->connection) != PEER_CONNECTION_CHECKING) {
		return -EPROTO;
	}
	description = peer_connection_create_answer(peer->connection);
	if (description == NULL || libpeer_endpoint_failed(peer)) {
		return -EPROTO;
	}
	length = strnlen(description, capacity);
	if (length == 0U || length >= capacity) {
		return -E2BIG;
	}
	memcpy(answer, description, length + 1U);
	return (int)length;
}

static int fail_answer(const char *offer, char *answer, size_t capacity, void *user_data)
{
	ARG_UNUSED(offer);
	ARG_UNUSED(answer);
	ARG_UNUSED(capacity);
	ARG_UNUSED(user_data);
	return -EPROTO;
}

static int start_offerer_case(const char *name, const struct signaling_test_script *script,
			      uint16_t *port)
{
	int result = libpeer_endpoint_init(&offerer, name, 0x31, 0xa7, true, false);

	if (result == 0) {
		result = signaling_test_server_start(script, port);
	}
	return result;
}

static bool expect_offerer_failure(const char *name, const struct signaling_test_script *script,
				   int expected, int expected_server, bool expect_libpeer_failed)
{
	struct libpeer_offerer_config config = {.timeout_ms = CASE_TIMEOUT};
	char url[48];
	uint16_t port;
	int result = -EIO;
	int server;
	bool pass;

	if (start_offerer_case(name, script, &port) != 0) {
		libpeer_endpoint_destroy(&offerer);
		return false;
	}
	(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
	config.url = url;
	result = libpeer_offerer_negotiate(&offerer, &config, answer_buffer, sizeof(answer_buffer));
	server = signaling_test_server_finish();
	pass = result == expected && server == expected_server &&
	       libpeer_endpoint_failed(&offerer) == expect_libpeer_failed;
	platform_log(pass ? "info" : "error", "offerer_case",
		     "name=%s expected=%d got=%d server=%d libpeer_failed=%d status=%s", name,
		     expected, result, server, libpeer_endpoint_failed(&offerer),
		     pass ? "pass" : "fail");
	libpeer_endpoint_destroy(&offerer);
	return pass;
}

static int run_negative_offerer_cases(void)
{
	struct signaling_test_script script = {.hold_ms = 1000U};
	struct libpeer_offerer_config config = {.timeout_ms = CASE_TIMEOUT};
	char url[48];
	uint16_t port;
	bool pass = true;
	int result;

	pass = libpeer_sdp_is_h264_pcmu(TEST_SDP_H264_PCMU) &&
	       !libpeer_sdp_is_h264_pcmu(TEST_SDP_H264_PCMU_BAD_PROFILE) &&
	       !libpeer_sdp_is_h264_pcmu(TEST_SDP) &&
	       !libpeer_sdp_is_pcmu_audio_only(TEST_SDP_H264_PCMU);
	platform_log(pass ? "info" : "error", "offerer_profile_cases", "status=%s",
		     pass ? "pass" : "fail");

	/* A well-formed HTTP answer for the wrong media profile never reaches libpeer. */
	script.wire_length = (size_t)snprintk(wire_buffer, sizeof(wire_buffer),
					      "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
					      "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
					      strlen(TEST_SDP_VIDEO), TEST_SDP_VIDEO);
	script.wire = wire_buffer;
	pass &= expect_offerer_failure("answer_profile_video_only", &script, -EBADMSG, 0, false);

	script.wire_length =
		(size_t)snprintk(wire_buffer, sizeof(wire_buffer),
				 "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
				 "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
				 strlen(TEST_SDP_PCMU_AND_VIDEO), TEST_SDP_PCMU_AND_VIDEO);
	pass &= expect_offerer_failure("answer_profile_pcmu_plus_video", &script, -EBADMSG, 0,
				       false);

	script.wire_length = (size_t)snprintk(wire_buffer, sizeof(wire_buffer),
					      "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
					      "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
					      strlen(TEST_SDP_PORT_ZERO), TEST_SDP_PORT_ZERO);
	pass &= expect_offerer_failure("answer_profile_port_zero", &script, -EBADMSG, 0, false);

	script.wire_length = (size_t)snprintk(wire_buffer, sizeof(wire_buffer),
					      "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
					      "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
					      strlen(TEST_SDP_RECVONLY), TEST_SDP_RECVONLY);
	pass &= expect_offerer_failure("answer_profile_recvonly", &script, -EBADMSG, 0, false);

	script.wire_length = (size_t)snprintk(wire_buffer, sizeof(wire_buffer),
					      "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
					      "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
					      strlen(TEST_SDP_RTPMAP_BEFORE_MEDIA),
					      TEST_SDP_RTPMAP_BEFORE_MEDIA);
	pass &= expect_offerer_failure("answer_profile_rtpmap_before_media", &script, -EBADMSG, 0,
				       false);

	/* A PCMU-only answer without ICE or DTLS parameters is refused by libpeer itself. */
	script.wire_length = (size_t)snprintk(wire_buffer, sizeof(wire_buffer),
					      "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
					      "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
					      strlen(TEST_SDP), TEST_SDP);
	pass &= expect_offerer_failure("answer_without_dtls_parameters", &script, -EPROTO, 0, true);

	/* The answerer fails after a valid request: the client sees HTTP 500. */
	memset(&script, 0, sizeof(script));
	script.answer = fail_answer;
	pass &= expect_offerer_failure("answerer_http_500", &script, -EBADMSG, -EPROTO, false);

	/* Nothing listens on the port. */
	result = libpeer_endpoint_init(&offerer, "refused", 0x31, 0xa7, true, false);
	if (result == 0 && signaling_test_closed_port(&port) == 0) {
		(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
		config.url = url;
		result = libpeer_offerer_negotiate(&offerer, &config, answer_buffer,
						   sizeof(answer_buffer));
		platform_log(result == -ECONNREFUSED ? "info" : "error", "offerer_case",
			     "name=unreachable expected=%d got=%d status=%s", -ECONNREFUSED, result,
			     result == -ECONNREFUSED ? "pass" : "fail");
		pass &= result == -ECONNREFUSED;
	} else {
		pass = false;
	}
	libpeer_endpoint_destroy(&offerer);
	return pass ? 0 : -EIO;
}

static int run_full_transaction(void)
{
	struct signaling_test_script script = {
		.answer = answer_with_libpeer,
		.user_data = &answerer,
	};
	struct libpeer_offerer_config config = {.timeout_ms = CASE_TIMEOUT};
	char url[48];
	uint16_t port;
	int64_t started;
	int result;
	int server = -EIO;

	memset(&offerer, 0, sizeof(offerer));
	memset(&answerer, 0, sizeof(answerer));
	result = libpeer_endpoint_init(&answerer, "answerer", 0xa7, 0x31, true, false);
	if (result == 0) {
		result = libpeer_endpoint_init(&offerer, "offerer", 0x31, 0xa7, true, false);
	}
	if (result == 0) {
		result = signaling_test_server_start(&script, &port);
	}
	if (result != 0) {
		goto cleanup;
	}
	(void)snprintk(url, sizeof(url), "http://127.0.0.1:%u/offer", (unsigned int)port);
	config.url = url;
	started = k_uptime_get();
	result = libpeer_offerer_negotiate(&offerer, &config, answer_buffer, sizeof(answer_buffer));
	server = signaling_test_server_finish();
	if (result == 0 && server != 0) {
		result = server;
	}
	if (result != 0) {
		goto cleanup;
	}
	platform_log("info", "signaling_negotiated", "elapsed_ms=%lld",
		     (long long)(k_uptime_get() - started));

	(void)libpeer_endpoint_start(&offerer, offerer_stack, K_THREAD_STACK_SIZEOF(offerer_stack));
	(void)libpeer_endpoint_start(&answerer, answerer_stack,
				     K_THREAD_STACK_SIZEOF(answerer_stack));
	result = libpeer_pair_wait_connected(&offerer, &answerer);
	if (result == 0) {
		result = libpeer_pair_exchange_audio(&offerer, &answerer);
	}
	if (atomic_get(&offerer.invalid_frames) != 0 || atomic_get(&answerer.invalid_frames) != 0) {
		result = -EBADMSG;
	}

cleanup:
	libpeer_endpoint_destroy(&offerer);
	libpeer_endpoint_destroy(&answerer);
	platform_log(result == 0 ? "info" : "error", "signaling_transaction",
		     "status=%s result=%d offerer_rx=%ld answerer_rx=%ld invalid=%ld/%ld",
		     result == 0 ? "pass" : "fail", result,
		     (long)atomic_get(&offerer.received_frames),
		     (long)atomic_get(&answerer.received_frames),
		     (long)atomic_get(&offerer.invalid_frames),
		     (long)atomic_get(&answerer.invalid_frames));
	return result;
}

int libpeer_signaling_tests_run(void)
{
	int result = run_matrix();
	int negative;
	int transaction;

	if (libpeer_stack_init() != 0) {
		platform_log("error", "signaling_tests", "libpeer_init=failed");
		return -EIO;
	}
	negative = run_negative_offerer_cases();
	platform_log(negative == 0 ? "info" : "error", "signaling_offerer_cases", "status=%s",
		     negative == 0 ? "pass" : "fail");
	transaction = run_full_transaction();
	libpeer_stack_deinit();
	return result != 0 ? result : (negative != 0 ? negative : transaction);
}
