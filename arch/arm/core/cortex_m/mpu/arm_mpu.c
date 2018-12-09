/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arm/cortex_m/mpu/arm_core_mpu_dev.h>
#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>
#include <linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(mpu);

/*
 * Global status variable holding the number of HW MPU region indices, which
 * have been reserved by the MPU driver to program the static (fixed) memory
 * regions.
 */
static u8_t static_regions_num;

/**
 *  Get the number of supported MPU regions.
 */
static inline u8_t _get_num_regions(void)
{
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4)
	/* Cortex-M0+, Cortex-M3, and Cortex-M4 MCUs may
	 * have a fixed number of 8 MPU regions.
	 */
	return 8;
#else
	u32_t type = MPU->TYPE;

	type = (type & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

	return (u8_t)type;
#endif
}

/* Include architecture-specific internal headers. */
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M7)
#include <arm_mpu_v7_internal.h>
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33)
#include <arm_mpu_v8_internal.h>
#else
#error "Unsupported ARM CPU"
#endif

static int _region_allocate_and_init(const u8_t index,
	const struct arm_mpu_region *region_conf)
{
	/* Attempt to allocate new region index. */
	if (index > (_get_num_regions() - 1)) {

		/* No available MPU region index. */
		LOG_ERR("Failed to allocate new MPU region %u\n", index);
		return -EINVAL;
	}

	LOG_DBG("Program MPU region at index 0x%x", index);

	/* Program region */
	_region_init(index, region_conf);

	return index;
}

/* This internal function programs an MPU region
 * of a given configuration at a given MPU index.
 */
static int _mpu_configure_region(const u8_t index,
	const struct k_mem_partition *new_region)
{
	struct arm_mpu_region region_conf;

	LOG_DBG("Configure MPU region at index 0x%x", index);

	/* Populate internal ARM MPU region configuration structure. */
	region_conf.base = new_region->start;
	_get_region_attr_from_k_mem_partition_info(&region_conf.attr,
		&new_region->attr, new_region->start, new_region->size);

	/* Allocate and program region */
	return _region_allocate_and_init(index,
		(const struct arm_mpu_region *)&region_conf);
}

/* ARM Core MPU Driver API Implementation for ARM MPU */

/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void)
{
	/* Enable MPU and use the default memory map as a
	 * background region for privileged software access.
	 */
	MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	/* Force any outstanding transfers to complete before disabling MPU */
	__DMB();

	/* Disable MPU */
	MPU->CTRL = 0;
}

/**
 * @brief configure fixed (static) MPU regions.
 */
void arm_core_mpu_configure_static_mpu_regions(const struct k_mem_partition
	static_regions[], const u8_t regions_num,
	const u32_t background_area_start, const u32_t background_area_end)
{
	if (_mpu_configure_static_mpu_regions(static_regions, regions_num,
		background_area_start, background_area_end) == -EINVAL) {

		__ASSERT(0, "Configuring %u static MPU regions failed\n",
			regions_num);
	}
}

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
/**
 * @brief mark memory areas for dynamic region configuration
 */
void arm_core_mpu_mark_areas_for_dynamic_regions(
	const struct k_mem_partition dyn_region_areas[],
	const u8_t dyn_region_areas_num)
{
	if (_mpu_mark_areas_for_dynamic_regions(dyn_region_areas,
			dyn_region_areas_num) == -EINVAL) {

		__ASSERT(0, "Marking %u areas for dynamic regions failed\n",
			dyn_region_areas_num);
	}
}
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */


#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD) || \
	defined(CONFIG_APPLICATION_MEMORY) || defined(CONFIG_NOCACHE_MEMORY)

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and to fill-in the correct parameter
 * set to the provided attribute structure.
 *
 * @return 0 on Success, otherwise return -EINVAL.
 */
