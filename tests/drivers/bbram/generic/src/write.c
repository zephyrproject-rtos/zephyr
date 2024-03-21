/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_bbram.h>
#include <zephyr/ztest.h>

#include "fixture.h"

static void run_test_write_invalid_size(const struct device *dev, const struct emul *emulator)
{
	uint8_t data = 0;
	size_t bbram_size = 0;
	int rc;

	ARG_UNUSED(emulator);

	rc = bbram_write(dev, 0, 0, &data);
	zassert_equal(-EINVAL, rc, "got %d", rc);

	rc = bbram_get_size(dev, &bbram_size);
	zassert_ok(rc, "got %d", rc);

	rc = bbram_write(dev, 0, bbram_size + 1, &data);
	zassert_equal(-EINVAL, rc, "got %d", rc);
}

static void run_test_write_bytes(const struct device *dev, const struct emul *emulator)
{
	uint8_t data = 0;
	uint8_t expected_data = 0;
	size_t bbram_size = 0;
	int rc;

	rc = bbram_get_size(dev, &bbram_size);
	zassert_ok(rc, "got %d", rc);

	for (size_t i = 0; i < bbram_size; ++i) {
		expected_data = i % BIT(8);
		rc = bbram_write(dev, i, 1, &expected_data);
		zassert_ok(rc, "Failed to set expected data at offset %zu", i);

		rc = emul_bbram_backend_get_data(emulator, i, 1, &data);
		zassert_ok(rc, "Failed to get byte at offset %zu", i);
		zassert_equal(expected_data, data, "Expected %u, but got %u", expected_data, data);
	}
}

#define DECLARE_ZTEST_PER_DEVICE(inst)                                                             \
	BBRAM_TEST_IMPL(write_invalid_size, inst)                                                  \
	BBRAM_TEST_IMPL(write_bytes, inst)

BBRAM_FOR_EACH(DECLARE_ZTEST_PER_DEVICE)
