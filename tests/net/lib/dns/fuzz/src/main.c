/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * libFuzzer harness for the DNS message parser. Every byte arrives in an
 * unauthenticated datagram, so the fuzz case is fed as that datagram
 * through both consumers:
 *
 * - responder path: mdns_unpack_query_header() + dns_unpack_query() per
 *   question, mirroring dns_read() in mdns_responder.c/llmnr_responder.c
 * - resolver path: dns_validate_msg(), what dns_read() in resolve.c calls
 *   on a reply
 *
 * dns_validate_msg() only accepts a reply matching an outstanding query,
 * so the harness sets the pending query from the fuzz case's own question -
 * the state an off-path spoofer reaches by echoing it back.
 *
 * See README.rst for build/run instructions.
 */

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/toolchain.h>

#include <dns_internal.h>
#include <dns_pack.h>

#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <nsi_cpu_if.h>
#include <nsi_main_semipublic.h>
#else
#error "Platform not supported"
#endif

/* Both consumers clamp the datagram to their receive buffer before the
 * parser sees it, so a longer input is never parsed as one message.
 */
#define FUZZ_MAX_PACKET DNS_RESOLVER_MAX_BUF_SIZE

/* Matches the buffer sizes the code under test actually uses (not
 * DNS_NAME_MAX_SIZE), so tail-room checks in dns_unpack_name() and
 * dns_copy_qname() are exercised against the real buffers. The two differ;
 * CONFIG_DNS_RESOLVER_MAX_QUERY_LEN (255) matching the CNAME buffer is
 * coincidental today.
 */
#define FUZZ_QUERY_NAME_BUF_SIZE CONFIG_MDNS_RESOLVER_BUF_SIZE
#define FUZZ_CNAME_BUF_SIZE CONFIG_DNS_RESOLVER_MAX_QUERY_LEN

NET_BUF_POOL_DEFINE(fuzz_query_name_pool, 2, FUZZ_QUERY_NAME_BUF_SIZE, 0, NULL);
NET_BUF_POOL_DEFINE(fuzz_cname_pool, 2, FUZZ_CNAME_BUF_SIZE, 0, NULL);

/* Each consumer gets its own copy: parsers keep the buffer pointer, and
 * update_query_idx() lower-cases the question in place.
 *
 * The copy sits flush against the end of packet_buf, not its start, so
 * the sanitizer's red zone follows the message immediately. A record
 * whose declared length runs past the message end - this parser's known
 * defect class - is caught here instead of silently reading stale bytes
 * from receive-buffer slack.
 */
static uint8_t packet_buf[FUZZ_MAX_PACKET];

static uint8_t *fuzz_packet(const uint8_t *data, size_t size)
{
	uint8_t *msg = packet_buf + sizeof(packet_buf) - size;

	return memcpy(msg, data, size);
}

int main(void)
{
	/* Fuzz cases are executed from LLVMFuzzerTestOneInput(), outside
	 * the OS. Nothing is left for the application thread to do.
	 */
	return 0;
}

static void fuzz_resolve_cb(enum dns_resolve_status status, struct dns_addrinfo *info,
			    void *user_data)
{
	ARG_UNUSED(status);
	ARG_UNUSED(info);
	ARG_UNUSED(user_data);
}

/* Responder side: mirrors mdns_responder.c/llmnr_responder.c - unpack
 * header, unpack each question, then read the name back out as the
 * responder does to match it against its own hostname.
 */
