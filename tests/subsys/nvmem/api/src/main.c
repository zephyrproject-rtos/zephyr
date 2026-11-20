/**
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/nvmem.h>

#define consumer0 DT_NODELABEL(test_consumer0)
#define nvmem0    DT_NODELABEL(test_nvmem0)

static const struct nvmem_cell cell0 = NVMEM_CELL_GET_BY_IDX(consumer0, 0);
static const struct nvmem_cell cell10 = NVMEM_CELL_GET_BY_NAME(consumer0, cell10);

ZTEST(nvmem_api, test_nvmem_api)
{
	uint8_t buf[0x10];
	int ret;

	zexpect_equal_ptr(cell0.dev, DEVICE_DT_GET(nvmem0));
	zexpect_equal(cell0.offset, 0);
	zexpect_equal(cell0.size, 0x10);
	zexpect_false(nvmem_cell_is_read_only(&cell0));

	zexpect_equal_ptr(cell10.dev, DEVICE_DT_GET(nvmem0));
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

ZTEST(nvmem_api, test_nvmem_reversed)
{
	static const struct nvmem_cell cell20 = NVMEM_CELL_GET_BY_NAME(consumer0, cell20);
	const struct nvmem_cell stored = {
		.dev = cell20.dev,
		.offset = cell20.offset,
		.size = cell20.size,
	};
	uint8_t data[0x18];
	uint8_t buf[sizeof(data)];
	uint64_t val = 0;
	int ret;

	zexpect_equal(cell20.size, sizeof(data));
	zexpect_not_equal(cell20.flags & NVMEM_CELL_FLAG_REVERSED, 0);
	zexpect_equal(cell0.flags & NVMEM_CELL_FLAG_REVERSED, 0);

	for (size_t i = 0; i < sizeof(data); ++i) {
		data[i] = i + 1;
	}

	/* Two writes, split at an offset that is not in the middle of the cell */
	ret = nvmem_cell_write(&cell20, data, 0, 3);
	zassert_ok(ret, "Failed to write NVMEM");
	ret = nvmem_cell_write(&cell20, &data[3], 3, sizeof(data) - 3);
	zassert_ok(ret, "Failed to write NVMEM");

	ret = nvmem_cell_read(&cell20, buf, 0, sizeof(buf));
	zassert_ok(ret, "Failed to read NVMEM");
	zexpect_mem_equal(buf, data, sizeof(data));

	ret = nvmem_cell_read(&cell20, buf, 2, 5);
	zassert_ok(ret, "Failed to read NVMEM");
	zexpect_mem_equal(buf, &data[2], 5);

	ret = nvmem_cell_read_be48(&cell20, &val, 0);
	zassert_ok(ret, "Failed to read NVMEM");
	zexpect_equal(val, 0x010203040506ULL);

	/* The same region without the reversal shows what is stored */
	ret = nvmem_cell_read(&stored, buf, 0, sizeof(buf));
	zassert_ok(ret, "Failed to read NVMEM");

	for (size_t i = 0; i < sizeof(buf); ++i) {
		zexpect_equal(buf[i], data[sizeof(data) - 1 - i]);
	}

	zexpect_equal(nvmem_cell_read(&cell20, buf, 1, sizeof(buf)), -EINVAL);
}

ZTEST(nvmem_api, test_nvmem_missing)
{
	const struct nvmem_cell missing_idx = NVMEM_CELL_GET_BY_IDX_OR(consumer0, 10, {});
	const struct nvmem_cell missing_name = NVMEM_CELL_GET_BY_NAME_OR(consumer0, missing, {});

	zassert_is_null(missing_idx.dev);
	zassert_is_null(missing_name.dev);
}

ZTEST_SUITE(nvmem_api, NULL, NULL, NULL, NULL, NULL);
