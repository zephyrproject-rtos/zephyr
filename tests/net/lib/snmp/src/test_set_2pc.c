/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_ber.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>

#define ASSERT(c)       zassert_true(c, #c)
#define ASSERT_EQ(a, b) zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b) zassert_not_equal(a, b, NULL)

/* Legacy test state */
static struct snmp_agent s_2pc_agent;
static int32_t s_legacy_val1 = 10;
static int32_t s_legacy_val2 = 20;

static int get_legacy_val1(struct snmp_agent *agent, const struct snmp_mib_node *node,
			   struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = s_legacy_val1;
	return 0;
}

static int set_legacy_val1(struct snmp_agent *agent, struct snmp_mib_node *node,
			   const struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	if (vb->type != ASN1_TAG_INTEGER) {
		return -EINVAL;
	}
	s_legacy_val1 = vb->val.int_val;
	return 0;
}

static int get_legacy_val2(struct snmp_agent *agent, const struct snmp_mib_node *node,
			   struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = s_legacy_val2;
	return 0;
}

static int set_legacy_val2(struct snmp_agent *agent, struct snmp_mib_node *node,
			   const struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	if (vb->type != ASN1_TAG_INTEGER) {
		return -EINVAL;
	}
	s_legacy_val2 = vb->val.int_val;
	return 0;
}

/* 2PC Transactional test state */
static int32_t s_tx_val_a = 50;
static int32_t s_tx_val_b = 60;
static bool s_tx_b_fail_reserve;
static bool s_tx_b_fail_commit;
static bool s_tx_a_undo_called;
static bool s_tx_a_cleanup_called;

static int get_tx_val_a(struct snmp_agent *agent, const struct snmp_mib_node *node,
			struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = s_tx_val_a;
	return 0;
}

static int set_tx_val_a(struct snmp_agent *agent, struct snmp_mib_node *node,
			const struct snmp_varbind *vb, enum snmp_set_action action, void **tx_ctx)
{
	(void)agent;
	(void)node;
	switch (action) {
	case SNMP_SET_RESERVE:
		if (vb->type != ASN1_TAG_INTEGER) {
			return SNMP_ERR_WRONG_TYPE;
		}
		*tx_ctx = (void *)(intptr_t)s_tx_val_a; /* Cache old value in ctx */
		return 0;
	case SNMP_SET_COMMIT:
		s_tx_val_a = vb->val.int_val;
		return 0;
	case SNMP_SET_UNDO:
		s_tx_val_a = (int32_t)(intptr_t)(*tx_ctx);
		s_tx_a_undo_called = true;
		return 0;
	case SNMP_SET_CLEANUP:
		s_tx_a_cleanup_called = true;
		return 0;
	default:
		return -EINVAL;
	}
}

static int get_tx_val_b(struct snmp_agent *agent, const struct snmp_mib_node *node,
			struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = s_tx_val_b;
	return 0;
}

static int set_tx_val_b(struct snmp_agent *agent, struct snmp_mib_node *node,
			const struct snmp_varbind *vb, enum snmp_set_action action, void **tx_ctx)
{
	(void)agent;
	(void)node;
	switch (action) {
	case SNMP_SET_RESERVE:
		if (s_tx_b_fail_reserve) {
			return SNMP_ERR_WRONG_VALUE;
		}
		if (vb->type != ASN1_TAG_INTEGER) {
			return SNMP_ERR_WRONG_TYPE;
		}
		*tx_ctx = (void *)(intptr_t)s_tx_val_b;
		return 0;
	case SNMP_SET_COMMIT:
		if (s_tx_b_fail_commit) {
			return SNMP_ERR_COMMIT_FAILED;
		}
		s_tx_val_b = vb->val.int_val;
		return 0;
	case SNMP_SET_UNDO:
		s_tx_val_b = (int32_t)(intptr_t)(*tx_ctx);
		return 0;
	case SNMP_SET_CLEANUP:
		return 0;
	default:
		return -EINVAL;
	}
}

