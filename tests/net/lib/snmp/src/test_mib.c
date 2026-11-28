/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIB Engine Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/net/snmp/snmp_ber.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>

#define ASSERT(c)          zassert_true(c, #c)
#define ASSERT_EQ(a, b)    zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b)    zassert_not_equal(a, b, NULL)
#define ASSERT_GT(a, b)    zassert_true((a) > (b), NULL)
#define ASSERT_LT(a, b)    zassert_true((a) < (b), NULL)
#define ASSERT_STREQ(a, b) zassert_mem_equal(a, b, strlen(b), NULL)

/*
 * All MIB engine tests run against one explicit instance (no default-agent reliance):
 * the dispatcher and every lookup now takes the owning agent.
 */
static struct snmp_agent s_mib_agent;

static int test_counter = 42;
static int test_get_called;

static int my_get_cb(struct snmp_agent *agent, const struct snmp_mib_node *node,
		     struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	test_get_called++;
	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = test_counter;
	return 0;
}

__attribute__((unused)) static int my_set_cb(struct snmp_agent *agent, struct snmp_mib_node *node,
					     const struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	if (vb->type == SNMP_TAG_COUNTER32) {
		test_counter = vb->val.uint_val;
		return 0;
	}
	return -22;
}

ZTEST(snmp_mib, test_mib_register_get)
{
	test_get_called = 0;
	int ret = snmp_mib_init(&s_mib_agent);
	ASSERT_EQ(ret, 0);

	struct snmp_mib_node node = {0};
	node.oid.len = 9;

	node.oid.ids[0] = 1;
	node.oid.ids[1] = 3;
	node.oid.ids[2] = 6;
	node.oid.ids[3] = 1;
	node.oid.ids[4] = 4;
	node.oid.ids[5] = 1;
	node.oid.ids[6] = 99;
	node.oid.ids[7] = 1;
	node.oid.ids[8] = 0;
	node.type = SNMP_TAG_COUNTER32;
	node.get_cb = my_get_cb;
	node.set_cb = NULL;

	ret = snmp_mib_register(&s_mib_agent, &node);
	ASSERT_EQ(ret, 0);

	struct snmp_varbind vb = {0};
	ret = snmp_mib_get(&s_mib_agent, &node.oid, &vb);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(test_get_called, 1);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 42);
}

ZTEST(snmp_mib, test_mib_get_next)
{
	snmp_mib_init(&s_mib_agent);

	struct snmp_mib_node node_a = {0};
	node_a.oid.len = 9;

	node_a.oid.ids[0] = 1;
	node_a.oid.ids[1] = 3;
	node_a.oid.ids[2] = 6;
	node_a.oid.ids[3] = 1;
	node_a.oid.ids[4] = 4;
	node_a.oid.ids[5] = 1;
	node_a.oid.ids[6] = 99;
	node_a.oid.ids[7] = 1;
	node_a.oid.ids[8] = 0;
	node_a.type = SNMP_TAG_COUNTER32;
	node_a.get_cb = my_get_cb;
	snmp_mib_register(&s_mib_agent, &node_a);

	struct snmp_mib_node node_b = {0};
	node_b.oid.len = 9;

	node_b.oid.ids[0] = 1;
	node_b.oid.ids[1] = 3;
	node_b.oid.ids[2] = 6;
	node_b.oid.ids[3] = 1;
	node_b.oid.ids[4] = 4;
	node_b.oid.ids[5] = 1;
	node_b.oid.ids[6] = 99;
	node_b.oid.ids[7] = 2;
	node_b.oid.ids[8] = 0;
	node_b.type = SNMP_TAG_GAUGE32;
	node_b.get_cb = my_get_cb;
	snmp_mib_register(&s_mib_agent, &node_b);

	struct snmp_oid query = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 0, 99}};
	struct snmp_varbind vb = {0};
	int ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(vb.oid.ids[7], 1);

	ret = snmp_mib_get_next(&s_mib_agent, &node_a.oid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.ids[7], 2);

	struct snmp_oid past_end = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 99, 0}};
	ret = snmp_mib_get_next(&s_mib_agent, &past_end, &vb);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_END_MIB_VIEW);
}

