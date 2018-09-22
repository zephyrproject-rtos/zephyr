/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_CORE_CORTEX_M_MPU_ARM_MPU_V7_INTERNAL_H_
#define ZEPHYR_ARCH_ARM_CORE_CORTEX_M_MPU_ARM_MPU_V7_INTERNAL_H_

/* Global MPU configuration at system initialization. */
static void _mpu_init(void)
{
	/* No specific configuration at init for ARMv7-M MPU. */
}

/* This internal function performs MPU region initialization.
 *
 * Note:
 *   The caller must provide a valid region index.
 */
static void _region_init(u32_t index, struct arm_mpu_region *region_conf)
{
	/* Select the region you want to access */
	MPU->RNR = index;
	/* Configure the region */
	MPU->RBAR = (region_conf->base & MPU_RBAR_ADDR_Msk)
				| MPU_RBAR_VALID_Msk | index;
	MPU->RASR = region_conf->attr.rasr | MPU_RASR_ENABLE_Msk;
	SYS_LOG_DBG("[%d] 0x%08x 0x%08x",
		index, region_conf->base, region_conf->attr.rasr);
}

#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD) || \
	defined(CONFIG_APPLICATION_MEMORY)

static inline u8_t _get_num_regions(void);
static inline u32_t _get_region_index_by_type(u32_t type);

/**
 * This internal function converts the region size to
 * the SIZE field value of MPU_RASR.
 *
 * Note: If size is not a power-of-two, it is rounded-up to the next
 * power-of-two value, and the returned SIZE field value corresponds
 * to that power-of-two value.
 */
static inline u32_t _size_to_mpu_rasr_size(u32_t size)
{
	/* The minimal supported region size is 32 bytes */
	if (size <= 32) {
		return REGION_32B;
	}

	/*
	 * A size value greater than 2^31 could not be handled by
	 * round_up_to_next_power_of_two() properly. We handle
	 * it separately here.
	 */
	if (size > (1UL << 31)) {
		return REGION_4G;
	}

	return ((32 - __builtin_clz(size - 1) - 2 + 1) << MPU_RASR_SIZE_Pos) &
		MPU_RASR_SIZE_Msk;
}

/**
 * Generate the value of the MPU Region Attribute and Size Register
 * (MPU_RASR) that corresponds to the supplied MPU region attributes.
 * This function is internal to the driver.
 */
static inline u32_t _get_region_attr(u32_t xn, u32_t ap, u32_t tex,
				     u32_t c, u32_t b, u32_t s,
				     u32_t srd, u32_t region_size)
{
	int size = _size_to_mpu_rasr_size(region_size);

	return (((xn << MPU_RASR_XN_Pos) & MPU_RASR_XN_Msk)
		| ((ap << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
		| ((tex << MPU_RASR_TEX_Pos) & MPU_RASR_TEX_Msk)
		| ((s << MPU_RASR_S_Pos) & MPU_RASR_S_Msk)
		| ((c << MPU_RASR_C_Pos) & MPU_RASR_C_Msk)
		| ((b << MPU_RASR_B_Pos) & MPU_RASR_B_Msk)
		| ((srd << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
		| (size));
}

/**
 * This internal function allocates default RAM cache-ability, share-ability,
 * and execution allowance attributes along with the requested access
 * permissions and size.
 */
static inline void _get_mpu_ram_region_attr(arm_mpu_region_attr_t *p_attr,
	u32_t ap, u32_t base, u32_t size)
{
	/* in ARMv7-M MPU the base address is not required
	 * to determine region attributes.
	 */
	(void) base;

	p_attr->rasr = _get_region_attr(1, ap, 1, 1, 1, 1, 0, size);
}

/**
 * This internal function is utilized by the MPU driver to combine a given
 * MPU RAM attribute configuration and region size and return the correct
 * parameter set.
 */
static inline void _get_ram_region_attr_by_conf(arm_mpu_region_attr_t *p_attr,
	u32_t attr, u32_t base, u32_t size)
{
	/* in ARMv7-M MPU the base address is not required
	 * to determine region attributes
	 */
	(void) base;

	p_attr->rasr = attr | _size_to_mpu_rasr_size(size);
}

/**
 * This internal function checks if region is enabled or not.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int _is_enabled_region(u32_t r_index)
{
	MPU->RNR = r_index;

	return MPU->RASR & MPU_RASR_ENABLE_Msk;
}

/**
 * This internal function checks if the given buffer is in the region.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int _is_in_region(u32_t r_index, u32_t start, u32_t size)
{
	u32_t r_addr_start;
	u32_t r_size_lshift;
	u32_t r_addr_end;

	MPU->RNR = r_index;
	r_addr_start = MPU->RBAR & MPU_RBAR_ADDR_Msk;
	r_size_lshift = ((MPU->RASR & MPU_RASR_SIZE_Msk) >>
			MPU_RASR_SIZE_Pos) + 1;
	r_addr_end = r_addr_start + (1UL << r_size_lshift) - 1;

	if (start >= r_addr_start && (start + size - 1) <= r_addr_end) {
		return 1;
	}

	return 0;
}

/**
 * This internal function returns the access permissions of an MPU region
 * specified by its region index.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline u32_t _get_region_ap(u32_t r_index)
{
	MPU->RNR = r_index;
	return (MPU->RASR & MPU_RASR_AP_Msk) >> MPU_RASR_AP_Pos;
}

/* Only a single bit is set for all user accessible permissions.
 * In ARMv7-M MPU this is bit AP[1].
 */
#define MPU_USER_READ_ACCESSIBLE_Msk (P_RW_U_RO & P_RW_U_RW & P_RO_U_RO & RO)

#if defined(CONFIG_USERSPACE)
/**
 * This internal function checks if the region is user accessible or not.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int _is_user_accessible_region(u32_t r_index, int write)
{
	u32_t r_ap = _get_region_ap(r_index);

	/* always return true if this is the thread stack region */
	if (_get_region_index_by_type(THREAD_STACK_REGION) == r_index) {
		return 1;
	}

	if (write) {
		return r_ap == P_RW_U_RW;
	}

	return r_ap & MPU_USER_READ_ACCESSIBLE_Msk;
}

/**
 * This internal function validates whether a given memory buffer
 * is user accessible or not.
 */
static inline int _mpu_buffer_validate(void *addr, size_t size, int write)
{
	s32_t r_index;

	/* Iterate all mpu regions in reversed order */
	for (r_index = _get_num_regions() - 1; r_index >= 0;  r_index--) {
		if (!_is_enabled_region(r_index) ||
		    !_is_in_region(r_index, (u32_t)addr, size)) {
			continue;
		}

		/* For ARM MPU, higher region number takes priority.
		 * Since we iterate all mpu regions in reversed order, so
		 * we can stop the iteration immediately once we find the
		 * matched region that grants permission or denies access.
		 */
		if (_is_user_accessible_region(r_index, write)) {
			return 0;
		} else {
			return -EPERM;
		}
	}

	return -EPERM;

}
#endif /* CONFIG_USERSPACE */

#endif /* USERSPACE || MPU_STACK_GUARD || APPLICATION_MEMORY */

#endif	/* ZEPHYR_ARCH_ARM_CORE_CORTEX_M_MPU_ARM_MPU_V7_INTERNAL_H_ */
