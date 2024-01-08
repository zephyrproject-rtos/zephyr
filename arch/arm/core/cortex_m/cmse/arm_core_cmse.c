/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <cortex_m/cmse.h>

int arm_cmse_mpu_region_get(uint32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flags.mpu_region_valid) {
		return addr_info.flags.mpu_region;
	}

	return -EINVAL;
}

static int arm_cmse_addr_read_write_ok(uint32_t addr, int force_npriv, int rw)
{
	cmse_address_info_t addr_info;
	if (force_npriv) {
		addr_info = cmse_TTT((void *)addr);
	} else {
		addr_info = cmse_TT((void *)addr);
	}

	return rw ? addr_info.flags.readwrite_ok : addr_info.flags.read_ok;
}

int arm_cmse_addr_read_ok(uint32_t addr, int force_npriv)
{
	return arm_cmse_addr_read_write_ok(addr, force_npriv, 0);
}

int arm_cmse_addr_readwrite_ok(uint32_t addr, int force_npriv)
{
	return arm_cmse_addr_read_write_ok(addr, force_npriv, 1);
}

static int arm_cmse_addr_range_read_write_ok(uint32_t addr, uint32_t size,
	int force_npriv, int rw)
{
	int flags = 0;

	if (force_npriv != 0) {
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

int arm_cmse_addr_range_read_ok(uint32_t addr, uint32_t size, int force_npriv)
{
	return arm_cmse_addr_range_read_write_ok(addr, size, force_npriv, 0);
}

int arm_cmse_addr_range_readwrite_ok(uint32_t addr, uint32_t size, int force_npriv)
{
	return arm_cmse_addr_range_read_write_ok(addr, size, force_npriv, 1);
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)

int arm_cmse_mpu_nonsecure_region_get(uint32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TTA((void *)addr);

	if (addr_info.flags.mpu_region_valid) {
		return  addr_info.flags.mpu_region;
	}

	return -EINVAL;
}

int arm_cmse_sau_region_get(uint32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flags.sau_region_valid) {
		return addr_info.flags.sau_region;
	}

	return -EINVAL;
}

int arm_cmse_idau_region_get(uint32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flags.idau_region_valid) {
		return addr_info.flags.idau_region;
	}

	return -EINVAL;
}

int arm_cmse_addr_is_secure(uint32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	return addr_info.flags.secure;
}

static int arm_cmse_addr_nonsecure_read_write_ok(uint32_t addr,
	int force_npriv, int rw)
{
	cmse_address_info_t addr_info;
	if (force_npriv) {
		addr_info = cmse_TTAT((void *)addr);
	} else {
		addr_info = cmse_TTA((void *)addr);
	}

	return rw ? addr_info.flags.nonsecure_readwrite_ok :
		addr_info.flags.nonsecure_read_ok;
}

int arm_cmse_addr_nonsecure_read_ok(uint32_t addr, int force_npriv)
{
	return arm_cmse_addr_nonsecure_read_write_ok(addr, force_npriv, 0);
}

int arm_cmse_addr_nonsecure_readwrite_ok(uint32_t addr, int force_npriv)
{
	return arm_cmse_addr_nonsecure_read_write_ok(addr, force_npriv, 1);
}

static int arm_cmse_addr_range_nonsecure_read_write_ok(uint32_t addr, uint32_t size,
	int force_npriv, int rw)
{
	int flags = CMSE_NONSECURE;

	if (force_npriv != 0) {
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

int arm_cmse_addr_range_nonsecure_read_ok(uint32_t addr, uint32_t size,
	int force_npriv)
{
	return arm_cmse_addr_range_nonsecure_read_write_ok(addr, size,
		force_npriv, 0);
}

int arm_cmse_addr_range_nonsecure_readwrite_ok(uint32_t addr, uint32_t size,
	int force_npriv)
{
	return arm_cmse_addr_range_nonsecure_read_write_ok(addr, size,
		force_npriv, 1);
}

#endif /* CONFIG_ARM_SECURE_FIRMWARE */