ZTEST(snmp_mib, test_mib_duplicate_oid)
{
	snmp_mib_init(&s_mib_agent);

	struct snmp_mib_node node = {0};
	node.oid.len = 9;

	node.oid.ids[0] = 1;
	node.oid.ids[1] = 3;
	node.oid.ids[2] = 6;
	node.oid.ids[3] = 1;
	node.oid.ids[4] = 4;
	node.oid.ids[5] = 1;
	node.oid.ids[6] = 99;
	node.oid.ids[7] = 1;
	node.oid.ids[8] = 0;
	node.type = SNMP_TAG_COUNTER32;
	node.get_cb = my_get_cb;

	int ret = snmp_mib_register(&s_mib_agent, &node);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib_register(&s_mib_agent, &node);
	ASSERT_EQ(ret, 0);
}

static int32_t g_test_table_data[3][3] = {
	{0, 0, 0},
	{0, 101, 102}, /* row 1: col 1, col 2 */
	{0, 201, 202}  /* row 2: col 1, col 2 */
};

static int test_col1_get(struct snmp_agent *agent, const struct snmp_table_row *row, uint8_t column,
			 struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = g_test_table_data[row->index][1];
	return 0;
}

static int test_col2_get(struct snmp_agent *agent, const struct snmp_table_row *row, uint8_t column,
			 struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = g_test_table_data[row->index][2];
	return 0;
}

static int test_col2_set(struct snmp_agent *agent, struct snmp_table_row *row, uint8_t column,
			 const struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (vb->type != ASN1_TAG_INTEGER) {
		return -EINVAL;
	}
	g_test_table_data[row->index][2] = vb->val.int_val;
	return 0;
}

static const uint8_t g_test_cols[] = {1, 2};
static const uint8_t g_test_types[] = {ASN1_TAG_INTEGER, ASN1_TAG_INTEGER};
static snmp_table_get_cb_t g_test_get_cbs[] = {test_col1_get, test_col2_get};
static snmp_table_set_cb_t g_test_set_cbs[] = {NULL, test_col2_set};

static struct snmp_table_row g_test_rows[2] = {{.index = 1, .next = NULL, .user_data = NULL},
					       {.index = 2, .next = NULL, .user_data = NULL}};

static struct snmp_mib_table g_test_tbl = {
	.table_oid = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 10, 1}},
	.column_cnt = 2,
	.column_subids = g_test_cols,
	.column_types = g_test_types,
	.get_cbs = g_test_get_cbs,
	.set_cbs = g_test_set_cbs,
	.rows = NULL,
	.user_data = NULL};

ZTEST(snmp_mib, test_table_register_get)
{
	snmp_mib_init(&s_mib_agent);

	g_test_tbl.rows = NULL;
	g_test_rows[0].next = NULL;
	g_test_rows[1].next = NULL;

	int ret = snmp_mib_register_table(&s_mib_agent, &g_test_tbl);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[0]);
	ASSERT_EQ(ret, 0);
	ret = snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[1]);
	ASSERT_EQ(ret, 0);

	/* Get col 1, row 1 */
	struct snmp_oid oid_c1_r1 = {.len = 11, .ids = {1, 3, 6, 1, 4, 1, 99, 10, 1, 1, 1}};
	struct snmp_varbind vb = {0};
	ret = snmp_mib_get(&s_mib_agent, &oid_c1_r1, &vb);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.val.int_val, 101);

	/* Get col 2, row 2 */
	struct snmp_oid oid_c2_r2 = {.len = 11, .ids = {1, 3, 6, 1, 4, 1, 99, 10, 1, 2, 2}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&s_mib_agent, &oid_c2_r2, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.val.int_val, 202);

	/* Non-existent row 3 */
	struct snmp_oid oid_c1_r3 = {.len = 11, .ids = {1, 3, 6, 1, 4, 1, 99, 10, 1, 1, 3}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&s_mib_agent, &oid_c1_r3, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_INST);
}

ZTEST(snmp_mib, test_table_get_next_order)
{
	snmp_mib_init(&s_mib_agent);

	g_test_tbl.rows = NULL;
	g_test_rows[0].next = NULL;
	g_test_rows[1].next = NULL;

	snmp_mib_register_table(&s_mib_agent, &g_test_tbl);
	snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[0]);
	snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[1]);

	struct snmp_varbind vb = {0};

	/* Query before table: 1.3.6.1.4.1.99.10.0 */
	struct snmp_oid query = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 10, 0}};
	int ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);

	/* Should hit col 1, row 1 */
	ASSERT_EQ(vb.oid.len, 11);
	ASSERT_EQ(vb.oid.ids[9], 1);  /* col 1 */
	ASSERT_EQ(vb.oid.ids[10], 1); /* row 1 */
	ASSERT_EQ(vb.val.int_val, 101);

	/* Next from col 1, row 1 -> col 1, row 2 */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.ids[9], 1);  /* col 1 */
	ASSERT_EQ(vb.oid.ids[10], 2); /* row 2 */
	ASSERT_EQ(vb.val.int_val, 201);

	/* Next from col 1, row 2 -> col 2, row 1 (column-first ordering!) */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.ids[9], 2);  /* col 2 */
	ASSERT_EQ(vb.oid.ids[10], 1); /* row 1 */
	ASSERT_EQ(vb.val.int_val, 102);

	/* Next from col 2, row 1 -> col 2, row 2 */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.ids[9], 2);  /* col 2 */
	ASSERT_EQ(vb.oid.ids[10], 2); /* row 2 */
	ASSERT_EQ(vb.val.int_val, 202);

	/* Next from col 2, row 2 -> end of MIB view */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_END_MIB_VIEW);
}

