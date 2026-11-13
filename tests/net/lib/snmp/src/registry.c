/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "snmp_internal.h"
#include "test_objects.h"

#define OID(...)     ((const uint32_t[]){__VA_ARGS__})
#define OID_LEN(...) ARRAY_SIZE(OID(__VA_ARGS__))

static void expect_next(const uint32_t *target, size_t target_len, const uint32_t *expected,
			size_t expected_len, void *expected_row)
{
	const struct snmp_object *obj;
	uint32_t inst[SNMP_OID_MAX_LEN];
	size_t inst_len = 0;
	void *row = NULL;

	obj = snmp_registry_next(target, target_len, inst, &inst_len, &row);
	zassert_not_null(obj, "No next instance");
	zassert_equal(inst_len, expected_len, "Next has %zu arcs, expected %zu", inst_len,
		      expected_len);
	zassert_mem_equal(inst, expected, expected_len * sizeof(uint32_t), "Wrong next instance");
	zassert_equal_ptr(row, expected_row, "Wrong row handle");
}

ZTEST(snmp_registry, test_oid_cmp)
{
	zassert_equal(snmp_oid_cmp(OID(1, 3), 2, OID(1, 3), 2), 0);
	zassert_true(snmp_oid_cmp(OID(1, 3), 2, OID(1, 3, 0), 3) < 0, "Prefix sorts first");
	zassert_true(snmp_oid_cmp(OID(1, 3, 0), 3, OID(1, 3), 2) > 0);
	zassert_true(snmp_oid_cmp(OID(1, 2, 9), 3, OID(1, 3), 2) < 0);
	zassert_true(snmp_oid_cmp(OID(1, 3, 10), 3, OID(1, 3, 9), 3) > 0, "Numeric, not string");
	zassert_equal(snmp_oid_cmp(NULL, 0, NULL, 0), 0);
	zassert_true(snmp_oid_cmp(NULL, 0, OID(0), 1) < 0);

	zassert_true(snmp_oid_has_prefix(OID(1, 3, 6), 3, OID(1, 3), 2));
	zassert_true(snmp_oid_has_prefix(OID(1, 3), 2, OID(1, 3), 2));
	zassert_false(snmp_oid_has_prefix(OID(1, 3), 2, OID(1, 3, 6), 3));
	zassert_false(snmp_oid_has_prefix(OID(1, 4, 6), 3, OID(1, 3), 2));
	zassert_true(snmp_oid_has_prefix(OID(1, 3), 2, NULL, 0));
}

