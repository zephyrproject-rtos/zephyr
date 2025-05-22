/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_CORE_AARCH32_MPU_ARM_MPU_V7_INTERNAL_H_
#define ZEPHYR_ARCH_ARM_CORE_AARCH32_MPU_ARM_MPU_V7_INTERNAL_H_


#include <zephyr/sys/math_extras.h>
#include <arm_mpu_internal.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <zephyr/logging/log.h>

/* Global MPU configuration at system initialization. */
static void mpu_init(void)
{
	/* No specific configuration at init for ARMv7-M MPU. */
}

/* This internal function performs MPU region initialization.
 *
 * Note:
 *   The caller must provide a valid region index.
 */
static void region_init(const uint32_t index,
	const struct arm_mpu_region *region_conf)
{
	/* Select the region you want to access */
	set_region_number(index);

	/* Configure the region */
#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	/*
	 * Clear size register, which disables the entry.  It cannot be
	 * enabled as we reconfigure it.
	 */
	set_region_size(0);

	set_region_base_address(region_conf->base & MPU_RBAR_ADDR_Msk);
	set_region_attributes(region_conf->attr.rasr);
	set_region_size(region_conf->size | MPU_RASR_ENABLE_Msk);
#else
	MPU->RBAR = (region_conf->base & MPU_RBAR_ADDR_Msk)
				| MPU_RBAR_VALID_Msk | index;
	MPU->RASR = region_conf->attr.rasr | MPU_RASR_ENABLE_Msk;
	LOG_DBG("[%d] 0x%08x 0x%08x",
		index, region_conf->base, region_conf->attr.rasr);
#endif
}

/* @brief Partition sanity check
 *
 * This internal function performs run-time sanity check for
 * MPU region start address and size.
 *
 * @param part Pointer to the data structure holding the partition
 *             information (must be valid).
 */
