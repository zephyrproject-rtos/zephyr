/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include "arm_core_mpu_dev.h"
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mpu);

#define NODE_HAS_PROP_AND_OR(node_id, prop) \
	DT_NODE_HAS_PROP(node_id, prop) ||

BUILD_ASSERT((DT_FOREACH_STATUS_OKAY_NODE_VARGS(
	      NODE_HAS_PROP_AND_OR, zephyr_memory_region_mpu) false) == false,
	      "`zephyr,memory-region-mpu` was deprecated in favor of `zephyr,memory-attr`");

/*
 * Global status variable holding the number of HW MPU region indices, which
 * have been reserved by the MPU driver to program the static (fixed) memory
 * regions.
 */
static uint8_t static_regions_num;

/* Global MPU configuration at system initialization. */
static void mpu_init(void)
{
#if defined(CONFIG_SOC_FAMILY_KINETIS)
	/* Enable clock for the Memory Protection Unit (MPU). */
	CLOCK_EnableClock(kCLOCK_Sysmpu0);
#endif
}

/**
 *  Get the number of supported MPU regions.
 */
static inline uint8_t get_num_regions(void)
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
static int mpu_partition_is_valid(const struct z_arm_mpu_partition *part)
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
static void region_init(const uint32_t index,
	const struct nxp_mpu_region *region_conf)
{
	uint32_t region_base = region_conf->base;
	uint32_t region_end = region_conf->end;
	uint32_t region_attr = region_conf->attr.attr;

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
			 index, region_base, (uint32_t)SYSMPU->WORD[index][0]);

		__ASSERT(region_end == SYSMPU->WORD[index][1],
			 "Region %d end address got 0x%08x expected 0x%08x",
			 index, region_end, (uint32_t)SYSMPU->WORD[index][1]);

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

	LOG_DBG("[%02d] 0x%08x 0x%08x 0x%08x 0x%08x", index,
		    (uint32_t)SYSMPU->WORD[index][0],
		    (uint32_t)SYSMPU->WORD[index][1],
		    (uint32_t)SYSMPU->WORD[index][2],
		    (uint32_t)SYSMPU->WORD[index][3]);

}