ZTEST(snmp_registry, test_lookup)
{
	const struct snmp_object *obj = NULL;
	void *row = (void *)1;

	zassert_equal(snmp_registry_lookup(OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0), &obj, &row),
		      SNMP_LOOKUP_FOUND);
	zassert_not_null(obj);
	zassert_is_null(row, "Scalars have no row");
	zassert_equal(obj->oid_len, OID_LEN(TEST_OID_A));

	zassert_equal(snmp_registry_lookup(OID(TEST_OID_A), OID_LEN(TEST_OID_A), &obj, &row),
		      SNMP_LOOKUP_NO_SUCH_INSTANCE, "Object without instance suffix");
	zassert_not_null(obj, "The matching object is reported for a missing instance");
	zassert_equal(obj->oid_len, OID_LEN(TEST_OID_A));
	zassert_equal(snmp_registry_lookup(OID(TEST_OID_A, 1), OID_LEN(TEST_OID_A, 1), &obj, &row),
		      SNMP_LOOKUP_NO_SUCH_INSTANCE);
	zassert_equal(
		snmp_registry_lookup(OID(TEST_OID_A, 0, 0), OID_LEN(TEST_OID_A, 0, 0), &obj, &row),
		SNMP_LOOKUP_NO_SUCH_INSTANCE);

	zassert_equal(
		snmp_registry_lookup(OID(TEST_BASE, 9, 0), OID_LEN(TEST_BASE, 9, 0), &obj, &row),
		SNMP_LOOKUP_NO_SUCH_OBJECT);
	zassert_is_null(obj, "No object is reported when none matches");
	zassert_equal(snmp_registry_lookup(OID(TEST_BASE), OID_LEN(TEST_BASE), &obj, &row),
		      SNMP_LOOKUP_NO_SUCH_OBJECT, "Prefix of objects is not an object");

	zassert_equal(snmp_registry_lookup(OID(TEST_OID_T_COL1, 5), OID_LEN(TEST_OID_T_COL1, 5),
					   &obj, &row),
		      SNMP_LOOKUP_FOUND);
	zassert_equal_ptr(row, &test_t.rows[0]);
	zassert_equal_ptr(obj->table, &test_t.table);

	zassert_equal(snmp_registry_lookup(OID(TEST_OID_T_COL1, 6), OID_LEN(TEST_OID_T_COL1, 6),
					   &obj, &row),
		      SNMP_LOOKUP_NO_SUCH_INSTANCE);
	zassert_equal_ptr(obj->table, &test_t.table, "Column reported for a missing row");
	zassert_equal(
		snmp_registry_lookup(OID(TEST_OID_T_COL1), OID_LEN(TEST_OID_T_COL1), &obj, &row),
		SNMP_LOOKUP_NO_SUCH_INSTANCE, "Column without index");
	zassert_equal(snmp_registry_lookup(OID(TEST_OID_T_COL1, 5, 0),
					   OID_LEN(TEST_OID_T_COL1, 5, 0), &obj, &row),
		      SNMP_LOOKUP_NO_SUCH_INSTANCE, "Index with extra arc");

	zassert_equal(snmp_registry_lookup(OID(TEST_OID_U_COL1, 1, 9),
					   OID_LEN(TEST_OID_U_COL1, 1, 9), &obj, &row),
		      SNMP_LOOKUP_FOUND);
	zassert_equal_ptr(row, &test_u.rows[2]);
	zassert_equal(snmp_registry_lookup(OID(TEST_OID_U_COL1, 1), OID_LEN(TEST_OID_U_COL1, 1),
					   &obj, &row),
		      SNMP_LOOKUP_NO_SUCH_INSTANCE, "Partial two-arc index");
}

ZTEST(snmp_registry, test_next_walk)
{
	/* Expected walk over the test subtree in lexicographic order. */
	struct {
		const uint32_t *oid;
		size_t len;
		void *row;
	} expected[] = {
		{OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0), NULL},
		{OID(TEST_OID_B, 0), OID_LEN(TEST_OID_B, 0), NULL},
		{OID(TEST_OID_T_COL1, 2), OID_LEN(TEST_OID_T_COL1, 2), &test_t.rows[1]},
		{OID(TEST_OID_T_COL1, 5), OID_LEN(TEST_OID_T_COL1, 5), &test_t.rows[0]},
		{OID(TEST_OID_T_COL1, 10), OID_LEN(TEST_OID_T_COL1, 10), &test_t.rows[2]},
		{OID(TEST_OID_T_COL2, 2), OID_LEN(TEST_OID_T_COL2, 2), &test_t.rows[1]},
		{OID(TEST_OID_T_COL2, 5), OID_LEN(TEST_OID_T_COL2, 5), &test_t.rows[0]},
		{OID(TEST_OID_T_COL2, 10), OID_LEN(TEST_OID_T_COL2, 10), &test_t.rows[2]},
		{OID(TEST_OID_U_COL1, 1, 9), OID_LEN(TEST_OID_U_COL1, 1, 9), &test_u.rows[2]},
		{OID(TEST_OID_U_COL1, 1, 10), OID_LEN(TEST_OID_U_COL1, 1, 10), &test_u.rows[0]},
		{OID(TEST_OID_U_COL1, 2, 0), OID_LEN(TEST_OID_U_COL1, 2, 0), &test_u.rows[1]},
		{OID(TEST_OID_D, 0), OID_LEN(TEST_OID_D, 0), NULL},
		{OID(TEST_OID_E, 0), OID_LEN(TEST_OID_E, 0), NULL},
		{OID(TEST_OID_C, 0), OID_LEN(TEST_OID_C, 0), NULL},
	};
	const struct snmp_object *obj;
	uint32_t inst[SNMP_OID_MAX_LEN];
	size_t inst_len = OID_LEN(TEST_BASE);
	void *row;

	memcpy(inst, OID(TEST_BASE), inst_len * sizeof(uint32_t));

	for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
		uint32_t target[SNMP_OID_MAX_LEN];
		size_t target_len = inst_len;

		memcpy(target, inst, inst_len * sizeof(uint32_t));
		expect_next(target, target_len, expected[i].oid, expected[i].len, expected[i].row);

		obj = snmp_registry_next(target, target_len, inst, &inst_len, &row);
		zassert_not_null(obj);
	}

	uint32_t last[SNMP_OID_MAX_LEN];
	size_t last_len = inst_len;

	memcpy(last, inst, inst_len * sizeof(uint32_t));
	obj = snmp_registry_next(last, last_len, inst, &inst_len, &row);
	zassert_is_null(obj, "Walk must end after the last test object");
}

