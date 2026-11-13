/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/snmp.h>
#include <zephyr/ztest.h>

#include "helpers.h"

#define SYS_CONTACT  1, 3, 6, 1, 2, 1, 1, 4
#define SYS_NAME     1, 3, 6, 1, 2, 1, 1, 5
#define SYS_LOCATION 1, 3, 6, 1, 2, 1, 1, 6
#define AUTHEN_TRAPS 1, 3, 6, 1, 2, 1, 11, 30

#define ITEM_COUNT 4

/* A RAM-backed storage with failure injection and a call log. */
static struct {
	uint8_t data[CONFIG_SNMP_SYS_STRING_MAX_LEN + 8];
	size_t len;
	bool present;
} items[ITEM_COUNT];

static int failing_item = -1;
static int store_calls;
static int load_calls;

static int fake_load(enum snmp_storage_item item, void *buf, size_t size)
{
	load_calls++;

	if (!items[item].present) {
		return -ENOENT;
	}

	if (items[item].len > size) {
		return -EMSGSIZE;
	}

	memcpy(buf, items[item].data, items[item].len);

	return (int)items[item].len;
}

static int fake_store(enum snmp_storage_item item, const void *data, size_t len)
{
	store_calls++;

	if ((int)item == failing_item) {
		return -EIO;
	}

	zassert_true(len <= sizeof(items[item].data));
	memcpy(items[item].data, data, len);
	items[item].len = len;
	items[item].present = true;

	return 0;
}

static const struct snmp_storage fake = {.load = fake_load, .store = fake_store};

static void preset(enum snmp_storage_item item, const void *data, size_t len)
{
	memcpy(items[item].data, data, len);
	items[item].len = len;
	items[item].present = true;
}

static bool stored_equals(enum snmp_storage_item item, const char *s)
{
	return items[item].present && items[item].len == strlen(s) &&
	       memcmp(items[item].data, s, items[item].len) == 0;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(items, 0, sizeof(items));
	failing_item = -1;
	store_calls = 0;
	load_calls = 0;

	zassert_ok(snmp_agent_set_storage(NULL));
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_READ, "public"));
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_WRITE, "private"));
	zassert_ok(snmp_mib2_system_set_name(""));
	zassert_ok(snmp_mib2_system_set_contact(""));
	zassert_ok(snmp_mib2_system_set_location(""));
	zassert_ok(snmp_mib2_snmp_set_authen_traps(true));
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)snmp_agent_stop();
	zassert_ok(snmp_agent_set_storage(NULL));
	zassert_ok(snmp_mib2_system_set_name(""));
	zassert_ok(snmp_mib2_system_set_contact(""));
	zassert_ok(snmp_mib2_system_set_location(""));
	zassert_ok(snmp_mib2_snmp_set_authen_traps(true));
}

ZTEST(snmp_storage, test_registration)
{
	const struct snmp_storage no_load = {.store = fake_store};
	const struct snmp_storage no_store = {.load = fake_load};

	zassert_equal(snmp_agent_set_storage(&no_load), -EINVAL);
	zassert_equal(snmp_agent_set_storage(&no_store), -EINVAL);
	zassert_ok(snmp_agent_set_storage(&fake));
	zassert_ok(snmp_agent_set_storage(NULL));
}

/* Stored values win over defaults and over setters called before the start;
 * unusable stored values are ignored.
 */
