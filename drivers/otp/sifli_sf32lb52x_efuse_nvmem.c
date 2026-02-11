/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/nvmem.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/otp/sifli_sf32lb52x_efuse.h>

#define SF32LB52X_UID_CELL   DT_NODELABEL(sf32lb52_uid)
#define SF32LB52X_BANK1_CELL DT_NODELABEL(sf32lb52_calib_bank1)

static const struct nvmem_cell sf32lb52x_uid_cell = NVMEM_CELL_INIT(SF32LB52X_UID_CELL);
static const struct nvmem_cell sf32lb52x_bank1_cell = NVMEM_CELL_INIT(SF32LB52X_BANK1_CELL);

int sf32lb52x_efuse_read_uid(uint8_t uid[SF32LB52X_EFUSE_UID_SIZE])
{
	if (uid == NULL) {
		return -EINVAL;
	}

	if (sf32lb52x_uid_cell.size < SF32LB52X_EFUSE_UID_SIZE) {
		return -EINVAL;
	}

	return nvmem_cell_read(&sf32lb52x_uid_cell, uid, 0, SF32LB52X_EFUSE_UID_SIZE);
}

int sf32lb52x_efuse_read_bank1(uint8_t bank1[SF32LB52X_EFUSE_BANK1_SIZE])
{
	if (bank1 == NULL) {
		return -EINVAL;
	}

	if (sf32lb52x_bank1_cell.size < SF32LB52X_EFUSE_BANK1_SIZE) {
		return -EINVAL;
	}

	return nvmem_cell_read(&sf32lb52x_bank1_cell, bank1, 0, SF32LB52X_EFUSE_BANK1_SIZE);
}

int sf32lb52x_efuse_read_bank1_field(enum sf32lb52x_efuse_bank1_field field, uint32_t *out)
{
	uint8_t bank1[SF32LB52X_EFUSE_BANK1_SIZE];
	int ret;

	if (out == NULL) {
		return -EINVAL;
	}

	ret = sf32lb52x_efuse_read_bank1(bank1);
	if (ret < 0) {
		return ret;
	}

	return sf32lb52x_efuse_bank1_extract(bank1, field, out);
}

