/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_NET_LIB_SNMP_SRC_TEST_OBJECTS_H_
#define TESTS_NET_LIB_SNMP_SRC_TEST_OBJECTS_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/snmp.h>

/* Everything the tests register lives under an unassigned arc that sorts
 * after every standard object, so that walks end in the test subtree.
 */
#define TEST_BASE 1, 3, 6, 1, 7, 1

/* Scalars: a INTEGER, b OCTET STRING, c writable INTEGER */
#define TEST_OID_A TEST_BASE, 1
#define TEST_OID_B TEST_BASE, 2
#define TEST_OID_C TEST_BASE, 5

/* Table t: single-arc index, columns 1 and 2 */
#define TEST_OID_T_ENTRY TEST_BASE, 3, 1
#define TEST_OID_T_COL1  TEST_OID_T_ENTRY, 1
#define TEST_OID_T_COL2  TEST_OID_T_ENTRY, 2

/* Table u: two-arc index, column 1 */
#define TEST_OID_U_ENTRY TEST_BASE, 4, 1
#define TEST_OID_U_COL1  TEST_OID_U_ENTRY, 1

/* Writable scalars after table u: d with a range check, e whose commit can fail */
#define TEST_OID_D TEST_BASE, 4, 2
#define TEST_OID_E TEST_BASE, 4, 3

/* Value of e that passes the check but fails the commit */
#define TEST_E_COMMIT_FAILS 13

/* An AGENT-CAPABILITIES statement the tests list in sysORTable */
#define TEST_CAPABILITY_OID   1, 3, 6, 1, 4, 1, 99999, 2, 1
#define TEST_CAPABILITY_DESCR "Test agent capabilities"

struct test_row {
	uint32_t idx[2];
	size_t idx_len;
	int32_t value;
};

struct test_table {
	struct snmp_table table;
	struct test_row *rows;
	size_t count;
};

extern struct test_table test_t;
extern struct test_table test_u;

/* Current values of the writable scalars, changed by SET */
int32_t test_c_value(void);
int32_t test_d_value(void);
int32_t test_e_value(void);

/* How the getters of c and e behave */
enum test_get_mode {
	TEST_GET_OK,
	TEST_GET_ENOENT,
	TEST_GET_EIO,
};

void test_set_get_mode(enum test_get_mode mode);

/* Restore the writable values and the getter mode */
void test_c_reset(void);

#endif /* TESTS_NET_LIB_SNMP_SRC_TEST_OBJECTS_H_ */