static void consume_query(uint8_t *buf, uint16_t size)
{
	struct dns_msg_t dns_msg = { .msg = buf, .msg_size = size };
	struct net_buf *result;
	uint16_t src_id;
	int queries;

	queries = mdns_unpack_query_header(&dns_msg, &src_id);
	if (queries < 0) {
		return;
	}

	result = net_buf_alloc(&fuzz_query_name_pool, K_NO_WAIT);
	if (result == NULL) {
		return;
	}

	do {
		enum dns_rr_type qtype;
		enum dns_class qclass;
		const char *lquery;

		(void)memset(result->data, 0, net_buf_tailroom(result));
		result->len = 0U;

		if (dns_unpack_query(&dns_msg, result, &qtype, &qclass) < 0) {
			break;
		}

		/* Mirrors the responder's ".local" suffix check that
		 * decides whether the query is answered.
		 */
		lquery = strrchr((const char *)result->data, '.');
		if (lquery == NULL) {
			continue;
		}

		if (memcmp(lquery, (const void *){ ".local" }, 7) != 0) {
			continue;
		}
	} while (--queries);

	net_buf_unref(result);
}

/* Resolver side: mirrors dns_read() in resolve.c calling dns_validate_msg().
 * The pending query is set up from the message's own question, so a reply
 * that echoes it reaches the records like a real one would.
 */
static void consume_response(uint8_t *buf, uint16_t size)
{
	static struct dns_resolve_context ctx;
	static uint8_t question[FUZZ_MAX_PACKET];
	struct dns_msg_t dns_msg = { .msg = buf, .msg_size = size };
	struct net_buf *cname;
	uint16_t query_hash = 0U;
	uint16_t dns_id;
	int query_idx = -1;
	size_t name_len;

	if (size < DNS_MSG_HEADER_SIZE) {
		return;
	}

	(void)memset(&ctx, 0, sizeof(ctx));
	ctx.state = DNS_RESOLVE_CONTEXT_ACTIVE;
	ctx.queries[0].cb = fuzz_resolve_cb;
	ctx.queries[0].query = "fuzz";
	ctx.queries[0].query_type = DNS_QUERY_TYPE_A;
	ctx.queries[0].id = dns_unpack_header_id(buf);

	/* Reproduces update_query_idx()'s hash (lower-cased question + type)
	 * on a copy, so the pending query matches without touching the
	 * bytes handed to the parser.
	 */
	(void)memcpy(question, buf, size);

	for (name_len = DNS_MSG_HEADER_SIZE; name_len < size; name_len++) {
		if (question[name_len] == 0U) {
			break;
		}
	}

	if (name_len + DNS_QTYPE_LEN < size) {
		uint8_t *name = question + DNS_MSG_HEADER_SIZE;
		size_t label_len = name_len - DNS_MSG_HEADER_SIZE;

		for (size_t i = 0; i < label_len; i++) {
			name[i] = tolower(name[i]);
		}

		query_hash = crc16_ansi(name, label_len + 1 + DNS_QTYPE_LEN);

		switch (sys_get_be16(&question[name_len + 1])) {
		case DNS_RR_TYPE_AAAA:
			ctx.queries[0].query_type = DNS_QUERY_TYPE_AAAA;
			break;
		case DNS_RR_TYPE_PTR:
			ctx.queries[0].query_type = DNS_QUERY_TYPE_PTR;
			break;
		case DNS_RR_TYPE_ANY:
			ctx.queries[0].query_type = DNS_QUERY_TYPE_ANY;
			break;
		default:
			break;
		}
	}

	ctx.queries[0].query_hash = query_hash;

	cname = net_buf_alloc(&fuzz_cname_pool, K_NO_WAIT);
	if (cname == NULL) {
		return;
	}

	dns_id = ctx.queries[0].id;

	(void)dns_validate_msg(&ctx, &dns_msg, &dns_id, &query_idx, cname, &query_hash, -1);

	net_buf_unref(cname);
}

/**
 * Fuzzing entry point. The simulator boots once for an initialised
 * runtime; each case runs directly, no Zephyr thread needed.
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* We expose this function to the final runner link stage */
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	if (size > sizeof(packet_buf)) {
		/* Skip rather than truncate, so a case the corpus keeps is
		 * the case that was executed. Pass -max_len=512 to keep the
		 * mutator inside the range that is executed at all.
		 */
		return 0;
	}

	consume_query(fuzz_packet(data, size), (uint16_t)size);
	consume_response(fuzz_packet(data, size), (uint16_t)size);

	return 0;
}
