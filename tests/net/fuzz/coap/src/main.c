/*
 * Copyright (c) 2026 Hubble Network
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Coverage-guided fuzz target for the CoAP packet parser.
 *
 * CoAP packets arrive from the network, so coap_packet_parse() runs on wholly
 * attacker-controlled bytes. This target drives it directly rather than through
 * the OS: LLVMFuzzerTestOneInput() runs in the host environment "outside" the
 * embedded OS, so there is no scheduling, no simulated time, and no per-input
 * simulator teardown. That keeps every input deterministic and costs roughly
 * one microsecond, which is what makes a useful number of executions reachable.
 *
 * Build and run (clang only, and note the toolchain variant must be
 * "host/llvm" rather than "llvm", or the build silently falls back to gcc):
 *
 *   ZEPHYR_TOOLCHAIN_VARIANT=host/llvm \
 *   west build -b native_sim/native/64 tests/net/lib/coap/fuzz
 *   build/zephyr/zephyr.exe tests/net/lib/coap/fuzz/corpus
 *
 * See samples/subsys/debug/fuzz for the alternative model, where input is
 * delivered to a running Zephyr instance through a simulated interrupt. That
 * one is the right choice for code which cannot be called directly, but it is
 * substantially slower because the OS is scheduled once per input.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <nsi_cpu_if.h>
#include <string.h>

/* Largest input handed to the parser. A CoAP datagram is bounded in practice by
 * the link MTU, and letting the fuzzer explore far beyond that spends the
 * budget on length handling rather than on parser structure.
 */
#define FUZZ_BUF_SIZE 1280

/* Enough to cover the option numbers a packet realistically carries while
 * still exercising the "more options than the caller provided room for" path.
 */
#define FUZZ_MAX_OPTIONS 16

/**
 * @brief Exercise the accessors a real CoAP application calls after a parse.
 *
 * A successful parse is only half the surface: applications immediately walk
 * the header, token, options and payload, and those accessors trust the offsets
 * coap_packet_parse() recorded. Reading them here means a parse which succeeds
 * but leaves the packet internally inconsistent is caught as well.
 */
static void walk_parsed_packet(const struct coap_packet *cpkt)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	struct coap_option opt;
	const uint8_t *payload;
	uint16_t payload_len;

	(void)coap_header_get_version(cpkt);
	(void)coap_header_get_type(cpkt);
	(void)coap_header_get_code(cpkt);
	(void)coap_header_get_id(cpkt);
	(void)coap_packet_is_request(cpkt);

	/* Writes up to COAP_TOKEN_MAX_LEN bytes and returns the length used */
	(void)coap_header_get_token(cpkt, token);

	payload = coap_packet_get_payload(cpkt, &payload_len);
	if (payload != NULL && payload_len > 0U) {
		/* Touch the payload so a bad offset or length is caught by the
		 * sanitizer rather than passing silently.
		 */
		volatile uint8_t sink = payload[payload_len - 1U];

		(void)sink;
	}

	/* COAP_OPTION_URI_PATH is the option applications reach for most, and
	 * repeats, so it exercises the multi-instance lookup path.
	 */
	if (coap_find_options(cpkt, COAP_OPTION_URI_PATH, &opt, 1) > 0) {
		(void)coap_option_value_to_int(&opt);
	}
}

#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* Expose the entry point to the final runner link stage */
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t sz)
{
	static uint8_t buf[FUZZ_BUF_SIZE];
	struct coap_option opts[FUZZ_MAX_OPTIONS];
	struct coap_packet cpkt;

	if (sz == 0U || sz > sizeof(buf)) {
		return 0;
	}

	/* coap_packet_parse() takes a non-const pointer and retains it in the
	 * returned packet, so hand it a private copy. Fuzzer-owned memory must
	 * not be written to, and a parser which mutates its input would
	 * otherwise corrupt the corpus entry underneath libFuzzer.
	 */
	memcpy(buf, data, sz);

	if (coap_packet_parse(&cpkt, buf, (uint16_t)sz, opts, ARRAY_SIZE(opts)) < 0) {
		return 0;
	}

	walk_parsed_packet(&cpkt);

	return 0;
}
