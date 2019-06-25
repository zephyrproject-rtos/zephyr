/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/hwinfo.h>
#include <ztest.h>
#include <strings.h>
#include <errno.h>

/*
 * @addtogroup t_hwinfo_get_device_id_api
 * @{
 * @defgroup t_hwinfo_get_device_id test_hwinfo_get_device_id
 * @brief TestPurpose: verify device id get works
 * @details
 * - Test Steps
 *   -# Read the ID
 *   -# Check if to many bytes are written to the buffer
 *   -# Check if UID is plausible
 * - Expected Results
 *   -# Device uid with correct length should be written to the buffer.
 * @}
 */

#define BUFFER_LENGTH 17
#define BUFFER_CANARY 0xFF

/*
 * Function invokes the get_entropy callback in driver
 * to get the random data and fill to passed buffer
 */
static void test_device_id_get(void)
{
	u8_t buffer_1[BUFFER_LENGTH];
	u8_t buffer_2[BUFFER_LENGTH];
	ssize_t length_read_1, length_read_2;
	int i;

	length_read_1 = hwinfo_get_device_id(buffer_1, 1);
	zassert_not_equal(length_read_1, -ENOTSUP, "Not supported by hardware");
	zassert_false((length_read_1 < 0), "Error returned: %d", length_read_1);
	zassert_not_equal(length_read_1, 0, "Zero bytes read");
	zassert_equal(length_read_1, 1, "Length not adhered");

	memset(buffer_1, BUFFER_CANARY, sizeof(buffer_1));

	length_read_1 = hwinfo_get_device_id(buffer_1, BUFFER_LENGTH - 1);
	zassert_equal(buffer_1[length_read_1], BUFFER_CANARY,
		      "Too many bytes are written");

	memcpy(buffer_2, buffer_1, length_read_1);

	for (i = 0; i < BUFFER_LENGTH; i++) {
		buffer_1[i] ^= 0xA5;
	}

	length_read_2 = hwinfo_get_device_id(buffer_1, BUFFER_LENGTH - 1);
	zassert_equal(length_read_1, length_read_2, "Length varied");

	zassert_equal(buffer_1[length_read_1], (BUFFER_CANARY ^ 0xA5),
		      "Too many bytes are written");

	for (i = 0; i < length_read_1; i++) {
		zassert_equal(buffer_1[i], buffer_2[i],
			      "Two consecutively readings don't match");
	}
}

void test_main(void)
{
	ztest_test_suite(hwinfo_device_id_api,
			 ztest_unit_test(test_device_id_get));
	ztest_run_test_suite(hwinfo_device_id_api);
}
