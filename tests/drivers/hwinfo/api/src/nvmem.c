/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/nvmem.h>
#include <zephyr/ztest.h>

#define HWINFO_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_hwinfo_nvmem)

#define BUFFER_CANARY 0x5AU

#define DEVICE_ID_SIZE DT_REG_SIZE(DT_NVMEM_CELL_BY_NAME(HWINFO_NODE, device_id))
#define EUI64_SIZE     DT_REG_SIZE(DT_NVMEM_CELL_BY_NAME(HWINFO_NODE, eui64))

static const struct nvmem_cell device_id_cell = NVMEM_CELL_GET_BY_NAME(HWINFO_NODE, device_id);
static const struct nvmem_cell eui64_cell = NVMEM_CELL_GET_BY_NAME(HWINFO_NODE, eui64);

static uint8_t device_id[DEVICE_ID_SIZE];
static uint8_t eui64[EUI64_SIZE];
static int seed_ret;

static void *hwinfo_nvmem_setup(void)
{
	for (size_t i = 0; i < sizeof(device_id); i++) {
		device_id[i] = (uint8_t)(0xA0U + i);
	}

	for (size_t i = 0; i < sizeof(eui64); i++) {
		eui64[i] = (uint8_t)(0x11U * i);
	}

	seed_ret = nvmem_cell_write(&device_id_cell, device_id, 0, sizeof(device_id));
	if (seed_ret == 0) {
		seed_ret = nvmem_cell_write(&eui64_cell, eui64, 0, sizeof(eui64));
	}

	return NULL;
}

ZTEST(hwinfo_nvmem, test_device_id_content)
{
	uint8_t buffer[DEVICE_ID_SIZE * 2];
	ssize_t length;

	zassert_equal(seed_ret, 0, "Failed to seed the nvmem cells: %d", seed_ret);

	memset(buffer, BUFFER_CANARY, sizeof(buffer));
	length = hwinfo_get_device_id(buffer, sizeof(buffer));
	zexpect_equal(length, (ssize_t)DEVICE_ID_SIZE, "Length not clamped to the cell size: %zd",
		      length);
	zexpect_mem_equal(buffer, device_id, DEVICE_ID_SIZE,
			  "Device ID does not match the cell content");
	zexpect_equal(buffer[DEVICE_ID_SIZE], BUFFER_CANARY,
		      "Bytes beyond the device ID were written");

	memset(buffer, BUFFER_CANARY, sizeof(buffer));
	length = hwinfo_get_device_id(buffer, 4);
	zexpect_equal(length, 4, "Length not clamped to the request: %zd", length);
	zexpect_mem_equal(buffer, device_id, 4, "Device ID does not match the cell content");
	zexpect_equal(buffer[4], BUFFER_CANARY, "Bytes beyond the requested length were written");
}

ZTEST(hwinfo_nvmem, test_eui64_content)
{
	uint8_t buffer[EUI64_SIZE];
	int ret;

	zassert_equal(seed_ret, 0, "Failed to seed the nvmem cells: %d", seed_ret);

	ret = hwinfo_get_device_eui64(buffer);
	zexpect_equal(ret, 0, "Failed to get the EUI64: %d", ret);
	zexpect_mem_equal(buffer, eui64, sizeof(eui64), "EUI64 does not match the cell content");
}

ZTEST_SUITE(hwinfo_nvmem, NULL, hwinfo_nvmem_setup, NULL, NULL, NULL);