static int region_allocate_and_init(const uint8_t index,
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

#define _BUILD_REGION_CONF(reg, _ATTR)						\
	(struct nxp_mpu_region) { .name = (reg).dt_name,			\
				  .base = (reg).dt_addr,			\
				  .end  = (reg).dt_addr + (reg).dt_size,	\
				  .attr = _ATTR,				\
				}

/* This internal function programs the MPU regions defined in the DT when using
 * the `zephyr,memory-attr = <( DT_MEM_ARM(...) )>` property.
 */
static int mpu_configure_regions_from_dt(uint8_t *reg_index)
{
	const struct mem_attr_region_t *region;
	size_t num_regions;

	num_regions = mem_attr_get_regions(&region);

	for (size_t idx = 0; idx < num_regions; idx++) {
		struct nxp_mpu_region region_conf;

		switch (DT_MEM_ARM_GET(region[idx].dt_attr)) {
		case DT_MEM_ARM_MPU_RAM:
			region_conf = _BUILD_REGION_CONF(region[idx], REGION_RAM_ATTR);
			break;
#ifdef REGION_FLASH_ATTR
		case DT_MEM_ARM_MPU_FLASH:
			region_conf = _BUILD_REGION_CONF(region[idx], REGION_FLASH_ATTR);
			break;
#endif
#ifdef REGION_IO_ATTR
		case DT_MEM_ARM_MPU_IO:
			region_conf = _BUILD_REGION_CONF(region[idx], REGION_IO_ATTR);
			break;
#endif
		default:
			/* Either the specified `ATTR_MPU_*` attribute does not
			 * exists or the `REGION_*_ATTR` macro is not defined
			 * for that attribute.
			 */
			LOG_ERR("Invalid attribute for the region\n");
			return -EINVAL;
		}

		if (region_allocate_and_init((*reg_index),
					     (const struct nxp_mpu_region *) &region_conf) < 0) {
			return -EINVAL;
		}

		(*reg_index)++;
	}

	return 0;
}

/**
 * This internal function is utilized by the MPU driver to combine a given
 * region attribute configuration and size and fill-in a driver-specific
 * structure with the correct MPU region attribute configuration.
 */
static inline void get_region_attr_from_mpu_partition_info(
	nxp_mpu_region_attr_t *p_attr,
	const k_mem_partition_attr_t *attr, uint32_t base, uint32_t size)
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
static int mpu_configure_region(const uint8_t index,
	const struct z_arm_mpu_partition *new_region)
{
	struct nxp_mpu_region region_conf;

	LOG_DBG("Configure MPU region at index 0x%x", index);

	/* Populate internal NXP MPU region configuration structure. */
	region_conf.base = new_region->start;
	region_conf.end = (new_region->start + new_region->size - 1);
	get_region_attr_from_mpu_partition_info(&region_conf.attr,
		&new_region->attr, new_region->start, new_region->size);

	/* Allocate and program region */
	return region_allocate_and_init(index,
		(const struct nxp_mpu_region *)&region_conf);
}

#if defined(CONFIG_MPU_STACK_GUARD)
/* This internal function partitions the SRAM MPU region */
static int mpu_sram_partitioning(uint8_t index,
	const struct z_arm_mpu_partition *p_region)
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
static int mpu_configure_regions(const struct z_arm_mpu_partition regions[],
				 uint8_t regions_num, uint8_t start_reg_index,
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

#if defined(CONFIG_MPU_STACK_GUARD)
		if (regions[i].attr.ap_attr == MPU_REGION_SU_RX) {
			unsigned int key;

			/* Attempt to configure an MPU Stack Guard region; this
			 * will require splitting of the underlying SRAM region
			 * into two SRAM regions, leaving out the guard area to
			 * be programmed afterwards.
			 */
			key = irq_lock();
			reg_index =
				mpu_sram_partitioning(reg_index, &regions[i]);
			irq_unlock(key);
		}
#endif /* CONFIG_MPU_STACK_GUARD */

		if (reg_index == -EINVAL) {
			return reg_index;
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

/* This internal function programs the static MPU regions.
 *
 * It returns the number of MPU region indices configured.
 *
 * Note:
 * If the static MPU regions configuration has not been successfully
 * performed, the error signal is propagated to the caller of the function.
 */
static int mpu_configure_static_mpu_regions(
	const struct z_arm_mpu_partition static_regions[],
	const uint8_t regions_num, const uint32_t background_area_base,
	const uint32_t background_area_end)
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
static int mpu_configure_dynamic_mpu_regions(
	const struct z_arm_mpu_partition dynamic_regions[],
	uint8_t regions_num)
{
	unsigned int key;

	/*
	 * Programming the NXP MPU has to be done with care to avoid race
	 * conditions that will cause memory faults. The NXP MPU is composed
	 * of a number of memory region descriptors. The number of descriptors
	 * varies depending on the SOC. Each descriptor has a start addr, end
	 * addr, attribute, and valid. When the MPU is enabled, access to
	 * memory space is checked for access protection errors through an
	 * OR operation of all of the valid MPU descriptors.
	 *
	 * Writing the start/end/attribute descriptor register will clear the
	 * valid bit for that descriptor. This presents a problem because if
	 * the current program stack is in that region or if an ISR occurs
	 * that switches state and uses that region a memory fault will be
	 * triggered. Note that local variable access can also cause stack
	 * accesses while programming these registers depending on the compiler
	 * optimization level.
	 *
	 * To avoid the race condition a temporary descriptor is set to enable
	 * access to all of memory before the call to mpu_configure_regions()
	 * to configure the dynamic memory regions. After, the temporary
	 * descriptor is invalidated if the mpu_configure_regions() didn't
	 * overwrite it.
	 */
	key = irq_lock();
	/* Use last descriptor region as temporary descriptor */
	region_init(get_num_regions()-1, (const struct nxp_mpu_region *)
		&mpu_config.mpu_regions[mpu_config.sram_region]);

	/* Now reset the main SRAM region */
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
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	/* Force any outstanding transfers to complete before disabling MPU */
	barrier_dmem_fence_full();

	/* Disable MPU */
	SYSMPU->CESR &= ~SYSMPU_CESR_VLD_MASK;
	/* Clear MPU error status */
	SYSMPU->CESR |=  SYSMPU_CESR_SPERR_MASK;
}

#if defined(CONFIG_USERSPACE)

static inline uint32_t mpu_region_get_base(uint32_t r_index)
{
	return SYSMPU->WORD[r_index][0];
}

static inline uint32_t mpu_region_get_size(uint32_t r_index)
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
static inline int is_enabled_region(uint32_t r_index)
{
	return SYSMPU->WORD[r_index][3] & SYSMPU_WORD_VLD_MASK;
}

/**
 * This internal function checks if the given buffer is in the region.
 *
 * Note:
 *   The caller must provide a valid region number.
 */
static inline int is_in_region(uint32_t r_index, uint32_t start, uint32_t size)
{
	uint32_t r_addr_start;
	uint32_t r_addr_end;
	uint32_t end;

	r_addr_start = SYSMPU->WORD[r_index][0];
	r_addr_end = SYSMPU->WORD[r_index][1];

	size = size == 0U ? 0U : size - 1U;
	if (u32_add_overflow(start, size, &end)) {
		return 0;
	}

	if ((start >= r_addr_start) && (end <= r_addr_end)) {
		return 1;
	}

	return 0;
}

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

	for (i = static_regions_num; i < get_num_regions(); i++) {
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
static inline int is_user_accessible_region(uint32_t r_index, int write)
{
	uint32_t r_ap = SYSMPU->WORD[r_index][2];

	if (write != 0) {
		return (r_ap & MPU_REGION_WRITE) == MPU_REGION_WRITE;
	}

	return (r_ap & MPU_REGION_READ) == MPU_REGION_READ;
}

/**
 * @brief validate the given buffer is user accessible or not
 */
int arm_core_mpu_buffer_validate(const void *addr, size_t size, int write)
{
	uint8_t r_index;

	/* Iterate through all MPU regions */
	for (r_index = 0U; r_index < get_num_regions(); r_index++) {
		if (!is_enabled_region(r_index) ||
		    !is_in_region(r_index, (uint32_t)addr, size)) {
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
void arm_core_mpu_configure_static_mpu_regions(
	const struct z_arm_mpu_partition static_regions[],
	const uint8_t regions_num, const uint32_t background_area_start,
	const uint32_t background_area_end)
{
	if (mpu_configure_static_mpu_regions(static_regions, regions_num,
					     background_area_start,
					     background_area_end) == -EINVAL) {

		__ASSERT(0, "Configuring %u static MPU regions failed\n",
			regions_num);
	}
}

/**
 * @brief configure dynamic MPU regions.
 */
void arm_core_mpu_configure_dynamic_mpu_regions(
	const struct z_arm_mpu_partition dynamic_regions[], uint8_t regions_num)
{
	if (mpu_configure_dynamic_mpu_regions(dynamic_regions,
					      regions_num) == -EINVAL) {

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
int z_arm_mpu_init(void)
{
	uint32_t r_index;

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

	/* DT-defined MPU regions. */
	if (mpu_configure_regions_from_dt(&static_regions_num) == -EINVAL) {
		__ASSERT(0, "Failed to allocate MPU regions from DT\n");
		return -EINVAL;
	}

	arm_core_mpu_enable();

	return 0;
}
