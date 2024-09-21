/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/phy.h>

static uint32_t written = 0xFFFFFFFF;

static int test_phy_read_mmd(const struct device *dev, uint8_t dev_addr, uint16_t reg_addr,
			     uint32_t *data)
{
	*data = reg_addr + 1;
	return 0;
}

static int test_phy_write_mmd(const struct device *dev, uint8_t dev_addr, uint16_t reg_addr,
			      uint32_t data)
{
	written = data;
	return 0;
}

struct ethphy_driver_api test_phy_api = {
	.read_mmd = test_phy_read_mmd,
	.write_mmd = test_phy_write_mmd,
};

static struct device_state test_phy_state = {.init_res = 0, .initialized = true};

static const struct device test_phy = {
	.name = "test_phy",
	.state = &test_phy_state,
	.api = &test_phy_api,
};

static void test_setup(void *fixture)
{
	ARG_UNUSED(fixture);
}

ZTEST(eth_phy_tests, test_read_mmd)
{
	uint32_t value;
	/* Normal read */
	zassert_ok(phy_read_mmd(&test_phy, 0x0, 0x1000, &value));
	zassert_equal(value, 0x1001);
	/* Out of range mmd */
	zassert_equal(phy_read_mmd(&test_phy, 0x20, 0x1000, &value), -EINVAL);
	/* No implementaion */
	test_phy_api.read_mmd = NULL;
	zassert_equal(phy_read_mmd(&test_phy, 0x0, 0x1000, &value), -ENOSYS);
}

ZTEST(eth_phy_tests, test_write_mmd)
{
	/* Normal write */
	written = 0;
	zassert_ok(phy_write_mmd(&test_phy, 0x0, 0x1000, 0xAFAF));
	zassert_equal(written, 0xAFAF);
	/* Out of range mmd */
	zassert_equal(phy_write_mmd(&test_phy, 0x20, 0x1000, 0xAFAF), -EINVAL);
	/* No implementaion */
	test_phy_api.write_mmd = NULL;
	zassert_equal(phy_write_mmd(&test_phy, 0x0, 0x1000, 0xAFAF), -ENOSYS);
}

ZTEST_SUITE(eth_phy_tests, NULL, NULL, test_setup, NULL, NULL);
