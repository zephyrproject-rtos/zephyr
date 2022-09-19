/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include "arm_core_mpu_dev.h"
#include <zephyr/linker/linker-defs.h>
#include <kernel_arch_data.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mpu);

#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
/* The order here is on purpose since ARMv8-M SoCs may define
 * CONFIG_ARMV6_M_ARMV8_M_BASELINE or CONFIG_ARMV7_M_ARMV8_M_MAINLINE
 * so we want to check for ARMv8-M first.
 */
#define MPU_NODEID DT_INST(0, arm_armv8m_mpu)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
#define MPU_NODEID DT_INST(0, arm_armv7m_mpu)
#elif defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
#define MPU_NODEID DT_INST(0, arm_armv6m_mpu)
#endif

#if DT_NODE_HAS_PROP(MPU_NODEID, arm_num_mpu_regions)
#define NUM_MPU_REGIONS   DT_PROP(MPU_NODEID, arm_num_mpu_regions)
#endif

/*
 * Global status variable holding the number of HW MPU region indices, which
 * have been reserved by the MPU driver to program the static (fixed) memory
 * regions.
 */
static uint8_t static_regions_num;

/* Include architecture-specific internal headers. */
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M7) || \
	defined(CONFIG_ARMV7_R)
#include "arm_mpu_v7_internal.h"
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33) || \
	defined(CONFIG_CPU_CORTEX_M55) || \
	defined(CONFIG_AARCH32_ARMV8_R)
#include "arm_mpu_v8_internal.h"
#else
#error "Unsupported ARM CPU"
#endif

static int region_allocate_and_init(const uint8_t index,
	const struct arm_mpu_region *region_conf)
{
	/* Attempt to allocate new region index. */
	if (index > (get_num_regions() - 1U)) {

		/* No available MPU region index. */
		LOG_ERR("Failed to allocate new MPU region %u\n", index);
		return -EINVAL;
	}

	LOG_DBG("Program MPU region at index 0x%x", index);

	/* Program region */
	region_init(index, region_conf);

	return index;
}

/* This internal function programs an MPU region
 * of a given configuration at a given MPU index.
 */
static int mpu_configure_region(const uint8_t index,
	const struct z_arm_mpu_partition *new_region)
{
	struct arm_mpu_region region_conf;

	LOG_DBG("Configure MPU region at index 0x%x", index);

	/* Populate internal ARM MPU region configuration structure. */
	region_conf.base = new_region->start;
#if defined(CONFIG_ARMV7_R)
	region_conf.size = size_to_mpu_rasr_size(new_region->size);
#endif
	get_region_attr_from_mpu_partition_info(&region_conf.attr,
		&new_region->attr, new_region->start, new_region->size);

	/* Allocate and program region */
	return region_allocate_and_init(index,
		(const struct arm_mpu_region *)&region_conf);
}

#if !defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) || \
	!defined(CONFIG_MPU_GAP_FILLING)
/* This internal function programs a set of given MPU regions
 * over a background memory area, optionally performing a
 * sanity check of the memory regions to be programmed.
 */
static int mpu_configure_regions(const struct z_arm_mpu_partition
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

		reg_index = mpu_configure_region(reg_index, &regions[i]);

		if (reg_index == -EINVAL) {
			return reg_index;
		}

		/* Increment number of programmed MPU indices. */
		reg_index++;
	}

	return reg_index;
}
#endif

/* ARM Core MPU Driver API Implementation for ARM MPU */


#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
/**
 * @brief enable the MPU by setting bit in SCTRL register
 */
