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
