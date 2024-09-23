/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_COMMON_INTERNAL_H_
#define ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_COMMON_INTERNAL_H_

#if CONFIG_ARC_MPU_VER == 2 || CONFIG_ARC_MPU_VER == 3
#include "arc_mpu_v2_internal.h"
#elif CONFIG_ARC_MPU_VER == 6
#include "arc_mpu_v6_internal.h"
#else
#error "Unsupported MPU version"
#endif

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param type MPU region type
 * @param base base address in RAM
 * @param size size of the region
 */
static inline int _mpu_configure(uint8_t type, uint32_t base, uint32_t size)
{
	int32_t region_index =  get_region_index_by_type(type);
	uint32_t region_attr = get_region_attr_by_type(type);

	LOG_DBG("Region info: 0x%x 0x%x", base, size);

	if (region_attr == 0U || region_index < 0) {
		return -EINVAL;
	}

	/*
	 * For ARC MPU, MPU regions can be overlapped, smaller
	 * region index has higher priority.
	 */
	_region_init(region_index, base, size, region_attr);

	return 0;
}

/* ARC Core MPU Driver API Implementation for ARC MP */

/**
 * @brief enable the MPU
 */
void arc_core_mpu_enable(void)
{
	/* Enable MPU */
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		z_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) | AUX_MPU_EN_ENABLE);
}

/**
 * @brief disable the MPU
 */
void arc_core_mpu_disable(void)
{
    /* Disable MPU */
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		z_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) & AUX_MPU_EN_DISABLE);
}

/**
 * @brief configure the thread's MPU regions
 *
 * @param thread the target thread
 */
void arc_core_mpu_configure_thread(struct k_thread *thread)
{
#if defined(CONFIG_USERSPACE)
	/* configure stack region of user thread */
	if (thread->base.user_options & K_USER) {
		LOG_DBG("configure user thread %p's stack", thread);
		if (_mpu_configure(THREAD_STACK_USER_REGION,
					 (uint32_t)thread->stack_info.start,
					 thread->stack_info.size) < 0) {
			LOG_ERR("user thread %p's stack failed", thread);
			return;
		}
	}

	LOG_DBG("configure thread %p's domain", thread);
	arc_core_mpu_configure_mem_domain(thread);
#endif
}


/**
 * @brief configure the default region
 *
 * @param region_attr region attribute of default region
 */
void arc_core_mpu_default(uint32_t region_attr)
{
	uint32_t val =  z_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) & (~AUX_MPU_RDP_ATTR_MASK);

	region_attr &= AUX_MPU_RDP_ATTR_MASK;
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_EN, region_attr | val);
}

/**
 * @brief configure the MPU region
 *
 * @param index   MPU region index
 * @param base    base address
 * @param region_attr region attribute
 */
int arc_core_mpu_region(uint32_t index, uint32_t base, uint32_t size, uint32_t region_attr)
{
	if (index >= get_num_regions()) {
		return -EINVAL;
	}

	region_attr &= AUX_MPU_RDP_ATTR_MASK;

	_region_init(index, base, size, region_attr);

	return 0;
}

#if defined(CONFIG_USERSPACE)

/**
 * @brief configure MPU regions for the memory partitions of the memory domain
 *
 * @param thread the thread which has memory domain
 */
void arc_core_mpu_configure_mem_domain(struct k_thread *thread)
{
	int region_index = get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
	uint32_t num_partitions;
	struct k_mem_partition *pparts;
	struct k_mem_domain *mem_domain = NULL;

	if (thread) {
		mem_domain = thread->mem_domain_info.mem_domain;
	}

	if (mem_domain) {
		LOG_DBG("configure domain: %p", mem_domain);
		num_partitions = mem_domain->num_partitions;
		pparts = mem_domain->partitions;
	} else {
		LOG_DBG("disable domain partition regions");
		num_partitions = 0U;
		pparts = NULL;
	}

	for (; region_index >= 0; region_index--) {
		if (num_partitions) {
			LOG_DBG("set region 0x%x 0x%lx 0x%x",
				 region_index, pparts->start, pparts->size);
			_region_init(region_index, pparts->start, pparts->size, pparts->attr);
			num_partitions--;
		} else {
			/* clear the left mpu entries */
			_region_init(region_index, 0, 0, 0);
		}
		pparts++;
	}
}

/**
 * @brief remove MPU regions for the memory partitions of the memory domain
 *
 * @param mem_domain the target memory domain
 */
void arc_core_mpu_remove_mem_domain(struct k_mem_domain *mem_domain)
{
	ARG_UNUSED(mem_domain);

	int region_index = get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

	for (; region_index >= 0; region_index--) {
		_region_init(region_index, 0, 0, 0);
	}
}

/**
 * @brief reset MPU region for a single memory partition
 *
 * @param domain  the target memory domain
 * @param partition_id  memory partition id
 */
void arc_core_mpu_remove_mem_partition(struct k_mem_domain *domain, uint32_t part_id)
{
	ARG_UNUSED(domain);

	int region_index = get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

	LOG_DBG("disable region 0x%x", region_index + part_id);
	/* Disable region */
	_region_init(region_index + part_id, 0, 0, 0);
}

/**
 * @brief get the maximum number of free regions for memory domain partitions
 */
int arc_core_mpu_get_max_domain_partition_regions(void)
{
	return get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION) + 1;
}

/**
 * @brief validate the given buffer is user accessible or not
 */
int arc_core_mpu_buffer_validate(const void *addr, size_t size, int write)
{
	/*
	 * For ARC MPU, smaller region number takes priority.
	 * we can stop the iteration immediately once we find the
	 * matched region that grants permission or denies access.
	 *
	 */
	for (int r_index = 0; r_index < get_num_regions(); r_index++) {
		if (!_is_enabled_region(r_index) || !_is_in_region(r_index, (uint32_t)addr, size)) {
			continue;
		}

		if (_is_user_accessible_region(r_index, write)) {
			return 0;
		} else {
			return -EPERM;
		}
	}

	return -EPERM;
}
#endif /* CONFIG_USERSPACE */

/* ARC MPU Driver Initial Setup */
/*
 * @brief MPU default initialization and configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
void arc_mpu_init(void)
{

	uint32_t num_regions = get_num_regions();

	if (mpu_config.num_regions > num_regions) {
		__ASSERT(0, "Request to configure: %u regions (supported: %u)\n",
			 mpu_config.num_regions, num_regions);
	}

	/* Disable MPU */
	arc_core_mpu_disable();

	/*
	 * the MPU regions are filled in the reverse order.
	 * According to ARCv2 ISA, the MPU region with smaller
	 * index has higher priority. The static background MPU
	 * regions in mpu_config will be in the bottom. Then
	 * the special type regions will be above.
	 */
	int r_index = num_regions - mpu_config.num_regions;

	/* clear all the regions first */
	for (uint32_t i = 0U; i < r_index; i++) {
		_region_init(i, 0, 0, 0);
	}

	/* configure the static regions */
	for (uint32_t i = 0U; i < mpu_config.num_regions; i++) {
		_region_init(r_index, mpu_config.mpu_regions[i].base,
			 mpu_config.mpu_regions[i].size, mpu_config.mpu_regions[i].attr);
		r_index++;
	}

	/* default region: no read, write and execute */
	arc_core_mpu_default(0);

	/* Enable MPU */
	arc_core_mpu_enable();
}


#endif /* ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_COMMON_INTERNAL_H_ */
