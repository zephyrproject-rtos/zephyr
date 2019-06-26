/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include "arm_core_mpu_dev.h"
#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>
#include <sys/__assert.h>
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

/* Global MPU configuration at system initialization. */
static void mpu_init(void)
{
	/* Enable clock for the Memory Protection Unit (MPU). */
	CLOCK_EnableClock(kCLOCK_Sysmpu0);
}

/**
 *  Get the number of supported MPU regions.
 */
static inline u8_t get_num_regions(void)
{
	return FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT;
}

/* @brief Partition sanity check
 *
 * This internal function performs run-time sanity check for
 * MPU region start address and size.
 *
 * @param part Pointer to the data structure holding the partition
 *             information (must be valid).
 */
static int mpu_partition_is_valid(const struct k_mem_partition *part)
{
	/* Partition size must be a multiple of the minimum MPU region
	 * size. Start address of the partition must align with the
	 * minimum MPU region size.
	 */
	int partition_is_valid =
		(part->size != 0U)
		&&
		((part->size &
			(~(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1)))
			== part->size)
		&&
		((part->start &
			(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1)) == 0U);

	return partition_is_valid;
}

/* This internal function performs MPU region initialization.
 *
 * Note:
 *   The caller must provide a valid region index.
 */
static void region_init(const u32_t index,
	const struct nxp_mpu_region *region_conf)
{
	u32_t region_base = region_conf->base;
	u32_t region_end = region_conf->end;
	u32_t region_attr = region_conf->attr.attr;

	if (index == 0U) {
		/* The MPU does not allow writes from the core to affect the
		 * RGD0 start or end addresses nor the permissions associated
		 * with the debugger; it can only write the permission fields
		 * associated with the other masters. These protections
		 * guarantee that the debugger always has access to the entire
		 * address space.
		 */
		__ASSERT(region_base == SYSMPU->WORD[index][0],
			 "Region %d base address got 0x%08x expected 0x%08x",
			 index, region_base, (u32_t)SYSMPU->WORD[index][0]);

		__ASSERT(region_end == SYSMPU->WORD[index][1],
			 "Region %d end address got 0x%08x expected 0x%08x",
			 index, region_end, (u32_t)SYSMPU->WORD[index][1]);

		/* Changes to the RGD0_WORD2 alterable fields should be done
		 * via a write to RGDAAC0.
		 */
		SYSMPU->RGDAAC[index] = region_attr;

	} else {
		SYSMPU->WORD[index][0] = region_base;
		SYSMPU->WORD[index][1] = region_end;
		SYSMPU->WORD[index][2] = region_attr;
		SYSMPU->WORD[index][3] = SYSMPU_WORD_VLD_MASK;
	}

	LOG_DBG("[%d] 0x%08x 0x%08x 0x%08x 0x%08x", index,
		    (u32_t)SYSMPU->WORD[index][0],
		    (u32_t)SYSMPU->WORD[index][1],
		    (u32_t)SYSMPU->WORD[index][2],
		    (u32_t)SYSMPU->WORD[index][3]);

}

static int region_allocate_and_init(const u8_t index,
	const struct nxp_mpu_region *region_conf)
{
	/* Attempt to allocate new region index. */
	if (index > (get_num_regions() - 1)) {

		/* No available MPU region index. */
		LOG_ERR("Failed to allocate new MPU region %u\n", index);
		return -EINVAL;
	}

	LOG_DBG("Program MPU region at index 0x%x", index);

	/* Program region */
	region_init(index, region_conf);

	return index;
}

/**
 * This internal function is utilized by the MPU driver to combine a given
 * region attribute configuration and size and fill-in a driver-specific
 * structure with the correct MPU region attribute configuration.
 */
static inline void get_region_attr_from_k_mem_partition_info(
	nxp_mpu_region_attr_t *p_attr,
	const k_mem_partition_attr_t *attr, u32_t base, u32_t size)
{
	/* in NXP MPU the base address and size are not required
	 * to determine region attributes
	 */
	(void) base;
	(void) size;

	p_attr->attr = attr->ap_attr;
}

/* This internal function programs an MPU region
 * of a given configuration at a given MPU index.
 */
static int mpu_configure_region(const u8_t index,
	const struct k_mem_partition *new_region)
{
	struct nxp_mpu_region region_conf;

	LOG_DBG("Configure MPU region at index 0x%x", index);

	/* Populate internal NXP MPU region configuration structure. */
	region_conf.base = new_region->start;
	region_conf.end = (new_region->start + new_region->size - 1);
	get_region_attr_from_k_mem_partition_info(&region_conf.attr,
		&new_region->attr, new_region->start, new_region->size);

	/* Allocate and program region */
	return region_allocate_and_init(index,
		(const struct nxp_mpu_region *)&region_conf);
}

