/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>
#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>
#include <linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(mpu);

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

#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD) || \
	defined(CONFIG_APPLICATION_MEMORY) || \
	defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)

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
#ifdef CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS
#ifdef CONFIG_APP_SHARED_MEM
	case APP_SHARED_MEM_BKGND_REGION:
		/* Intentional fallthrough */
#endif
	case KERNEL_BKGND_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_NA, base, size);
		return 0;
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
#ifdef CONFIG_USERSPACE
	case THREAD_STACK_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_RW, base, size);
		return 0;
#ifdef CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS
	case KERNEL_BKGND_THREAD_STACK_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_NA, base, size);
		return 0;
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
#endif
#ifdef CONFIG_MPU_STACK_GUARD
	case THREAD_STACK_GUARD_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RO_U_NA, base, size);
		return 0;
#ifdef CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS
	case KERNEL_BKGND_STACK_GUARD_REGION:
		_get_mpu_ram_region_attr(p_attr, P_RW_U_NA, base, size);
		return 0;
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
#endif
#ifdef CONFIG_APPLICATION_MEMORY
	case THREAD_APP_DATA_REGION:
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
 * This internal function programs the MPU region of the given type,
 * base address, and size.
 */
static void _mpu_configure_by_type(u8_t type, u32_t base, u32_t size)
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

#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD) || \
	defined(CONFIG_APPLICATION_MEMORY)

/**
 * This internal function disables a given MPU region.
 *
 * @param r_index MPU region index
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

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
/**
 * This internal function disables the MPU region of the given type
 *
 * @param   type    MPU region type
 */
static void _disable_region_by_type(u8_t type)
{
	LOG_DBG("disable region type ox%x", type);
	u32_t region_index = _get_region_index_by_type(type);
	if (region_index >= _get_num_regions()) {
		return;
	}
	_disable_region(region_index);
}

#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD)
/**
 * This internal function determines the background region
 * type corresponding to the requested MPU region type.
 */
static inline int _get_background_region_type(u8_t type)
{
	switch (type) {
#if defined(CONFIG_USERSPACE)
	case THREAD_STACK_REGION:
		return KERNEL_BKGND_THREAD_STACK_REGION;
#endif
#if defined(CONFIG_MPU_STACK_GUARD)
	case THREAD_STACK_GUARD_REGION:
		return KERNEL_BKGND_STACK_GUARD_REGION;
#endif
	default:
		__ASSERT(0,
			"Failed to get background type for region type %u\n",
			type);
		return -EINVAL;
	}
}

#endif /* CONFIG_USERSPACE || CONFIG_MPU_STACK_GUARD */

/**
 * @brief insert a new MPU region, specified with the requested base and size,
 *        partitioning the underlying MPU region.
 *
 * This function configures a requested MPU region, specified by its base
 * address and region size, over an existing (and currently active) SRAM
 * region, setting up a partition, so the requested MPU region is not
 * overlapping with any other MPU regions. To implement the partitioning
 * an additional background SRAM region may be required to be configured.
 *
 * @param   type            MPU region type
 * @param   background_type Type of the additional MPU background region
 *                          to be configured
 * @param   base            base address in RAM
 * @param   size            size of the region
 */
static void _mpu_configure_and_partition(u8_t type, u8_t background_type,
	u32_t start, u32_t size)
{
	/* Obtain the MPU region index of the (background) SRAM region,
	 * within which we are inserting an MPU region with the given
	 * <type, start, size> configuration.
	 */
	int current_region_index = _get_region_index(start, size);
	u32_t current_region_base =
		_get_region_base_addr(current_region_index);
	u32_t current_region_end = _get_region_end_addr(current_region_index);

	arm_core_mpu_disable();

	/* Adjust the borders of the current background region and (possibly)
	 * configure an additional background region to maintain the proper
	 * access permissions in the kernel ram area above and below the
	 * region we are inserting.
	 */
	if (current_region_base == start) {
		/* The current background region index starts exactly at
		 * the start or the requested region. Disable the current
		 * (background) region.
		 */
		ARM_MPU_ClrRegion(current_region_index);
	} else {
		/* Otherwise, adjust the end of the current background region
		 * to the boundary of the requested region.
		 */
		_set_region_end_addr(current_region_index, start - 1);

	}
	if (current_region_end == (start + size - 1)) {
		/* The current background region ends exactly at the end of the,
		 * requested region. Do not configure the additional background
		 * region.
		 */
	} else {
		/* Otherwise, adjust the additional background region to end
		 * at the end of the current background region and to start at
		 * the end of the requested region.
		 */
		_mpu_configure_by_type(background_type,
			start + size,
			current_region_end + 1 - (start + size));
	}

	/* Configure the requested region */
	_mpu_configure_by_type(type, start, size);

}

