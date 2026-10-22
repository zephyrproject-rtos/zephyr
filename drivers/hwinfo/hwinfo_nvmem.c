/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_hwinfo_nvmem

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/nvmem.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one zephyr,hwinfo-nvmem node must be enabled");

BUILD_ASSERT(DT_INST_NVMEM_CELLS_HAS_NAME(0, device_id),
	     "zephyr,hwinfo-nvmem node must have a \"device-id\" nvmem cell");

#define DEVICE_ID_SIZE DT_REG_SIZE(DT_INST_NVMEM_CELL_BY_NAME(0, device_id))

static const struct nvmem_cell device_id_cell = NVMEM_CELL_INST_GET_BY_NAME(0, device_id);

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	int ret;

	length = MIN(length, (size_t)DEVICE_ID_SIZE);

	ret = nvmem_cell_read(&device_id_cell, buffer, 0, length);
	if (ret < 0) {
		return ret;
	}

	return (ssize_t)length;
}

#if DT_INST_NVMEM_CELLS_HAS_NAME(0, eui64)

BUILD_ASSERT(DT_REG_SIZE(DT_INST_NVMEM_CELL_BY_NAME(0, eui64)) == 8U,
	     "\"eui64\" nvmem cell must be exactly 8 bytes");

static const struct nvmem_cell eui64_cell = NVMEM_CELL_INST_GET_BY_NAME(0, eui64);

int z_impl_hwinfo_get_device_eui64(uint8_t *buffer)
{
	return nvmem_cell_read(&eui64_cell, buffer, 0, 8U);
}

#endif /* DT_INST_NVMEM_CELLS_HAS_NAME(0, eui64) */