void arm_core_mpu_enable(void)
{
	uint32_t val;

	val = __get_SCTLR();
	val |= SCTLR_MPU_ENABLE;
	__set_SCTLR(val);

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

/**
 * @brief disable the MPU by clearing bit in SCTRL register
 */
void arm_core_mpu_disable(void)
{
	uint32_t val;

	/* Force any outstanding transfers to complete before disabling MPU */
	__DSB();

	val = __get_SCTLR();
	val &= ~SCTLR_MPU_ENABLE;
	__set_SCTLR(val);

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}
#else
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
#endif

#if defined(CONFIG_USERSPACE)
/**
 * @brief update configuration of an active memory partition
 */
void arm_core_mpu_mem_partition_config_update(
	struct z_arm_mpu_partition *partition,
	k_mem_partition_attr_t *new_attr)
{
	/* Find the partition. ASSERT if not found. */
	uint8_t i;
	uint8_t reg_index = get_num_regions();

	for (i = get_dyn_region_min_index(); i < get_num_regions(); i++) {
		if (!is_enabled_region(i)) {
			continue;
		}

		uint32_t base = mpu_region_get_base(i);

		if (base != partition->start) {
			continue;
		}

		uint32_t size = mpu_region_get_size(i);

		if (size != partition->size) {
			continue;
		}

		/* Region found */
		reg_index = i;
		break;
	}
	__ASSERT(reg_index != get_num_regions(),
		 "Memory domain partition %p size %zu not found\n",
		 (void *)partition->start, partition->size);

	/* Modify the permissions */
	partition->attr = *new_attr;
	mpu_configure_region(reg_index, partition);
}

/**
 * @brief get the maximum number of available (free) MPU region indices
 *        for configuring dynamic MPU partitions
 */
int arm_core_mpu_get_max_available_dyn_regions(void)
{
	return get_num_regions() - static_regions_num;
}

/**
 * @brief validate the given buffer is user accessible or not
 *
 * Presumes the background mapping is NOT user accessible.
 */
int arm_core_mpu_buffer_validate(void *addr, size_t size, int write)
{
	return mpu_buffer_validate(addr, size, write);
}

#endif /* CONFIG_USERSPACE */

/**
 * @brief configure fixed (static) MPU regions.
 */
void arm_core_mpu_configure_static_mpu_regions(const struct z_arm_mpu_partition
	*static_regions, const uint8_t regions_num,
	const uint32_t background_area_start, const uint32_t background_area_end)
{
	if (mpu_configure_static_mpu_regions(static_regions, regions_num,
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
	const struct z_arm_mpu_partition dyn_region_areas[],
	const uint8_t dyn_region_areas_num)
{
	if (mpu_mark_areas_for_dynamic_regions(dyn_region_areas,
						 dyn_region_areas_num) == -EINVAL) {

		__ASSERT(0, "Marking %u areas for dynamic regions failed\n",
			dyn_region_areas_num);
	}
}
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief configure dynamic MPU regions.
 */
void arm_core_mpu_configure_dynamic_mpu_regions(const struct z_arm_mpu_partition
	*dynamic_regions, uint8_t regions_num)
{
	if (mpu_configure_dynamic_mpu_regions(dynamic_regions, regions_num)
		== -EINVAL) {

		__ASSERT(0, "Configuring %u dynamic MPU regions failed\n",
			regions_num);
	}
}

/* ARM MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
int z_arm_mpu_init(void)
{
	uint32_t r_index;

	if (mpu_config.num_regions > get_num_regions()) {
		/* Attempt to configure more MPU regions than
		 * what is supported by hardware. As this operation
		 * is executed during system (pre-kernel) initialization,
		 * we want to ensure we can detect an attempt to
		 * perform invalid configuration.
		 */
		__ASSERT(0,
			"Request to configure: %u regions (supported: %u)\n",
			mpu_config.num_regions,
			get_num_regions()
		);
		return -1;
	}

	LOG_DBG("total region count: %d", get_num_regions());

	arm_core_mpu_disable();

#if defined(CONFIG_NOCACHE_MEMORY)
	/* Clean and invalidate data cache if it is enabled and
	 * that was not already done at boot
	 */
#if !defined(CONFIG_INIT_ARCH_HW_AT_BOOT)
	if (SCB->CCR & SCB_CCR_DC_Msk) {
		SCB_CleanInvalidateDCache();
	}
#endif
#endif /* CONFIG_NOCACHE_MEMORY */

	/* Architecture-specific configuration */
	mpu_init();

	/* Program fixed regions configured at SOC definition. */
	for (r_index = 0U; r_index < mpu_config.num_regions; r_index++) {
		region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

	/* Update the number of programmed MPU regions. */
	static_regions_num = mpu_config.num_regions;


	arm_core_mpu_enable();

	/* Program additional fixed flash region for null-pointer
	 * dereferencing detection (debug feature)
	 */
#if defined(CONFIG_NULL_POINTER_EXCEPTION_DETECTION_MPU)
#if (defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)) && \
	(CONFIG_FLASH_BASE_ADDRESS > CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE)
#pragma message "Null-Pointer exception detection cannot be configured on un-mapped flash areas"
#else
	const struct z_arm_mpu_partition unmap_region =	{
		.start = 0x0,
		.size = CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
		/* Overlapping region (with any permissions)
		 * will result in fault generation
		 */
		.attr = K_MEM_PARTITION_P_RO_U_NA,
#else
		/* Explicit no-access policy */
		.attr = K_MEM_PARTITION_P_NA_U_NA,
#endif
	};

	/* The flash region for null pointer dereferencing detection shall
	 * comply with the regular MPU partition definition restrictions
	 * (size and alignment).
	 */
	_ARCH_MEM_PARTITION_ALIGN_CHECK(0x0,
		CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE);

#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
	/* ARMv8-M requires that the area:
	 * 0x0 - CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE
	 * is not unmapped (belongs to a valid MPU region already).
	 */
	if ((arm_cmse_mpu_region_get(0x0) == -EINVAL) ||
		(arm_cmse_mpu_region_get(
			CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE - 1)
		== -EINVAL)) {
		__ASSERT(0,
			"Null pointer detection page unmapped\n");
		}
#endif

	if (mpu_configure_region(static_regions_num, &unmap_region) == -EINVAL) {

		__ASSERT(0,
			"Programming null-pointer detection region failed\n");
		return -EINVAL;
	}

	static_regions_num++;

#endif
#endif /* CONFIG_NULL_POINTER_EXCEPTION_DETECTION_MPU */

	/* Sanity check for number of regions in Cortex-M0+, M3, and M4. */
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4)
	__ASSERT(
		(MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos == 8,
		"Invalid number of MPU regions\n");
#elif defined(NUM_MPU_REGIONS)
	__ASSERT(
		(MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos ==
		NUM_MPU_REGIONS,
		"Invalid number of MPU regions\n");
#endif /* CORTEX_M0PLUS || CPU_CORTEX_M3 || CPU_CORTEX_M4 */

	return 0;
}