static void setup_test_nodes(void)
{
	snmp_mib_init(&s_2pc_agent);

	s_legacy_val1 = 10;
	s_legacy_val2 = 20;
	s_tx_val_a = 50;
	s_tx_val_b = 60;
	s_tx_b_fail_reserve = false;
	s_tx_b_fail_commit = false;
	s_tx_a_undo_called = false;
	s_tx_a_cleanup_called = false;

	/* Node 1: 1.3.6.1.4.1.9999.1.0 (legacy) */
	struct snmp_mib_node node1 = {
		.oid = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 9999, 1}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = get_legacy_val1,
		.set_cb = set_legacy_val1,
		.set_tx_cb = NULL,
	};
	snmp_mib_register(&s_2pc_agent, &node1);

	/* Node 2: 1.3.6.1.4.1.9999.2.0 (legacy) */
	struct snmp_mib_node node2 = {
		.oid = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 9999, 2}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = get_legacy_val2,
		.set_cb = set_legacy_val2,
		.set_tx_cb = NULL,
	};
	snmp_mib_register(&s_2pc_agent, &node2);

	/* Node A: 1.3.6.1.4.1.9999.3.0 (2PC) */
	struct snmp_mib_node node_a = {
		.oid = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 9999, 3}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = get_tx_val_a,
		.set_cb = NULL,
		.set_tx_cb = set_tx_val_a,
	};
	snmp_mib_register(&s_2pc_agent, &node_a);

	/* Node B: 1.3.6.1.4.1.9999.4.0 (2PC) */
	struct snmp_mib_node node_b = {
		.oid = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 9999, 4}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = get_tx_val_b,
		.set_cb = NULL,
		.set_tx_cb = set_tx_val_b,
	};
	snmp_mib_register(&s_2pc_agent, &node_b);

	/* Read-only Node: 1.3.6.1.4.1.9999.5.0 */
	struct snmp_mib_node node_ro = {
		.oid = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 9999, 5}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = get_legacy_val1,
		.set_cb = NULL,
		.set_tx_cb = NULL,
	};
	snmp_mib_register(&s_2pc_agent, &node_ro);

	/* Write-only Node (no get_cb): 1.3.6.1.4.1.9999.6.0 */
	struct snmp_mib_node node_wo = {
		.oid = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 9999, 6}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = NULL,
		.set_cb = set_legacy_val1,
		.set_tx_cb = NULL,
	};
	snmp_mib_register(&s_2pc_agent, &node_wo);
}

ZTEST(snmp_set_2pc, test_set_2pc_atomic_failure_legacy)
{
	setup_test_nodes();

	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* SNMPv2c */
	pdu.type = SNMP_PDU_SET_REQ;
	pdu.varbind_cnt = 2;

	/* Varbind 0: valid target Node 1 */
	pdu.varbinds[0].oid.len = 8;
	memcpy(pdu.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 1},
	       8 * sizeof(uint32_t));
	pdu.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu.varbinds[0].val.int_val = 100;

	/* Varbind 1: Node 2 with WRONG ASN.1 TYPE (Octet String instead of Integer) */
	pdu.varbinds[1].oid.len = 8;
	memcpy(pdu.varbinds[1].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 2},
	       8 * sizeof(uint32_t));
	pdu.varbinds[1].type = ASN1_TAG_OCTET_STRING;
	pdu.varbinds[1].val.octet_str.len = 4;
	memcpy(pdu.varbinds[1].val.octet_str.data, "test", 4);

	int ret = snmp_mib_dispatch(&s_2pc_agent, &pdu);
	ASSERT_EQ(ret, 0);

	/* Assert atomic failure: error status is wrongType, error index is 2 */
	ASSERT_EQ(pdu.error_status, SNMP_ERR_WRONG_TYPE);
	ASSERT_EQ(pdu.error_index, 2);

	/* CRITICAL ATOMICITY CHECK: Node 1 must remain 10, not 100! */
	ASSERT_EQ(s_legacy_val1, 10);
	ASSERT_EQ(s_legacy_val2, 20);
}

