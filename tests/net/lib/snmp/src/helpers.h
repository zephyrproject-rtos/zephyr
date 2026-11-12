/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Building requests for and decoding responses from snmp_process_message(). */

#ifndef TESTS_NET_LIB_SNMP_SRC_HELPERS_H_
#define TESTS_NET_LIB_SNMP_SRC_HELPERS_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#include "snmp_ber.h"
#include "snmp_internal.h"

#define OID(...) ((const uint32_t[]){__VA_ARGS__})
#define OID_LEN(...) ARRAY_SIZE(OID(__VA_ARGS__))

#define V1  0
#define V2C 1

#define REQUEST_ID 0x12345

/* Value carried by a request binding. */
struct req_value {
	uint8_t tag; /* 0 encodes NULL */
	int32_t i32;
	const char *str;
};

struct req_vb {
	const uint32_t *oid;
	size_t oid_len;
	struct req_value value;
};

#define VB_NULL(...) {OID(__VA_ARGS__), OID_LEN(__VA_ARGS__), {0}}
#define VB_INT(_v, ...) {OID(__VA_ARGS__), OID_LEN(__VA_ARGS__), {SNMP_BER_TAG_INTEGER, _v, NULL}}
#define VB_STR(_s, ...)                                                                            \
	{OID(__VA_ARGS__), OID_LEN(__VA_ARGS__), {SNMP_BER_TAG_OCTET_STRING, 0, _s}}
#define VB_TAG(_t, ...) {OID(__VA_ARGS__), OID_LEN(__VA_ARGS__), {_t, 0, NULL}}

/* One decoded binding of a response. */
struct rsp_vb {
	uint32_t oid[SNMP_OID_MAX_LEN];
	size_t oid_len;
	uint8_t tag;
	int32_t i32;
	uint32_t u32;
	uint64_t u64;
	uint8_t str[64];
	size_t str_len;
	uint32_t val_oid[SNMP_OID_MAX_LEN];
	size_t val_oid_len;
};

struct response {
	int32_t version;
	uint8_t community[32];
	size_t community_len;
	int32_t request_id;
	int32_t error_status;
	int32_t error_index;
	struct rsp_vb vb[20];
	size_t vb_count;
	/* Raw variable binding list content for echo comparisons */
	const uint8_t *raw_vbs;
	size_t raw_vbs_len;
};

extern uint8_t req_buf[512];
extern uint8_t rsp_buf[512];

/* Build a request by prepending, so bindings are added last to first. */
size_t build_request(int32_t version, const char *community, uint8_t pdu_type,
		     int32_t err_status, int32_t err_index, const struct req_vb *vbs, size_t count,
		     size_t *start);

void parse_response(const uint8_t *data, size_t len, struct response *rsp);

/* Send a request through the engine and parse the reply. Returns the reply
 * length, 0 when the engine dropped the request.
 */
size_t exchange(int32_t version, const char *community, uint8_t pdu_type, int32_t err_status,
		int32_t err_index, const struct req_vb *vbs, size_t count, struct response *rsp);

void expect_oid(const struct rsp_vb *vb, const uint32_t *oid, size_t len);
void expect_ok(const struct response *rsp, size_t count);
void expect_error(const struct response *rsp, int32_t status, int32_t index);
/* The bindings of an error response are those of the request, byte for byte. */
void expect_echo(const struct response *rsp, const struct req_vb *vbs, size_t count);

/* GET one OCTET STRING instance into a NUL terminated buffer. */
void get_display_string(const uint32_t *oid, size_t len, char *out, size_t size);
/* GET one INTEGER instance. */
int32_t get_int32(const uint32_t *oid, size_t len);

#endif /* TESTS_NET_LIB_SNMP_SRC_HELPERS_H_ */