ZTEST(snmp_registry, test_next_from_arbitrary_points)
{
	const struct snmp_object *obj;
	uint32_t inst[SNMP_OID_MAX_LEN];
	size_t inst_len;
	void *row;

	/* Between two rows of the same column */
	expect_next(OID(TEST_OID_T_COL1, 6), OID_LEN(TEST_OID_T_COL1, 6), OID(TEST_OID_T_COL1, 10),
		    OID_LEN(TEST_OID_T_COL1, 10), &test_t.rows[2]);

	/* Past the last row of column 1 moves on to column 2 */
	expect_next(OID(TEST_OID_T_COL1, 11), OID_LEN(TEST_OID_T_COL1, 11), OID(TEST_OID_T_COL2, 2),
		    OID_LEN(TEST_OID_T_COL2, 2), &test_t.rows[1]);

	/* A prefix of the table gives its first instance */
	expect_next(OID(TEST_BASE, 3), OID_LEN(TEST_BASE, 3), OID(TEST_OID_T_COL1, 2),
		    OID_LEN(TEST_OID_T_COL1, 2), &test_t.rows[1]);

	/* Two-arc index ordering is lexicographic, not by first arc only */
	expect_next(OID(TEST_OID_U_COL1, 1, 9, 5), OID_LEN(TEST_OID_U_COL1, 1, 9, 5),
		    OID(TEST_OID_U_COL1, 1, 10), OID_LEN(TEST_OID_U_COL1, 1, 10), &test_u.rows[0]);
	expect_next(OID(TEST_OID_U_COL1, 1), OID_LEN(TEST_OID_U_COL1, 1),
		    OID(TEST_OID_U_COL1, 1, 9), OID_LEN(TEST_OID_U_COL1, 1, 9), &test_u.rows[2]);

	/* The scalar instance itself is not greater than itself */
	expect_next(OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0), OID(TEST_OID_B, 0),
		    OID_LEN(TEST_OID_B, 0), NULL);
	expect_next(OID(TEST_OID_A, 0, 0), OID_LEN(TEST_OID_A, 0, 0), OID(TEST_OID_B, 0),
		    OID_LEN(TEST_OID_B, 0), NULL);

	/* Past the last object: end of MIB view */
	obj = snmp_registry_next(OID(TEST_OID_C, 0), OID_LEN(TEST_OID_C, 0), inst, &inst_len, &row);
	zassert_is_null(obj);
	obj = snmp_registry_next(OID(TEST_OID_C, 1), OID_LEN(TEST_OID_C, 1), inst, &inst_len, &row);
	zassert_is_null(obj);
	obj = snmp_registry_next(OID(TEST_BASE, 6), OID_LEN(TEST_BASE, 6), inst, &inst_len, &row);
	zassert_is_null(obj);
}

ZTEST_SUITE(snmp_registry, NULL, NULL, NULL, NULL, NULL);
