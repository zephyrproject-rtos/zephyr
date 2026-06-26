/**
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/nvmem.h>

#define consumer0 DT_NODELABEL(test_consumer0)
#define nvmem0    DT_NODELABEL(test_nvmem0)

/*
 * MMIO is a property of the cell's parent (the fixed-layout), same as how
 * NVMEM_CELL_INIT() selects the cell type. Detecting it from devicetree lets
 * the MMIO overlay reuse this test without a separate source file.
 */
#define CELL_IS_MMIO DT_PROP(DT_PARENT(DT_NVMEM_CELL_BY_IDX(consumer0, 0)), mmio)

static const struct nvmem_cell cell0 = NVMEM_CELL_GET_BY_IDX(consumer0, 0);
static const struct nvmem_cell cell10 = NVMEM_CELL_GET_BY_NAME(consumer0, cell10);

ZTEST(nvmem_api, test_nvmem_api)
{
	uint8_t buf[0x10];
	int ret;

#if CELL_IS_MMIO
	/* MMIO cells reference the controller's physical base, not a device. */
	zexpect_equal(cell0.phys_addr, (uintptr_t)DT_REG_ADDR(nvmem0));
	zexpect_equal(cell10.phys_addr, (uintptr_t)DT_REG_ADDR(nvmem0));
#else
	zexpect_equal_ptr(cell0.dev, DEVICE_DT_GET(nvmem0));
	zexpect_equal_ptr(cell10.dev, DEVICE_DT_GET(nvmem0));
#endif

	zexpect_true(nvmem_cell_is_ready(&cell0));
	zexpect_equal(cell0.offset, 0);
	zexpect_equal(cell0.size, 0x10);
	zexpect_false(nvmem_cell_is_read_only(&cell0));

	zexpect_true(nvmem_cell_is_ready(&cell10));
	zexpect_equal(cell10.offset, 0x10);
	zexpect_equal(cell10.size, 0x10);
	zexpect_true(nvmem_cell_is_read_only(&cell10));

	for (size_t i = 0; i < sizeof(buf); ++i) {
		buf[i] = 2 * i;
	}

	ret = nvmem_cell_write(&cell0, buf, 0, sizeof(buf));
	zassert_ok(ret, "Failed to write NVMEM");

	memset(buf, 0, sizeof(buf));

	ret = nvmem_cell_read(&cell0, buf, 0, sizeof(buf));
	zassert_ok(ret, "Failed to read NVMEM");

	for (size_t i = 0; i < sizeof(buf); ++i) {
		zexpect_equal(buf[i], 2 * i);
	}

	ret = nvmem_cell_write(&cell10, buf, 0, sizeof(buf));
	zassert_equal(ret, -EROFS, "Expected read-only NVMEM");
}

ZTEST(nvmem_api, test_nvmem_missing)
{
	const struct nvmem_cell missing_idx = NVMEM_CELL_GET_BY_IDX_OR(consumer0, 10, {});
	const struct nvmem_cell missing_name = NVMEM_CELL_GET_BY_NAME_OR(consumer0, missing, {});

	zassert_is_null(missing_idx.dev);
	zassert_is_null(missing_name.dev);
}

ZTEST_SUITE(nvmem_api, NULL, NULL, NULL, NULL, NULL);
