/*
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>

#include "fixture.h"

ZTEST_SUITE(generic, NULL, NULL, NULL, NULL, NULL);

const struct emul *get_and_check_emul(const struct device *dev)
{
	zassert_not_null(dev, "Cannot get device pointer. Is this driver properly instantiated?");

	const struct emul *emul = emul_get_binding(dev->name);

	/* Skip this sensor if there is no emulator or backend loaded. */
	if (emul == NULL || emul->backend_api == NULL) {
		ztest_test_skip();
	}
	return emul;
}
