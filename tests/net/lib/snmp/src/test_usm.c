/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SNMPv3 / USM receive-path tests (RFC 3412 §7, RFC 3414 §3.2/§6.3, RFC 3415).
 *
 * These run on the emulated targets only: they need PSA Crypto, a CSPRNG and the
 * socket-service-capable build the module uses there. The host suite compiles
 * snmp_core.c without CONFIG_SNMP_VERSION_3, so none of this is reachable there.
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <string.h>

#include <zephyr/net/snmp/snmp_usm.h>
#include <zephyr/net/snmp/snmp_vacm.h>

/* Module-internal verifier used to check the Response exactly as a peer would. */
#include "snmp_internal.h"

/* 1.3.6.1.2.1.1.1.0 sysDescr.0 — inside every default VACM view. */
static const uint32_t s_oid_sysdescr[] = {1, 3, 6, 1, 2, 1, 1, 1, 0};
/* 1.3.6.1.4.1.54321.1.1.1.0 — outside the "readonly" view. */
static const uint32_t s_oid_private[] = {1, 3, 6, 1, 4, 1, 54321, 1, 1, 1, 0};

#define USM_REPORT_OID_LEN 11

static struct snmp_agent s_agent;

static void usm_setup(void)
{
	zassert_ok(snmp_agent_init(&s_agent, NULL), "agent init failed");
}

static void oid_set(struct snmp_oid *oid, const uint32_t *ids, uint8_t len)
{
	oid->len = len;
	memcpy(oid->ids, ids, len * sizeof(uint32_t));
}

static void make_get(struct snmp_v3_msg *msg, int32_t request_id, const uint32_t *ids,
		     uint8_t oid_len)
{
	memset(msg, 0, sizeof(*msg));
	msg->global.msg_id = request_id;
	msg->global.msg_max_size = CONFIG_SNMP_MAX_PDU_SIZE;
	msg->global.msg_security_model = SNMP_SEC_MODEL_USM;
	msg->global.msg_flags = 0x04; /* reportable, noAuthNoPriv */
	msg->scoped.pdu.version = 3;
	msg->scoped.pdu.type = SNMP_PDU_GET_REQ;
	msg->scoped.pdu.request_id = request_id;
	msg->scoped.pdu.varbind_cnt = 1;
	oid_set(&msg->scoped.pdu.varbinds[0].oid, ids, oid_len);
	msg->scoped.pdu.varbinds[0].type = ASN1_TAG_NULL;
}

/* Fill the authoritative-engine fields with this agent's own values. */
static void fill_engine(struct snmp_v3_msg *msg)
{
	uint8_t len = sizeof(msg->usm.engine_id);

	zassert_ok(snmp_usm_get_engine_id(&s_agent, msg->usm.engine_id, &len));
	msg->usm.engine_id_len = len;
	msg->usm.engine_boots = snmp_usm_get_engine_boots(&s_agent);
	msg->usm.engine_time = snmp_usm_get_engine_time(&s_agent);
}

static void set_user(struct snmp_v3_msg *msg, const char *name)
{
	size_t len = strlen(name);

	zassert_true(len < sizeof(msg->usm.user_name));
	memcpy(msg->usm.user_name, name, len);
	msg->usm.user_name_len = len;
}

static size_t encode(const struct snmp_v3_msg *msg, uint8_t *buf, size_t buf_sz)
{
	size_t len = 0;

	zassert_ok(snmp_ber_encode_v3_msg(buf, buf_sz, &len, msg));
	return len;
}

static int process(const uint8_t *req, size_t req_len, uint8_t *resp, size_t *resp_len)
{
	*resp_len = 0;
	return snmp_agent_process_pdu(&s_agent, req, req_len, resp, CONFIG_SNMP_MAX_PDU_SIZE,
				      resp_len);
}

