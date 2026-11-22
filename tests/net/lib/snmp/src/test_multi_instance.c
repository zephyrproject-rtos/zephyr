/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * Multi-Instance Coexistence & Zero-Singleton Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_ber.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>
#include <zephyr/net/snmp/snmp_mib2.h>

#define ASSERT(c)          zassert_true(c, #c)
#define ASSERT_EQ(a, b)    zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b)    zassert_not_equal(a, b, NULL)
#define ASSERT_GT(a, b)    zassert_true((a) > (b), NULL)
#define ASSERT_LT(a, b)    zassert_true((a) < (b), NULL)
#define ASSERT_STREQ(a, b) zassert_mem_equal(a, b, strlen(b), NULL)

static int get_val_1111(struct snmp_agent *agent, const struct snmp_mib_node *node,
			struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 1234;
	return 0;
}

static int get_val_2222(struct snmp_agent *agent, const struct snmp_mib_node *node,
			struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 5678;
	return 0;
}

static size_t build_get_req_packet(uint8_t *buf, size_t buf_sz, const char *community,
				   const struct snmp_oid *oid, int32_t req_id)
{
	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* SNMPv2c */
	pdu.type = SNMP_PDU_GET_REQ;
	pdu.request_id = req_id;
	strncpy((char *)pdu.community, community, sizeof(pdu.community) - 1);
	pdu.community_len = strlen((char *)pdu.community);

	pdu.varbind_cnt = 1;
	pdu.varbinds[0].oid = *oid;
	pdu.varbinds[0].type = ASN1_TAG_NULL;

	size_t enc_len = 0;
	int ret = snmp_ber_encode_pdu(buf, buf_sz, &enc_len, &pdu);
	ASSERT_EQ(ret, 0);

	return enc_len;
}

