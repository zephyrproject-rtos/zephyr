/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/misc/interconn/renesas_elc/renesas_elc.h>

/* Define DT aliases for ELC devices */
#define ELC_DEVICE_NODE DT_ALIAS(elc_link)

/* ELC peripheral to be used in this test */
#define ELC_PERIPHERAL_ID_TEST     0
#define ELC_EVENT_ID_TEST          1
#define ELC_SOFTWARE_EVENT_ID_TEST 0

/* Retrieve the ELC device from devicetree */
static const struct device *get_renesas_elc_device(void)
{
	const struct device *dev = DEVICE_DT_GET(ELC_DEVICE_NODE);

	zassert_true(device_is_ready(dev), "ELC device not ready");
	return dev;
}

/* Test enabling and disabling the ELC device */
ZTEST(renesas_elc_api, test_renesas_elc_enable_disable)
{
	const struct device *dev = get_renesas_elc_device();
	int ret;

	/* Enable ELC */
	ret = renesas_elc_enable(dev);
	zassert_ok(ret, "Failed to enable ELC");

	/* Disable ELC */
	ret = renesas_elc_disable(dev);
	zassert_ok(ret, "Failed to disable ELC");
}

/* Test linking an event to a peripheral and then breaking the link */
ZTEST(renesas_elc_api, test_renesas_elc_link_set_break)
{
	const struct device *dev = get_renesas_elc_device();
	int ret;

	uint32_t test_peripheral = ELC_PERIPHERAL_ID_TEST;
	uint32_t test_event = ELC_EVENT_ID_TEST;
	uint32_t test_sw_event = ELC_SOFTWARE_EVENT_ID_TEST;

	/* Enable first, so we can set up links */
	ret = renesas_elc_enable(dev);
	zassert_ok(ret, "Failed to enable ELC");

	/* Link test_event to test_peripheral */
	ret = renesas_elc_link_set(dev, test_peripheral, test_event);
	zassert_ok(ret, "Failed to link event to peripheral");

	/* Trigger a software event */
	ret = renesas_elc_software_event_generate(dev, test_sw_event);
	zassert_ok(ret, "Failed to generate software event");

	/* Now break the link */
	ret = renesas_elc_link_break(dev, test_peripheral);
	zassert_ok(ret, "Failed to break link");

	/* Finally disable */
	ret = renesas_elc_disable(dev);
	zassert_ok(ret, "Failed to disable ELC");
}

/* The test suite definition */
ZTEST_SUITE(renesas_elc_api, NULL, NULL, NULL, NULL, NULL);
