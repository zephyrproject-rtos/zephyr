/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/snmp.h>
#include <zephyr/ztest.h>

#include "helpers.h"

#define SYSTEM(n) 1, 3, 6, 1, 2, 1, 1, (n)

#define SYS_DESCR          SYSTEM(1)
#define SYS_OBJECT_ID      SYSTEM(2)
#define SYS_UPTIME         SYSTEM(3)
#define SYS_CONTACT        SYSTEM(4)
#define SYS_NAME           SYSTEM(5)
#define SYS_LOCATION       SYSTEM(6)
#define SYS_SERVICES       SYSTEM(7)
#define SYS_OR_LAST_CHANGE SYSTEM(8)
#define SYS_OR_ENTRY       SYSTEM(9), 1

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_READ, "public"));
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_WRITE, "private"));
	zassert_ok(snmp_mib2_system_set_name(""));
	zassert_ok(snmp_mib2_system_set_contact(""));
	zassert_ok(snmp_mib2_system_set_location(""));
}

ZTEST(snmp_mib2_system, test_read_only_scalars)
{
	const struct req_vb vbs[] = {
		VB_NULL(SYS_DESCR, 0),
		VB_NULL(SYS_OBJECT_ID, 0),
		VB_NULL(SYS_UPTIME, 0),
		VB_NULL(SYS_SERVICES, 0),
	};
	struct response rsp;
	uint32_t before_ticks;

	before_ticks = (uint32_t)(k_uptime_get() / 10);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
			      &rsp) > 0);
	expect_ok(&rsp, 4);

	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_true(rsp.vb[0].str_len >= 6);
	zassert_mem_equal(rsp.vb[0].str, "Zephyr", 6, "sysDescr does not start with Zephyr");

	zassert_equal(rsp.vb[1].tag, SNMP_BER_TAG_OID);

	zassert_equal(rsp.vb[2].tag, SNMP_BER_TAG_TIMETICKS);
	zassert_true(rsp.vb[2].u32 >= before_ticks, "sysUpTime went backwards");
	zassert_true(rsp.vb[2].u32 < before_ticks + 100, "sysUpTime is off by over a second");

	zassert_equal(rsp.vb[3].tag, SNMP_BER_TAG_INTEGER);
	zassert_equal(rsp.vb[3].i32, CONFIG_SNMP_SYS_SERVICES);
}

ZTEST(snmp_mib2_system, test_object_id)
{
	static const uint32_t vendor[] = {1, 3, 6, 1, 4, 1, 99999, 1};
	static const uint32_t too_long[SNMP_OID_MAX_LEN + 1] = {1, 3};
	const struct req_vb vbs[] = {VB_NULL(SYS_OBJECT_ID, 0)};
	struct response rsp;

	static const uint32_t bad_first[] = {3, 1};
	static const uint32_t bad_second[] = {1, 40};

	zassert_equal(snmp_mib2_system_set_object_id(NULL, 2), -EINVAL);
	zassert_equal(snmp_mib2_system_set_object_id(vendor, 1), -EINVAL);
	zassert_equal(snmp_mib2_system_set_object_id(bad_first, 2), -EINVAL,
		      "Unencodable identifiers must be refused");
	zassert_equal(snmp_mib2_system_set_object_id(bad_second, 2), -EINVAL);
	zassert_equal(snmp_mib2_system_set_object_id(too_long, ARRAY_SIZE(too_long)), -EMSGSIZE);

	/* zeroDotZero until the application sets one */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
			      &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_OID);
	zassert_equal(rsp.vb[0].val_oid_len, 2);
	zassert_equal(rsp.vb[0].val_oid[0], 0);
	zassert_equal(rsp.vb[0].val_oid[1], 0);

	zassert_ok(snmp_mib2_system_set_object_id(vendor, ARRAY_SIZE(vendor)));

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
			      &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].val_oid_len, ARRAY_SIZE(vendor));
	zassert_mem_equal(rsp.vb[0].val_oid, vendor, sizeof(vendor));

	/* Not writable through SNMP */
	{
		const struct req_vb set[] = {VB_INT(1, SYS_OBJECT_ID, 0)};

		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set,
				      ARRAY_SIZE(set), &rsp) > 0);
		expect_error(&rsp, SNMP_ERR_NOT_WRITABLE, 1);
	}
}

