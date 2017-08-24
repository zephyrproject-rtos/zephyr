/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <random.h>
#include <ztest.h>

#define BUFFER_LENGTH 10

/*
 * Function invokes the get_entropy callback in driver
 * to get the random data and fill to passed buffer
 */
static int get_entropy(void)
{
	struct device *dev;
	u8_t buffer[BUFFER_LENGTH] = {0};
	int r;

	TC_PRINT("Random Example! %s\n", CONFIG_ARCH);
	dev = device_get_binding(CONFIG_RANDOM_NAME);
	if (!dev) {
		TC_PRINT("error: no random device\n");
		return TC_FAIL;
	}

	TC_PRINT("random device is %p, name is %s\n",
			dev, dev->config->name);

	/* The BUFFER_LENGTH-1 is used so the driver will not
	 * write the last byte of the buffer. If that last
	 * byte is not 0 on return it means the driver wrote
	 * outside the passed buffer, and that should never
	 * happen.
	 */
	r = random_get_entropy(dev, buffer, BUFFER_LENGTH-1);
	if (r) {
		TC_PRINT("Error: random_get_entropy failed: %d\n", r);
		return TC_FAIL;
	}
	if (buffer[BUFFER_LENGTH-1] != 0) {
		TC_PRINT("Error: random_get_entropy buffer overflow\n");
		return TC_FAIL;
	}

	for (int i = 0; i < BUFFER_LENGTH-1; i++)
		TC_PRINT("  0x%02x\n", buffer[i]);

	TC_PRINT("PROJECT EXECUTION SUCCESSFUL\n");

	return TC_PASS;
}


void test_random_get_entropy(void)
{
	zassert_true(get_entropy() == TC_PASS, NULL);
}

void test_main(void)
{
	ztest_test_suite(random,
			ztest_unit_test(test_random_get_entropy));
	ztest_run_test_suite(random);
}