static void assert_report(const struct snmp_v3_msg *resp, uint32_t last_arc, uint8_t flags)
{
	zassert_equal(resp->scoped.pdu.type, SNMP_PDU_REPORT, "expected a Report PDU");
	zassert_equal(resp->global.msg_flags & 0x07, flags, "unexpected msgFlags 0x%02x",
		      resp->global.msg_flags);
	zassert_equal(resp->scoped.pdu.varbind_cnt, 1, "Report must carry one counter");
	zassert_equal(resp->scoped.pdu.varbinds[0].type, SNMP_TAG_COUNTER32);

	const struct snmp_oid *oid = &resp->scoped.pdu.varbinds[0].oid;

	zassert_equal(oid->len, USM_REPORT_OID_LEN);
	zassert_equal(oid->ids[0], 1);
	zassert_equal(oid->ids[1], 3);
	zassert_equal(oid->ids[2], 6);
	zassert_equal(oid->ids[3], 1);
	zassert_equal(oid->ids[4], 6);
	zassert_equal(oid->ids[5], 3);
	zassert_equal(oid->ids[6], 15);
	zassert_equal(oid->ids[7], 1);
	zassert_equal(oid->ids[8], 1);
	zassert_equal(oid->ids[9], last_arc);
	zassert_equal(oid->ids[10], 0);
}

/* ---- Engine discovery (RFC 3414 §3.2 step 4) ---- */

ZTEST(snmp_usm, test_engine_discovery_report)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	/* Empty engineID and zero boots/time is exactly what a manager probes with. */
	make_get(&req, 0x11223344, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_true(out_len > 0);
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	assert_report(&resp, 4, 0x00); /* usmStatsUnknownEngineIDs.0 */
	zassert_equal(resp.scoped.pdu.varbinds[0].val.uint_val, 1);
	zassert_equal(s_agent.usm_stats.unknown_engine_ids, 1);

	/* The Report must carry this engine's ID so the manager can proceed. */
	uint8_t engine_id[32];
	uint8_t engine_id_len = sizeof(engine_id);

	zassert_ok(snmp_usm_get_engine_id(&s_agent, engine_id, &engine_id_len));
	zassert_equal(resp.usm.engine_id_len, engine_id_len);
	zassert_mem_equal(resp.usm.engine_id, engine_id, engine_id_len);
}

ZTEST(snmp_usm, test_discovery_without_reportable_flag_is_dropped)
{
	struct snmp_v3_msg req;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	make_get(&req, 1, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	req.global.msg_flags = 0x00; /* not reportable */
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_equal(process(raw, req_len, out, &out_len), -EACCES);
	zassert_equal(out_len, 0, "a non-reportable discovery probe must not be answered");
}

/* ---- USM user / security-level errors ---- */

ZTEST(snmp_usm, test_unknown_user_reports)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	make_get(&req, 2, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	fill_engine(&req);
	set_user(&req, "ghost");
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	assert_report(&resp, 3, 0x00); /* usmStatsUnknownUserNames.0 */
	zassert_equal(resp.scoped.pdu.varbinds[0].val.uint_val, 1);
}

ZTEST(snmp_usm, test_privacy_without_authentication_reports)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	make_get(&req, 3, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	fill_engine(&req);
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.global.msg_flags = 0x04 | 0x02; /* reportable + priv, but no auth */
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	assert_report(&resp, 1, 0x00); /* usmStatsUnsupportedSecLevels.0 */
}

/* ---- Auth / privacy round trips ---- */

ZTEST(snmp_usm, test_auth_priv_request_roundtrip)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	make_get(&req, 0x55aa, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_PRIV;

	/* Sign and encrypt exactly as a peer holding the same localized keys would. */
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_equal(resp.global.msg_flags & 0x07, 0x03);

	/* Verify the Response as the manager would: HMAC over the wire bytes + decrypt. */
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.type, SNMP_PDU_GET_RESP);
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbind_cnt, 1);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, ASN1_TAG_OCTET_STRING);
	zassert_true(resp.scoped.pdu.varbinds[0].val.octet_str.len > 0);
}

