/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_CORE_AARCH32_MPU_ARM_MPU_V8_INTERNAL_H_
#define ZEPHYR_ARCH_ARM_CORE_AARCH32_MPU_ARM_MPU_V8_INTERNAL_H_

#include <cortex_m/cmse.h>
#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/barrier.h>

/**
 * @brief internal structure holding information of
 *        memory areas where dynamic MPU programming
 *        is allowed.
 */
struct dynamic_region_info {
	int index;
	struct arm_mpu_region region_conf;
};

/**
 * Global array, holding the MPU region index of
 * the memory region inside which dynamic memory
 * regions may be configured.
 */
static struct dynamic_region_info dyn_reg_info[MPU_DYNAMIC_REGION_AREAS_NUM];
#if defined(CONFIG_CPU_CORTEX_M23) || defined(CONFIG_CPU_CORTEX_M33) || \
	defined(CONFIG_CPU_CORTEX_M55)
static inline void mpu_set_mair0(uint32_t mair0)
{
	MPU->MAIR0 = mair0;
}

static inline void mpu_set_rnr(uint32_t rnr)
{
	MPU->RNR = rnr;
}

static inline void mpu_set_rbar(uint32_t rbar)
{
	MPU->RBAR = rbar;
}

static inline uint32_t mpu_get_rbar(void)
{
	return MPU->RBAR;
}

static inline void mpu_set_rlar(uint32_t rlar)
{
	MPU->RLAR = rlar;
}

static inline uint32_t mpu_get_rlar(void)
{
	return MPU->RLAR;
}

