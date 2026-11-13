/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/decision.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "decision_internal.h"

DEFINE_FFF_GLOBALS;

static const uint8_t key[BT_LE_DECISION_KEY_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};

static const uint8_t no_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN];

static struct bt_hci_le_decision_test hci_tests[BT_LE_DECISION_TESTS_MAX];

static void encode_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Any parameter octet that the encoder does not write would then differ from zero */
	(void)memset(hci_tests, 0xaa, sizeof(hci_tests));
}

ZTEST_SUITE(bt_le_decision_instructions_encode, NULL, NULL, encode_before, NULL, NULL);

static int encode(const struct bt_le_decision_group *groups, size_t num_groups)
{
	return bt_le_decision_instructions_encode(groups, num_groups, hci_tests,
						  ARRAY_SIZE(hci_tests));
}

static void expect_test(size_t i, uint8_t flags, uint8_t field, const uint8_t *params)
{
	zassert_equal(hci_tests[i].flags, flags, "Test %zu: Test_Flags 0x%02x, expected 0x%02x", i,
		      hci_tests[i].flags, flags);
	zassert_equal(hci_tests[i].field, field, "Test %zu: Test_Field %u, expected %u", i,
		      hci_tests[i].field, field);
	zassert_mem_equal(hci_tests[i].params, params, BT_HCI_LE_DECISION_TEST_PARAMS_LEN,
			  "Test %zu: unexpected Test_Parameters", i);
}

/*
 *  Test Example 1 of the LE Set Decision Instructions command (Core Vol 4, Part E, 7.8.145)
 *
 *  Expected behaviour:
 *   - The tests match Table 7.8: one group with a tag test and an advertising mode test
 *   - With the tests in separate groups, the second test also starts a group
 */
ZTEST(bt_le_decision_instructions_encode, test_spec_example_1)
{
	const struct bt_le_decision_test tests[] = {
		BT_LE_DECISION_TEST_TAG(key),
		BT_LE_DECISION_TEST_ADV_MODE(BT_LE_DECISION_ADV_MODE_NON_CONN_NON_SCAN |
					     BT_LE_DECISION_ADV_MODE_SCAN),
	};
	const struct bt_le_decision_group one_group[] = {
		BT_LE_DECISION_GROUP(tests),
	};
	const struct bt_le_decision_group two_groups[] = {
		{.tests = &tests[0], .num_tests = 1},
		{.tests = &tests[1], .num_tests = 1},
	};
	const uint8_t adv_mode_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {0x05};

	zassert_equal(encode(one_group, ARRAY_SIZE(one_group)), 2);
	expect_test(0, 0b0011, 0, key);
	expect_test(1, 0b0010, 6, adv_mode_params);

	zassert_equal(encode(two_groups, ARRAY_SIZE(two_groups)), 2);
	expect_test(0, 0b0011, 0, key);
	expect_test(1, 0b0011, 6, adv_mode_params);
}

/*
 *  Test Example 2 of the LE Set Decision Instructions command (Core Vol 4, Part E, 7.8.145)
 *
 *  Expected behaviour:
 *   - The tests match Table 7.9: three groups, each with one arbitrary data test
 */