ZTEST(snmp_multi_instance, test_multi_instance_coexistence)
{
	static struct snmp_agent lan_agent;
	static struct snmp_agent wan_agent;

	/* 1. Initialize both independent instances */
	int ret = snmp_agent_init(&lan_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_agent_init(&wan_agent, NULL);
	ASSERT_EQ(ret, 0);

	/* 2. Configure distinct community strings */
	ret = snmp_agent_set_read_community(&lan_agent, "lan_pub");
	ASSERT_EQ(ret, 0);
	ret = snmp_agent_set_read_community(&wan_agent, "wan_pub");
	ASSERT_EQ(ret, 0);

	/* 3. Configure distinct sysName strings */
	ret = snmp_mib2_set_sysname(&lan_agent, "router-lan");
	ASSERT_EQ(ret, 0);
	ret = snmp_mib2_set_sysname(&wan_agent, "gateway-wan");
	ASSERT_EQ(ret, 0);

	/* 4. Register scalar .1.3.6.1.4.1.1111.1.0 on lan_agent only */
	struct snmp_mib_node node1 = {0};
	node1.oid.len = 9;

	node1.oid.ids[0] = 1;
	node1.oid.ids[1] = 3;
	node1.oid.ids[2] = 6;
	node1.oid.ids[3] = 1;
	node1.oid.ids[4] = 4;
	node1.oid.ids[5] = 1;
	node1.oid.ids[6] = 1111;
	node1.oid.ids[7] = 1;
	node1.oid.ids[8] = 0;
	node1.type = ASN1_TAG_INTEGER;
	node1.get_cb = get_val_1111;
	ret = snmp_mib_register(&lan_agent, &node1);
	ASSERT_EQ(ret, 0);

	/* 5. Register scalar .1.3.6.1.4.1.2222.1.0 on wan_agent only */
	struct snmp_mib_node node2 = {0};
	node2.oid.len = 9;

	node2.oid.ids[0] = 1;
	node2.oid.ids[1] = 3;
	node2.oid.ids[2] = 6;
	node2.oid.ids[3] = 1;
	node2.oid.ids[4] = 4;
	node2.oid.ids[5] = 1;
	node2.oid.ids[6] = 2222;
	node2.oid.ids[7] = 1;
	node2.oid.ids[8] = 0;
	node2.type = ASN1_TAG_INTEGER;
	node2.get_cb = get_val_2222;
	ret = snmp_mib_register(&wan_agent, &node2);
	ASSERT_EQ(ret, 0);

	uint8_t req_buf[512];
	uint8_t resp_buf[512];
	size_t req_len;
	size_t resp_len;
	struct snmp_pdu resp_pdu;

	/* 6. Query lan_agent for .1.3.6.1.4.1.1111.1.0 with community "lan_pub" */
	req_len = build_get_req_packet(req_buf, sizeof(req_buf), "lan_pub", &node1.oid, 101);
	ret = snmp_agent_process_pdu(&lan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, 0);
	ASSERT(resp_len > 0);

	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, 0);
	ASSERT_EQ(resp_pdu.varbind_cnt, 1);
	ASSERT_EQ(resp_pdu.varbinds[0].type, ASN1_TAG_INTEGER);
	ASSERT_EQ(resp_pdu.varbinds[0].val.int_val, 1234);

	/* Assert LAN agent statistics incremented, WAN agent untouched */
	ASSERT_EQ(lan_agent.counters.snmpInPkts, 1);
	ASSERT_EQ(lan_agent.counters.snmpOutPkts, 1);
	ASSERT_EQ(wan_agent.counters.snmpInPkts, 0);
	ASSERT_EQ(wan_agent.counters.snmpOutPkts, 0);

	/* 7. Send "lan_pub" request to wan_agent -> must be rejected (-EACCES) */
	ret = snmp_agent_process_pdu(&wan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, -EACCES);
	ASSERT_EQ(wan_agent.counters.snmpInPkts, 1);
	ASSERT_EQ(wan_agent.counters.snmpInBadCommunityNames, 1);
	ASSERT_EQ(wan_agent.counters.snmpOutPkts, 0);
	ASSERT_EQ(lan_agent.counters.snmpInBadCommunityNames, 0);

	/* 8. Query wan_agent for .1.3.6.1.4.1.2222.1.0 with community "wan_pub" */
	req_len = build_get_req_packet(req_buf, sizeof(req_buf), "wan_pub", &node2.oid, 202);
	ret = snmp_agent_process_pdu(&wan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, 0);
	ASSERT(resp_len > 0);

	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, 0);
	ASSERT_EQ(resp_pdu.varbind_cnt, 1);
	ASSERT_EQ(resp_pdu.varbinds[0].type, ASN1_TAG_INTEGER);
	ASSERT_EQ(resp_pdu.varbinds[0].val.int_val, 5678);
	ASSERT_EQ(wan_agent.counters.snmpInPkts, 2);
	ASSERT_EQ(wan_agent.counters.snmpOutPkts, 1);

	/* 9. Cross-instance isolation: Query lan_agent for .1.3.6.1.4.1.2222.1.0 (WAN OID) */
	req_len = build_get_req_packet(req_buf, sizeof(req_buf), "lan_pub", &node2.oid, 103);
	ret = snmp_agent_process_pdu(&lan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, 0);
	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	/* In SNMPv2c, an unmapped OID produces SNMP_TAG_EXCEPT_NO_SUCH_OBJ or noSuchName error */
	ASSERT(resp_pdu.varbinds[0].type == SNMP_TAG_EXCEPT_NO_SUCH_OBJ ||
	       resp_pdu.varbinds[0].type == SNMP_TAG_EXCEPT_NO_SUCH_INST ||
	       resp_pdu.error_status == SNMP_ERR_NO_SUCH_NAME);

	/* 10. Cross-instance isolation: Query wan_agent for .1.3.6.1.4.1.1111.1.0 (LAN OID) */
	req_len = build_get_req_packet(req_buf, sizeof(req_buf), "wan_pub", &node1.oid, 204);
	ret = snmp_agent_process_pdu(&wan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, 0);
	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT(resp_pdu.varbinds[0].type == SNMP_TAG_EXCEPT_NO_SUCH_OBJ ||
	       resp_pdu.varbinds[0].type == SNMP_TAG_EXCEPT_NO_SUCH_INST ||
	       resp_pdu.error_status == SNMP_ERR_NO_SUCH_NAME);

	/* 11. Query MIB-II sysName on lan_agent -> should return "router-lan" */
	struct snmp_oid sysname_oid = {.len = 9, .ids = {1, 3, 6, 1, 2, 1, 1, 5, 0}};
	req_len = build_get_req_packet(req_buf, sizeof(req_buf), "lan_pub", &sysname_oid, 105);

	ret = snmp_agent_process_pdu(&lan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, 0);
	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, 0);
	ASSERT_EQ(resp_pdu.varbinds[0].type, ASN1_TAG_OCTET_STRING);
	ASSERT_EQ(strcmp((char *)resp_pdu.varbinds[0].val.octet_str.data, "router-lan"), 0);

	/* 12. Query MIB-II sysName on wan_agent -> should return "gateway-wan" */
	req_len = build_get_req_packet(req_buf, sizeof(req_buf), "wan_pub", &sysname_oid, 206);
	ret = snmp_agent_process_pdu(&wan_agent, req_buf, req_len, resp_buf, sizeof(resp_buf),
				     &resp_len);
	ASSERT_EQ(ret, 0);
	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, 0);
	ASSERT_EQ(resp_pdu.varbinds[0].type, ASN1_TAG_OCTET_STRING);
	ASSERT_EQ(strcmp((char *)resp_pdu.varbinds[0].val.octet_str.data, "gateway-wan"), 0);
}

