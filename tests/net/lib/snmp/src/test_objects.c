/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/snmp.h>
#include <zephyr/sys/util.h>

#include "test_objects.h"

/* Rows are deliberately out of order to prove the agent sorts them. */
static struct test_row t_rows[] = {
	{.idx = {5}, .idx_len = 1, .value = 50},
	{.idx = {2}, .idx_len = 1, .value = 20},
	{.idx = {10}, .idx_len = 1, .value = 100},
};

static struct test_row u_rows[] = {
	{.idx = {1, 10}, .idx_len = 2, .value = 110},
	{.idx = {2, 0}, .idx_len = 2, .value = 200},
	{.idx = {1, 9}, .idx_len = 2, .value = 19},
};

static void test_foreach_row(const struct snmp_table *tbl, snmp_row_cb_t cb, void *user_data)
{
	const struct test_table *tt = CONTAINER_OF(tbl, struct test_table, table);

	for (size_t i = 0; i < tt->count; i++) {
		cb(&tt->rows[i], user_data);
	}
}

static int test_row_index(const struct snmp_table *tbl, void *row, uint32_t *arcs, size_t max_arcs)
{
	const struct test_row *r = row;

	ARG_UNUSED(tbl);

	if (max_arcs < r->idx_len) {
		return -EMSGSIZE;
	}

	memcpy(arcs, r->idx, r->idx_len * sizeof(uint32_t));

	return (int)r->idx_len;
}

static void *test_row_lookup(const struct snmp_table *tbl, const uint32_t *arcs, size_t len)
{
	const struct test_table *tt = CONTAINER_OF(tbl, struct test_table, table);

	for (size_t i = 0; i < tt->count; i++) {
		struct test_row *r = &tt->rows[i];

		if (r->idx_len == len && memcmp(r->idx, arcs, len * sizeof(uint32_t)) == 0) {
			return r;
		}
	}

	return NULL;
}

struct test_table test_t = {
	.table = {
		.foreach_row = test_foreach_row,
		.row_index = test_row_index,
		.row_lookup = test_row_lookup,
	},
	.rows = t_rows,
	.count = ARRAY_SIZE(t_rows),
};

struct test_table test_u = {
	.table = {
		.foreach_row = test_foreach_row,
		.row_index = test_row_index,
		.row_lookup = test_row_lookup,
	},
	.rows = u_rows,
	.count = ARRAY_SIZE(u_rows),
};

static int32_t c_value = 42;
static int32_t d_value = 42;
static int32_t e_value = 42;
static enum test_get_mode get_mode = TEST_GET_OK;

int32_t test_c_value(void)
{
	return c_value;
}

int32_t test_d_value(void)
{
	return d_value;
}

int32_t test_e_value(void)
{
	return e_value;
}

void test_set_get_mode(enum test_get_mode mode)
{
	get_mode = mode;
}

void test_c_reset(void)
{
	c_value = 42;
	d_value = 42;
	e_value = 42;
	get_mode = TEST_GET_OK;
}

/* Getter failure selected by test_set_get_mode(), 0 when the getter works */
static int get_failure(void)
{
	switch (get_mode) {
	case TEST_GET_ENOENT:
		return -ENOENT;
	case TEST_GET_EIO:
		return -EIO;
	default:
		return 0;
	}
}

static int a_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = 1;

	return 0;
}

static int b_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_OCTET_STRING;
	val->octets.data = (const uint8_t *)"b";
	val->octets.len = 1;

	return 0;
}

static int c_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	if (get_failure() != 0) {
		return get_failure();
	}

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = c_value;

	return 0;
}

/* Shared by c and d: INTEGER in 0..100, applied on commit only */
static int range_set(int32_t *dst, const struct snmp_value *val, enum snmp_set_phase phase)
{
	if (val->type != SNMP_TYPE_INTEGER) {
		return -EINVAL;
	}

	if (val->i32 < 0 || val->i32 > 100) {
		return -ERANGE;
	}

	if (phase == SNMP_SET_COMMIT) {
		*dst = val->i32;
	}

	return 0;
}

static int c_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		 enum snmp_set_phase phase)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	return range_set(&c_value, val, phase);
}

static int d_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = d_value;

	return 0;
}

static int d_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		 enum snmp_set_phase phase)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	return range_set(&d_value, val, phase);
}

static int e_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	if (get_failure() != 0) {
		return get_failure();
	}

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = e_value;

	return 0;
}

/* Passes every check, but the commit of one value fails. */
static int e_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		 enum snmp_set_phase phase)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	if (val->type != SNMP_TYPE_INTEGER) {
		return -EINVAL;
	}

	if (phase == SNMP_SET_CHECK) {
		return 0;
	}

	if (val->i32 == TEST_E_COMMIT_FAILS) {
		return -EIO;
	}

	e_value = val->i32;

	return 0;
}

static int t_col1_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const struct test_row *r = row;

	ARG_UNUSED(obj);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = r->value;

	return 0;
}

/* Column 1 of t is writable so that SET on rows can be tested. */
static int t_col1_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		      enum snmp_set_phase phase)
{
	struct test_row *r = row;

	ARG_UNUSED(obj);

	if (val->type != SNMP_TYPE_INTEGER) {
		return -EINVAL;
	}

	if (phase == SNMP_SET_COMMIT) {
		r->value = val->i32;
	}

	return 0;
}

static int t_col2_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const struct test_row *r = row;

	ARG_UNUSED(obj);

	val->type = SNMP_TYPE_GAUGE32;
	val->u32 = r->idx[0];

	return 0;
}

static int u_col1_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const struct test_row *r = row;

	ARG_UNUSED(obj);

	val->type = SNMP_TYPE_COUNTER64;
	val->u64 = (uint64_t)r->value;

	return 0;
}

SNMP_CAPABILITY_DEFINE(test_capability, TEST_CAPABILITY_DESCR, TEST_CAPABILITY_OID);

/* Defined in an order that differs from their OID order. */
SNMP_SCALAR_DEFINE(test_b, b_get, NULL, NULL, TEST_OID_B);
SNMP_COLUMN_DEFINE(test_t_col2, test_t.table, t_col2_get, NULL, NULL, TEST_OID_T_COL2);
SNMP_SCALAR_DEFINE(test_c, c_get, c_set, NULL, TEST_OID_C);
SNMP_SCALAR_DEFINE(test_e, e_get, e_set, NULL, TEST_OID_E);
SNMP_SCALAR_DEFINE(test_d, d_get, d_set, NULL, TEST_OID_D);
SNMP_COLUMN_DEFINE(test_u_col1, test_u.table, u_col1_get, NULL, NULL, TEST_OID_U_COL1);
SNMP_SCALAR_DEFINE(test_a, a_get, NULL, NULL, TEST_OID_A);
SNMP_COLUMN_DEFINE(test_t_col1, test_t.table, t_col1_get, t_col1_set, NULL, TEST_OID_T_COL1);