ZTEST(snmp_set_2pc, test_set_2pc_atomic_failure_tx)
{
	setup_test_nodes();
	s_tx_b_fail_reserve = true;

	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* SNMPv2c */
	pdu.type = SNMP_PDU_SET_REQ;
	pdu.varbind_cnt = 2;

	/* Varbind 0: Node A */
	pdu.varbinds[0].oid.len = 8;
	memcpy(pdu.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 3},
	       8 * sizeof(uint32_t));
	pdu.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu.varbinds[0].val.int_val = 500;

	/* Varbind 1: Node B (will fail in Reserve phase) */
	pdu.varbinds[1].oid.len = 8;
	memcpy(pdu.varbinds[1].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 4},
	       8 * sizeof(uint32_t));
	pdu.varbinds[1].type = ASN1_TAG_INTEGER;
	pdu.varbinds[1].val.int_val = 600;

	int ret = snmp_mib_dispatch(&s_2pc_agent, &pdu);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(pdu.error_status, SNMP_ERR_WRONG_VALUE);
	ASSERT_EQ(pdu.error_index, 2);

	/* Node A must be unchanged and cleanup called */
	ASSERT_EQ(s_tx_val_a, 50);
	ASSERT_EQ(s_tx_a_cleanup_called, true);
	ASSERT_EQ(s_tx_val_b, 60);
}

ZTEST(snmp_set_2pc, test_set_2pc_rollback_on_commit_failure)
{
	setup_test_nodes();
	s_tx_b_fail_commit = true;

	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* SNMPv2c */
	pdu.type = SNMP_PDU_SET_REQ;
	pdu.varbind_cnt = 2;

	/* Varbind 0: Node A */
	pdu.varbinds[0].oid.len = 8;
	memcpy(pdu.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 3},
	       8 * sizeof(uint32_t));
	pdu.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu.varbinds[0].val.int_val = 500;

	/* Varbind 1: Node B (fails in Commit phase) */
	pdu.varbinds[1].oid.len = 8;
	memcpy(pdu.varbinds[1].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 4},
	       8 * sizeof(uint32_t));
	pdu.varbinds[1].type = ASN1_TAG_INTEGER;
	pdu.varbinds[1].val.int_val = 600;

	int ret = snmp_mib_dispatch(&s_2pc_agent, &pdu);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(pdu.error_status, SNMP_ERR_COMMIT_FAILED);
	ASSERT_EQ(pdu.error_index, 2);

	/* Node A committed then rolled back via UNDO! */
	ASSERT_EQ(s_tx_a_undo_called, true);
	ASSERT_EQ(s_tx_val_a, 50);
	ASSERT_EQ(s_tx_val_b, 60);
}

ZTEST(snmp_set_2pc, test_set_2pc_atomic_success)
{
	setup_test_nodes();

	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* SNMPv2c */
	pdu.type = SNMP_PDU_SET_REQ;
	pdu.varbind_cnt = 2;

	/* Varbind 0: Node 1 (legacy) -> 111 */
	pdu.varbinds[0].oid.len = 8;
	memcpy(pdu.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 1},
	       8 * sizeof(uint32_t));
	pdu.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu.varbinds[0].val.int_val = 111;

	/* Varbind 1: Node A (2PC) -> 222 */
	pdu.varbinds[1].oid.len = 8;
	memcpy(pdu.varbinds[1].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 3},
	       8 * sizeof(uint32_t));
	pdu.varbinds[1].type = ASN1_TAG_INTEGER;
	pdu.varbinds[1].val.int_val = 222;

	int ret = snmp_mib_dispatch(&s_2pc_agent, &pdu);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(pdu.error_status, SNMP_ERR_NO_ERROR);
	ASSERT_EQ(pdu.error_index, 0);

	ASSERT_EQ(s_legacy_val1, 111);
	ASSERT_EQ(s_tx_val_a, 222);

	/* Read back verified in response varbinds per RFC 3416 */
	ASSERT_EQ(pdu.varbinds[0].val.int_val, 111);
	ASSERT_EQ(pdu.varbinds[1].val.int_val, 222);
}