ZTEST(snmp_mib2_system, test_writable_strings)
{
	const struct req_vb get[] = {
		VB_NULL(SYS_CONTACT, 0),
		VB_NULL(SYS_NAME, 0),
		VB_NULL(SYS_LOCATION, 0),
	};
	const struct req_vb set[] = {
		VB_STR("admin@example.com", SYS_CONTACT, 0),
		VB_STR("node1", SYS_NAME, 0),
		VB_STR("rack 3", SYS_LOCATION, 0),
	};
	const struct req_vb wrong_type[] = {VB_INT(1, SYS_NAME, 0)};
	char buf[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	char too_long[CONFIG_SNMP_SYS_STRING_MAX_LEN + 2];
	struct response rsp;

	/* Defaults from the application */
	zassert_ok(snmp_mib2_system_set_name("zephyr.example.com"));
	zassert_ok(snmp_mib2_system_set_contact("nobody"));
	zassert_ok(snmp_mib2_system_set_location("lab"));

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get, ARRAY_SIZE(get),
			      &rsp) > 0);
	expect_ok(&rsp, 3);
	zassert_equal(rsp.vb[0].str_len, 6);
	zassert_mem_equal(rsp.vb[0].str, "nobody", 6);
	zassert_equal(rsp.vb[1].str_len, 18);
	zassert_mem_equal(rsp.vb[1].str, "zephyr.example.com", 18);
	zassert_equal(rsp.vb[2].str_len, 3);
	zassert_mem_equal(rsp.vb[2].str, "lab", 3);

	/* A manager changes all three in one request */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set, ARRAY_SIZE(set),
			      &rsp) > 0);
	expect_ok(&rsp, 3);

	get_display_string(OID(SYS_CONTACT, 0), OID_LEN(SYS_CONTACT, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "admin@example.com");
	get_display_string(OID(SYS_NAME, 0), OID_LEN(SYS_NAME, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "node1");
	get_display_string(OID(SYS_LOCATION, 0), OID_LEN(SYS_LOCATION, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "rack 3");

	/* Wrong type and overlong values are rejected without change */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, wrong_type,
			      ARRAY_SIZE(wrong_type), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_TYPE, 1);

	memset(too_long, 'x', sizeof(too_long) - 1);
	too_long[sizeof(too_long) - 1] = '\0';
	{
		const struct req_vb overlong[] = {VB_STR(too_long, SYS_NAME, 0)};

		zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, overlong,
				      ARRAY_SIZE(overlong), &rsp) > 0);
		expect_error(&rsp, SNMP_ERR_WRONG_LENGTH, 1);
	}
	zassert_equal(snmp_mib2_system_set_name(too_long), -EMSGSIZE);
	zassert_equal(snmp_mib2_system_set_name(NULL), -EINVAL);

	get_display_string(OID(SYS_NAME, 0), OID_LEN(SYS_NAME, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "node1", "Rejected SET changed the value");

	/* The longest permitted value fits exactly */
	too_long[sizeof(too_long) - 2] = '\0';
	zassert_ok(snmp_mib2_system_set_location(too_long));
	get_display_string(OID(SYS_LOCATION, 0), OID_LEN(SYS_LOCATION, 0), buf, sizeof(buf));
	zassert_equal(strlen(buf), CONFIG_SNMP_SYS_STRING_MAX_LEN);
}

/* RFC 2579 DisplayString: ASCII only, CR only as part of CR LF. */
ZTEST(snmp_mib2_system, test_display_string)
{
	const struct req_vb non_ascii[] = {VB_STR("caf\xc3\xa9", SYS_NAME, 0)};
	const struct req_vb bare_cr[] = {VB_STR("line\r", SYS_NAME, 0)};
	const struct req_vb crlf[] = {VB_STR("line\r\nnext", SYS_NAME, 0)};
	char buf[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	struct response rsp;

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, non_ascii,
			      ARRAY_SIZE(non_ascii), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_VALUE, 1);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, bare_cr,
			      ARRAY_SIZE(bare_cr), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_VALUE, 1);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, crlf,
			      ARRAY_SIZE(crlf), &rsp) > 0);
	expect_ok(&rsp, 1);
	get_display_string(OID(SYS_NAME, 0), OID_LEN(SYS_NAME, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "line\r\nnext");

	zassert_equal(snmp_mib2_system_set_name("caf\xc3\xa9"), -EINVAL);
	zassert_equal(snmp_mib2_system_set_location("x\r"), -EINVAL);
	zassert_ok(snmp_mib2_system_set_location("x\r\ny"));
}

