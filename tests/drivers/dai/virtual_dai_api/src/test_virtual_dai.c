/*
 * Copyright (c) 2025 Suraj Sonawane <surajsonawane0215@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_virtual_dai, CONFIG_DAI_LOG_LEVEL);

/* Get the virtual DAI device */
static const struct device *const virtual_dai_dev = DEVICE_DT_GET(DT_NODELABEL(virtual_dai));

ZTEST_SUITE(virtual_dai, NULL, NULL, NULL, NULL, NULL);

/* Test 1: Verify device exists and is ready */
ZTEST(virtual_dai, test_device_exists)
{
	zassert_not_null(virtual_dai_dev, "Virtual DAI device should exist");
	zassert_true(device_is_ready(virtual_dai_dev), "Device should be ready");
}

/* Test 2: Test dai_config_set using values from dai_config_get */
ZTEST(virtual_dai, test_dai_config_set_using_retrieved_config)
{
	struct dai_config config;
	int ret;

	/* First get the configuration */
	ret = dai_config_get(virtual_dai_dev, &config, 0);
	zassert_ok(ret, "dai_config_get should succeed");

	/* Log the configuration */
	LOG_INF("Config: type=%d, dai_index=%d, rate=%d, channels=%d",
			config.type, config.dai_index);

	/* Set the configuration */
	ret = dai_config_set(virtual_dai_dev, &config, NULL);
	zassert_ok(ret, "dai_config_set should return success (0)");
}

/* Test 3: Test dai_config_set with invalid type (should fail) */
ZTEST(virtual_dai, test_dai_config_set_invalid_type)
{
	struct dai_config config;
	struct dai_config invalid_config;
	int ret;

	/* Get the current configuration to see what type is valid */
	ret = dai_config_get(virtual_dai_dev, &config, 0);
	zassert_ok(ret, "dai_config_get should succeed");

	/* Create an invalid configuration (use a different type) */
	invalid_config.type = config.type + 100; /* Invalid type */
	invalid_config.dai_index = config.dai_index;

	/* This should fail with -EINVAL */
	ret = dai_config_set(virtual_dai_dev, &invalid_config, NULL);
	zassert_equal(ret, -EINVAL, "dai_config_set should return -EINVAL for invalid type");
}

/* Test 4: Test dai_trigger commands */
ZTEST(virtual_dai, test_dai_trigger_commands)
{
	int ret;

	/* Test START trigger*/
	ret = dai_trigger(virtual_dai_dev, 0, DAI_TRIGGER_START); /* dir = 0 (TX) */
	zassert_ok(ret, "START trigger should return success (0)");

	/* Test STOP trigger*/
	ret = dai_trigger(virtual_dai_dev, 0, DAI_TRIGGER_STOP);
	zassert_ok(ret, "STOP trigger should return success (0)");

	/* Test PAUSE trigger*/
	ret = dai_trigger(virtual_dai_dev, 0, DAI_TRIGGER_PAUSE);
	zassert_ok(ret, "PAUSE trigger should return success (0)");

	/* Test COPY trigger*/
	ret = dai_trigger(virtual_dai_dev, 0, DAI_TRIGGER_COPY);
	zassert_ok(ret, "COPY trigger should return success (0)");

	/* Test invalid trigger command */
	ret = dai_trigger(virtual_dai_dev, 0, 99); /* Invalid command */
	zassert_equal(ret, -EINVAL, "Should return -EINVAL for invalid trigger");
}

/* Test 5: Test dai_get_properties */
ZTEST(virtual_dai, test_dai_get_properties)
{
	const struct dai_properties *props;

	props = dai_get_properties(virtual_dai_dev, 0, 0); /* dir = 0 (TX), stream_id = 0 */
	zassert_is_null(props, "dai_get_properties should return NULL");
}

/* Test 6: Test dai_probe and dai_remove functions */
ZTEST(virtual_dai, test_probe_remove)
{
	int ret;

	ret = dai_probe(virtual_dai_dev);
	zassert_ok(ret, "Probe should succeed");

	ret = dai_remove(virtual_dai_dev);
	zassert_ok(ret, "Remove should succeed");
}
