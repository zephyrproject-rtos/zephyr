/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <zephyr/ztest.h>

/* Exercise the driver with RAM-backed registers and no hardware device. */
#include "intc_bcm2712_mip.c"

#define TEST_MIP DT_NODELABEL(test_mip)

DT_FOREACH_MAP_ENTRY(TEST_MIP, interrupt_map, MIP_CHANNEL_CHECK)

ZTEST(bcm2712_mip, test_hardware_channels)
{
	const struct {
		unsigned int channel;
		uint32_t mask_low;
		uint32_t mask_high;
	} cases[] = {
		{31U, 0x7fffffffU, 0xffffffffU},
		{32U, 0xffffffffU, 0xfffffffeU},
		{33U, 0xffffffffU, 0xfffffffdU},
	};
	const unsigned int invalid[] = {0U, 30U, 34U, 63U, 64U, UINT_MAX};
	uint32_t registers[0xc0U / sizeof(uint32_t)] = {0};
	const struct mip_config config = {
		.base = (uintptr_t)registers,
		.size = sizeof(registers),
		.first_channel = MIP_FIRST_CHANNEL(TEST_MIP),
		.num_channels = DT_MAP_LEN(TEST_MIP, interrupt_map),
		.pci_addr = 0xfffffff000ULL,
	};
	struct mip_data data = {0};
	const struct device dev = {
		.config = &config,
		.data = &data,
		.api = &mip_api,
	};
	struct msi_proxy_message message;

	zassert_ok(mip_init(&dev));

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		zassert_ok(msi_proxy_compose(&dev, cases[i].channel, &message));
		zassert_equal(message.address, config.pci_addr);
		zassert_equal(message.data, cases[i].channel);
		zassert_ok(msi_proxy_enable(&dev, cases[i].channel));
		zassert_equal(registers[0x40U / sizeof(uint32_t)], cases[i].mask_low);
		zassert_equal(registers[0x50U / sizeof(uint32_t)], cases[i].mask_high);

		msi_proxy_disable(&dev, cases[i].channel);
		zassert_equal(registers[0x40U / sizeof(uint32_t)], UINT32_MAX);
		zassert_equal(registers[0x50U / sizeof(uint32_t)], UINT32_MAX);
	}

	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		zassert_equal(msi_proxy_compose(&dev, invalid[i], &message), -EINVAL);
		zassert_equal(msi_proxy_enable(&dev, invalid[i]), -EINVAL);
		msi_proxy_disable(&dev, invalid[i]);
		zassert_equal(registers[0x40U / sizeof(uint32_t)], UINT32_MAX);
		zassert_equal(registers[0x50U / sizeof(uint32_t)], UINT32_MAX);
	}
}

ZTEST_SUITE(bcm2712_mip, NULL, NULL, NULL, NULL, NULL);
