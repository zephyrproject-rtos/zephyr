/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/ztest.h>

/*
 * @addtogroup t_entropy_api
 * @{
 * @defgroup t_entropy_get_entropy test_entropy_get_entropy
 * @brief TestPurpose: verify Get entropy works
 * @details
 * - Test Steps
 *   -# Read random numbers from Entropy driver.
 *   -# Verify whether buffer overflow occurred or not.
 *   -# Verify whether buffer completely filled or not.
 * - Expected Results
 *   -# Random number should be generated.
 * @}
 */

#define BUFFER_LENGTH           10
#define RECHECK_RANDOM_ENTROPY  0x10

#ifdef CONFIG_RANDOM_BUFFER_NOCACHED
__attribute__((__section__(".nocache")))
static uint8_t entropy_buffer[BUFFER_LENGTH] = {0};
#else
static uint8_t entropy_buffer[BUFFER_LENGTH] = {0};
#endif

static int random_entropy(const struct device *dev, char *buffer, char num, bool test_isr)
{
	int ret, i;
	int count = 0;

	(void)memset(buffer, num, BUFFER_LENGTH);

	/* The BUFFER_LENGTH-1 is used so the driver will not
	 * write the last byte of the buffer. If that last
	 * byte is not 0 on return it means the driver wrote
	 * outside the passed buffer, and that should never
	 * happen.
	 */
	if (test_isr) {
		/* 1 sec should be enough to get the samples */
		k_timepoint_t tp = sys_timepoint_calc(Z_TIMEOUT_MS(1000));
		int remaining = BUFFER_LENGTH - 1;

		/* Get the random bytes possibly in several calls */
		do {
			uint32_t offset = BUFFER_LENGTH - 1 - remaining;

			ret = entropy_get_entropy_isr(dev, buffer + offset, remaining, 0);
			if (ret < 0) {
				if (ret ==  -ENOSYS) {
					TC_PRINT("Skip ISR test: .get_entropy_isr not supported\n");
					return TC_PASS;
				}

				TC_PRINT("Error: entropy_get_entropy_isr failed: %d\n", ret);
				return TC_FAIL;
			}
			remaining -= ret;

			if (ret == 0 && sys_timepoint_expired(tp)) {
				TC_PRINT("Error: Timeout on entropy_get_entropy_isr\n");
				return TC_FAIL;
			}
		} while (remaining > 0);
	} else {
		ret = entropy_get_entropy(dev, buffer, BUFFER_LENGTH - 1);
		if (ret) {
			TC_PRINT("Error: entropy_get_entropy failed: %d\n", ret);
			return TC_FAIL;
		}
	}

	if (buffer[BUFFER_LENGTH - 1] != num) {
		TC_PRINT("Error: buffer overflow\n");
		return TC_FAIL;
	}

	for (i = 0; i < BUFFER_LENGTH - 1; i++) {
		TC_PRINT("  0x%02x\n", buffer[i]);
		if (buffer[i] == num) {
			count++;
		}
	}

	if (count >= 2) {
		return RECHECK_RANDOM_ENTROPY;
	} else {
		return TC_PASS;
	}
}

/*
 * Function invokes the get_entropy callback in driver
 * to get the random data and fill to passed buffer
 */
static int get_entropy(bool test_isr)
{
	const struct device *const dev = entropy_get_default_device();
	int ret;

	if (!device_is_ready(dev)) {
		TC_PRINT("error: random device not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("random device is %p, name is %s\n",
		 dev, dev->name);

	ret = random_entropy(dev, entropy_buffer, 0, test_isr);

	/* Check whether 20% or more of buffer still filled with default
	 * value(0), if yes then recheck again by filling nonzero value(0xa5)
	 * to buffer. Recheck random_entropy and verify whether 20% or more
	 * of buffer filled with value(0xa5) or not.
	 */
	if (ret == RECHECK_RANDOM_ENTROPY) {
		ret = random_entropy(dev, entropy_buffer, 0xa5, test_isr);
		if (ret == RECHECK_RANDOM_ENTROPY) {
			return TC_FAIL;
		} else {
			return ret;
		}
	}

	return ret;
}

ZTEST(entropy_api, test_entropy_get_entropy)
{
	zassert_true(get_entropy(false) == TC_PASS);
}

ZTEST(entropy_api, test_entropy_get_entropy_isr)
{
	zassert_true(get_entropy(true) == TC_PASS);
}

void *entropy_api_setup(void)
{
#ifdef CONFIG_BT
	bt_enable(NULL);
#endif /* CONFIG_BT */

	return NULL;
}

ZTEST_SUITE(entropy_api, NULL, entropy_api_setup, NULL, NULL, NULL);