ZTEST(bt_le_decision_instructions_encode, test_spec_example_2)
{
	const struct bt_le_decision_test exact[] = {
		BT_LE_DECISION_TEST_DATA_EQUALS('A', 'B', 'C', 'D'),
	};
	const struct bt_le_decision_test at_least[] = {
		BT_LE_DECISION_TEST_DATA_STARTS_WITH('E', 'F', 'G', 'H'),
	};
	const struct bt_le_decision_test at_most[] = {
		{
			.field = BT_LE_DECISION_FIELD_DATA_AT_MOST(4),
			.data = {.mask = {0xff}, .target = {'J'}},
		},
	};
	const struct bt_le_decision_group groups[] = {
		BT_LE_DECISION_GROUP(exact),
		BT_LE_DECISION_GROUP(at_least),
		BT_LE_DECISION_GROUP(at_most),
	};
	const uint8_t params[][BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {
		{0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x41, 0x42, 0x43, 0x44},
		{0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x45, 0x46, 0x47, 0x48},
		{0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a},
	};

	zassert_equal(encode(groups, ARRAY_SIZE(groups)), 3);
	expect_test(0, 0b0011, 20, params[0]);
	expect_test(1, 0b0011, 36, params[1]);
	expect_test(2, 0b0011, 52, params[2]);
}

/*
 *  Test Example 3 of the LE Set Decision Instructions command (Core Vol 4, Part E, 7.8.145)
 *
 *  Expected behaviour:
 *   - The tests match Table 7.10: two groups that combine the pass conditions
 *   - The parameters of a test that can only pass on an absent field are zero
 */
ZTEST(bt_le_decision_instructions_encode, test_spec_example_3)
{
	const struct bt_le_decision_test no_tag[] = {
		BT_LE_DECISION_TEST_NO_TAG(),
		{
			.field = BT_LE_DECISION_FIELD_PATH_LOSS,
			.flags = BT_LE_DECISION_PASS_IF_MATCH | BT_LE_DECISION_PASS_IF_ABSENT,
			.path_loss = {.min = 80, .max = 100},
		},
		BT_LE_DECISION_TEST_DATA_STARTS_WITH('C', 'D', 'W', 'F'),
	};
	const struct bt_le_decision_test tag[] = {
		BT_LE_DECISION_TEST_TAG(key),
		{
			.field = BT_LE_DECISION_FIELD_DATA_EXACT(2),
			.data = {.mask = {0xdf, 0xdf}, .target = {'B', 'T'}},
		},
	};
	const struct bt_le_decision_group groups[] = {
		BT_LE_DECISION_GROUP(no_tag),
		BT_LE_DECISION_GROUP(tag),
	};
	const uint8_t path_loss_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {0x50, 0x64};
	const uint8_t cdwf_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {
		0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x43, 0x44, 0x57, 0x46,
	};
	const uint8_t bt_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {
		0xdf, 0xdf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x54,
	};

	zassert_equal(encode(groups, ARRAY_SIZE(groups)), 5);
	expect_test(0, 0b1001, 0, no_params);
	expect_test(1, 0b1010, 8, path_loss_params);
	expect_test(2, 0b0010, 36, cdwf_params);
	expect_test(3, 0b0011, 0, key);
	expect_test(4, 0b0010, 18, bt_params);
}

/*
 *  Test every combination of pass conditions
 *
 *  Expected behaviour:
 *   - The pass conditions map to Test_Flags bits 1 to 3
 *   - A test without pass conditions passes on a match
 */
ZTEST(bt_le_decision_instructions_encode, test_pass_conditions)
{
	static const struct {
		uint8_t flags;
		uint8_t test_flags;
	} cases[] = {
		{0, 0b0011},
		{BT_LE_DECISION_PASS_IF_MATCH, 0b0011},
		{BT_LE_DECISION_PASS_IF_NO_MATCH, 0b0101},
		{BT_LE_DECISION_PASS_IF_ABSENT, 0b1001},
		{BT_LE_DECISION_PASS_IF_MATCH | BT_LE_DECISION_PASS_IF_NO_MATCH, 0b0111},
		{BT_LE_DECISION_PASS_IF_MATCH | BT_LE_DECISION_PASS_IF_ABSENT, 0b1011},
		{BT_LE_DECISION_PASS_IF_NO_MATCH | BT_LE_DECISION_PASS_IF_ABSENT, 0b1101},
		{BT_LE_DECISION_PASS_IF_MATCH | BT_LE_DECISION_PASS_IF_NO_MATCH |
			 BT_LE_DECISION_PASS_IF_ABSENT,
		 0b1111},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct bt_le_decision_test test = BT_LE_DECISION_TEST_RSSI(-10, 10);
		const struct bt_le_decision_group group = {.tests = &test, .num_tests = 1};

		test.flags = cases[i].flags;

		zassert_equal(encode(&group, 1), 1);
		zassert_equal(hci_tests[0].flags, cases[i].test_flags,
			      "Flags 0x%02x: Test_Flags 0x%02x, expected 0x%02x", cases[i].flags,
			      hci_tests[0].flags, cases[i].test_flags);
	}
}

/*
 *  Test the parameter layout of the fields that are not in the specification examples
 *
 *  Expected behaviour:
 *   - RSSI, advertising address and vendor-specific parameters match the HCI layout
 */
ZTEST(bt_le_decision_instructions_encode, test_field_parameters)
{
	const bt_addr_le_t addr_1 = {
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = {0x22, 0x33, 0x44, 0x55, 0x66, 0x77},
	};
	const bt_addr_le_t addr_2 = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = {0x01, 0x00, 0x00, 0x00, 0x00, 0xc0},
	};
	const struct bt_le_decision_test tests[] = {
		BT_LE_DECISION_TEST_RSSI(-70, 20),
		BT_LE_DECISION_TEST_ADVA_FAL(),
		BT_LE_DECISION_TEST_ADVA(&addr_1),
		{
			.field = BT_LE_DECISION_FIELD_ADVA,
			.adva = {.check = BT_LE_DECISION_ADVA_CHECK_ADDR_ANY,
				 .addr = {&addr_1, &addr_2}},
		},
		{
			.field = BT_LE_DECISION_FIELD_VS(3),
			.raw = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
				0x0c, 0x0d, 0x0e, 0x0f, 0x10},
		},
	};
	const struct bt_le_decision_group groups[] = {
		BT_LE_DECISION_GROUP(tests),
	};
	const uint8_t rssi_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {0xba, 0x14};
	const uint8_t addr_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {
		0x01, 0x00, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	};
	const uint8_t addr_any_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {
		0x02, 0x00, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xc0,
	};

	zassert_equal(encode(groups, ARRAY_SIZE(groups)), 5);
	expect_test(0, 0b0011, 7, rssi_params);
	expect_test(1, 0b0010, 9, no_params);
	expect_test(2, 0b0010, 9, addr_params);
	expect_test(3, 0b0010, 9, addr_any_params);
	expect_test(4, 0b0010, 0xf3, tests[4].raw);
}