ZTEST(snmp_usm, test_wrong_digest_is_dropped_and_counted)
{
	struct snmp_v3_msg req;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	make_get(&req, 0x77, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	zassert_true(req.usm.auth_params_len > 0);

	/* Corrupt the digest, leave everything else decodable. */
	req.usm.auth_params[0] ^= 0xFF;
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_equal(process(raw, req_len, out, &out_len), -EACCES);
	zassert_equal(out_len, 0, "a wrong digest must be discarded, never answered");
	zassert_equal(s_agent.usm_stats.wrong_digests, 1);
}

ZTEST(snmp_usm, test_stale_authenticated_message_reports_not_in_time_window)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;
	uint32_t t0;

	usm_setup();

	t0 = snmp_usm_get_engine_time(&s_agent);

	/* Sign a message while the engine clock is far ahead, then rewind the engine,
	 * so the (validly signed) datagram is 10000 s outside the 150 s window.
	 */
	zassert_ok(snmp_usm_set_engine_time(&s_agent, t0 + 10000));
	make_get(&req, 0x99, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	size_t req_len = encode(&req, raw, sizeof(raw));
	zassert_ok(snmp_usm_set_engine_time(&s_agent, t0));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	/* RFC 3414 §3.2 step 7: notInTimeWindows is reported, authenticated. */
	assert_report(&resp, 2, 0x01);
	zassert_equal(s_agent.usm_stats.not_in_time_windows, 1);
}

/* ---- Timeliness is only meaningful for authenticated messages ---- */

static void add_monitor_user(void)
{
	struct snmp_usm_user monitor = {
		.auth_protocol = SNMP_USM_AUTH_NONE,
		.priv_protocol = SNMP_USM_PRIV_NONE,
	};

	strcpy(monitor.user_name, "monitor");
	zassert_ok(snmp_usm_add_user(&s_agent, &monitor));
}

ZTEST(snmp_usm, test_noauth_skips_timeliness_check)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();
	add_monitor_user();

	make_get(&req, 0xabc, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	fill_engine(&req);
	set_user(&req, "monitor");
	/* Unauthenticated engineTime is attacker-controlled; it must not gate the request. */
	req.usm.engine_time += 10000;
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	zassert_equal(resp.scoped.pdu.type, SNMP_PDU_GET_RESP);
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, ASN1_TAG_OCTET_STRING);
}

/* ---- VACM -- verified level, and RFC 3415 denial semantics ---- */

ZTEST(snmp_usm, test_vacm_denial_returns_authorization_error)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();
	add_monitor_user();

	make_get(&req, 0xdef, s_oid_private, ARRAY_SIZE(s_oid_private));
	fill_engine(&req);
	set_user(&req, "monitor");
	size_t req_len = encode(&req, raw, sizeof(raw));

	/* RFC 3415 §5.5: answer with authorizationError, do not drop. */
	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	zassert_equal(resp.scoped.pdu.type, SNMP_PDU_GET_RESP);
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_AUTHORIZATION_ERR);
	zassert_equal(resp.scoped.pdu.error_index, 0);
}

ZTEST(snmp_usm, test_unauthenticated_request_denied_for_authenticated_user)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	usm_setup();

	/*
	 * The default user maps to VACM group "admin", whose access rule requires
	 * authNoPriv or better. Send a well-formed noAuthNoPriv message with the right
	 * engine ID: USM verifies it (nothing to verify, no timeliness check), derives
	 * sec_level = noAuthNoPriv from what it actually checked, and VACM must then
	 * refuse the read instead of trusting anything the peer put in msgFlags.
	 */
	make_get(&req, 0x1234, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	fill_engine(&req);
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	zassert_equal(req.global.msg_flags & 0x03, 0);
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));

	zassert_equal(resp.scoped.pdu.type, SNMP_PDU_GET_RESP);
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_AUTHORIZATION_ERR);
}

/* v3 msgData has no version field: the MIB engine must use v2 error semantics. */
ZTEST(snmp_usm, test_v3_uses_v2_error_semantics)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;
	static const uint32_t missing[] = {1, 3, 6, 1, 4, 1, 54321, 9, 9, 0};

	usm_setup();

	make_get(&req, 7, missing, ARRAY_SIZE(missing));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));

	zassert_equal(resp.scoped.pdu.type, SNMP_PDU_GET_RESP);
	/* v1's noSuchName (2) would have been the bug; v2 answers noError + exception. */
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);
}

