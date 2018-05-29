/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <cortex_m/cmse.h>

int arm_cmse_mpu_region_get(u32_t addr, u8_t *p_region)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flags.mpu_region_valid) {
		*p_region = addr_info.flags.mpu_region;
	}

	return addr_info.flags.mpu_region_valid;
}

static int arm_cmse_addr_read_write_ok(u32_t addr, int force_npriv, int rw)
{
	cmse_address_info_t addr_info;
	if (force_npriv) {
		addr_info = cmse_TTT((void *)addr);
	} else {
		addr_info = cmse_TT((void *)addr);
	}

	return rw ? addr_info.flags.readwrite_ok : addr_info.flags.read_ok;
}

int arm_cmse_addr_read_ok(u32_t addr, int force_npriv)
{
	return arm_cmse_addr_read_write_ok(addr, force_npriv, 0);
}

int arm_cmse_addr_readwrite_ok(u32_t addr, int force_npriv)
{
	return arm_cmse_addr_read_write_ok(addr, force_npriv, 1);
}

static int arm_cmse_addr_range_read_write_ok(u32_t addr, u32_t size,
	int force_npriv, int rw)
{
	int flags = 0;

	if (force_npriv) {
		flags |= CMSE_MPU_UNPRIV;
	}
	if (rw) {
		flags |= CMSE_MPU_READWRITE;
	} else {
		flags |= CMSE_MPU_READ;
	}
	if (cmse_check_address_range((void *)addr, size, flags) != NULL) {
		return 1;
	} else {
		return 0;
	}
}

int arm_cmse_addr_range_read_ok(u32_t addr, u32_t size, int force_npriv)
{
	return arm_cmse_addr_range_read_write_ok(addr, size, force_npriv, 0);
}

int arm_cmse_addr_range_readwrite_ok(u32_t addr, u32_t size, int force_npriv)
{
	return arm_cmse_addr_range_read_write_ok(addr, size, force_npriv, 1);
}

