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
		{35U, 0xffffffffU, 0xfffffff7U},
	};
	const unsigned int invalid[] = {0U, 30U, 33U, 34U, 63U, 64U, UINT_MAX};
	uint32_t registers[0xc0U / sizeof(uint32_t)] = {0};
	const struct mip_config config = {
		.base = (uintptr_t)registers,
		.size = sizeof(registers),
		.window_size = DT_PROP(TEST_MIP, brcm_msi_window_size),
		.channel_mask =
			(0ULL DT_FOREACH_MAP_ENTRY(TEST_MIP, interrupt_map, MIP_CHANNEL_BIT)),
		.pci_addr = 0xfffffff000ULL,
	};
	struct mip_data data = {0};
	const struct device dev = {
		.config = &config,
		.data = &data,
		.api = &mip_api,
	};
	struct msi_proxy_message message;
	uint64_t bus_addr;
	uint64_t cpu_addr;
	size_t window_size;

	zassert_ok(mip_init(&dev));
	zassert_ok(msi_proxy_get_window(&dev, &bus_addr, &cpu_addr, &window_size));
	zassert_equal(bus_addr, config.pci_addr);
	zassert_equal(cpu_addr, (uint64_t)config.base);
	zassert_equal(window_size, config.window_size);
	zassert_equal(msi_proxy_get_window(&dev, NULL, &cpu_addr, &window_size), -EINVAL);

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

	zassert_ok(msi_proxy_enable(&dev, 32U));
	zassert_ok(msi_proxy_enable(&dev, 35U));
	zassert_equal(registers[MIP_INT_MASKH_HOST / sizeof(uint32_t)],
		      UINT32_MAX & ~(BIT(0) | BIT(3)));
	msi_proxy_disable(&dev, 32U);
	zassert_equal(registers[MIP_INT_MASKH_HOST / sizeof(uint32_t)], UINT32_MAX & ~BIT(3));
	msi_proxy_disable(&dev, 35U);
	zassert_equal(registers[MIP_INT_MASKH_HOST / sizeof(uint32_t)], UINT32_MAX);

	/* A stray mask write must be visible for every invalid channel. */
	registers[MIP_INT_MASKL_HOST / sizeof(uint32_t)] = 0U;
	registers[MIP_INT_MASKH_HOST / sizeof(uint32_t)] = 0U;
	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		zassert_equal(msi_proxy_compose(&dev, invalid[i], &message), -EINVAL);
		zassert_equal(msi_proxy_enable(&dev, invalid[i]), -EINVAL);
		msi_proxy_disable(&dev, invalid[i]);
		zassert_equal(registers[MIP_INT_MASKL_HOST / sizeof(uint32_t)], 0U);
		zassert_equal(registers[MIP_INT_MASKH_HOST / sizeof(uint32_t)], 0U);
	}
}

ZTEST_SUITE(bcm2712_mip, NULL, NULL, NULL, NULL, NULL);