ZTEST(snmp_usm, test_v3_framework_mib_engine_queries)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	static const uint32_t oid_engine_id[] = {1, 3, 6, 1, 6, 3, 10, 2, 1, 1, 0};
	static const uint32_t oid_engine_boots[] = {1, 3, 6, 1, 6, 3, 10, 2, 1, 2, 0};
	static const uint32_t oid_engine_time[] = {1, 3, 6, 1, 6, 3, 10, 2, 1, 3, 0};
	static const uint32_t oid_engine_max_msg[] = {1, 3, 6, 1, 6, 3, 10, 2, 1, 4, 0};

	usm_setup();

	/* 1. snmpEngineID.0 query */
	make_get(&req, 0x101, oid_engine_id, ARRAY_SIZE(oid_engine_id));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, ASN1_TAG_OCTET_STRING);
	zassert_equal(resp.scoped.pdu.varbinds[0].val.octet_str.len, s_agent.engine_id_len);
	zassert_mem_equal(resp.scoped.pdu.varbinds[0].val.octet_str.data, s_agent.engine_id,
			  s_agent.engine_id_len);

	/* 2. snmpEngineBoots.0 query */
	make_get(&req, 0x102, oid_engine_boots, ARRAY_SIZE(oid_engine_boots));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, ASN1_TAG_INTEGER);
	zassert_equal(resp.scoped.pdu.varbinds[0].val.int_val, (int32_t)s_agent.engine_boots);

	/* 3. snmpEngineTime.0 query */
	make_get(&req, 0x103, oid_engine_time, ARRAY_SIZE(oid_engine_time));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, ASN1_TAG_INTEGER);

	/* 4. snmpEngineMaxMessageSize.0 query */
	make_get(&req, 0x104, oid_engine_max_msg, ARRAY_SIZE(oid_engine_max_msg));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, ASN1_TAG_INTEGER);
	zassert_equal(resp.scoped.pdu.varbinds[0].val.int_val, CONFIG_SNMP_MAX_PDU_SIZE);
}

ZTEST(snmp_usm, test_v3_usm_stats_mib_queries)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	static const uint32_t oid_wrong_digests[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 5, 0};
	static const uint32_t oid_unsupported_sec[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 1, 0};

	usm_setup();

	/* Inject a bad digest to trigger wrong_digests increment */
	make_get(&req, 0x881, s_oid_sysdescr, ARRAY_SIZE(s_oid_sysdescr));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	req.usm.auth_params[0] ^= 0x55;
	size_t req_len = encode(&req, raw, sizeof(raw));
	zassert_equal(process(raw, req_len, out, &out_len), -EACCES);

	zassert_equal(s_agent.usm_stats.wrong_digests, 1);

	/* Query usmStatsWrongDigests.0 via valid authenticated GET */
	make_get(&req, 0x882, oid_wrong_digests, ARRAY_SIZE(oid_wrong_digests));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, SNMP_TAG_COUNTER32);
	zassert_equal(resp.scoped.pdu.varbinds[0].val.uint_val, 1);

	/* Query usmStatsUnsupportedSecLevels.0 */
	make_get(&req, 0x883, oid_unsupported_sec, ARRAY_SIZE(oid_unsupported_sec));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, SNMP_TAG_COUNTER32);
	zassert_equal(resp.scoped.pdu.varbinds[0].val.uint_val, 0);
}

ZTEST(snmp_usm, test_v3_mpd_stats_mib_queries)
{
	struct snmp_v3_msg req;
	struct snmp_v3_msg resp;
	uint8_t raw[CONFIG_SNMP_MAX_PDU_SIZE];
	uint8_t out[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t out_len;

	static const uint32_t oid_unknown_sec_models[] = {1, 3, 6, 1, 6, 3, 11, 2, 1, 1, 0};

	usm_setup();

	/* Query snmpUnknownSecurityModels.0 */
	make_get(&req, 0x901, oid_unknown_sec_models, ARRAY_SIZE(oid_unknown_sec_models));
	set_user(&req, CONFIG_SNMP_V3_USER_NAME);
	req.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	zassert_ok(snmp_usm_build_outgoing(&s_agent, &req));
	size_t req_len = encode(&req, raw, sizeof(raw));

	zassert_ok(process(raw, req_len, out, &out_len));
	zassert_ok(snmp_ber_decode_v3_msg(out, out_len, &resp));
	zassert_ok(snmp_usm_verify_incoming(&s_agent, &resp, out, out_len));
	zassert_equal(resp.scoped.pdu.error_status, SNMP_ERR_NO_ERROR);
	zassert_equal(resp.scoped.pdu.varbinds[0].type, SNMP_TAG_COUNTER32);
}