ZTEST(snmp_mib, test_table_set)
{
	snmp_mib_init(&s_mib_agent);

	g_test_tbl.rows = NULL;
	g_test_rows[0].next = NULL;
	g_test_rows[1].next = NULL;

	snmp_mib_register_table(&s_mib_agent, &g_test_tbl);
	snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[0]);
	snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[1]);

	/* Set col 2, row 1 to 999 */
	struct snmp_varbind vb_set = {
		.oid = {.len = 11, .ids = {1, 3, 6, 1, 4, 1, 99, 10, 1, 2, 1}},
		.type = ASN1_TAG_INTEGER,
		.val = {.int_val = 999}};
	int ret = snmp_mib_set(&s_mib_agent, &vb_set.oid, &vb_set);
	ASSERT_EQ(ret, 0);

	/* Verify via get */
	struct snmp_varbind vb_get = {0};
	ret = snmp_mib_get(&s_mib_agent, &vb_set.oid, &vb_get);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb_get.val.int_val, 999);
}

ZTEST(snmp_mib, test_interleaved_scalar_and_table_walk)
{
	snmp_mib_init(&s_mib_agent);

	/* Scalar before table: 1.3.6.1.4.1.99.5.0 */
	struct snmp_mib_node node_before = {0};
	node_before.oid.len = 9;

	node_before.oid.ids[0] = 1;
	node_before.oid.ids[1] = 3;
	node_before.oid.ids[2] = 6;
	node_before.oid.ids[3] = 1;
	node_before.oid.ids[4] = 4;
	node_before.oid.ids[5] = 1;
	node_before.oid.ids[6] = 99;
	node_before.oid.ids[7] = 5;
	node_before.oid.ids[8] = 0;
	node_before.type = SNMP_TAG_COUNTER32;
	node_before.get_cb = my_get_cb;
	snmp_mib_register(&s_mib_agent, &node_before);

	/* Table: 1.3.6.1.4.1.99.10.1 */
	g_test_tbl.rows = NULL;
	g_test_rows[0].next = NULL;
	g_test_rows[1].next = NULL;

	snmp_mib_register_table(&s_mib_agent, &g_test_tbl);
	snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[0]);
	snmp_mib_table_add_row(&s_mib_agent, &g_test_tbl, &g_test_rows[1]);

	/* Scalar after table: 1.3.6.1.4.1.99.20.0 */
	struct snmp_mib_node node_after = {0};
	node_after.oid.len = 9;

	node_after.oid.ids[0] = 1;
	node_after.oid.ids[1] = 3;
	node_after.oid.ids[2] = 6;
	node_after.oid.ids[3] = 1;
	node_after.oid.ids[4] = 4;
	node_after.oid.ids[5] = 1;
	node_after.oid.ids[6] = 99;
	node_after.oid.ids[7] = 20;
	node_after.oid.ids[8] = 0;
	node_after.type = SNMP_TAG_GAUGE32;
	node_after.get_cb = my_get_cb;
	snmp_mib_register(&s_mib_agent, &node_after);

	struct snmp_varbind vb = {0};

	/* Query 1.3.6.1.4.1.99.0 */
	struct snmp_oid query = {.len = 8, .ids = {1, 3, 6, 1, 4, 1, 99, 0}};
	int ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(vb.oid.ids[7], 5); /* scalar before */

	/* Next -> table col 1, row 1 */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.ids[7], 10);
	ASSERT_EQ(vb.oid.ids[9], 1);
	ASSERT_EQ(vb.oid.ids[10], 1);

	/* Advance through table */
	query = vb.oid;
	snmp_mib_get_next(&s_mib_agent, &query, &vb); /* col 1, row 2 */
	ASSERT_EQ(vb.oid.ids[9], 1);
	ASSERT_EQ(vb.oid.ids[10], 2);

	query = vb.oid;
	snmp_mib_get_next(&s_mib_agent, &query, &vb); /* col 2, row 1 */
	ASSERT_EQ(vb.oid.ids[9], 2);
	ASSERT_EQ(vb.oid.ids[10], 1);

	query = vb.oid;
	snmp_mib_get_next(&s_mib_agent, &query, &vb); /* col 2, row 2 */
	ASSERT_EQ(vb.oid.ids[9], 2);
	ASSERT_EQ(vb.oid.ids[10], 2);

	/* Next -> scalar after table (1.3.6.1.4.1.99.20.0) */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.ids[7], 20); /* scalar after */

	/* Next -> endOfMibView */
	query = vb.oid;
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_END_MIB_VIEW);
}