#if defined(CONFIG_MPU_STACK_GUARD)
/* This internal function partitions the SRAM MPU region */
static int mpu_sram_partitioning(u8_t index,
	const struct k_mem_partition *p_region)
{
	/*
	 * The NXP MPU manages the permissions of the overlapping regions
	 * doing the logical OR in between them, hence they can't be used
	 * for stack/stack guard protection. For this reason we need to
	 * perform a partitioning of the SRAM area in such a way that the
	 * guard region does not overlap with the (background) SRAM regions
	 * holding the default SRAM access permission configuration.
	 * In other words, the SRAM is split in two different regions.
	 */

	/*
	 * SRAM partitioning needs to be performed in a strict order.
	 * First, we program a new MPU region with the default SRAM
	 * access permissions for the SRAM area _after_ the stack
	 * guard. Note that the permissions are stored in the global
	 * array:
	 *      'mpu_config.mpu_regions[]', on 'sram_region' index.
	 */
	struct nxp_mpu_region added_sram_region;

	added_sram_region.base = p_region->start + p_region->size;
	added_sram_region.end =
		mpu_config.mpu_regions[mpu_config.sram_region].end;
	added_sram_region.attr.attr =
		mpu_config.mpu_regions[mpu_config.sram_region].attr.attr;

	if (region_allocate_and_init(index,
				     (const struct nxp_mpu_region *)&added_sram_region) < 0) {
		return -EINVAL;
	}

	/* Increment, as an additional region index has been consumed. */
	index++;

	/* Second, adjust the original SRAM region to end at the beginning
	 * of the stack guard.
	 */
	struct nxp_mpu_region adjusted_sram_region;

	adjusted_sram_region.base =
		mpu_config.mpu_regions[mpu_config.sram_region].base;
	adjusted_sram_region.end = p_region->start - 1;
	adjusted_sram_region.attr.attr =
		mpu_config.mpu_regions[mpu_config.sram_region].attr.attr;

	region_init(mpu_config.sram_region,
		(const struct nxp_mpu_region *)&adjusted_sram_region);

	return index;
}
#endif /* CONFIG_MPU_STACK_GUARD */

/* This internal function programs a set of given MPU regions
 * over a background memory area, optionally performing a
 * sanity check of the memory regions to be programmed.
 */