/* -------------------------------------------------------------------------- */
/* MIB-II snmp-group (1.3.6.1.2.1.11) counter isolation                        */
/*                                                                            */
/* Every double-count / global mirror listed in design contract 6.2 leaks the  */
/* traffic of one agent into another agent's statistics. Driving traffic       */
/* exclusively on agent A and then reading the whole .11 group out of both    */
/* agents' trees catches all of them at once, host-side, without sockets.     */
/* -------------------------------------------------------------------------- */

/* RFC 3418 / RFC 1213: sub-identifiers 7 and 23 of the snmp group do not exist. */
static const uint32_t ci_subids[] = {
	1,  2,  3,  4,  5,  6,  8,  9,  10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 24, 25, 26, 27, 28, 29, 30, 31, 32,
};

static void ci_make_counter_oid(struct snmp_oid *oid, uint32_t subid)
{
	oid->len = 9;
	oid->ids[0] = 1;
	oid->ids[1] = 3;
	oid->ids[2] = 6;
	oid->ids[3] = 1;
	oid->ids[4] = 2;
	oid->ids[5] = 1;
	oid->ids[6] = 11;
	oid->ids[7] = subid;
	oid->ids[8] = 0;
}

static size_t ci_build_request(uint8_t *buf, size_t buf_sz, const char *community,
			       const struct snmp_oid *oid, uint8_t pdu_type, int32_t req_id,
			       int32_t max_rep)
{
	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* SNMPv2c */
	pdu.type = pdu_type;
	pdu.request_id = req_id;
	pdu.max_repetitions = max_rep;
	strncpy((char *)pdu.community, community, sizeof(pdu.community) - 1);
	pdu.community_len = strlen((char *)pdu.community);
	pdu.varbind_cnt = 1;
	pdu.varbinds[0].oid = *oid;
	pdu.varbinds[0].type = ASN1_TAG_NULL;

	size_t enc_len = 0;
	ASSERT_EQ(snmp_ber_encode_pdu(buf, buf_sz, &enc_len, &pdu), 0);

	return enc_len;
}

/* Read one .11.<subid>.0 node of this agent through its own MIB tree. */
static uint32_t ci_read_counter(struct snmp_agent *agent, uint32_t subid)
{
	struct snmp_oid oid;
	ci_make_counter_oid(&oid, subid);

	struct snmp_varbind vb;
	memset(&vb, 0, sizeof(vb));

	int ret = snmp_mib_get(agent, &oid, &vb);
	zassert_equal(ret, 0, "snmp_mib_get(.11.%u.0) -> %d", subid, ret);

	return vb.val.uint_val;
}

