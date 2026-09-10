/*
 * Copyright (c) 2024 Jeferson Fernando <jeferson.santos@edge.ufal.br>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

/**
 * @brief Test channels mask with size 1
 *
 * This test will request the channels mask changes, passing valid
 * and invalid arguments and checking when it's successful or returns error
 *
 */
ZTEST(channels_mask, test_mask_size_1)
{
	int err = 0;
	uint16_t channels_mask[LORAWAN_CHANNELS_MASK_SIZE_AS923] = {0xffff};

	/* Test the function when a region with mask size 1 is being used */
	err = lorawan_set_region(LORAWAN_REGION_AS923);
	zassert_equal(err, 0, "Could not set region");

	err = lorawan_start();
	zassert_equal(err, 0, "Could not start stack");

	/* Configure channels mask with expected parameters */
	err = lorawan_set_channels_mask(channels_mask, LORAWAN_CHANNELS_MASK_SIZE_AS923);
	zassert_equal(err, 0, "Denied right channels mask configuration");

	/* Configure channels mask with unexpected channels mask size */
	err = lorawan_set_channels_mask(channels_mask, LORAWAN_CHANNELS_MASK_SIZE_AU915);
	zassert_equal(err, -EINVAL, "Accepted an unexpected mask size for the selected region");

	/* Configure channels mask with pointer to NULL */
	err = lorawan_set_channels_mask(NULL, LORAWAN_CHANNELS_MASK_SIZE_AS923);
	zassert_equal(err, -EINVAL, "Accepted a pointer to NULL");
}

/**
 * @brief Test channels mask with size 6
 *
 * This test will request the channels mask changes, passing valid
 * and invalid arguments and checking when it's successful or returns error
 *
 */
ZTEST(channels_mask, test_mask_size_6)
{
	int err = 0;
	uint16_t channels_mask[LORAWAN_CHANNELS_MASK_SIZE_AU915] = {0};

	/* Test the function when a region with mask size 6 is being used */
	err = lorawan_set_region(LORAWAN_REGION_AU915);
	zassert_equal(err, 0, "Could not set region");

	err = lorawan_start();
	zassert_equal(err, 0, "Could not start stack");

	/* Configure channels mask with expected parameters */
	err = lorawan_set_channels_mask(channels_mask, LORAWAN_CHANNELS_MASK_SIZE_AU915);
	zassert_equal(err, 0, "Denied right channels mask configuration");

	/* Configure channels mask with unexpected channels mask size */
	err = lorawan_set_channels_mask(channels_mask, LORAWAN_CHANNELS_MASK_SIZE_AS923);
	zassert_equal(err, -EINVAL, "Accepted an unexpected mask size for the selected region");

	/* Configure channels mask with pointer to NULL */
	err = lorawan_set_channels_mask(NULL, LORAWAN_CHANNELS_MASK_SIZE_AU915);
	zassert_equal(err, -EINVAL, "Accepted a pointer to NULL");
}

ZTEST_SUITE(channels_mask, NULL, NULL, NULL, NULL, NULL);