static int mpu_configure_regions(const struct k_mem_partition
	*regions[], u8_t regions_num, u8_t start_reg_index,
	bool do_sanity_check)
{
	int i;
	int reg_index = start_reg_index;

	for (i = 0; i < regions_num; i++) {
		if (regions[i]->size == 0U) {
			continue;
		}
		/* Non-empty region. */

		if (do_sanity_check &&
				(!mpu_partition_is_valid(regions[i]))) {
			LOG_ERR("Partition %u: sanity check failed.", i);
			return -EINVAL;
		}

#if defined(CONFIG_MPU_STACK_GUARD)
		if (regions[i]->attr.ap_attr == MPU_REGION_SU_RX) {
			unsigned int key;

			/* Attempt to configure an MPU Stack Guard region; this
			 * will require splitting of the underlying SRAM region
			 * into two SRAM regions, leaving out the guard area to
			 * be programmed afterwards.
			 */
			key = irq_lock();
			reg_index =
				mpu_sram_partitioning(reg_index, regions[i]);
			irq_unlock(key);
		}
#endif /* CONFIG_MPU_STACK_GUARD */

		if (reg_index == -EINVAL) {
			return reg_index;
		}

		reg_index = mpu_configure_region(reg_index, regions[i]);

		if (reg_index == -EINVAL) {
			return reg_index;
		}

		/* Increment number of programmed MPU indices. */
		reg_index++;
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
static int mpu_configure_static_mpu_regions(const struct k_mem_partition
	*static_regions[], const u8_t regions_num,
	const u32_t background_area_base,
	const u32_t background_area_end)
{
	int mpu_reg_index = static_regions_num;

	/* In NXP MPU architecture the static regions are
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
static int mpu_configure_dynamic_mpu_regions(const struct k_mem_partition
	*dynamic_regions[], u8_t regions_num)
{
	unsigned int key;

	/* Reset MPU regions inside which dynamic memory regions may
	 * be programmed.
	 *
	 * Re-programming these regions will temporarily leave memory areas
	 * outside all MPU regions.
	 * This might trigger memory faults if ISRs occurring during
	 * re-programming perform access in those areas.
	 */
	key = irq_lock();
	region_init(mpu_config.sram_region, (const struct nxp_mpu_region *)
		&mpu_config.mpu_regions[mpu_config.sram_region]);
	irq_unlock(key);

	int mpu_reg_index = static_regions_num;

	/* In NXP MPU architecture the dynamic regions are
	 * programmed on top of existing SRAM region configuration.
	 */

	mpu_reg_index = mpu_configure_regions(dynamic_regions,
		regions_num, mpu_reg_index, false);

	if (mpu_reg_index != -EINVAL) {

		/* Disable the non-programmed MPU regions. */
		for (int i = mpu_reg_index; i < get_num_regions(); i++) {

			LOG_DBG("disable region 0x%x", i);
			/* Disable region */
			SYSMPU->WORD[i][0] = 0;
			SYSMPU->WORD[i][1] = 0;
			SYSMPU->WORD[i][2] = 0;
			SYSMPU->WORD[i][3] = 0;
		}
	}

	return mpu_reg_index;
}

/* ARM Core MPU Driver API Implementation for NXP MPU */

/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void)
{
	/* Enable MPU */
	SYSMPU->CESR |= SYSMPU_CESR_VLD_MASK;

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
	SYSMPU->CESR &= ~SYSMPU_CESR_VLD_MASK;
	/* Clear MPU error status */
	SYSMPU->CESR |=  SYSMPU_CESR_SPERR_MASK;
}

#if defined(CONFIG_USERSPACE)

static inline u32_t mpu_region_get_base(u32_t r_index)
{
	return SYSMPU->WORD[r_index][0];
}

static inline u32_t mpu_region_get_size(u32_t r_index)
{
	/* <END> + 1 - <BASE> */
	return (SYSMPU->WORD[r_index][1] + 1) - SYSMPU->WORD[r_index][0];
}

/**
 * This internal function checks if region is enabled or not.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_enabled_region(u32_t r_index)
{
	return SYSMPU->WORD[r_index][3] & SYSMPU_WORD_VLD_MASK;
}

/**
 * This internal function checks if the given buffer is in the region.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_in_region(u32_t r_index, u32_t start, u32_t size)
{
	u32_t r_addr_start;
	u32_t r_addr_end;

	r_addr_start = SYSMPU->WORD[r_index][0];
	r_addr_end = SYSMPU->WORD[r_index][1];

	if (start >= r_addr_start && (start + size - 1) <= r_addr_end) {
		return 1;
	}

	return 0;
}

/**
 * @brief update configuration of an active memory partition
 */
void arm_core_mpu_mem_partition_config_update(
	struct k_mem_partition *partition,
	k_mem_partition_attr_t *new_attr)
{
	/* Find the partition. ASSERT if not found. */
	u8_t i;
	u8_t reg_index = get_num_regions();

	for (i = static_regions_num; i < get_num_regions(); i++) {
		if (!is_enabled_region(i)) {
			continue;
		}

		u32_t base = mpu_region_get_base(i);

		if (base != partition->start) {
			continue;
		}

		u32_t size = mpu_region_get_size(i);

		if (size != partition->size) {
			continue;
		}

		/* Region found */
		reg_index = i;
		break;
	}
	__ASSERT(reg_index != get_num_regions(),
		 "Memory domain partition not found\n");

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
 * This internal function checks if the region is user accessible or not
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_user_accessible_region(u32_t r_index, int write)
{
	u32_t r_ap = SYSMPU->WORD[r_index][2];

	if (write) {
		return (r_ap & MPU_REGION_WRITE) == MPU_REGION_WRITE;
	}

	return (r_ap & MPU_REGION_READ) == MPU_REGION_READ;
}

/**
 * @brief validate the given buffer is user accessible or not
 */
int arm_core_mpu_buffer_validate(void *addr, size_t size, int write)
{
	u8_t r_index;

	/* Iterate through all MPU regions */
	for (r_index = 0U; r_index < get_num_regions(); r_index++) {
		if (!is_enabled_region(r_index) ||
		    !is_in_region(r_index, (u32_t)addr, size)) {
			continue;
		}

		/* For NXP MPU, priority is given to granting permission over
		 * denying access for overlapping region.
		 * So we can stop the iteration immediately once we find the
		 * matched region that grants permission.
		 */
		if (is_user_accessible_region(r_index, write)) {
			return 0;
		}
	}

	return -EPERM;
}

#endif /* CONFIG_USERSPACE */

/**
 * @brief configure fixed (static) MPU regions.
 */
void arm_core_mpu_configure_static_mpu_regions(const struct k_mem_partition
	*static_regions[], const u8_t regions_num,
	const u32_t background_area_start, const u32_t background_area_end)
{
	if (mpu_configure_static_mpu_regions(static_regions, regions_num,
					     background_area_start, background_area_end) == -EINVAL) {

		__ASSERT(0, "Configuring %u static MPU regions failed\n",
			regions_num);
	}
}

/**
 * @brief configure dynamic MPU regions.
 */
void arm_core_mpu_configure_dynamic_mpu_regions(const struct k_mem_partition
	*dynamic_regions[], u8_t regions_num)
{
	if (mpu_configure_dynamic_mpu_regions(dynamic_regions, regions_num)
		== -EINVAL) {

		__ASSERT(0, "Configuring %u dynamic MPU regions failed\n",
			regions_num);
	}
}

/* NXP MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static int nxp_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	u32_t r_index;

	if (mpu_config.num_regions > get_num_regions()) {
		/* Attempt to configure more MPU regions than
		 * what is supported by hardware. As this operation
		 * may be executed during system (pre-kernel) initialization,
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

	/* Architecture-specific configuration */
	mpu_init();

	/* Program fixed regions configured at SOC definition. */
	for (r_index = 0U; r_index < mpu_config.num_regions; r_index++) {
		region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

	/* Update the number of programmed MPU regions. */
	static_regions_num = mpu_config.num_regions;


	arm_core_mpu_enable();


	return 0;
}

#if defined(CONFIG_LOG)
/* To have logging the driver needs to be initialized later */
SYS_INIT(nxp_mpu_init, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else
SYS_INIT(nxp_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