ZTEST(snmp_multi_instance, test_multi_instance_counter_isolation)
{
	static struct snmp_agent agent_a;
	static struct snmp_agent agent_b;

	ASSERT_EQ(snmp_agent_init(&agent_a, NULL), 0);
	ASSERT_EQ(snmp_agent_init(&agent_b, NULL), 0);
	ASSERT_EQ(snmp_agent_set_read_community(&agent_a, "pubA"), 0);
	ASSERT_EQ(snmp_agent_set_read_community(&agent_b, "pubB"), 0);

	uint8_t req[512];
	uint8_t resp[512];
	size_t len;
	size_t resp_len;

	struct snmp_oid sysname = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 1, 5, 0},
	};
	struct snmp_oid absent = {
		.len = 9,
		.ids = {1, 3, 6, 1, 4, 1, 99999, 1, 0},
	};
	struct snmp_oid uptime = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 1, 3, 0},
	};

	/* --- Traffic on agent A only, exercising as many counter paths as possible --- */

	/* 1. Successful GET */
	len = ci_build_request(req, sizeof(req), "pubA", &sysname, SNMP_PDU_GET_REQ, 1, 0);
	snmp_agent_process_pdu(&agent_a, req, len, resp, sizeof(resp), &resp_len);

	/* 2. Successful GETNEXT */
	len = ci_build_request(req, sizeof(req), "pubA", &sysname, SNMP_PDU_GET_NEXT_REQ, 2, 0);
	snmp_agent_process_pdu(&agent_a, req, len, resp, sizeof(resp), &resp_len);

	/* 3. GETBULK: the snmpInTotalReqVars accumulation path of contract 6.2 */
	len = ci_build_request(req, sizeof(req), "pubA", &sysname, SNMP_PDU_GET_BULK_REQ, 3, 3);
	snmp_agent_process_pdu(&agent_a, req, len, resp, sizeof(resp), &resp_len);

	/* 4. Wrong community (snmpInBadCommunityNames path) */
	len = ci_build_request(req, sizeof(req), "wrong", &sysname, SNMP_PDU_GET_REQ, 4, 0);
	ASSERT_EQ(snmp_agent_process_pdu(&agent_a, req, len, resp, sizeof(resp), &resp_len),
		  -EACCES);

	/* 5. Unknown OID (snmpInNoSuchNames / snmpOutNoSuchNames path) */
	len = ci_build_request(req, sizeof(req), "pubA", &absent, SNMP_PDU_GET_REQ, 5, 0);
	snmp_agent_process_pdu(&agent_a, req, len, resp, sizeof(resp), &resp_len);

	/* 6. SET on a read-only object (snmpInReadOnlys path) */
	len = ci_build_request(req, sizeof(req), "pubA", &uptime, SNMP_PDU_SET_REQ, 6, 0);
	snmp_agent_process_pdu(&agent_a, req, len, resp, sizeof(resp), &resp_len);

	/* --- The assertion that catches every mirror bug of contract 6.2 --- */

	/* 7. A saw all of that traffic; every counter of B must still read zero. */
	ASSERT(ci_read_counter(&agent_a, 1) > 0); /* snmpInPkts */
	for (size_t i = 0; i < ARRAY_SIZE(ci_subids); i++) {
		uint32_t sub = ci_subids[i];
		uint32_t val_b = ci_read_counter(&agent_b, sub);
		if (sub == 30) { /* snmpEnableAuthenTraps defaults to 1, not a counter */

			ASSERT_EQ(val_b, 1);
		} else {
			zassert_equal(val_b, 0, "agent B .11.%u.0 reads %u, expected 0", sub,
				      val_b);
		}
	}

	/* 8. Cross-check the MIB nodes against the snapshot API of the same fields. */
	struct snmp_mib2_counters cb;
	snmp_agent_get_counters(&agent_b, &cb);

	const uint32_t *words = (const uint32_t *)&cb;
	size_t authen_word =
		offsetof(struct snmp_mib2_counters, snmpEnableAuthenTraps) / sizeof(uint32_t);
	for (size_t i = 0; i < sizeof(cb) / sizeof(uint32_t); i++) {
		if (i == authen_word) {
			continue;
		}
		zassert_equal(words[i], 0, "agent B counter word %zu reads %u", i, words[i]);
	}

	/* 9. A's own counters did move, and they are visible through the MIB too. */
	struct snmp_mib2_counters ca;
	snmp_agent_get_counters(&agent_a, &ca);

	ASSERT(ca.snmpInPkts > 0);
	ASSERT(ca.snmpInBadCommunityNames > 0);
	ASSERT(ca.snmpInTotalReqVars > 0);
	ASSERT_EQ(ci_read_counter(&agent_a, 1), ca.snmpInPkts);
	ASSERT_EQ(ci_read_counter(&agent_a, 13), ca.snmpInTotalReqVars);

	/* 10. The one writable snmp-group object is per-agent as well: setting .11.30.0
	 *     on A (RFC 3418 INTEGER 1|2) must not change B.
	 */
	struct snmp_varbind vb_set;
	memset(&vb_set, 0, sizeof(vb_set));

	vb_set.type = ASN1_TAG_INTEGER;
	vb_set.val.int_val = 2;
	struct snmp_oid authen = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 11, 30, 0},
	};
	ASSERT_EQ(snmp_mib_set(&agent_a, &authen, &vb_set), 0);
	ASSERT_EQ(agent_a.counters.snmpEnableAuthenTraps, 2);
	ASSERT_EQ(agent_b.counters.snmpEnableAuthenTraps, 1);
	ASSERT_EQ(ci_read_counter(&agent_b, 30), 1);

	/* 11. MIB-II default nodes must not alias instance state via user_data
	 *     (decision 2): every system-group and snmp-group node carries NULL.
	 */
	for (size_t i = 0; i < agent_a.mib_node_cnt; i++) {
		const struct snmp_mib_node *n = &agent_a.mib_nodes[i];
		if (n->oid.len == 9 && n->oid.ids[0] == 1 && n->oid.ids[3] == 1 &&

		    n->oid.ids[5] == 1 && (n->oid.ids[6] == 1 || n->oid.ids[6] == 11)) {
			zassert_is_null(
				n->user_data,
				"default node still aliases instance state through user_data");
		}
	}

	/* 12. Counter clearing is per-agent too. */
	snmp_agent_zero_counters(&agent_a);
	snmp_agent_get_counters(&agent_a, &ca);
	snmp_agent_get_counters(&agent_b, &cb);
	ASSERT_EQ(ca.snmpInPkts, 0);
	ASSERT_EQ(ca.snmpEnableAuthenTraps, 1);
	/* B was never touched: not by A's SET of .11.30.0, not by A's clearing. */
	ASSERT_EQ(cb.snmpInPkts, 0);
	ASSERT_EQ(cb.snmpEnableAuthenTraps, 1);
}