static inline uint8_t mpu_get_num_regions(void)
{
	uint32_t type = MPU->TYPE;

	type = (type & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

	return (uint8_t)type;
}

static inline void mpu_clear_region(uint32_t rnr)
{
	ARM_MPU_ClrRegion(rnr);
}

#elif defined(CONFIG_AARCH32_ARMV8_R)
static inline void mpu_set_mair0(uint32_t mair0)
{
	write_mair0(mair0);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static inline void mpu_set_rnr(uint32_t rnr)
{
	write_prselr(rnr);
	barrier_dsync_fence_full();
}

static inline void mpu_set_rbar(uint32_t rbar)
{
	write_prbar(rbar);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static inline uint32_t mpu_get_rbar(void)
{
	return read_prbar();
}

static inline void mpu_set_rlar(uint32_t rlar)
{
	write_prlar(rlar);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static inline uint32_t mpu_get_rlar(void)
{
	return read_prlar();
}

static inline uint8_t mpu_get_num_regions(void)
{
	uint32_t type = read_mpuir();

	type = (type >> MPU_IR_REGION_Pos) & MPU_IR_REGION_Msk;

	return (uint8_t)type;
}

static inline void mpu_clear_region(uint32_t rnr)
{
	mpu_set_rnr(rnr);
	mpu_set_rbar(0);
	mpu_set_rlar(0);
}

#else
#error "Unsupported ARM CPU"
#endif

/* Global MPU configuration at system initialization. */
static void mpu_init(void)
{
	/* Configure the cache-ability attributes for all the
	 * different types of memory regions.
	 */
	mpu_set_mair0(MPU_MAIR_ATTRS);
}

static void mpu_set_region(uint32_t rnr, uint32_t rbar, uint32_t rlar)
{
	mpu_set_rnr(rnr);
	mpu_set_rbar(rbar);
	mpu_set_rlar(rlar);
}

/* This internal function performs MPU region initialization.
 *
 * Note:
 *   The caller must provide a valid region index.
 */
static void region_init(const uint32_t index,
	const struct arm_mpu_region *region_conf)
{
	mpu_set_region(
		/* RNR */
		index,
		/* RBAR */
		(region_conf->base & MPU_RBAR_BASE_Msk)
		| (region_conf->attr.rbar &
			(MPU_RBAR_XN_Msk | MPU_RBAR_AP_Msk | MPU_RBAR_SH_Msk)),
		/* RLAR */
		(region_conf->attr.r_limit & MPU_RLAR_LIMIT_Msk)
		| ((region_conf->attr.mair_idx << MPU_RLAR_AttrIndx_Pos)
			& MPU_RLAR_AttrIndx_Msk)
		| MPU_RLAR_EN_Msk
	);

	LOG_DBG("[%d] 0x%08x 0x%08x 0x%08x 0x%08x",
			index, region_conf->base, region_conf->attr.rbar,
			region_conf->attr.mair_idx, region_conf->attr.r_limit);
}

/* @brief Partition sanity check
 *
 * This internal function performs run-time sanity check for
 * MPU region start address and size.
 *
 * @param part Pointer to the data structure holding the partition
 *             information (must be valid).
 * */
static int mpu_partition_is_valid(const struct z_arm_mpu_partition *part)
{
	/* Partition size must be a multiple of the minimum MPU region
	 * size. Start address of the partition must align with the
	 * minimum MPU region size.
	 */
	int partition_is_valid =
		(part->size >= CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
		&&
		((part->size &
			(~(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1)))
			== part->size)
		&&
		((part->start &
			(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1)) == 0U);

	return partition_is_valid;
}

/**
 * This internal function returns the MPU region, in which a
 * buffer, specified by its start address and size, lies. If
 * a valid MPU region cannot be derived the function returns
 * -EINVAL.
 *
 * Note that, for the function to work properly, the ARM MPU
 * needs to be enabled.
 *
 */
#if defined(CONFIG_AARCH32_ARMV8_R)
static inline int get_region_index(uint32_t start, uint32_t size)
{
	uint32_t limit = (start + size - 1) & MPU_RLAR_LIMIT_Msk;

	for (uint8_t idx = 0; idx < mpu_get_num_regions(); idx++) {
		mpu_set_rnr(idx);
		if (start >= (mpu_get_rbar() & MPU_RBAR_BASE_Msk) &&
		    limit <= (mpu_get_rlar() & MPU_RLAR_LIMIT_Msk)) {
			return idx;
		}
	}
	return -EINVAL;
}
#else
static inline int get_region_index(uint32_t start, uint32_t size)
{
	uint32_t region_start_addr = arm_cmse_mpu_region_get(start);
	uint32_t region_end_addr = arm_cmse_mpu_region_get(start + size - 1);

	/* MPU regions are contiguous so return the region number,
	 * if both start and end address are in the same region.
	 */
	if (region_start_addr == region_end_addr) {
		return region_start_addr;
	}
	return -EINVAL;
}
#endif

static inline uint32_t mpu_region_get_base(const uint32_t index)
{
	mpu_set_rnr(index);
	return mpu_get_rbar() & MPU_RBAR_BASE_Msk;
}

static inline void mpu_region_set_base(const uint32_t index, const uint32_t base)
{
	mpu_set_rnr(index);
	mpu_set_rbar((mpu_get_rbar() & (~MPU_RBAR_BASE_Msk))
		     | (base & MPU_RBAR_BASE_Msk));
}

static inline uint32_t mpu_region_get_last_addr(const uint32_t index)
{
	mpu_set_rnr(index);
	return (mpu_get_rlar() & MPU_RLAR_LIMIT_Msk) | (~MPU_RLAR_LIMIT_Msk);
}

static inline void mpu_region_set_limit(const uint32_t index, const uint32_t limit)
{
	mpu_set_rnr(index);
	mpu_set_rlar((mpu_get_rlar() & (~MPU_RLAR_LIMIT_Msk))
		     | (limit & MPU_RLAR_LIMIT_Msk));
}

static inline void mpu_region_get_access_attr(const uint32_t index,
	arm_mpu_region_attr_t *attr)
{
	mpu_set_rnr(index);

	attr->rbar = mpu_get_rbar() &
		(MPU_RBAR_XN_Msk | MPU_RBAR_AP_Msk | MPU_RBAR_SH_Msk);
	attr->mair_idx = (mpu_get_rlar() & MPU_RLAR_AttrIndx_Msk) >>
		MPU_RLAR_AttrIndx_Pos;
}

static inline void mpu_region_get_conf(const uint32_t index,
	struct arm_mpu_region *region_conf)
{
	mpu_set_rnr(index);

	/* Region attribution:
	 * - Cache-ability
	 * - Share-ability
	 * - Access Permissions
	 */
	mpu_region_get_access_attr(index, &region_conf->attr);

	/* Region base address */
	region_conf->base = mpu_get_rbar() & MPU_RBAR_BASE_Msk;

	/* Region limit address */
	region_conf->attr.r_limit = mpu_get_rlar() & MPU_RLAR_LIMIT_Msk;
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
	p_attr->rbar = attr->rbar &
		(MPU_RBAR_XN_Msk | MPU_RBAR_AP_Msk | MPU_RBAR_SH_Msk);
	p_attr->mair_idx = attr->mair_idx;
	p_attr->r_limit = REGION_LIMIT_ADDR(base, size);
}

#if defined(CONFIG_USERSPACE)

/**
 * This internal function returns the minimum HW MPU region index
 * that may hold the configuration of a dynamic memory region.
 *
 * Browse through the memory areas marked for dynamic MPU programming,
 * pick the one with the minimum MPU region index. Return that index.
 *
 * The function is optimized for the (most common) use-case of a single
 * marked area for dynamic memory regions.
 */
static inline int get_dyn_region_min_index(void)
{
	int dyn_reg_min_index = dyn_reg_info[0].index;
#if MPU_DYNAMIC_REGION_AREAS_NUM > 1
	for (int i = 1; i < MPU_DYNAMIC_REGION_AREAS_NUM; i++) {
		if ((dyn_reg_info[i].index != -EINVAL) &&
			(dyn_reg_info[i].index < dyn_reg_min_index)
		) {
			dyn_reg_min_index = dyn_reg_info[i].index;
		}
	}
#endif
	return dyn_reg_min_index;
}

static inline uint32_t mpu_region_get_size(uint32_t index)
{
	return mpu_region_get_last_addr(index) + 1
		- mpu_region_get_base(index);
}

/**
 * This internal function checks if region is enabled or not.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_enabled_region(uint32_t index)
{
	mpu_set_rnr(index);

	return (mpu_get_rlar() & MPU_RLAR_EN_Msk) ? 1 : 0;
}

#if defined(CONFIG_AARCH32_ARMV8_R)
/**
 * This internal function checks if the given buffer is in the region.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_in_region(uint32_t rnr, uint32_t start, uint32_t size)
{
	uint32_t r_addr_start;
	uint32_t r_addr_end;
	uint32_t end;

	r_addr_start = mpu_region_get_base(rnr);
	r_addr_end = mpu_region_get_last_addr(rnr);

	size = size == 0U ? 0U : size - 1U;
	if (u32_add_overflow(start, size, &end)) {
		return 0;
	}

	if ((start >= r_addr_start) && (end <= r_addr_end)) {
		return 1;
	}

	return 0;
}

static inline int is_user_accessible_region(uint32_t rnr, int write)
{
	uint32_t r_ap;

	mpu_set_rnr(rnr);

	r_ap = (mpu_get_rbar() & MPU_RBAR_AP_Msk) >> MPU_RBAR_AP_Pos;

	if (write != 0) {
		return r_ap == P_RW_U_RW;
	}

	return ((r_ap == P_RW_U_RW) ||  (r_ap == P_RO_U_RO));
}

/**
 * This internal function validates whether a given memory buffer
 * is user accessible or not.
 */
static inline int mpu_buffer_validate(void *addr, size_t size, int write)
{
	int32_t rnr;
	int rc = -EPERM;

	int key = arch_irq_lock();

	/* Iterate all mpu regions in reversed order */
	for (rnr = 0; rnr < mpu_get_num_regions(); rnr++) {
		if (!is_enabled_region(rnr) ||
		    !is_in_region(rnr, (uint32_t)addr, size)) {
			continue;
		}

		if (is_user_accessible_region(rnr, write)) {
			rc = 0;
		}
	}

	arch_irq_unlock(key);
	return rc;
}

#else
/**
 * This internal function validates whether a given memory buffer
 * is user accessible or not.
 *
 * Note: [Doc. number: ARM-ECM-0359818]
 * "Some SAU, IDAU, and MPU configurations block the efficient implementation
 * of an address range check. The CMSE intrinsic operates under the assumption
 * that the configuration of the SAU, IDAU, and MPU is constrained as follows:
 * - An object is allocated in a single MPU/SAU/IDAU region.
 * - A stack is allocated in a single region.
 *
 * These points imply that the memory buffer does not span across multiple MPU,
 * SAU, or IDAU regions."
 *
 * MPU regions are configurable, however, some platforms might have fixed-size
 * SAU or IDAU regions. So, even if a buffer is allocated inside a single MPU
 * region, it might span across multiple SAU/IDAU regions, which will make the
 * TT-based address range check fail.
 *
 * Therefore, the function performs a second check, which is based on MPU only,
 * in case the fast address range check fails.
 *
 */
static inline int mpu_buffer_validate(void *addr, size_t size, int write)
{
	uint32_t _addr = (uint32_t)addr;
	uint32_t _size = (uint32_t)size;

	if (write) {
		if (arm_cmse_addr_range_readwrite_ok(_addr, _size, 1)) {
			return 0;
		}
	} else {
		if (arm_cmse_addr_range_read_ok(_addr, _size, 1)) {
			return 0;
		}
	}

#if defined(CONFIG_CPU_HAS_TEE)
	/*
	 * Validation failure may be due to SAU/IDAU presence.
	 * We re-check user accessibility based on MPU only.
	 */
	int32_t r_index_base = arm_cmse_mpu_region_get(_addr);
	int32_t r_index_last = arm_cmse_mpu_region_get(_addr + _size - 1);

	if ((r_index_base != -EINVAL) && (r_index_base == r_index_last)) {
		/* Valid MPU region, check permissions on base address only. */
		if (write) {
			if (arm_cmse_addr_readwrite_ok(_addr, 1)) {
				return 0;
			}
		} else {
			if (arm_cmse_addr_read_ok(_addr, 1)) {
				return 0;
			}
		}
	}
#endif /* CONFIG_CPU_HAS_TEE */
	return -EPERM;
}
#endif /* CONFIG_AARCH32_ARMV8_R */

#endif /* CONFIG_USERSPACE */

static int region_allocate_and_init(const uint8_t index,
	const struct arm_mpu_region *region_conf);

static int mpu_configure_region(const uint8_t index,
	const struct z_arm_mpu_partition *new_region);

#if !defined(CONFIG_MPU_GAP_FILLING)
static int mpu_configure_regions(const struct z_arm_mpu_partition
	regions[], uint8_t regions_num, uint8_t start_reg_index,
	bool do_sanity_check);
#endif

/* This internal function programs a set of given MPU regions
 * over a background memory area, optionally performing a
 * sanity check of the memory regions to be programmed.
 *
 * The function performs a full partition of the background memory
 * area, effectively, leaving no space in this area uncovered by MPU.
 */
static int mpu_configure_regions_and_partition(const struct z_arm_mpu_partition
	regions[], uint8_t regions_num, uint8_t start_reg_index,
	bool do_sanity_check)
{
	int i;
	int reg_index = start_reg_index;

	for (i = 0; i < regions_num; i++) {
		if (regions[i].size == 0U) {
			continue;
		}
		/* Non-empty region. */

		if (do_sanity_check &&
			(!mpu_partition_is_valid(&regions[i]))) {
			LOG_ERR("Partition %u: sanity check failed.", i);
			return -EINVAL;
		}

		/* Derive the index of the underlying MPU region,
		 * inside which the new region will be configured.
		 */
		int u_reg_index =
			get_region_index(regions[i].start, regions[i].size);

		if ((u_reg_index == -EINVAL) ||
			(u_reg_index > (reg_index - 1))) {
			LOG_ERR("Invalid underlying region index %u",
				u_reg_index);
			return -EINVAL;
		}

		/*
		 * The new memory region is to be placed inside the underlying
		 * region, possibly splitting the underlying region into two.
		 */
		uint32_t u_reg_base = mpu_region_get_base(u_reg_index);
		uint32_t u_reg_last = mpu_region_get_last_addr(u_reg_index);
		uint32_t reg_last = regions[i].start + regions[i].size - 1;

		if ((regions[i].start == u_reg_base) &&
			(reg_last == u_reg_last)) {
			/* The new region overlaps entirely with the
			 * underlying region. In this case we simply
			 * update the partition attributes of the
			 * underlying region with those of the new
			 * region.
			 */
			mpu_configure_region(u_reg_index, &regions[i]);
		} else if (regions[i].start == u_reg_base) {
			/* The new region starts exactly at the start of the
			 * underlying region; the start of the underlying
			 * region needs to be set to the end of the new region.
			 */
			mpu_region_set_base(u_reg_index,
				regions[i].start + regions[i].size);

			reg_index =
				mpu_configure_region(reg_index, &regions[i]);

			if (reg_index == -EINVAL) {
				return reg_index;
			}

			reg_index++;
		} else if (reg_last == u_reg_last) {
			/* The new region ends exactly at the end of the
			 * underlying region; the end of the underlying
			 * region needs to be set to the start of the
			 * new region.
			 */
			mpu_region_set_limit(u_reg_index,
				regions[i].start - 1);

			reg_index =
				mpu_configure_region(reg_index, &regions[i]);

			if (reg_index == -EINVAL) {
				return reg_index;
			}

			reg_index++;
		} else {
			/* The new regions lies strictly inside the
			 * underlying region, which needs to split
			 * into two regions.
			 */
			mpu_region_set_limit(u_reg_index,
				regions[i].start - 1);

			reg_index =
				mpu_configure_region(reg_index, &regions[i]);

			if (reg_index == -EINVAL) {
				return reg_index;
			}
			reg_index++;

			/* The additional region shall have the same
			 * access attributes as the initial underlying
			 * region.
			 */
			struct arm_mpu_region fill_region;

			mpu_region_get_access_attr(u_reg_index,
				&fill_region.attr);
			fill_region.base = regions[i].start +
				regions[i].size;
			fill_region.attr.r_limit =
			REGION_LIMIT_ADDR((regions[i].start +
				regions[i].size), (u_reg_last - reg_last));

			reg_index =
				region_allocate_and_init(reg_index,
					(const struct arm_mpu_region *)
						&fill_region);

			if (reg_index == -EINVAL) {
				return reg_index;
			}

			reg_index++;
		}
	}

	return reg_index;
}

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

	/* In ARMv8-M architecture the static regions are programmed on SRAM,
	 * forming a full partition of the background area, specified by the
	 * given boundaries.
	 */
	ARG_UNUSED(background_area_base);
	ARG_UNUSED(background_area_end);

	mpu_reg_index = mpu_configure_regions_and_partition(static_regions,
		regions_num, mpu_reg_index, true);

	static_regions_num = mpu_reg_index;

	return mpu_reg_index;
}

/* This internal function marks and stores the configuration of memory areas
 * where dynamic region programming is allowed. Return zero on success, or
 * -EINVAL on error.
 */
static int mpu_mark_areas_for_dynamic_regions(
		const struct z_arm_mpu_partition dyn_region_areas[],
		const uint8_t dyn_region_areas_num)
{
	/* In ARMv8-M architecture we need to store the index values
	 * and the default configuration of the MPU regions, inside
	 * which dynamic memory regions may be programmed at run-time.
	 */
	for (int i = 0; i < dyn_region_areas_num; i++) {
		if (dyn_region_areas[i].size == 0U) {
			continue;
		}
		/* Non-empty area */

		/* Retrieve HW MPU region index */
		dyn_reg_info[i].index =
			get_region_index(dyn_region_areas[i].start,
					dyn_region_areas[i].size);

		if (dyn_reg_info[i].index == -EINVAL) {

			return -EINVAL;
		}

		if (dyn_reg_info[i].index >= static_regions_num) {

			return -EINVAL;
		}

		/* Store default configuration */
		mpu_region_get_conf(dyn_reg_info[i].index,
			&dyn_reg_info[i].region_conf);
	}

	return 0;
}

/**
 *  Get the number of supported MPU regions.
 */
static inline uint8_t get_num_regions(void)
{
#if defined(NUM_MPU_REGIONS)
	/* Retrieve the number of regions from DTS configuration. */
	return NUM_MPU_REGIONS;
#else
	return mpu_get_num_regions();
#endif /* NUM_MPU_REGIONS */
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

	/* Disable all MPU regions except for the static ones. */
	for (int i = mpu_reg_index; i < get_num_regions(); i++) {
		mpu_clear_region(i);
	}

#if defined(CONFIG_MPU_GAP_FILLING)
	/* Reset MPU regions inside which dynamic memory regions may
	 * be programmed.
	 */
	for (int i = 0; i < MPU_DYNAMIC_REGION_AREAS_NUM; i++) {
		region_init(dyn_reg_info[i].index,
			&dyn_reg_info[i].region_conf);
	}

	/* In ARMv8-M architecture the dynamic regions are programmed on SRAM,
	 * forming a full partition of the background area, specified by the
	 * given boundaries.
	 */
	mpu_reg_index = mpu_configure_regions_and_partition(dynamic_regions,
		regions_num, mpu_reg_index, true);
#else

	/* We are going to skip the full partition of the background areas.
	 * So we can disable MPU regions inside which dynamic memory regions
	 * may be programmed.
	 */
	for (int i = 0; i < MPU_DYNAMIC_REGION_AREAS_NUM; i++) {
		mpu_clear_region(dyn_reg_info[i].index);
	}

	/* The dynamic regions are now programmed on top of
	 * existing SRAM region configuration.
	 */
	mpu_reg_index = mpu_configure_regions(dynamic_regions,
		regions_num, mpu_reg_index, true);

#endif /* CONFIG_MPU_GAP_FILLING */
	return mpu_reg_index;
}

#endif	/* ZEPHYR_ARCH_ARM_CORE_AARCH32_MPU_ARM_MPU_V8_INTERNAL_H_ */