static int mpu_partition_is_valid(const struct z_arm_mpu_partition *part)
{
	/* Partition size must be power-of-two,
	 * and greater or equal to the minimum
	 * MPU region size. Start address of the
	 * partition must align with size.
	 */
	int partition_is_valid =
		((part->size & (part->size - 1U)) == 0U)
		&&
		(part->size >= CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
		&&
		((part->start & (part->size - 1U)) == 0U);

	return partition_is_valid;
}

/**
 * This internal function converts the region size to
 * the SIZE field value of MPU_RASR.
 *
 * Note: If size is not a power-of-two, it is rounded-up to the next
 * power-of-two value, and the returned SIZE field value corresponds
 * to that power-of-two value.
 */
static inline uint32_t size_to_mpu_rasr_size(uint32_t size)
{
	/* The minimal supported region size is 32 bytes */
	if (size <= 32U) {
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

	return ((32 - __builtin_clz(size - 1U) - 2 + 1) << MPU_RASR_SIZE_Pos) &
		MPU_RASR_SIZE_Msk;
}

/**
 * This internal function is utilized by the MPU driver to combine a given
 * region attribute configuration and size and fill-in a driver-specific
 * structure with the correct MPU region configuration.
 */
static inline void get_region_attr_from_mpu_partition_info(
	arm_mpu_region_attr_t *p_attr,
	const k_mem_partition_attr_t *attr, uint32_t base, uint32_t size)
{
	/* in ARMv7-M MPU the base address is not required
	 * to determine region attributes
	 */
	(void) base;

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	(void) size;

	p_attr->rasr = attr->rasr_attr;
#else
	p_attr->rasr = attr->rasr_attr | size_to_mpu_rasr_size(size);
#endif
}

#if defined(CONFIG_USERSPACE)

/**
 * This internal function returns the minimum HW MPU region index
 * that may hold the configuration of a dynamic memory region.
 *
 * Trivial for ARMv7-M MPU, where dynamic memory areas are programmed
 * in MPU regions indices right after the static regions.
 */
static inline int get_dyn_region_min_index(void)
{
	return static_regions_num;
}

/* Only a single bit is set for all user accessible permissions.
 * In ARMv7-M MPU this is bit AP[1].
 */
#define MPU_USER_READ_ACCESSIBLE_Msk (P_RW_U_RO & P_RW_U_RW & P_RO_U_RO & RO)

/**
 * This internal function checks if the region is user accessible or not.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_user_accessible_region(uint32_t r_index, int write)
{
	uint32_t r_ap = get_region_ap(r_index);


	if (write != 0) {
		return r_ap == P_RW_U_RW;
	}

	return r_ap & MPU_USER_READ_ACCESSIBLE_Msk;
}

/**
 * This internal function validates whether a given memory buffer
 * is user accessible or not.
 */
static inline int mpu_buffer_validate(const void *addr, size_t size, int write)
{
	int32_t r_index;
	int rc = -EPERM;

	int key = arch_irq_lock();

	/* Iterate all mpu regions in reversed order */
	for (r_index = get_num_regions() - 1U; r_index >= 0;  r_index--) {
		if (!is_enabled_region(r_index) ||
		    !is_in_region(r_index, (uint32_t)addr, size)) {
			continue;
		}

		/* For ARM MPU, higher region number takes priority.
		 * Since we iterate all mpu regions in reversed order, so
		 * we can stop the iteration immediately once we find the
		 * matched region that grants permission or denies access.
		 */
		if (is_user_accessible_region(r_index, write)) {
			rc = 0;
		} else {
			rc = -EPERM;
		}
		break;
	}

	arch_irq_unlock(key);
	return rc;
}

#endif /* CONFIG_USERSPACE */

static int mpu_configure_region(const uint8_t index,
	const struct z_arm_mpu_partition *new_region);

static int mpu_configure_regions(const struct z_arm_mpu_partition
	regions[], uint8_t regions_num, uint8_t start_reg_index,
	bool do_sanity_check);

/* This internal function programs the static MPU regions.
 *
 * It returns the number of MPU region indices configured.
 *
 * Note:
 * If the static MPU regions configuration has not been successfully
 * performed, the error signal is propagated to the caller of the function.
 */
static int mpu_configure_static_mpu_regions(const struct z_arm_mpu_partition
	static_regions[], const uint8_t regions_num,
	const uint32_t background_area_base,
	const uint32_t background_area_end)
{
	int mpu_reg_index = static_regions_num;

	/* In ARMv7-M architecture the static regions are
	 * programmed on top of SRAM region configuration.
	 */
	ARG_UNUSED(background_area_base);
	ARG_UNUSED(background_area_end);

	mpu_reg_index = mpu_configure_regions(static_regions,
		regions_num, mpu_reg_index, true);

	static_regions_num = mpu_reg_index;

	return mpu_reg_index;
}

/* This internal function programs the dynamic MPU regions.
 *
 * It returns the number of MPU region indices configured.
 *
 * Note:
 * If the dynamic MPU regions configuration has not been successfully
 * performed, the error signal is propagated to the caller of the function.
 */
static int mpu_configure_dynamic_mpu_regions(const struct z_arm_mpu_partition
	dynamic_regions[], uint8_t regions_num)
{
	int mpu_reg_index = static_regions_num;

	/* In ARMv7-M architecture the dynamic regions are
	 * programmed on top of existing SRAM region configuration.
	 */

	mpu_reg_index = mpu_configure_regions(dynamic_regions,
		regions_num, mpu_reg_index, false);

	if (mpu_reg_index != -EINVAL) {

		/* Disable the non-programmed MPU regions. */
		for (int i = mpu_reg_index; i < get_num_regions(); i++) {
			ARM_MPU_ClrRegion(i);
		}
	}

	return mpu_reg_index;
}

static inline void mpu_clear_region(uint32_t rnr)
{
	ARM_MPU_ClrRegion(rnr);
}

#endif	/* ZEPHYR_ARCH_ARM_CORE_AARCH32_MPU_ARM_MPU_V7_INTERNAL_H_ */
