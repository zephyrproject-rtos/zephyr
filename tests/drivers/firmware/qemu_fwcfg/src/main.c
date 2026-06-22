/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/firmware/qemu_fwcfg/qemu_fwcfg.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static const struct device *get_fwcfg_device(void)
{
	const struct device *fwcfg = DEVICE_DT_GET(DT_NODELABEL(fwcfg0));

	zassert_not_null(fwcfg, "Unable to get fwcfg device");
	zassert_true(device_is_ready(fwcfg), "fwcfg device not ready");

	return fwcfg;
}

ZTEST(fwcfg, test_fwcfg_device_instantiation)
{
	(void)get_fwcfg_device();
}

ZTEST(fwcfg, test_fwcfg_device_features)
{
	const struct device *fwcfg = get_fwcfg_device();
	int ret;
	uint32_t features;
	bool dma_supported;

	ret = qemu_fwcfg_get_features(fwcfg, &features);
	dma_supported = qemu_fwcfg_dma_supported(fwcfg);

	zassert_ok(ret, "Failed to get features: (%d)", ret);
	zassert_true(features & FW_CFG_ID_F_TRADITIONAL,
		     "Required feature (traditional interface) not available");
	zassert_equal((features & FW_CFG_ID_F_DMA) != 0U, dma_supported,
		      "Feature DMA support bit mismatch: %u != %u",
		      (uint32_t)((features & FW_CFG_ID_F_DMA) != 0U), (uint32_t)dma_supported);
}

ZTEST(fwcfg, test_fwcfg_find_file)
{
	const struct device *fwcfg = get_fwcfg_device();
	int ret;
	uint16_t select;
	uint32_t size;

	ret = qemu_fwcfg_find_file(fwcfg, "etc/ramfb", &select, &size);
	zassert_not_ok(ret, "Returned success for non-existing directory");

	ret = qemu_fwcfg_find_file(fwcfg, "opt/zephyr/string", &select, &size);
	zassert_ok(ret, "Failed to get file selector: (%d)", ret);
	zassert(size > 0, "Returned size is invalid");
}

ZTEST(fwcfg, test_fwcfg_read)
{
	const struct device *fwcfg = get_fwcfg_device();
	static const char expected[] = "FWCFG_ZEPHYR_TEST";
	const size_t expected_len = sizeof(expected) - 1U;
	char *buf;
	int ret;
	uint16_t select;
	uint32_t size;

	ret = qemu_fwcfg_find_file(fwcfg, "opt/zephyr/string", &select, &size);
	zassert_ok(ret, "Failed to get file selector: (%d)", ret);
	zassert_true(size == expected_len || size == (expected_len + 1U),
		     "Unexpected file size: %u", (unsigned int)size);

	buf = k_malloc((size_t)size + 1U);
	zassert_not_null(buf, "Failed to allocate %u bytes", (unsigned int)(size + 1U));

	(void)memset(buf, 0, (size_t)size + 1U);
	ret = qemu_fwcfg_read_item(fwcfg, select, buf, size);
	zassert_ok(ret, "Failed to read data: %d", ret);
	zassert_mem_equal(buf, expected, expected_len, "Unexpected fwcfg file content");
	if (size == (expected_len + 1U)) {
		zassert_equal(buf[expected_len], '\0', "Missing trailing NUL");
	}

	k_free(buf);
}

ZTEST_SUITE(fwcfg, NULL, NULL, NULL, NULL, NULL);