/*
 *  Test flags that have no BT_LE_DECISION_PASS_IF_* name
 *
 *  Expected behaviour:
 *   - The flags reach Test_Flags bits 4 to 7 unchanged
 */
ZTEST(bt_le_decision_instructions_encode, test_unnamed_flags)
{
	const struct bt_le_decision_test test = {
		.field = BT_LE_DECISION_FIELD_ADV_MODE,
		.flags = BT_LE_DECISION_PASS_IF_MATCH | BIT(3) | BIT(6),
	};
	const struct bt_le_decision_group group = {.tests = &test, .num_tests = 1};

	zassert_equal(encode(&group, 1), 1);
	zassert_equal(hci_tests[0].flags, 0b10010011);
}

/*
 *  Test instructions without groups
 *
 *  Expected behaviour:
 *   - One test that never passes is written, so every decision PDU fails
 *   - Without room for that test, -EINVAL is returned
 */
ZTEST(bt_le_decision_instructions_encode, test_no_groups)
{
	const struct bt_le_decision_test test = BT_LE_DECISION_TEST_ADVA_FAL();
	const struct bt_le_decision_group group = {.tests = &test, .num_tests = 1};

	zassert_equal(encode(NULL, 0), 1);
	expect_test(0, 0b0001, 6, no_params);

	zassert_equal(encode(&group, 0), 1);
	expect_test(0, 0b0001, 6, no_params);

	zassert_equal(bt_le_decision_instructions_encode(NULL, 0, hci_tests, 0), -EINVAL);
}

/*
 *  Test the limit on the number of tests
 *
 *  Expected behaviour:
 *   - BT_LE_DECISION_TESTS_MAX tests are accepted, in one group or spread over several
 *   - One more test, or more tests than the output can hold, gives -EINVAL
 */
