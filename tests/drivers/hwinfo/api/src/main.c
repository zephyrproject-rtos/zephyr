/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/hwinfo.h>
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

ZTEST(hwinfo_device_id_api, test_device_id_get)
{
	uint8_t buffer_1[BUFFER_LENGTH];
	uint8_t buffer_2[BUFFER_LENGTH];
	ssize_t length_read_1, length_read_2;
	int i;

	length_read_1 = hwinfo_get_device_id(buffer_1, 1);
	if (length_read_1 == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_false((length_read_1 < 0),
		      "Unexpected negative return value: %d", length_read_1);
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

/*
 * @addtogroup t_hwinfo_get_reset_cause_api
 * @{
 * @defgroup t_hwinfo_get_reset_cause test_hwinfo_get_reset_cause
 * @brief TestPurpose: verify get reset cause works.
 * @details
 * - Test Steps
 *   -# Set target buffer to a known value
 *   -# Read the reset cause
 *   -# Check if target buffer has been altered
 * - Expected Results
 *   -# Target buffer contents should be changed after the call.
 * @}
 */

ZTEST(hwinfo_device_id_api, test_get_reset_cause)
{
	uint32_t cause;
	ssize_t ret;

	/* Set `cause` to a known value prior to call. */
	cause = 0xDEADBEEF;

	ret = hwinfo_get_reset_cause(&cause);
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	/* Verify that `cause` has been changed. */
	zassert_not_equal(cause, 0xDEADBEEF, "Reset cause not written.");
}

/*
 * @addtogroup t_hwinfo_clear_reset_cause_api
 * @{
 * @defgroup t_hwinfo_clear_reset_cause test_hwinfo_clear_reset_cause
 * @brief TestPurpose: verify clear reset cause works. This may
 *        not work on some platforms, depending on how reset cause register
 *        works on that SoC.
 * @details
 * - Test Steps
 *   -# Read the reset cause and store the result
 *   -# Call clear reset cause
 *   -# Read the reset cause again
 *   -# Check if the two readings match
 * - Expected Results
 *   -# Reset cause value should change after calling clear reset cause.
 * @}
 */
ZTEST(hwinfo_device_id_api, test_clear_reset_cause)
{
	uint32_t cause_1, cause_2;
	ssize_t ret;

	ret = hwinfo_get_reset_cause(&cause_1);
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	ret = hwinfo_clear_reset_cause();
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	ret = hwinfo_get_reset_cause(&cause_2);
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	/* Verify that `cause` has been changed. */
	zassert_not_equal(cause_1, cause_2,
		"Reset cause did not change after clearing");
}

/*
 * @addtogroup t_hwinfo_get_reset_cause_api
 * @{
 * @defgroup t_hwinfo_get_reset_cause test_hwinfo_get_supported_reset_cause
 * @brief TestPurpose: verify get supported reset cause works.
 * @details
 * - Test Steps
 *   -# Set target buffer to a known value
 *   -# Read the reset cause
 *   -# Check if target buffer has been altered
 * - Expected Results
 *   -# Target buffer contents should be changed after the call.
 * @}
 */
ZTEST(hwinfo_device_id_api, test_get_supported_reset_cause)
{
	uint32_t supported;
	ssize_t ret;

	/* Set `supported` to a known value prior to call. */
	supported = 0xDEADBEEF;

	ret = hwinfo_get_supported_reset_cause(&supported);
	if (ret == -ENOSYS) {
		ztest_test_skip();
		return;
	}

	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	/* Verify that `supported` has been changed. */
	zassert_not_equal(supported, 0xDEADBEEF,
					"Supported reset cause not written.");
}


ZTEST_SUITE(hwinfo_device_id_api, NULL, NULL, NULL, NULL, NULL);