/* sysUpTime counts from the agent's start, not from boot. */
ZTEST(snmp_mib2_system, test_uptime_epoch)
{
	const struct req_vb vbs[] = {VB_NULL(SYS_UPTIME, 0)};
	struct response rsp;
	uint32_t boot_ticks;

	/* Make sure boot time is clearly ahead of the agent's start */
	k_sleep(K_MSEC(500));
	zassert_ok(snmp_agent_start());
	boot_ticks = (uint32_t)(k_uptime_get() / 10);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
			      &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_TIMETICKS);
	zassert_true(rsp.vb[0].u32 < 30, "sysUpTime %u ticks just after start", rsp.vb[0].u32);
	zassert_true(boot_ticks >= 50, "Boot time %u ticks", boot_ticks);

	zassert_ok(snmp_agent_stop());
}

ZTEST(snmp_mib2_system, test_walk)
{
	const struct req_vb vbs[] = {VB_NULL(1, 3, 6, 1, 2, 1, 1)};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 9, vbs,
			      ARRAY_SIZE(vbs), &rsp) > 0);
	expect_ok(&rsp, 9);

	for (uint32_t i = 0; i < 8; i++) {
		uint32_t expected[] = {SYSTEM(i + 1), 0};

		expect_oid(&rsp.vb[i], expected, ARRAY_SIZE(expected));
	}

	/* The empty sysORTable adds nothing: the walk leaves the group */
	zassert_false(snmp_oid_has_prefix(rsp.vb[8].oid, rsp.vb[8].oid_len,
					  OID(1, 3, 6, 1, 2, 1, 1), OID_LEN(1, 3, 6, 1, 2, 1, 1)),
		      "A ninth object inside the system group");

	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_equal(rsp.vb[1].tag, SNMP_BER_TAG_OID);
	zassert_equal(rsp.vb[2].tag, SNMP_BER_TAG_TIMETICKS);
	zassert_equal(rsp.vb[3].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_equal(rsp.vb[4].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_equal(rsp.vb[5].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_equal(rsp.vb[6].tag, SNMP_BER_TAG_INTEGER);
	zassert_equal(rsp.vb[7].tag, SNMP_BER_TAG_TIMETICKS);
	zassert_equal(rsp.vb[7].u32, 0, "sysORLastChange must be zero");
}

/* RFC 3418: sysORTable exists but has no entries. */
ZTEST(snmp_mib2_system, test_or_table_empty)
{
	const struct req_vb get[] = {
		VB_NULL(SYS_OR_LAST_CHANGE, 0), VB_NULL(SYS_OR_ENTRY, 2, 1),
		VB_NULL(SYS_OR_ENTRY, 3, 1),    VB_NULL(SYS_OR_ENTRY, 4, 1),
		VB_NULL(SYS_OR_ENTRY, 1, 1),
	};
	const struct req_vb next[] = {VB_NULL(SYS_OR_ENTRY)};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get, ARRAY_SIZE(get),
			      &rsp) > 0);
	expect_ok(&rsp, 5);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_TIMETICKS);
	zassert_equal(rsp.vb[0].u32, 0);
	zassert_equal(rsp.vb[1].tag, SNMP_BER_TAG_NO_SUCH_INSTANCE, "sysORID.1");
	zassert_equal(rsp.vb[2].tag, SNMP_BER_TAG_NO_SUCH_INSTANCE, "sysORDescr.1");
	zassert_equal(rsp.vb[3].tag, SNMP_BER_TAG_NO_SUCH_INSTANCE, "sysORUpTime.1");
	zassert_equal(rsp.vb[4].tag, SNMP_BER_TAG_NO_SUCH_OBJECT, "sysORIndex is not accessible");

	/* GETNEXT into the table skips it entirely */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next,
			      ARRAY_SIZE(next), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_false(snmp_oid_has_prefix(rsp.vb[0].oid, rsp.vb[0].oid_len, OID(SYS_OR_ENTRY),
					  OID_LEN(SYS_OR_ENTRY)),
		      "GETNEXT returned an instance of the empty table");
}

ZTEST_SUITE(snmp_mib2_system, NULL, NULL, before, NULL, NULL);