static int bsearch_test_cb(struct snmp_agent *agent, const struct snmp_mib_node *node,
			   struct snmp_varbind *vb)
{
	(void)agent;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = (int32_t)node->oid.ids[7];
	return 0;
}

ZTEST(snmp_mib, test_mib_bsearch_lookup)
{
	static struct snmp_agent agent;
	int ret = snmp_mib_init(&agent);
	ASSERT_EQ(ret, 0);

	/* Register 5 nodes out of order to verify lexicographic sorting */
	static const uint32_t subids[] = {30, 10, 50, 20, 40};
	for (size_t i = 0; i < 5; i++) {

		struct snmp_mib_node node = {0};
		node.oid.len = 9;

		node.oid.ids[0] = 1;
		node.oid.ids[1] = 3;
		node.oid.ids[2] = 6;
		node.oid.ids[3] = 1;
		node.oid.ids[4] = 4;
		node.oid.ids[5] = 1;
		node.oid.ids[6] = 88;
		node.oid.ids[7] = subids[i];
		node.oid.ids[8] = 0;
		node.type = ASN1_TAG_INTEGER;
		node.get_cb = bsearch_test_cb;
		ret = snmp_mib_register(&agent, &node);
		ASSERT_EQ(ret, 0);
	}

	struct snmp_varbind vb;

	/* 1. First node (subid 10) */
	struct snmp_oid oid_first = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 88, 10, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_first, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 10);

	/* 2. Middle node (subid 30) */
	struct snmp_oid oid_mid = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 88, 30, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_mid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 30);

	/* 3. Last node (subid 50) */
	struct snmp_oid oid_last = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 88, 50, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_last, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 50);

	/* 4. Non-existent nodes */
	struct snmp_oid oid_none1 = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 88, 5, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_none1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);

	struct snmp_oid oid_none2 = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 88, 25, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_none2, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);

	struct snmp_oid oid_none3 = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 88, 99, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_none3, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);
}

static int scalar_cb_42(struct snmp_agent *agent, const struct snmp_mib_node *node,
			struct snmp_varbind *vb)
{
	(void)agent;
	(void)node;
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = 42;
	return 0;
}

