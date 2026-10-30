/**
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/nvmem.h>

#define consumer0 DT_NODELABEL(test_consumer0)
#define nvmem0    DT_NODELABEL(test_nvmem0)

#define CELL_IS_PSA                                                                                \
	(DT_NODE_HAS_COMPAT(nvmem0, zephyr_nvmem_psa_its) ||                                       \
	 DT_NODE_HAS_COMPAT(nvmem0, zephyr_nvmem_psa_ps))

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

#if CELL_IS_PSA

#include <psa/storage_common.h>
#if DT_NODE_HAS_COMPAT(nvmem0, zephyr_nvmem_psa_ps)
#include <psa/protected_storage.h>
#define PSA_SET(...) psa_ps_set(__VA_ARGS__)
#else
#include <psa/internal_trusted_storage.h>
#define PSA_SET(...) psa_its_set(__VA_ARGS__)
#endif

#define consumer1 DT_NODELABEL(test_consumer1)

/* The UID a cell's entry lives at: the provider's uid-base plus the cell address. */
#define PSA_UID(cell_addr)                                                                         \
	((psa_storage_uid_t)((((uint64_t)DT_PROP_BY_IDX(nvmem0, uid_base, 0) << 32) |              \
			      (uint64_t)DT_PROP_BY_IDX(nvmem0, uid_base, 1)) +                     \
			     (cell_addr)))

static const struct nvmem_cell cell20 = NVMEM_CELL_GET_BY_NAME(consumer1, cell20);
static const struct nvmem_cell cell30 = NVMEM_CELL_GET_BY_NAME(consumer1, cell30);
static const struct nvmem_cell cell40 = NVMEM_CELL_GET_BY_NAME(consumer1, cell40);

ZTEST(nvmem_api, test_nvmem_psa)
{
	uint8_t expected[0x10];
	uint8_t patch[4] = {0xaa, 0xbb, 0xcc, 0xdd};
	uint8_t buf[0x10];
	psa_status_t status;
	int ret;

	zexpect_true(nvmem_cell_is_ready(&cell20));

	/* An entry that does not exist has no data. */
	ret = nvmem_cell_read(&cell20, buf, 0, sizeof(buf));
	zexpect_equal(ret, -ENOENT, "Expected no entry, got %d", ret);

	/* Creating an entry with a hole is refused. */
	ret = nvmem_cell_write(&cell20, patch, 4, sizeof(patch));
	zexpect_equal(ret, -ENOENT, "Expected no entry, got %d", ret);

	/* A write from offset 0 creates the entry. */
	for (size_t i = 0; i < sizeof(expected); ++i) {
		expected[i] = i + 1;
	}
	ret = nvmem_cell_write(&cell20, expected, 0, sizeof(expected));
	zassert_ok(ret, "Failed to create the entry: %d", ret);
	ret = nvmem_cell_read(&cell20, buf, 0, sizeof(buf));
	zexpect_ok(ret, "Failed to read NVMEM: %d", ret);
	zexpect_mem_equal(buf, expected, sizeof(buf));

	/* A full write replaces the existing entry. */
	for (size_t i = 0; i < sizeof(expected); ++i) {
		expected[i] = 0xff - i;
	}
	ret = nvmem_cell_write(&cell20, expected, 0, sizeof(expected));
	zexpect_ok(ret, "Failed to write NVMEM: %d", ret);
	ret = nvmem_cell_read(&cell20, buf, 0, sizeof(buf));
	zexpect_ok(ret, "Failed to read NVMEM: %d", ret);
	zexpect_mem_equal(buf, expected, sizeof(buf));

	/* A partial write read-modify-writes the entry, keeping other bytes. */
	memcpy(&expected[4], patch, sizeof(patch));
	ret = nvmem_cell_write(&cell20, patch, 4, sizeof(patch));
	zexpect_ok(ret, "Failed to write NVMEM: %d", ret);
	ret = nvmem_cell_read(&cell20, buf, 0, sizeof(buf));
	zexpect_ok(ret, "Failed to read NVMEM: %d", ret);
	zexpect_mem_equal(buf, expected, sizeof(buf));

	/* Partial reads map to PSA partial reads. */
	memset(buf, 0, sizeof(buf));
	ret = nvmem_cell_read(&cell20, buf, 4, sizeof(patch));
	zexpect_ok(ret, "Failed to read NVMEM: %d", ret);
	zexpect_mem_equal(buf, patch, sizeof(patch));

	/* An entry shorter than the cell cannot fill a read beyond its size. */
	status = PSA_SET(PSA_UID(0x30), 8, expected, PSA_STORAGE_FLAG_NONE);
	zassert_equal(status, PSA_SUCCESS, "Failed to provision the entry: %d", status);
	ret = nvmem_cell_read(&cell30, buf, 0, sizeof(buf));
	zexpect_equal(ret, -ENODATA, "Expected a short entry, got %d", ret);
	ret = nvmem_cell_read(&cell30, buf, 0, 8);
	zexpect_ok(ret, "Failed to read NVMEM: %d", ret);
	zexpect_mem_equal(buf, expected, 8);

	/* Entries protected by the PSA implementation are read-only. */
	status = PSA_SET(PSA_UID(0x40), sizeof(expected), expected, PSA_STORAGE_FLAG_WRITE_ONCE);
	zassert_equal(status, PSA_SUCCESS, "Failed to provision the entry: %d", status);
	ret = nvmem_cell_write(&cell40, patch, 0, sizeof(patch));
	zexpect_equal(ret, -EROFS, "Expected a write-once entry, got %d", ret);
	ret = nvmem_cell_read(&cell40, buf, 0, sizeof(buf));
	zexpect_ok(ret, "Failed to read NVMEM: %d", ret);
	zexpect_mem_equal(buf, expected, sizeof(buf));
}

#endif /* CELL_IS_PSA */

ZTEST(nvmem_api, test_nvmem_missing)
{
	const struct nvmem_cell missing_idx = NVMEM_CELL_GET_BY_IDX_OR(consumer0, 10, {});
	const struct nvmem_cell missing_name = NVMEM_CELL_GET_BY_NAME_OR(consumer0, missing, {});

	zassert_is_null(missing_idx.dev);
	zassert_is_null(missing_name.dev);
}

ZTEST_SUITE(nvmem_api, NULL, NULL, NULL, NULL, NULL);
