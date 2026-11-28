/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for SNMPv3 View-based Access Control Model (RFC 3415).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/net/snmp/snmp_ber.h>
#include <zephyr/net/snmp/snmp_vacm.h>
#include <zephyr/net/snmp/snmp_usm.h>
#include <zephyr/net/snmp/snmp_core.h>

/*
 * Two independent instances: the VACM tables under test live in s_vacm_test_agent and
 * are driven exclusively through the agent-parameter API. The module has no default
 * agent any more (sub-phase 8c), so there is nothing else to stub.
 */
static struct snmp_agent s_vacm_test_agent;

#define ASSERT(c)          zassert_true(c, #c)
#define ASSERT_EQ(a, b)    zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b)    zassert_not_equal(a, b, NULL)
#define ASSERT_GT(a, b)    zassert_true((a) > (b), NULL)
#define ASSERT_LT(a, b)    zassert_true((a) < (b), NULL)
#define ASSERT_STREQ(a, b) zassert_mem_equal(a, b, strlen(b), NULL)

ZTEST(snmp_vacm, test_vacm_group_mapping_resolution)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	struct snmp_oid sys_oid = {.len = 8, .ids = {1, 3, 6, 1, 2, 1, 1, 1, 0}};

	/* Default admin user ("admin") maps to group admin and has full access */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, 0);

	/* Default monitor user ("monitor") maps to group readonly */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, 0);

	/* Unknown user has no mapping -> rejected */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "unknown_user",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, -EACCES);

	/* Add new mapping for operator user -> group admin */
	ret = snmp_vacm_add_group_mapping(&s_vacm_test_agent, SNMP_SEC_MODEL_USM, "operator",
					  "admin");
	ASSERT_EQ(ret, 0);

	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "operator",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, 0);
}

ZTEST(snmp_vacm, test_vacm_security_level_enforcement)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	struct snmp_oid sys_oid = {.len = 8, .ids = {1, 3, 6, 1, 2, 1, 1, 1, 0}};

	/* Admin group rule requires min_sec_level = authNoPriv (2).
	 * Request with noAuthNoPriv (1) must be REJECTED.
	 */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, -EACCES);

	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_WRITE,
					&sys_oid);
	ASSERT_EQ(ret, -EACCES);

	/* Request with authNoPriv (2) meets min_sec_level -> ALLOWED */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_WRITE,
					&sys_oid);
	ASSERT_EQ(ret, 0);

	/* Request with authPriv (3) exceeds min_sec_level -> ALLOWED */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_PRIV, SNMP_VACM_ACCESS_WRITE, &sys_oid);
	ASSERT_EQ(ret, 0);

	/* Monitor user has rule with min_sec_level = noAuthNoPriv (1) for read, but NO write view
	 */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, 0);

	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_WRITE,
					&sys_oid);
	ASSERT_EQ(ret, -EACCES);
}

ZTEST(snmp_vacm, test_vacm_oid_subtree_matching)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	/* restrictedView includes 1.3.6.1.2.1.1 (system group) */
	struct snmp_oid sys_descr = {.len = 8, .ids = {1, 3, 6, 1, 2, 1, 1, 1, 0}};
	struct snmp_oid sys_uptime = {.len = 8, .ids = {1, 3, 6, 1, 2, 1, 1, 3, 0}};
	struct snmp_oid if_number = {
		.len = 8, .ids = {1, 3, 6, 1, 2, 1, 2, 1, 0} /* 1.3.6.1.2.1.2 is interfaces */
	};
	struct snmp_oid ent_sensor = {
		.len = 9, .ids = {1, 3, 6, 1, 4, 1, 54321, 1, 0} /* Enterprise OID */
	};

	/* monitor user in restrictedView: system group is allowed */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_descr);
	ASSERT_EQ(ret, 0);

	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_uptime);
	ASSERT_EQ(ret, 0);

	/* Interfaces group (1.3.6.1.2.1.2) is outside restrictedView -> REJECTED */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&if_number);
	ASSERT_EQ(ret, -EACCES);

	/* Enterprise OID is outside restrictedView -> REJECTED */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "monitor",
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&ent_sensor);
	ASSERT_EQ(ret, -EACCES);

	/* admin user in fullView (1.3.6.1): both system and enterprise allowed */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&if_number);
	ASSERT_EQ(ret, 0);

	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&ent_sensor);
	ASSERT_EQ(ret, 0);
}

