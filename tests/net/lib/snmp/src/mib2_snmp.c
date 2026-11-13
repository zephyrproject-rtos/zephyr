/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/snmp.h>
#include <zephyr/ztest.h>

#include "helpers.h"
#include "test_objects.h"

#define SNMP_GROUP(n) 1, 3, 6, 1, 2, 1, 11, (n)

#define IN_PKTS                SNMP_GROUP(1)
#define IN_BAD_VERSIONS        SNMP_GROUP(3)
#define IN_BAD_COMMUNITY_NAMES SNMP_GROUP(4)
#define IN_BAD_COMMUNITY_USES  SNMP_GROUP(5)
#define IN_ASN_PARSE_ERRS      SNMP_GROUP(6)
#define ENABLE_AUTHEN_TRAPS    SNMP_GROUP(30)
#define SILENT_DROPS           SNMP_GROUP(31)
#define PROXY_DROPS            SNMP_GROUP(32)
#define SET_SERIAL_NO          1, 3, 6, 1, 6, 3, 1, 1, 6, 1

struct counters {
	uint32_t in_pkts;
	uint32_t in_bad_versions;
	uint32_t in_bad_community_names;
	uint32_t in_bad_community_uses;
	uint32_t in_asn_parse_errs;
	uint32_t silent_drops;
	uint32_t proxy_drops;
};

/* One GET, which itself counts as a received message. */
static void read_counters(struct counters *c)
{
	const struct req_vb vbs[] = {
		VB_NULL(IN_PKTS, 0),
		VB_NULL(IN_BAD_VERSIONS, 0),
		VB_NULL(IN_BAD_COMMUNITY_NAMES, 0),
		VB_NULL(IN_BAD_COMMUNITY_USES, 0),
		VB_NULL(IN_ASN_PARSE_ERRS, 0),
		VB_NULL(SILENT_DROPS, 0),
		VB_NULL(PROXY_DROPS, 0),
	};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
			      &rsp) > 0);
	expect_ok(&rsp, ARRAY_SIZE(vbs));

	for (size_t i = 0; i < ARRAY_SIZE(vbs); i++) {
		zassert_equal(rsp.vb[i].tag, SNMP_BER_TAG_COUNTER32, "Counter %zu", i);
	}

	c->in_pkts = rsp.vb[0].u32;
	c->in_bad_versions = rsp.vb[1].u32;
	c->in_bad_community_names = rsp.vb[2].u32;
	c->in_bad_community_uses = rsp.vb[3].u32;
	c->in_asn_parse_errs = rsp.vb[4].u32;
	c->silent_drops = rsp.vb[5].u32;
	c->proxy_drops = rsp.vb[6].u32;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_READ, "public"));
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_WRITE, "private"));
	snmp_mib2_snmp_set_authen_traps(true);
}