static inline int _get_region_attr_by_type(arm_mpu_region_attr_t *p_attr,
	u32_t type, u32_t base, u32_t size)
{
	switch (type) {
#ifdef CONFIG_NOCACHE_MEMORY
	case NOCACHE_MEMORY_REGION:
		_get_mpu_ram_nocache_region_attr(p_attr, P_RW_U_NA, base, size);
		return 0;
#endif
#ifdef CONFIG_USERSPACE
	case THREAD_STACK_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_RW, base, size);
		return 0;
#endif
#ifdef CONFIG_MPU_STACK_GUARD
	case THREAD_STACK_GUARD_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RO_U_NA, base, size);
		return 0;
#endif
#ifdef CONFIG_APPLICATION_MEMORY
	case THREAD_APP_DATA_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_RW, base, size);
		return 0;
#endif
#ifdef CONFIG_COVERAGE_GCOV
	case THREAD_GCOV_BSS_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_RW, base, size);
		return 0;
#endif
	default:
		/* Assert on MPU region types not supported in the
		 * implementation.  If asserts are disabled, the error
		 * can be tracked by the returned status (-EINVAL).
		 */
		__ASSERT(0,
			"Failed to derive attributes for MPU region type %u\n",
			type);
		/* Size 0 region */
		return -EINVAL;
	}
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct region index.
 */
static inline u32_t _get_region_index_by_type(u32_t type)
{
	u32_t region_index;

	__ASSERT(type < THREAD_MPU_REGION_LAST,
		 "unsupported region type");

	region_index = mpu_config.num_regions + type;

	__ASSERT(region_index < _get_num_regions(),
		 "out of MPU regions, requested %u max is %u",
		 region_index, _get_num_regions() - 1);

	return region_index;
}

/**
 * This internal function disables a given MPU region.
 */
static inline void _disable_region(u32_t r_index)
{
	/* Attempting to configure MPU_RNR with an invalid
	 * region number has unpredictable behavior. Therefore
	 * we add a check before disabling the requested MPU
	 * region.
	 */
	__ASSERT(r_index < _get_num_regions(),
		"Index 0x%x out-of-bound (supported regions: 0x%x)\n",
		r_index,
		_get_num_regions());
	LOG_DBG("disable region 0x%x", r_index);
	/* Disable region */
	ARM_MPU_ClrRegion(r_index);
}

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arm_core_mpu_configure(u8_t type, u32_t base, u32_t size)
{
	struct arm_mpu_region region_conf;

	LOG_DBG("Region info: 0x%x 0x%x", base, size);
	u32_t region_index = _get_region_index_by_type(type);

	if (_get_region_attr_by_type(&region_conf.attr, type, base, size)) {
		return;
	}
	region_conf.base = base;

	if (region_index >= _get_num_regions()) {
		return;
	}

	_region_init(region_index, &region_conf);
}

#if defined(CONFIG_USERSPACE)
void arm_core_mpu_configure_user_context(struct k_thread *thread)
{
	if (!thread->arch.priv_stack_start) {
		_disable_region(_get_region_index_by_type(
			THREAD_STACK_REGION));
		return;
	}

	u32_t base = (u32_t)thread->stack_obj;
	u32_t size = thread->stack_info.size;
#if !defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
	/* In user-mode the thread stack will include the (optional)
	 * guard area. For MPUs with arbitrary base address and limit
	 * it is essential to include this size increase, to avoid
	 * MPU faults.
	 */
	size += thread->stack_info.start - base;
#endif

	arm_core_mpu_configure(THREAD_STACK_REGION, base, size);
}

/**
 * @brief configure MPU regions for the memory partitions of the memory domain
 *
 * @param   mem_domain    memory domain that thread belongs to
 */