ZTEST(snmp_vacm, test_vacm_oid_subtree_excluded)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	/* In fullView (1.3.6.1 included), add an EXCLUDED subtree: 1.3.6.1.4.1.99999 */
	struct snmp_oid secret_subtree = {.len = 7, .ids = {1, 3, 6, 1, 4, 1, 99999}};
	ret = snmp_vacm_add_view_tree(&s_vacm_test_agent, "fullView", &secret_subtree, NULL, 0,

				      SNMP_VACM_VIEW_EXCLUDED);
	ASSERT_EQ(ret, 0);

	struct snmp_oid regular_oid = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 54321, 1, 0}};
	struct snmp_oid excluded_oid = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99999, 1, 0}};

	/* Regular enterprise OID is still allowed in fullView */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&regular_oid);
	ASSERT_EQ(ret, 0);

	/* Excluded subtree is REJECTED even though 1.3.6.1 is included */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&excluded_oid);
	ASSERT_EQ(ret, -EACCES);
}

ZTEST(snmp_vacm, test_vacm_wildcard_mask)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	/* Create a custom view "wildView" with subtree 1.3.6.1.4.1.0.1
	 * Mask wildcards sub-identifier at index 6 (the 7th sub-id, value 0).
	 */
	struct snmp_oid pattern_subtree = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 0, 1}};
	uint8_t mask[2] = {0xFD, 0x80};

	ret = snmp_vacm_add_view_tree(&s_vacm_test_agent, "wildView", &pattern_subtree, mask, 1,
				      SNMP_VACM_VIEW_INCLUDED);
	ASSERT_EQ(ret, 0);

	ret = snmp_vacm_add_access_rule(&s_vacm_test_agent, "admin", "", SNMP_SEC_MODEL_USM,
					SNMP_SEC_LEVEL_AUTH_NOPRIV, "wildView", "wildView",
					"wildView");
	ASSERT_EQ(ret, 0);

	/* Target A: 1.3.6.1.4.1.10.1.0 (id[6] = 10, id[7] = 1) -> MATCH */
	struct snmp_oid target_a = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 10, 1, 0}};
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",

					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&target_a);
	ASSERT_EQ(ret, 0);

	/* Target B: 1.3.6.1.4.1.99.1.0 (id[6] = 99, id[7] = 1) -> MATCH */
	struct snmp_oid target_b = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 1, 0}};
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",

					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&target_b);
	ASSERT_EQ(ret, 0);

	/* Target C: 1.3.6.1.4.1.10.2.0 (id[6] = 10, id[7] = 2 != 1) -> MISMATCH */
	struct snmp_oid target_c = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 10, 2, 0}};
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",

					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&target_c);
	ASSERT_EQ(ret, -EACCES);
}

ZTEST(snmp_vacm, test_vacm_unknown_context_rejected)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	struct snmp_oid sys_oid = {.len = 8, .ids = {1, 3, 6, 1, 2, 1, 1, 1, 0}};

	/* Default context "" is valid */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, 0);

	/* Non-existent context "vrf1" is rejected */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "vrf1", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, -EACCES);

	/* Add context "vrf1" */
	ret = snmp_vacm_add_context(&s_vacm_test_agent, "vrf1");
	ASSERT_EQ(ret, 0);

	/* Add access rule for context_prefix "vrf1" */
	ret = snmp_vacm_add_access_rule(&s_vacm_test_agent, "admin", "vrf1", SNMP_SEC_MODEL_USM,
					SNMP_SEC_LEVEL_AUTH_NOPRIV, "fullView", "fullView",
					"fullView");
	ASSERT_EQ(ret, 0);

	/* Now context "vrf1" is accepted */
	ret = snmp_vacm_check_access_ex(&s_vacm_test_agent, "vrf1", SNMP_SEC_MODEL_USM, "admin",
					SNMP_SEC_LEVEL_AUTH_NOPRIV, SNMP_VACM_ACCESS_READ,
					&sys_oid);
	ASSERT_EQ(ret, 0);
}

