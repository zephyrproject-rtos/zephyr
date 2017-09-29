/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <arch/cpu.h>

#include <tc_util.h>

#define BIT_INDEX(bit)  (bit >> 3)
#define BIT_VAL(bit)    (1 << (bit & 0x7))
#define BITFIELD_SIZE   512

void main(void)
{
	u32_t b1 = 0;
	unsigned char b2[BITFIELD_SIZE >> 3] = { 0 };
	int failed = 0;
	int test_rv;
	unsigned int bit;
	int ret;

	TC_PRINT("twiddling bits....\n");

	for (bit = 0; bit < 32; ++bit) {
		sys_set_bit((mem_addr_t)&b1, bit);
		if (b1 != (1 << bit)) {
			b1 = (1 << bit);
			TC_PRINT("sys_set_bit failed on bit %d\n",
				 bit);
			failed++;
		}

		if (!sys_test_bit((mem_addr_t)&b1, bit)) {
			TC_PRINT("sys_test_bit did not detect bit %d\n",
				 bit);
			failed++;
		}

		sys_clear_bit((mem_addr_t)&b1, bit);
		if (b1 != 0) {
			b1 = 0;
			TC_PRINT("sys_clear_bit failed for bit %d\n",
				 bit);
			failed++;
		}

		if (sys_test_bit((mem_addr_t)&b1, bit)) {
			TC_PRINT("sys_test_bit erroneously detected bit %d\n",
				 bit);
			failed++;
		}

		ret = sys_test_and_set_bit((mem_addr_t)&b1, bit);
		if (ret) {
			TC_PRINT("sys_test_and_set_bit erroneously detected bit %d\n",
				 bit);
			failed++;
		}
		if (b1 != (1 << bit)) {
			b1 = (1 << bit);
			TC_PRINT("sys_test_and_set_bit did not set bit %d\n",
				 bit);
			failed++;
		}
		ret = sys_test_and_set_bit((mem_addr_t)&b1, bit);
		if (!ret) {
			TC_PRINT("sys_test_and_set_bit did not detect bit %d\n",
				 bit);
			failed++;
		}
		if (b1 != (1 << bit)) {
			b1 = (1 << bit);
			TC_PRINT("sys_test_and_set_bit cleared bit %d\n",
				 bit);
			failed++;
		}

		ret = sys_test_and_clear_bit((mem_addr_t)&b1, bit);
		if (!ret) {
			TC_PRINT("sys_test_and_clear_bit did not detect bit %d\n",
				 bit);
			failed++;
		}
		if (b1 != 0) {
			b1 = 0;
			TC_PRINT("sys_test_and_clear_bit did not clear bit %d\n",
				 bit);
			failed++;
		}
		ret = sys_test_and_clear_bit((mem_addr_t)&b1, bit);
		if (ret) {
			TC_PRINT("sys_test_and_clear_bit erroneously detected bit %d\n",
				 bit);
			failed++;
		}
		if (b1 != 0) {
			b1 = 0;
			TC_PRINT("sys_test_and_clear_bit set bit %d\n",
				 bit);
			failed++;
		}
	}

	for (bit = 0; bit < BITFIELD_SIZE; ++bit) {
		sys_bitfield_set_bit((mem_addr_t)b2, bit);
		if (b2[BIT_INDEX(bit)] != BIT_VAL(bit)) {
			TC_PRINT("got %d expected %d\n", b2[BIT_INDEX(bit)],
				 BIT_VAL(bit));
			TC_PRINT("sys_bitfield_set_bit failed for bit %d\n",
				 bit);
			b2[BIT_INDEX(bit)] = BIT_VAL(bit);
			failed++;
		}

		if (!sys_bitfield_test_bit((mem_addr_t)b2, bit)) {
			TC_PRINT("sys_bitfield_test_bit did not detect bit %d\n",
				 bit);
			failed++;
		}

		sys_bitfield_clear_bit((mem_addr_t)b2, bit);
		if (b2[BIT_INDEX(bit)] != 0) {
			b2[BIT_INDEX(bit)] = 0;
			TC_PRINT("sys_bitfield_clear_bit failed for bit %d\n",
				 bit);
			failed++;
		}

		if (sys_bitfield_test_bit((mem_addr_t)b2, bit)) {
			TC_PRINT("sys_bitfield_test_bit erroneously detected bit %d\n",
				 bit);
			failed++;
		}

		ret = sys_bitfield_test_and_set_bit((mem_addr_t)b2, bit);
		if (ret) {
			TC_PRINT("sys_bitfield_test_and_set_bit erroneously detected bit %d\n",
				 bit);
			failed++;
		}
		if (b2[BIT_INDEX(bit)] != BIT_VAL(bit)) {
			b2[BIT_INDEX(bit)] = BIT_VAL(bit);
			TC_PRINT("sys_bitfield_test_and_set_bit did not set bit %d\n",
				 bit);
			failed++;
		}
		ret = sys_bitfield_test_and_set_bit((mem_addr_t)b2, bit);
		if (!ret) {
			TC_PRINT("sys_bitfield_test_and_set_bit did not detect bit %d\n",
				 bit);
			failed++;
		}
		if (b2[BIT_INDEX(bit)] != BIT_VAL(bit)) {
			b2[BIT_INDEX(bit)] = BIT_VAL(bit);
			TC_PRINT("sys_bitfield_test_and_set_bit cleared bit %d\n",
				 bit);
			failed++;
		}

		ret = sys_bitfield_test_and_clear_bit((mem_addr_t)b2, bit);
		if (!ret) {
			TC_PRINT("sys_bitfield_test_and_clear_bit did not detect bit %d\n",
				 bit);
			failed++;
		}
		if (b2[BIT_INDEX(bit)] != 0) {
			b2[BIT_INDEX(bit)] = 0;
			TC_PRINT("sys_bitfield_test_and_clear_bit did not clear bit %d\n",
				 bit);
			failed++;
		}
		ret = sys_bitfield_test_and_clear_bit((mem_addr_t)b2, bit);
		if (ret) {
			TC_PRINT("sys_bitfield_test_and_clear_bit erroneously detected bit %d\n",
				 bit);
			failed++;
		}
		if (b2[BIT_INDEX(bit)] != 0) {
			b2[BIT_INDEX(bit)] = 0;
			TC_PRINT("sys_bitfield_test_and_clear_bit set bit %d\n",
				 bit);
			failed++;
		}
	}

	if (failed) {
		TC_PRINT("%d tests failed\n", failed);
		test_rv = TC_FAIL;
	} else {
		test_rv = TC_PASS;
	}

	TC_END_RESULT(test_rv);
	TC_END_REPORT(test_rv);
}