void arm_core_mpu_configure_mem_domain(struct k_mem_domain *mem_domain)
{
	u32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
	u32_t num_partitions;
	struct k_mem_partition *pparts;
	struct arm_mpu_region region_conf;

	if (mem_domain) {
		LOG_DBG("configure domain: %p", mem_domain);
		num_partitions = mem_domain->num_partitions;
		pparts = mem_domain->partitions;
	} else {
		LOG_DBG("disable domain partition regions");
		num_partitions = 0U;
		pparts = NULL;
	}

	for (; region_index < _get_num_regions(); region_index++) {
		if (num_partitions && pparts->size) {
			LOG_DBG("set region 0x%x 0x%x 0x%x",
				    region_index, pparts->start, pparts->size);
			region_conf.base = pparts->start;
			_get_ram_region_attr_by_conf(&region_conf.attr,
				&pparts->attr, pparts->start, pparts->size);
			_region_init(region_index, &region_conf);
			num_partitions--;
		} else {
			_disable_region(region_index);
		}
		pparts++;
	}
}

/**
 * @brief configure MPU region for a single memory partition
 *
 * @param   part_index  memory partition index
 * @param   part        memory partition info
 */
void arm_core_mpu_configure_mem_partition(u32_t part_index,
					  struct k_mem_partition *part)
{
	u32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
	struct arm_mpu_region region_conf;

	LOG_DBG("configure partition index: %u", part_index);

	if (part &&
		(region_index + part_index < _get_num_regions())) {
		LOG_DBG("set region 0x%x 0x%x 0x%x",
			    region_index + part_index, part->start, part->size);
		_get_ram_region_attr_by_conf(&region_conf.attr,
			&part->attr, part->start, part->size);
		region_conf.base = part->start;
		_region_init(region_index + part_index, &region_conf);
	} else {
		_disable_region(region_index + part_index);
	}
}

/**
 * @brief Reset MPU region for a single memory partition
 *
 * @param   part_index  memory partition index
 */
void arm_core_mpu_mem_partition_remove(u32_t part_index)
{
	u32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

	_disable_region(region_index + part_index);
}

/**
 * @brief get the maximum number of free regions for memory domain partitions
 */
int arm_core_mpu_get_max_domain_partition_regions(void)
{
	/*
	 * Subtract the start of domain partition regions from total regions
	 * will get the maximum number of free regions for memory domain
	 * partitions.
	 */
	return _get_num_regions() -
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
}

/**
 * @brief validate the given buffer is user accessible or not
 *
 * Presumes the background mapping is NOT user accessible.
 */
int arm_core_mpu_buffer_validate(void *addr, size_t size, int write)
{
	return _mpu_buffer_validate(addr, size, write);
}
#endif /* CONFIG_USERSPACE */
#endif /* USERSPACE || MPU_STACK_GUARD || APPLICATION_MEMORY || NOCACHE_MEMORY */

/* ARM MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static int arm_mpu_init(struct device *arg)
{
	u32_t r_index;

	if (mpu_config.num_regions > _get_num_regions()) {
		/* Attempt to configure more MPU regions than
		 * what is supported by hardware. As this operation
		 * is executed during system (pre-kernel) initialization,
		 * we want to ensure we can detect an attempt to
		 * perform invalid configuration.
		 */
		__ASSERT(0,
			"Request to configure: %u regions (supported: %u)\n",
			mpu_config.num_regions,
			_get_num_regions()
		);
		return -1;
	}

	LOG_DBG("total region count: %d", _get_num_regions());

	arm_core_mpu_disable();

	/* Architecture-specific configuration */
	_mpu_init();

	/* Program fixed regions configured at SOC definition. */
	for (r_index = 0U; r_index < mpu_config.num_regions; r_index++) {
		_region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

	/* Update the number of programmed MPU regions. */
	static_regions_num = mpu_config.num_regions;


	arm_core_mpu_enable();

	/* Sanity check for number of regions in Cortex-M0+, M3, and M4. */
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4)
	__ASSERT(
		(MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos == 8,
		"Invalid number of MPU regions\n");
#endif
	return 0;
}

SYS_INIT(arm_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