ZTEST(snmp_storage, test_restore_at_start)
{
	static const uint8_t disabled = 2;
	static const uint8_t non_ascii[] = {'c', 'a', 'f', 0xC3, 0xA9};
	uint8_t too_long[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	const struct req_vb get_traps[] = {VB_NULL(AUTHEN_TRAPS, 0)};
	char buf[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	struct response rsp;

	memset(too_long, 'x', sizeof(too_long));
	preset(SNMP_STORAGE_SYS_NAME, "stored-name", strlen("stored-name"));
	preset(SNMP_STORAGE_SYS_CONTACT, non_ascii, sizeof(non_ascii));
	preset(SNMP_STORAGE_SYS_LOCATION, too_long, sizeof(too_long));
	preset(SNMP_STORAGE_AUTHEN_TRAPS, &disabled, sizeof(disabled));

	zassert_ok(snmp_agent_set_storage(&fake));
	zassert_ok(snmp_mib2_system_set_name("api-name"));
	zassert_true(stored_equals(SNMP_STORAGE_SYS_NAME, "api-name"),
		     "The setter persists as well");
	preset(SNMP_STORAGE_SYS_NAME, "stored-name", strlen("stored-name"));

	zassert_ok(snmp_agent_start());
	zassert_equal(load_calls, ITEM_COUNT, "Every item is loaded once");

	get_display_string(OID(SYS_NAME, 0), OID_LEN(SYS_NAME, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "stored-name");
	get_display_string(OID(SYS_CONTACT, 0), OID_LEN(SYS_CONTACT, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "", "A non-DisplayString value is ignored");
	get_display_string(OID(SYS_LOCATION, 0), OID_LEN(SYS_LOCATION, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "", "An overlong value is ignored");

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_traps,
			      ARRAY_SIZE(get_traps), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].i32, 2, "snmpEnableAuthenTraps restored as disabled");
	zassert_equal(get_int32(OID(AUTHEN_TRAPS, 0), OID_LEN(AUTHEN_TRAPS, 0)), 2);

	zassert_ok(snmp_agent_stop());
}

/* A SET is acknowledged only after the storage accepted the value. */
ZTEST(snmp_storage, test_set_persists)
{
	const struct req_vb set_contact[] = {VB_STR("ops@example.com", SYS_CONTACT, 0)};
	const struct req_vb disable[] = {VB_INT(2, AUTHEN_TRAPS, 0)};
	char buf[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	struct response rsp;

	zassert_ok(snmp_agent_set_storage(&fake));

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_contact,
			      ARRAY_SIZE(set_contact), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(store_calls, 1);
	zassert_true(stored_equals(SNMP_STORAGE_SYS_CONTACT, "ops@example.com"));
	get_display_string(OID(SYS_CONTACT, 0), OID_LEN(SYS_CONTACT, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "ops@example.com");

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, disable,
			      ARRAY_SIZE(disable), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(store_calls, 2);
	zassert_true(items[SNMP_STORAGE_AUTHEN_TRAPS].present);
	zassert_equal(items[SNMP_STORAGE_AUTHEN_TRAPS].len, 1);
	zassert_equal(items[SNMP_STORAGE_AUTHEN_TRAPS].data[0], 2);

	zassert_ok(snmp_mib2_system_set_location("lab"));
	zassert_equal(store_calls, 3);
	zassert_true(stored_equals(SNMP_STORAGE_SYS_LOCATION, "lab"));
}

/* A storage failure leaves the value unchanged and is reported. */
ZTEST(snmp_storage, test_store_failure)
{
	const struct req_vb set_contact[] = {VB_STR("new", SYS_CONTACT, 0)};
	const struct req_vb two[] = {VB_STR("n1", SYS_NAME, 0), VB_STR("new", SYS_CONTACT, 0)};
	const struct req_vb disable[] = {VB_INT(2, AUTHEN_TRAPS, 0)};
	char buf[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	struct response rsp;

	zassert_ok(snmp_agent_set_storage(&fake));
	zassert_ok(snmp_mib2_system_set_contact("old"));
	failing_item = SNMP_STORAGE_SYS_CONTACT;

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_contact,
			      ARRAY_SIZE(set_contact), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_COMMIT_FAILED, 1);
	get_display_string(OID(SYS_CONTACT, 0), OID_LEN(SYS_CONTACT, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "old", "A value that is not durable must not be visible");
	zassert_true(stored_equals(SNMP_STORAGE_SYS_CONTACT, "old"));

	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_contact,
			      ARRAY_SIZE(set_contact), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 1);

	/* The first binding is durable and applied, the second fails */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, two, ARRAY_SIZE(two),
			      &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_UNDO_FAILED, 0);
	get_display_string(OID(SYS_NAME, 0), OID_LEN(SYS_NAME, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "n1");
	zassert_true(stored_equals(SNMP_STORAGE_SYS_NAME, "n1"));
	get_display_string(OID(SYS_CONTACT, 0), OID_LEN(SYS_CONTACT, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "old");

	zassert_equal(snmp_mib2_system_set_contact("api"), -EIO);
	get_display_string(OID(SYS_CONTACT, 0), OID_LEN(SYS_CONTACT, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "old");

	failing_item = SNMP_STORAGE_AUTHEN_TRAPS;
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, disable,
			      ARRAY_SIZE(disable), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_COMMIT_FAILED, 1);
	zassert_equal(get_int32(OID(AUTHEN_TRAPS, 0), OID_LEN(AUTHEN_TRAPS, 0)), 1);
	zassert_equal(snmp_mib2_snmp_set_authen_traps(false), -EIO);
	zassert_equal(get_int32(OID(AUTHEN_TRAPS, 0), OID_LEN(AUTHEN_TRAPS, 0)), 1);
}

/* Without storage, nothing is loaded or stored and everything else works. */
ZTEST(snmp_storage, test_no_storage)
{
	const struct req_vb set_name[] = {VB_STR("volatile", SYS_NAME, 0)};
	char buf[CONFIG_SNMP_SYS_STRING_MAX_LEN + 1];
	struct response rsp;

	zassert_ok(snmp_agent_start());
	zassert_equal(load_calls, 0);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_name,
			      ARRAY_SIZE(set_name), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(store_calls, 0);
	get_display_string(OID(SYS_NAME, 0), OID_LEN(SYS_NAME, 0), buf, sizeof(buf));
	zassert_str_equal(buf, "volatile");

	zassert_ok(snmp_agent_stop());
}

ZTEST_SUITE(snmp_storage, NULL, NULL, before, after, NULL);