ZTEST(snmp_multi_instance, test_agent_lifecycle)
{
	static struct snmp_agent agent;
	struct snmp_agent_cfg cfg = {
		.port = 1161,
		.sys_name = "LifecycleTest",
	};

	/* 1. NULL checks and uninitialized operations return -EINVAL or -EPERM */
	ASSERT_EQ(snmp_agent_init(NULL, &cfg), -EINVAL);

	/* 2. Initialize agent */
	ASSERT_EQ(snmp_agent_init(&agent, &cfg), 0);
	ASSERT_EQ(agent.state, SNMP_AGENT_INITIALIZED);

#if defined(__ZEPHYR__)
	/* 3. Start agent */
	ASSERT_EQ(snmp_agent_start(&agent), 0);
	ASSERT_EQ(agent.state, SNMP_AGENT_RUNNING);

	/* 4. Double-start is rejected with -EPERM */
	ASSERT_EQ(snmp_agent_start(&agent), -EPERM);

	/* 5. Re-init while running is rejected with -EBUSY */
	ASSERT_EQ(snmp_agent_init(&agent, &cfg), -EBUSY);

	/* 6. Stop agent */
	ASSERT_EQ(snmp_agent_stop(&agent), 0);
	ASSERT_EQ(agent.state, SNMP_AGENT_STOPPED);

	/* 7. Stop is idempotent */
	ASSERT_EQ(snmp_agent_stop(&agent), 0);

	/* 8. Restart after stop */
	ASSERT_EQ(snmp_agent_start(&agent), 0);
	ASSERT_EQ(agent.state, SNMP_AGENT_RUNNING);
	ASSERT_EQ(snmp_agent_stop(&agent), 0);
#endif

	/* 9. Deinit agent resets to INITIALIZED */
	ASSERT_EQ(snmp_agent_deinit(&agent), 0);
	ASSERT_EQ(agent.state, SNMP_AGENT_INITIALIZED);
}