ZTEST(snmp_mib2_snmp, test_counters)
{
	const struct req_vb get_a[] = {VB_NULL(TEST_OID_A, 0)};
	const struct req_vb set_c[] = {VB_INT(1, TEST_OID_C, 0)};
	const struct req_vb carries[] = {VB_TAG(SNMP_BER_TAG_COUNTER64, TEST_OID_A, 0)};
	static const uint8_t garbage[] = {0x30, 0x03, 0x02, 0x01};
	struct counters c0, c1;
	size_t req_start, req_len, rsp_start;
	struct response rsp;

	read_counters(&c0);

	/* Two messages with an unknown community, one of them version 1 */
	zassert_equal(exchange(V2C, "wrong", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);
	zassert_equal(exchange(V1, "wrong", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);
	/* An unsupported version */
	zassert_equal(exchange(3, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);
	/* Undecodable, and a version 1 request carrying a Counter64 */
	zassert_equal(snmp_process_message(garbage, sizeof(garbage), rsp_buf, sizeof(rsp_buf),
					   &rsp_start),
		      0);
	zassert_equal(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, carries,
			       ARRAY_SIZE(carries), &rsp),
		      0);
	/* SET with the read community, in both versions */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			      ARRAY_SIZE(set_c), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_AUTHORIZATION_ERROR, 0);
	zassert_true(exchange(V1, "public", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			      ARRAY_SIZE(set_c), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 0);
	/* A version 1 error response that cannot carry its bindings */
	req_len = build_request(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
				ARRAY_SIZE(get_a), &req_start);
	zassert_equal(snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len - 1,
					   &rsp_start),
		      0);

	read_counters(&c1);

	/* Eight messages above plus the reading GET itself */
	zassert_equal(c1.in_pkts, c0.in_pkts + 9, "snmpInPkts");
	zassert_equal(c1.in_bad_community_names, c0.in_bad_community_names + 2,
		      "snmpInBadCommunityNames");
	zassert_equal(c1.in_bad_versions, c0.in_bad_versions + 1, "snmpInBadVersions");
	zassert_equal(c1.in_asn_parse_errs, c0.in_asn_parse_errs + 2, "snmpInASNParseErrs");
	zassert_equal(c1.in_bad_community_uses, c0.in_bad_community_uses + 2,
		      "snmpInBadCommunityUses");
	zassert_equal(c1.silent_drops, c0.silent_drops + 1, "snmpSilentDrops");
	zassert_equal(c1.proxy_drops, 0, "snmpProxyDrops");
}

ZTEST(snmp_mib2_snmp, test_enable_authen_traps)
{
	const struct req_vb get[] = {VB_NULL(ENABLE_AUTHEN_TRAPS, 0)};
	const struct req_vb disable[] = {VB_INT(2, ENABLE_AUTHEN_TRAPS, 0)};
	const struct req_vb bad[] = {VB_INT(3, ENABLE_AUTHEN_TRAPS, 0)};
	const struct req_vb wrong_type[] = {VB_STR("1", ENABLE_AUTHEN_TRAPS, 0)};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get, ARRAY_SIZE(get),
			      &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_INTEGER);
	zassert_equal(rsp.vb[0].i32, 1, "enabled(1) by default");

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, disable,
			      ARRAY_SIZE(disable), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(get_int32(OID(ENABLE_AUTHEN_TRAPS, 0), OID_LEN(ENABLE_AUTHEN_TRAPS, 0)), 2);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, bad, ARRAY_SIZE(bad),
			      &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_VALUE, 1);
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, wrong_type,
			      ARRAY_SIZE(wrong_type), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_TYPE, 1);
	zassert_equal(get_int32(OID(ENABLE_AUTHEN_TRAPS, 0), OID_LEN(ENABLE_AUTHEN_TRAPS, 0)), 2,
		      "Rejected SET changed the value");

	snmp_mib2_snmp_set_authen_traps(true);
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get, ARRAY_SIZE(get),
			      &rsp) > 0);
	zassert_equal(rsp.vb[0].i32, 1);
}

/* RFC 2579 TestAndIncr: only the current value may be written, and doing
 * so advances it.
 */
ZTEST(snmp_mib2_snmp, test_set_serial_no)
{
	const struct req_vb get[] = {VB_NULL(SET_SERIAL_NO, 0)};
	struct response rsp;
	int32_t serial;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get, ARRAY_SIZE(get),
			      &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_INTEGER);
	serial = rsp.vb[0].i32;
	/* RFC 2579: pseudo-random after a start, so within the TestAndIncr
	 * range; a fixed zero would let a request from before a restart pass.
	 */
	zassert_true(serial >= 0, "Initial value %d out of range", serial);
	zassert_true(serial != 0, "Initial value is not random");

	{
		const struct req_vb stale[] = {VB_INT(serial + 1, SET_SERIAL_NO, 0)};
		const struct req_vb negative[] = {VB_INT(-1, SET_SERIAL_NO, 0)};
		const struct req_vb current[] = {VB_INT(serial, SET_SERIAL_NO, 0)};
		/* The lock and a payload in one request, as managers use it */
		const struct req_vb locked[] = {VB_INT(serial + 1, SET_SERIAL_NO, 0),
						VB_INT(7, TEST_OID_C, 0)};

		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, stale,
				      ARRAY_SIZE(stale), &rsp) > 0);
		expect_error(&rsp, SNMP_ERR_INCONSISTENT_VALUE, 1);
		zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, stale,
				      ARRAY_SIZE(stale), &rsp) > 0);
		expect_error(&rsp, SNMP_ERR_BAD_VALUE, 1);

		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, negative,
				      ARRAY_SIZE(negative), &rsp) > 0);
		expect_error(&rsp, SNMP_ERR_WRONG_VALUE, 1);

		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, current,
				      ARRAY_SIZE(current), &rsp) > 0);
		expect_ok(&rsp, 1);

		zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get,
				      ARRAY_SIZE(get), &rsp) > 0);
		zassert_equal(rsp.vb[0].i32, serial + 1, "The value advances on a successful SET");

		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, locked,
				      ARRAY_SIZE(locked), &rsp) > 0);
		expect_ok(&rsp, 2);
		zassert_equal(test_c_value(), 7);
		test_c_reset();

		/* Replaying the same request fails without touching the payload */
		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, locked,
				      ARRAY_SIZE(locked), &rsp) > 0);
		expect_error(&rsp, SNMP_ERR_INCONSISTENT_VALUE, 1);
		zassert_equal(test_c_value(), 42);
	}
}

ZTEST_SUITE(snmp_mib2_snmp, NULL, NULL, before, NULL, NULL);