ZTEST(bt_le_decision_instructions_encode, test_too_many_tests)
{
	struct bt_le_decision_test tests[BT_LE_DECISION_TESTS_MAX + 1];
	const struct bt_le_decision_group split[] = {
		{.tests = &tests[0], .num_tests = BT_LE_DECISION_TESTS_MAX / 2},
		{.tests = &tests[BT_LE_DECISION_TESTS_MAX / 2],
		 .num_tests = BT_LE_DECISION_TESTS_MAX / 2 + 1},
	};
	struct bt_le_decision_group group = {.tests = tests};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		tests[i] = (struct bt_le_decision_test)BT_LE_DECISION_TEST_RSSI(-10, 10);
	}

	group.num_tests = BT_LE_DECISION_TESTS_MAX;
	zassert_equal(encode(&group, 1), BT_LE_DECISION_TESTS_MAX);

	group.num_tests = BT_LE_DECISION_TESTS_MAX + 1;
	zassert_equal(encode(&group, 1), -EINVAL);

	zassert_equal(encode(split, ARRAY_SIZE(split)), -EINVAL);

	group.num_tests = 2;
	zassert_equal(bt_le_decision_instructions_encode(&group, 1, hci_tests, 1), -EINVAL);
}

/*
 *  Test groups without tests
 *
 *  Expected behaviour:
 *   - A group with no tests, a group without a test array, or groups without an array give
 *     -EINVAL
 */
ZTEST(bt_le_decision_instructions_encode, test_empty_group)
{
	const struct bt_le_decision_test test = BT_LE_DECISION_TEST_ADVA_FAL();
	const struct bt_le_decision_group no_tests[] = {
		{.tests = &test, .num_tests = 1},
		{.tests = &test, .num_tests = 0},
	};
	const struct bt_le_decision_group no_array = {.tests = NULL, .num_tests = 1};

	zassert_equal(encode(no_tests, ARRAY_SIZE(no_tests)), -EINVAL);
	zassert_equal(encode(&no_array, 1), -EINVAL);
	zassert_equal(encode(NULL, 1), -EINVAL);
}

/*
 *  Test missing or invalid parameters on tests that compare the field
 *
 *  Expected behaviour:
 *   - A tag test without a key gives -EINVAL
 *   - An address test with fewer addresses than its check compares gives -EINVAL
 *   - An identity address type, which the HCI parameters cannot hold, gives -EINVAL
 */
ZTEST(bt_le_decision_instructions_encode, test_invalid_parameters)
{
	const bt_addr_le_t public_addr = {.type = BT_ADDR_LE_PUBLIC};
	const bt_addr_le_t public_id_addr = {.type = BT_ADDR_LE_PUBLIC_ID};
	const bt_addr_le_t random_id_addr = {.type = BT_ADDR_LE_RANDOM_ID};
	const struct bt_le_decision_test tests[] = {
		BT_LE_DECISION_TEST_TAG(NULL),
		{
			.field = BT_LE_DECISION_FIELD_ADVA,
			.adva = {.check = BT_LE_DECISION_ADVA_CHECK_ADDR},
		},
		{
			.field = BT_LE_DECISION_FIELD_ADVA,
			.adva = {.check = BT_LE_DECISION_ADVA_CHECK_ADDR_ANY,
				 .addr = {&public_addr}},
		},
		BT_LE_DECISION_TEST_ADVA(&public_id_addr),
		BT_LE_DECISION_TEST_ADVA(&random_id_addr),
	};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		const struct bt_le_decision_group group = {.tests = &tests[i], .num_tests = 1};

		zassert_equal(encode(&group, 1), -EINVAL, "Test %zu was accepted", i);
	}
}

/*
 *  Test missing parameters on tests that can only pass on an absent field
 *
 *  Expected behaviour:
 *   - The tests are accepted, because the Controller does not compare the field
 */
ZTEST(bt_le_decision_instructions_encode, test_absent_only)
{
	const struct bt_le_decision_test tests[] = {
		BT_LE_DECISION_TEST_NO_TAG(),
		{
			.field = BT_LE_DECISION_FIELD_ADVA,
			.flags = BT_LE_DECISION_PASS_IF_ABSENT,
			.adva = {.check = BT_LE_DECISION_ADVA_CHECK_ADDR_ANY},
		},
	};
	const struct bt_le_decision_group groups[] = {
		BT_LE_DECISION_GROUP(tests),
	};
	const uint8_t addr_any_params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN] = {0x02};

	zassert_equal(encode(groups, ARRAY_SIZE(groups)), 2);
	expect_test(0, 0b1001, 0, no_params);
	expect_test(1, 0b1000, 9, addr_any_params);
}