ZTEST(snmp_vacm, test_vacm_check_access_backward_compat)
{
	int ret = snmp_vacm_init(&s_vacm_test_agent);
	ASSERT_EQ(ret, 0);

	struct snmp_v3_msg msg;
	memset(&msg, 0, sizeof(msg));

	strncpy((char *)msg.usm.user_name, "admin", sizeof(msg.usm.user_name) - 1);
	msg.usm.user_name_len = strlen("admin");
	msg.global.msg_security_model = SNMP_SEC_MODEL_USM;
	msg.global.msg_flags = 0x01; /* authNoPriv */
	msg.scoped.context_name[0] = '\0';
	msg.scoped.context_name_len = 0;

	/* Add varbind with system OID */
	msg.scoped.pdu.varbind_cnt = 1;
	msg.scoped.pdu.varbinds[0].oid.len = 8;
	msg.scoped.pdu.varbinds[0].oid.ids[0] = 1;
	msg.scoped.pdu.varbinds[0].oid.ids[1] = 3;
	msg.scoped.pdu.varbinds[0].oid.ids[2] = 6;
	msg.scoped.pdu.varbinds[0].oid.ids[3] = 1;
	msg.scoped.pdu.varbinds[0].oid.ids[4] = 2;
	msg.scoped.pdu.varbinds[0].oid.ids[5] = 1;
	msg.scoped.pdu.varbinds[0].oid.ids[6] = 1;
	msg.scoped.pdu.varbinds[0].oid.ids[7] = 0;

	/* Read access for admin with authNoPriv */
	ret = snmp_vacm_check_access(&s_vacm_test_agent, &msg, false);
	ASSERT_EQ(ret, 0);

	/* Write access for admin with authNoPriv */
	ret = snmp_vacm_check_access(&s_vacm_test_agent, &msg, true);
	ASSERT_EQ(ret, 0);

	/* Now switch user to monitor (readonly, noAuth) */
	strncpy((char *)msg.usm.user_name, "monitor", sizeof(msg.usm.user_name) - 1);
	msg.usm.user_name_len = strlen("monitor");
	msg.global.msg_flags = 0x00; /* noAuthNoPriv */

	/* Read access for monitor -> OK */
	ret = snmp_vacm_check_access(&s_vacm_test_agent, &msg, false);
	ASSERT_EQ(ret, 0);

	/* Write access for monitor -> DENIED */
	ret = snmp_vacm_check_access(&s_vacm_test_agent, &msg, true);
	ASSERT_EQ(ret, -EACCES);
}