ZTEST(snmp_mib, test_mib_no_such_object_vs_instance)
{
	static struct snmp_agent agent;
	int ret = snmp_mib_init(&agent);
	ASSERT_EQ(ret, 0);

	/* Register scalar: 1.3.6.1.4.1.500.1.0 */
	struct snmp_mib_node scalar_node = {0};
	scalar_node.oid.len = 9;

	scalar_node.oid.ids[0] = 1;
	scalar_node.oid.ids[1] = 3;
	scalar_node.oid.ids[2] = 6;
	scalar_node.oid.ids[3] = 1;
	scalar_node.oid.ids[4] = 4;
	scalar_node.oid.ids[5] = 1;
	scalar_node.oid.ids[6] = 500;
	scalar_node.oid.ids[7] = 1;
	scalar_node.oid.ids[8] = 0;
	scalar_node.type = ASN1_TAG_INTEGER;
	scalar_node.get_cb = scalar_cb_42;
	ret = snmp_mib_register(&agent, &scalar_node);
	ASSERT_EQ(ret, 0);

	/* Register table: 1.3.6.1.4.1.500.2.1 with columns {1, 2} and row 1 */
	static const uint8_t t_cols[] = {1, 2};
	static const uint8_t t_types[] = {ASN1_TAG_INTEGER, ASN1_TAG_INTEGER};
	static snmp_table_get_cb_t t_get_cbs[] = {test_col1_get, test_col2_get};
	static struct snmp_table_row t_row = {.index = 1, .next = NULL, .user_data = NULL};
	struct snmp_mib_table t_table = {
		.table_oid = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 500, 2, 1}},
		.column_cnt = 2,
		.column_subids = t_cols,
		.column_types = t_types,
		.get_cbs = t_get_cbs,
		.set_cbs = NULL,
		.rows = NULL,
		.user_data = NULL};
	ret = snmp_mib_register_table(&agent, &t_table);
	ASSERT_EQ(ret, 0);
	ret = snmp_mib_table_add_row(&agent, &t_table, &t_row);
	ASSERT_EQ(ret, 0);

	struct snmp_varbind vb;

	/* 1. Query valid scalar .0: returns value */
	struct snmp_oid oid_scalar_0 = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 500, 1, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_scalar_0, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 42);

	/* 2. Query scalar .1: returns noSuchInstance */
	struct snmp_oid oid_scalar_1 = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 500, 1, 1}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_scalar_1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_INST);

	/* 3. Query completely foreign OID 1.3.6.1.2.1.99.1.0: returns noSuchObject */
	struct snmp_oid oid_foreign = {.len = 9, .ids = {1, 3, 6, 1, 2, 1, 99, 1, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_foreign, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);

	/* 4. Query table with existing column (1) and non-existing row (99): returns noSuchInstance
	 */
	struct snmp_oid oid_tbl_no_row = {.len = 11, .ids = {1, 3, 6, 1, 4, 1, 500, 2, 1, 1, 99}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_tbl_no_row, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_INST);

	/* 5. Query table with non-existing column (5) and row 1: returns noSuchObject */
	struct snmp_oid oid_tbl_no_col = {.len = 11, .ids = {1, 3, 6, 1, 4, 1, 500, 2, 1, 5, 1}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get(&agent, &oid_tbl_no_col, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);

	/* 6. Verify SNMPv1 converts both noSuchInstance and noSuchObject to noSuchName */
	struct snmp_pdu v1_pdu;
	memset(&v1_pdu, 0, sizeof(v1_pdu));

	v1_pdu.version = 0; /* SNMPv1 */
	v1_pdu.type = SNMP_PDU_GET_REQ;
	v1_pdu.varbind_cnt = 1;
	v1_pdu.varbinds[0].oid = oid_scalar_1; /* noSuchInstance */
	ret = snmp_mib_dispatch(&agent, &v1_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(v1_pdu.error_status, SNMP_ERR_NO_SUCH_NAME);
	ASSERT_EQ(v1_pdu.error_index, 1);

	memset(&v1_pdu, 0, sizeof(v1_pdu));
	v1_pdu.version = 0; /* SNMPv1 */
	v1_pdu.type = SNMP_PDU_GET_REQ;
	v1_pdu.varbind_cnt = 1;
	v1_pdu.varbinds[0].oid = oid_foreign; /* noSuchObject */
	ret = snmp_mib_dispatch(&agent, &v1_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(v1_pdu.error_status, SNMP_ERR_NO_SUCH_NAME);
	ASSERT_EQ(v1_pdu.error_index, 1);
}

ZTEST(snmp_mib, test_getbulk_response_pruning_on_buffer_limit)
{
	static struct snmp_agent agent;
	int ret = snmp_agent_init(&agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_agent_set_read_community(&agent, "public");
	ASSERT_EQ(ret, 0);

	/* Register 10 scalar nodes: 1.3.6.1.4.1.600.1.0 to 1.3.6.1.4.1.600.10.0 */
	for (uint32_t i = 1; i <= 10; i++) {
		struct snmp_mib_node node = {0};
		node.oid.len = 9;

		node.oid.ids[0] = 1;
		node.oid.ids[1] = 3;
		node.oid.ids[2] = 6;
		node.oid.ids[3] = 1;
		node.oid.ids[4] = 4;
		node.oid.ids[5] = 1;
		node.oid.ids[6] = 600;
		node.oid.ids[7] = i;
		node.oid.ids[8] = 0;
		node.type = ASN1_TAG_INTEGER;
		node.get_cb = scalar_cb_42;
		ret = snmp_mib_register(&agent, &node);
		ASSERT_EQ(ret, 0);
	}

	/* Build GETBULK request: non_repeaters = 0, max_repetitions = 10 */
	struct snmp_pdu req_pdu;
	memset(&req_pdu, 0, sizeof(req_pdu));

	req_pdu.version = 1; /* SNMPv2c */
	req_pdu.type = SNMP_PDU_GET_BULK_REQ;
	req_pdu.request_id = 999;
	req_pdu.non_repeaters = 0;
	req_pdu.max_repetitions = 10;
	strncpy((char *)req_pdu.community, "public", sizeof(req_pdu.community) - 1);
	req_pdu.community_len = strlen((char *)req_pdu.community);
	req_pdu.varbind_cnt = 1;
	req_pdu.varbinds[0].oid.len = 8;
	req_pdu.varbinds[0].oid.ids[0] = 1;
	req_pdu.varbinds[0].oid.ids[1] = 3;
	req_pdu.varbinds[0].oid.ids[2] = 6;
	req_pdu.varbinds[0].oid.ids[3] = 1;
	req_pdu.varbinds[0].oid.ids[4] = 4;
	req_pdu.varbinds[0].oid.ids[5] = 1;
	req_pdu.varbinds[0].oid.ids[6] = 600;
	req_pdu.varbinds[0].oid.ids[7] = 0; /* will getNext to .1.0 */
	req_pdu.varbinds[0].type = ASN1_TAG_NULL;

	uint8_t req_buf[256];
	size_t req_len = 0;
	ret = snmp_ber_encode_pdu(req_buf, sizeof(req_buf), &req_len, &req_pdu);

	ASSERT_EQ(ret, 0);

	/* 1. Large buffer test: full 10 repetitions fit */
	uint8_t resp_buf[1024];
	size_t resp_len = 0;
	ret = snmp_agent_process_pdu(&agent, req_buf, req_len, resp_buf, sizeof(resp_buf),

				     &resp_len);
	ASSERT_EQ(ret, 0);

	struct snmp_pdu resp_pdu;
	memset(&resp_pdu, 0, sizeof(resp_pdu));

	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, SNMP_ERR_NO_ERROR);
	ASSERT_EQ(resp_pdu.varbind_cnt, 10);
	size_t full_resp_len = resp_len;

	/* 2. Constrained buffer test: small buffer size that can hold some but not all varbinds */
	size_t small_buf_sz = full_resp_len / 2;
	memset(resp_buf, 0, sizeof(resp_buf));

	resp_len = 0;
	ret = snmp_agent_process_pdu(&agent, req_buf, req_len, resp_buf, small_buf_sz, &resp_len);
	ASSERT_EQ(ret, 0);
	ASSERT_GT(resp_len, 0);
	ASSERT(resp_len <= small_buf_sz);

	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, SNMP_ERR_NO_ERROR);
	ASSERT_GT(resp_pdu.varbind_cnt, 0);
	ASSERT_LT(resp_pdu.varbind_cnt, 10);

	/* 3. Extremely small buffer test: non_repeaters cannot fit in buffer */
	/* With non_repeaters = 1, repetition pruning stops at 1 varbind (~42 bytes). */
	/* When buffer is 35 bytes, non-repeater cannot fit -> returns tooBig with varbind_cnt = 0
	 */
	struct snmp_pdu req_pdu2;
	memset(&req_pdu2, 0, sizeof(req_pdu2));

	req_pdu2.version = 1; /* SNMPv2c */
	req_pdu2.type = SNMP_PDU_GET_BULK_REQ;
	req_pdu2.request_id = 1000;
	req_pdu2.non_repeaters = 1;
	req_pdu2.max_repetitions = 5;
	strncpy((char *)req_pdu2.community, "public", sizeof(req_pdu2.community) - 1);
	req_pdu2.community_len = strlen((char *)req_pdu2.community);
	req_pdu2.varbind_cnt = 1;
	req_pdu2.varbinds[0].oid.len = 8;
	req_pdu2.varbinds[0].oid.ids[0] = 1;
	req_pdu2.varbinds[0].oid.ids[1] = 3;
	req_pdu2.varbinds[0].oid.ids[2] = 6;
	req_pdu2.varbinds[0].oid.ids[3] = 1;
	req_pdu2.varbinds[0].oid.ids[4] = 4;
	req_pdu2.varbinds[0].oid.ids[5] = 1;
	req_pdu2.varbinds[0].oid.ids[6] = 600;
	req_pdu2.varbinds[0].oid.ids[7] = 0;
	req_pdu2.varbinds[0].type = ASN1_TAG_NULL;

	ret = snmp_ber_encode_pdu(req_buf, sizeof(req_buf), &req_len, &req_pdu2);
	ASSERT_EQ(ret, 0);

	uint32_t too_bigs_before = agent.counters.snmpOutTooBigs;
	size_t tiny_buf_sz = 35;
	memset(resp_buf, 0, sizeof(resp_buf));

	resp_len = 0;
	ret = snmp_agent_process_pdu(&agent, req_buf, req_len, resp_buf, tiny_buf_sz, &resp_len);
	ASSERT_EQ(ret, 0);
	ASSERT_GT(resp_len, 0);
	ASSERT(resp_len <= tiny_buf_sz);

	memset(&resp_pdu, 0, sizeof(resp_pdu));
	ret = snmp_ber_decode_pdu(resp_buf, resp_len, &resp_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(resp_pdu.error_status, SNMP_ERR_TOO_BIG);
	ASSERT_EQ(resp_pdu.error_index, 0);
	ASSERT_EQ(resp_pdu.varbind_cnt, 0);
	ASSERT_GT(agent.counters.snmpOutTooBigs, too_bigs_before);
}

static int32_t g_ip_table_data[2][3] = {
	{0, 10, 100}, /* row 0: col 1 = 10, col 2 = 100 */
	{0, 20, 200}  /* row 1: col 1 = 20, col 2 = 200 */
};

static int test_ip_col1_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			    uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	int32_t *data = (int32_t *)row->user_data;
	vb->type = ASN1_TAG_INTEGER;

	vb->val.int_val = data[1];
	return 0;
}

static int test_ip_col2_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			    uint8_t column, struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	int32_t *data = (int32_t *)row->user_data;
	vb->type = ASN1_TAG_INTEGER;

	vb->val.int_val = data[2];
	return 0;
}

static int test_ip_col2_set(struct snmp_agent *agent, struct snmp_table_row *row, uint8_t column,
			    const struct snmp_varbind *vb)
{
	(void)agent;
	(void)column;
	if (vb->type != ASN1_TAG_INTEGER) {
		return -EINVAL;
	}
	int32_t *data = (int32_t *)row->user_data;
	data[2] = vb->val.int_val;

	return 0;
}

static const uint8_t g_ip_cols[] = {1, 2};
static const uint8_t g_ip_types[] = {ASN1_TAG_INTEGER, ASN1_TAG_INTEGER};
static snmp_table_get_cb_t g_ip_get_cbs[] = {test_ip_col1_get, test_ip_col2_get};
static snmp_table_set_cb_t g_ip_set_cbs[] = {NULL, test_ip_col2_set};

ZTEST(snmp_mib, test_table_compound_index)
{
	snmp_mib_init(&s_mib_agent);

	g_ip_table_data[0][1] = 10;
	g_ip_table_data[0][2] = 100;
	g_ip_table_data[1][1] = 20;
	g_ip_table_data[1][2] = 200;

	struct snmp_mib_table ip_tbl = {
		.table_oid = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 20, 1}},
		.column_cnt = 2,
		.column_subids = g_ip_cols,
		.column_types = g_ip_types,
		.get_cbs = g_ip_get_cbs,
		.set_cbs = g_ip_set_cbs,
		.rows = NULL,
		.user_data = NULL};

	struct snmp_table_row row1 = {.index = 0,
				      .index_oid = {.len = 4, .subids = {192, 168, 1, 10}},
				      .next = NULL,
				      .user_data = g_ip_table_data[0]};

	struct snmp_table_row row2 = {.index = 0,
				      .index_oid = {.len = 4, .subids = {192, 168, 1, 20}},
				      .next = NULL,
				      .user_data = g_ip_table_data[1]};

	int ret = snmp_mib_register_table(&s_mib_agent, &ip_tbl);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib_table_add_row(&s_mib_agent, &ip_tbl, &row1);
	ASSERT_EQ(ret, 0);
	ret = snmp_mib_table_add_row(&s_mib_agent, &ip_tbl, &row2);
	ASSERT_EQ(ret, 0);

	/* 1. GET on exact compound OID: col 1, row 1 (192.168.1.10) */
	struct snmp_oid oid_c1_r1 = {.len = 14,
				     .ids = {1, 3, 6, 1, 4, 1, 99, 20, 1, 1, 192, 168, 1, 10}};
	struct snmp_varbind vb = {0};
	ret = snmp_mib_get(&s_mib_agent, &oid_c1_r1, &vb);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 10);

	/* GET on exact compound OID: col 2, row 2 (192.168.1.20) */
	struct snmp_oid oid_c2_r2 = {.len = 14,
				     .ids = {1, 3, 6, 1, 4, 1, 99, 20, 1, 2, 192, 168, 1, 20}};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_mib_agent, &oid_c2_r2, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 200);

	/* 2. GET on non-existent IP returns SNMP_TAG_EXCEPT_NO_SUCH_INST */
	struct snmp_oid oid_no_such_inst = {
		.len = 14, .ids = {1, 3, 6, 1, 4, 1, 99, 20, 1, 1, 192, 168, 1, 99}};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_mib_agent, &oid_no_such_inst, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_INST);

	/* GET on non-existent column returns SNMP_TAG_EXCEPT_NO_SUCH_OBJ */
	struct snmp_oid oid_no_such_col = {
		.len = 14, .ids = {1, 3, 6, 1, 4, 1, 99, 20, 1, 3, 192, 168, 1, 10}};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_mib_agent, &oid_no_such_col, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_OBJ);

	/* 3. GETNEXT order across compound rows and columns */
	/* Query before table: 1.3.6.1.4.1.99.20.0 */
	struct snmp_oid query = {.len = 9, .ids = {1, 3, 6, 1, 4, 1, 99, 20, 0}};
	memset(&vb, 0, sizeof(vb));

	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.len, 14);
	ASSERT_EQ(vb.oid.ids[9], 1);   /* col 1 */
	ASSERT_EQ(vb.oid.ids[13], 10); /* row 1 (.10) */
	ASSERT_EQ(vb.val.int_val, 10);

	/* Next from col 1, row 1 -> col 1, row 2 (.20) */
	query = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.len, 14);
	ASSERT_EQ(vb.oid.ids[9], 1);   /* col 1 */
	ASSERT_EQ(vb.oid.ids[13], 20); /* row 2 (.20) */
	ASSERT_EQ(vb.val.int_val, 20);

	/* Next from col 1, row 2 -> col 2, row 1 (.10) */
	query = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.len, 14);
	ASSERT_EQ(vb.oid.ids[9], 2);   /* col 2 */
	ASSERT_EQ(vb.oid.ids[13], 10); /* row 1 (.10) */
	ASSERT_EQ(vb.val.int_val, 100);

	/* Next from col 2, row 1 -> col 2, row 2 (.20) */
	query = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.oid.len, 14);
	ASSERT_EQ(vb.oid.ids[9], 2);   /* col 2 */
	ASSERT_EQ(vb.oid.ids[13], 20); /* row 2 (.20) */
	ASSERT_EQ(vb.val.int_val, 200);

	/* Next from col 2, row 2 -> end of MIB view */
	query = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_mib_agent, &query, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_END_MIB_VIEW);

	/* 4. SET on compound indexed row */
	struct snmp_varbind vb_set = {
		.oid = {.len = 14, .ids = {1, 3, 6, 1, 4, 1, 99, 20, 1, 2, 192, 168, 1, 10}},
		.type = ASN1_TAG_INTEGER,
		.val = {.int_val = 555}};
	ret = snmp_mib_set(&s_mib_agent, &vb_set.oid, &vb_set);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(g_ip_table_data[0][2], 555);

	/* Verify via GET */
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_mib_agent, &vb_set.oid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.val.int_val, 555);

	/* 5. 2PC SET dispatch on compound indexed row */
	struct snmp_pdu set_pdu;
	memset(&set_pdu, 0, sizeof(set_pdu));

	set_pdu.version = 1; /* SNMPv2c */
	set_pdu.type = SNMP_PDU_SET_REQ;
	set_pdu.varbind_cnt = 1;
	set_pdu.varbinds[0].oid = oid_c2_r2;
	set_pdu.varbinds[0].type = ASN1_TAG_INTEGER;
	set_pdu.varbinds[0].val.int_val = 777;

	ret = snmp_mib_dispatch(&s_mib_agent, &set_pdu);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(set_pdu.error_status, SNMP_ERR_NO_ERROR);
	ASSERT_EQ(g_ip_table_data[1][2], 777);

	/* 6. Remove row by compound index */
	struct snmp_table_index del_idx = {.len = 4, .subids = {192, 168, 1, 10}};
	ret = snmp_mib_table_remove_row_by_index(&s_mib_agent, &ip_tbl, &del_idx);

	ASSERT_EQ(ret, 0);

	/* GET on removed row returns noSuchInstance */
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_mib_agent, &oid_c1_r1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_EXCEPT_NO_SUCH_INST);

	/* Removing it again returns -ENOENT */
	ret = snmp_mib_table_remove_row_by_index(&s_mib_agent, &ip_tbl, &del_idx);
	ASSERT_EQ(ret, -ENOENT);
}
