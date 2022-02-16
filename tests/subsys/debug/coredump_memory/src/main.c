/*
 * Copyright (c) 2021 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <debug/coredump.h>

int values[3];
int to_unregister[2];
struct coredump_mem_region_node_t dump_region0 = {
	.start = (uintptr_t)&values,
	.end = (uintptr_t)&values + sizeof(values),
};
struct coredump_mem_region_node_t dump_region1 = {
	.start = (uintptr_t)&to_unregister,
	.end = (uintptr_t)&to_unregister + sizeof(to_unregister),
};

void test_coredump_memory(void)
{
	coredump_register_memory_region(&dump_region1);
	to_unregister[0] = 0x01010101;
	to_unregister[1] = 0x23232323;

	bool success;

	success = coredump_unregister_memory_region(&dump_region1);
	zassert_true(success, "unregister failed");
	success = coredump_unregister_memory_region(&dump_region1);
	zassert_false(success, "unregister should have failed");

	coredump_register_memory_region(&dump_region0);
	values[0] = 0xabababab;
	values[1] = 0xcdcdcdcd;
	values[2] = 0xefefefef;

	k_panic();
}

void test_main(void)
{
	ztest_test_suite(coredump_backends,
			 ztest_unit_test(test_coredump_memory));
	ztest_run_test_suite(coredump_backends);

}