#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arm_core_mpu_configure(u8_t type, u32_t base, u32_t size)
{
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
	/* Determine the kernel background region type, corresponding
	 * to the type of the region we are configuring.
	 */
	int background_type = _get_background_region_type(type);

	if (background_type == -EINVAL) {
		return;
	}
	/* Program the new region by partitioning the underlying ram area */
	_mpu_configure_and_partition(type, (u8_t)background_type, base, size);
#else
	_mpu_configure_by_type(type, base, size);
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
}

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
void arm_core_mpu_kernel_ram_region_reset(void)
{
	_mpu_configure_by_type(KERNEL_BKGND_REGION,
		(u32_t)&__kernel_ram_start,
		(u32_t)&__kernel_ram_end - (u32_t)&__kernel_ram_start);
#if defined(CONFIG_MPU_STACK_GUARD)
	_disable_region_by_type(THREAD_STACK_GUARD_REGION);
	_disable_region_by_type(KERNEL_BKGND_STACK_GUARD_REGION);
#endif
#if defined(CONFIG_USERSPACE)
	_disable_region_by_type(THREAD_STACK_REGION);
	_disable_region_by_type(KERNEL_BKGND_THREAD_STACK_REGION);
#endif
}
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

#if defined(CONFIG_USERSPACE)
void arm_core_mpu_configure_user_context(struct k_thread *thread)
{
	if (!thread->arch.priv_stack_start) {
		/* If this is a supervisor thread,
		 * disable thread stack MPU region.
		 */
#if !defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
		/* For ARMv8-M MPU with requirement for non-overlapping regions
		 * Kernel SRAM access permissions are reset in context-switch,
		 * so this region has already been disabled.
		 */
		_disable_region(_get_region_index_by_type(
			THREAD_STACK_REGION));
#endif
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
	size += thread->stack_info.start - thread->stack_obj;
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
		num_partitions = 0;
		pparts = NULL;
	}

	for (; region_index < _get_num_regions(); region_index++) {
		if (num_partitions && pparts->size) {
			LOG_DBG("set region 0x%x 0x%x 0x%x",
				    region_index, pparts->start, pparts->size);
			region_conf.base = pparts->start;
			_get_ram_region_attr_by_conf(&region_conf.attr,
				pparts->attr,	pparts->start, pparts->size);
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
			part->attr, part->start, part->size);
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
#endif /* USERSPACE || MPU_STACK_GUARD || APPLICATION_MEMORY */
#endif /* USERSPACE || MPU_STACK_GUARD || APPLICATION_MEMORY ||
	    * MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

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

	/* Configure regions */
	for (r_index = 0; r_index < mpu_config.num_regions; r_index++) {
		_region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

#if defined(CONFIG_APPLICATION_MEMORY)
	/* configure app data portion */
	_mpu_configure_by_type(THREAD_APP_DATA_REGION,
		(u32_t)&__app_ram_start,
		(u32_t)&__app_ram_end - (u32_t)&__app_ram_start);
#endif

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
	/* Set access permissions for Kernel SRAM at init. */
	_mpu_configure_by_type(KERNEL_BKGND_REGION,
		(u32_t)&__kernel_ram_start,
		(u32_t)&__kernel_ram_end - (u32_t)&__kernel_ram_start);
#if defined(CONFIG_APP_SHARED_MEM)
	/* Set access permissions for Application shared memory at init. */
	_mpu_configure_by_type(APP_SHARED_MEM_BKGND_REGION,
		(u32_t)&_app_smem_start,
		(u32_t)&_app_smem_end - (u32_t)&_app_smem_start);
#endif /* CONFIG_APP_SHARED_MEM */
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

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