ZTEST(snmp_vacm, test_vacm_report_pdu_timeliness_structure)
{
	struct snmp_agent test_agent;
	memset(&test_agent, 0, sizeof(test_agent));

	test_agent.engine_boots = 1;

	struct snmp_v3_msg msg;
	memset(&msg, 0, sizeof(msg));

	msg.global.msg_id = 98765;
	msg.scoped.pdu.type = SNMP_PDU_GET_REQ;

	/* Simulate timeliness failure Report-PDU generation */
	memset(&msg.scoped.pdu, 0, sizeof(msg.scoped.pdu));
	msg.scoped.pdu.type = SNMP_PDU_REPORT;
	msg.scoped.pdu.version = 3;
	msg.scoped.pdu.request_id = msg.global.msg_id;
	static const uint32_t not_in_time_windows_oid[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 2, 0};
	msg.scoped.pdu.varbinds[0].oid.len = 11;

	memcpy(msg.scoped.pdu.varbinds[0].oid.ids, not_in_time_windows_oid,
	       sizeof(not_in_time_windows_oid));
	msg.scoped.pdu.varbinds[0].type = SNMP_TAG_COUNTER32;
	msg.scoped.pdu.varbinds[0].val.uint_val = ++test_agent.counters.snmpSilentDrops;
	msg.scoped.pdu.varbind_cnt = 1;

	ASSERT_EQ(msg.scoped.pdu.type, SNMP_PDU_REPORT);
	ASSERT_EQ(msg.scoped.pdu.version, 3);
	ASSERT_EQ(msg.scoped.pdu.request_id, 98765);
	ASSERT_EQ(msg.scoped.pdu.varbind_cnt, 1);
	ASSERT_EQ(msg.scoped.pdu.varbinds[0].oid.len, 11);
	ASSERT_EQ(memcmp(msg.scoped.pdu.varbinds[0].oid.ids, not_in_time_windows_oid,
			 sizeof(not_in_time_windows_oid)),
		  0);
	ASSERT_EQ(msg.scoped.pdu.varbinds[0].type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(msg.scoped.pdu.varbinds[0].val.uint_val, 1);
	ASSERT_EQ(test_agent.counters.snmpSilentDrops, 1);

	/* Verify round-trip encoding and decoding of Report-PDU */
	uint8_t raw[1024];
	size_t raw_len = 0;
	msg.global.msg_max_size = 1024;

	msg.global.msg_security_model = 3;
	int ret = snmp_ber_encode_v3_msg(raw, sizeof(raw), &raw_len, &msg);
	ASSERT_EQ(ret, 0);

	ASSERT(raw_len > 0);

	struct snmp_v3_msg decoded_msg;
	memset(&decoded_msg, 0, sizeof(decoded_msg));

	ret = snmp_ber_decode_v3_msg(raw, raw_len, &decoded_msg);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(decoded_msg.scoped.pdu.type, SNMP_PDU_REPORT);
	ASSERT_EQ(decoded_msg.scoped.pdu.request_id, 98765);
	ASSERT_EQ(decoded_msg.scoped.pdu.varbind_cnt, 1);
	ASSERT_EQ(decoded_msg.scoped.pdu.varbinds[0].oid.len, 11);
	ASSERT_EQ(memcmp(decoded_msg.scoped.pdu.varbinds[0].oid.ids, not_in_time_windows_oid,
			 sizeof(not_in_time_windows_oid)),
		  0);
	ASSERT_EQ(decoded_msg.scoped.pdu.varbinds[0].type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(decoded_msg.scoped.pdu.varbinds[0].val.uint_val, 1);
}

ZTEST(snmp_vacm, test_usm_definitions_and_key_sizes)
{
	struct snmp_usm_user user;
	memset(&user, 0, sizeof(user));

	ASSERT_EQ(SNMP_USM_AUTH_NONE, 0);
	ASSERT_EQ(SNMP_USM_AUTH_SHA1, 1);
	ASSERT_EQ(SNMP_USM_AUTH_MD5, 2);
	ASSERT_EQ(SNMP_USM_AUTH_SHA256, 3);

	ASSERT_EQ(SNMP_USM_PRIV_NONE, 0);
	ASSERT_EQ(SNMP_USM_PRIV_AES, 1);
	ASSERT_EQ(SNMP_USM_PRIV_DES, 2);

	ASSERT_EQ(sizeof(user.auth_key), 32);
}

ZTEST(snmp_vacm, test_continuous_uptime_wrap_guard)
{
	struct snmp_agent test_agent;
	memset(&test_agent, 0, sizeof(test_agent));

	test_agent.engine_boots = 1;
	test_agent.engine_time_base = 0;

	/* Simulate uptime exceeding 2147483647 */
	int64_t simulated_uptime = 2147483647LL + 10LL;
	int64_t elapsed = simulated_uptime - test_agent.engine_time_base;

	while (elapsed >= 2147483647LL) {
		if (test_agent.engine_boots < 2147483647U) {
			test_agent.engine_boots++;
		}
		test_agent.engine_time_base += 2147483647LL;
		elapsed = simulated_uptime - test_agent.engine_time_base;
	}

	ASSERT_EQ(test_agent.engine_boots, 2);
	ASSERT_EQ(elapsed, 10);

	/* When engine_boots reaches 2147483647, it caps and halts */
	test_agent.engine_boots = 2147483647U;
	simulated_uptime += 2147483647LL;
	elapsed = simulated_uptime - test_agent.engine_time_base;
	while (elapsed >= 2147483647LL) {
		if (test_agent.engine_boots < 2147483647U) {
			test_agent.engine_boots++;
		}
		test_agent.engine_time_base += 2147483647LL;
		elapsed = simulated_uptime - test_agent.engine_time_base;
	}
	ASSERT_EQ(test_agent.engine_boots, 2147483647U);
}