ZTEST(snmp_set_2pc, test_smiv2_error_code_mapping_v2c_and_v1)
{
	setup_test_nodes();

	/* 1. Read-only node on v2c request -> notWritable (17) */
	struct snmp_pdu pdu_v2_ro;
	memset(&pdu_v2_ro, 0, sizeof(pdu_v2_ro));

	pdu_v2_ro.version = 1;
	pdu_v2_ro.type = SNMP_PDU_SET_REQ;
	pdu_v2_ro.varbind_cnt = 1;
	pdu_v2_ro.varbinds[0].oid.len = 8;
	memcpy(pdu_v2_ro.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 5},
	       8 * sizeof(uint32_t));
	pdu_v2_ro.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu_v2_ro.varbinds[0].val.int_val = 999;

	snmp_mib_dispatch(&s_2pc_agent, &pdu_v2_ro);
	ASSERT_EQ(pdu_v2_ro.error_status, SNMP_ERR_NOT_WRITABLE);
	ASSERT_EQ(pdu_v2_ro.error_index, 1);

	/* 2. Read-only node on v1 request -> readOnly (4) */
	struct snmp_pdu pdu_v1_ro;
	memset(&pdu_v1_ro, 0, sizeof(pdu_v1_ro));

	pdu_v1_ro.version = 0;
	pdu_v1_ro.type = SNMP_PDU_SET_REQ;
	pdu_v1_ro.varbind_cnt = 1;
	pdu_v1_ro.varbinds[0].oid.len = 8;
	memcpy(pdu_v1_ro.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 5},
	       8 * sizeof(uint32_t));
	pdu_v1_ro.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu_v1_ro.varbinds[0].val.int_val = 999;

	snmp_mib_dispatch(&s_2pc_agent, &pdu_v1_ro);
	ASSERT_EQ(pdu_v1_ro.error_status, SNMP_ERR_READ_ONLY);
	ASSERT_EQ(pdu_v1_ro.error_index, 1);

	/* 3. Type mismatch on v2c -> wrongType (7) */
	struct snmp_pdu pdu_v2_type;
	memset(&pdu_v2_type, 0, sizeof(pdu_v2_type));

	pdu_v2_type.version = 1;
	pdu_v2_type.type = SNMP_PDU_SET_REQ;
	pdu_v2_type.varbind_cnt = 1;
	pdu_v2_type.varbinds[0].oid.len = 8;
	memcpy(pdu_v2_type.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 1},
	       8 * sizeof(uint32_t));
	pdu_v2_type.varbinds[0].type = ASN1_TAG_OCTET_STRING;

	snmp_mib_dispatch(&s_2pc_agent, &pdu_v2_type);
	ASSERT_EQ(pdu_v2_type.error_status, SNMP_ERR_WRONG_TYPE);
	ASSERT_EQ(pdu_v2_type.error_index, 1);

	/* 4. Type mismatch on v1 -> badValue (3) */
	struct snmp_pdu pdu_v1_type;
	memset(&pdu_v1_type, 0, sizeof(pdu_v1_type));

	pdu_v1_type.version = 0;
	pdu_v1_type.type = SNMP_PDU_SET_REQ;
	pdu_v1_type.varbind_cnt = 1;
	pdu_v1_type.varbinds[0].oid.len = 8;
	memcpy(pdu_v1_type.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 1},
	       8 * sizeof(uint32_t));
	pdu_v1_type.varbinds[0].type = ASN1_TAG_OCTET_STRING;

	snmp_mib_dispatch(&s_2pc_agent, &pdu_v1_type);
	ASSERT_EQ(pdu_v1_type.error_status, SNMP_ERR_BAD_VALUE);
	ASSERT_EQ(pdu_v1_type.error_index, 1);
}

ZTEST(snmp_set_2pc, test_multi_instance_independence)
{
	setup_test_nodes();

	struct snmp_agent inst1;
	struct snmp_agent inst2;

	int ret1 = snmp_agent_init(&inst1, NULL);
	int ret2 = snmp_agent_init(&inst2, NULL);
	ASSERT_EQ(ret1, 0);

	ASSERT_EQ(ret2, 0);

	snmp_agent_set_read_community(&inst1, "dept_alpha");
	snmp_agent_set_write_community(&inst1, "secret_alpha");

	snmp_agent_set_read_community(&inst2, "dept_beta");
	snmp_agent_set_write_community(&inst2, "secret_beta");

	/* Build encoded SNMPv2c GET request for Node 1 with community 'dept_alpha' */
	struct snmp_pdu req_pdu;
	memset(&req_pdu, 0, sizeof(req_pdu));

	req_pdu.version = 1; /* v2c */
	strncpy((char *)req_pdu.community, "dept_alpha", sizeof(req_pdu.community) - 1);
	req_pdu.community_len = strlen((char *)req_pdu.community);
	req_pdu.type = SNMP_PDU_GET_REQ;
	req_pdu.request_id = 12345;
	req_pdu.varbind_cnt = 1;
	req_pdu.varbinds[0].oid.len = 8;
	memcpy(req_pdu.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 1},
	       8 * sizeof(uint32_t));
	req_pdu.varbinds[0].type = ASN1_TAG_NULL;

	uint8_t req_buf[256];
	size_t req_len = 0;
	int enc_ret = snmp_ber_encode_pdu(req_buf, sizeof(req_buf), &req_len, &req_pdu);
	ASSERT_EQ(enc_ret, 0);

	uint8_t resp_buf[256];
	size_t resp_len = 0;

	/* 1. Pass request with 'dept_alpha' to inst1 -> should succeed */
	int proc_ret1 = snmp_agent_process_pdu(&inst1, req_buf, req_len, resp_buf, sizeof(resp_buf),
					       &resp_len);
	ASSERT_EQ(proc_ret1, 0);
	ASSERT(resp_len > 0);
	ASSERT_EQ(inst1.counters.snmpInPkts, 1);
	ASSERT_EQ(inst1.counters.snmpInBadCommunityNames, 0);

	/* 2. Pass SAME request with 'dept_alpha' to inst2 -> should be rejected with -EACCES */
	resp_len = 0;
	int proc_ret2 = snmp_agent_process_pdu(&inst2, req_buf, req_len, resp_buf, sizeof(resp_buf),
					       &resp_len);
	ASSERT_EQ(proc_ret2, -EACCES);
	ASSERT_EQ(resp_len, 0);
	ASSERT_EQ(inst2.counters.snmpInPkts, 1);
	ASSERT_EQ(inst2.counters.snmpInBadCommunityNames, 1);

	/* inst1 counters must be completely unaffected by inst2 processing */
	ASSERT_EQ(inst1.counters.snmpInPkts, 1);
	ASSERT_EQ(inst1.counters.snmpInBadCommunityNames, 0);
}

ZTEST(snmp_set_2pc, test_set_write_only_node_preserves_varbind)
{
	setup_test_nodes();

	struct snmp_pdu pdu;
	memset(&pdu, 0, sizeof(pdu));

	pdu.version = 1; /* v2c */
	pdu.type = SNMP_PDU_SET_REQ;
	pdu.varbind_cnt = 1;
	pdu.varbinds[0].oid.len = 8;
	memcpy(pdu.varbinds[0].oid.ids, (uint32_t[]){1, 3, 6, 1, 4, 1, 9999, 6},
	       8 * sizeof(uint32_t));
	pdu.varbinds[0].type = ASN1_TAG_INTEGER;
	pdu.varbinds[0].val.int_val = 777;

	int ret = snmp_mib_dispatch(&s_2pc_agent, &pdu);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(pdu.error_status, SNMP_ERR_NO_ERROR);
	ASSERT_EQ(pdu.error_index, 0);
	ASSERT_EQ(s_legacy_val1, 777);

	/* RFC 1157 §4.1.5 and RFC 3416 §4.2.5: Response varbind contains received value */
	ASSERT_EQ(pdu.varbinds[0].type, ASN1_TAG_INTEGER);
	ASSERT_EQ(pdu.varbinds[0].val.int_val, 777);
}
