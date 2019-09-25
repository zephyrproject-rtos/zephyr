/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V2_INTERNAL_H_
#define ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V2_INTERNAL_H_

#define AUX_MPU_RDB_VALID_MASK (0x1)
#define AUX_MPU_EN_ENABLE   (0x40000000)
#define AUX_MPU_EN_DISABLE  (0xBFFFFFFF)

#define AUX_MPU_RDP_REGION_SIZE(bits) \
	(((bits - 1) & 0x3) | (((bits - 1) & 0x1C) << 7))

#define AUX_MPU_RDP_ATTR_MASK (0x1FC)
#define AUX_MPU_RDP_SIZE_MASK (0xE03)

#define _ARC_V2_MPU_EN   (0x409)
#define _ARC_V2_MPU_RDB0 (0x422)
#define _ARC_V2_MPU_RDP0 (0x423)

/* For MPU version 2, the minimum protection region size is 2048 bytes */
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 11

/**
 * This internal function initializes a MPU region
 */
static inline void _region_init(u32_t index, u32_t region_addr, u32_t size,
				u32_t region_attr)
{
	u8_t bits = find_msb_set(size) - 1;

	index = index * 2U;

	if (bits < ARC_FEATURE_MPU_ALIGNMENT_BITS) {
		bits = ARC_FEATURE_MPU_ALIGNMENT_BITS;
	}

	if ((1 << bits) < size) {
		bits++;
	}

	if (size > 0) {
		region_attr &= ~(AUX_MPU_RDP_SIZE_MASK);
		region_attr |= AUX_MPU_RDP_REGION_SIZE(bits);
		region_addr |= AUX_MPU_RDB_VALID_MASK;
	} else {
		region_addr = 0U;
	}

	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RDP0 + index, region_attr);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RDB0 + index, region_addr);
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct region index.
 */
static inline int get_region_index_by_type(u32_t type)
{
	/*
	 * The new MPU regions are allocated per type after the statically
	 * configured regions. The type is one-indexed rather than
	 * zero-indexed.
	 *
	 * For ARC MPU v2, the smaller index has higher priority, so the
	 * index is allocated in reverse order. Static regions start from
	 * the biggest index, then thread related regions.
	 *
	 */
	switch (type) {
	case THREAD_STACK_USER_REGION:
		return get_num_regions() - mpu_config.num_regions
		       - THREAD_STACK_REGION;
	case THREAD_STACK_REGION:
	case THREAD_APP_DATA_REGION:
	case THREAD_STACK_GUARD_REGION:
		return get_num_regions() - mpu_config.num_regions - type;
	case THREAD_DOMAIN_PARTITION_REGION:
#if defined(CONFIG_MPU_STACK_GUARD)
		return get_num_regions() - mpu_config.num_regions - type;
#else
		/*
		 * Start domain partition region from stack guard region
		 * since stack guard is not enabled.
		 */
		return get_num_regions() - mpu_config.num_regions - type + 1;
#endif
	default:
		__ASSERT(0, "Unsupported type");
		return -EINVAL;
	}
}

/**
 * This internal function checks if region is enabled or not
 */
static inline bool _is_enabled_region(u32_t r_index)
{
	return ((z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDB0 + r_index * 2U)
		 & AUX_MPU_RDB_VALID_MASK) == AUX_MPU_RDB_VALID_MASK);
}

/**
 * This internal function check if the given buffer in in the region
 */
static inline bool _is_in_region(u32_t r_index, u32_t start, u32_t size)
{
	u32_t r_addr_start;
	u32_t r_addr_end;
	u32_t r_size_lshift;

	r_addr_start = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDB0 + r_index * 2U)
		       & (~AUX_MPU_RDB_VALID_MASK);
	r_size_lshift = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDP0 + r_index * 2U)
			& AUX_MPU_RDP_SIZE_MASK;
	r_size_lshift = (r_size_lshift & 0x3) | ((r_size_lshift >> 7) & 0x1C);
	r_addr_end = r_addr_start  + (1 << (r_size_lshift + 1));

	if (start >= r_addr_start && (start + size) <= r_addr_end) {
		return 1;
	}

	return 0;
}

/**
 * This internal function check if the region is user accessible or not
 */
static inline bool _is_user_accessible_region(u32_t r_index, int write)
{
	u32_t r_ap;

	r_ap = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDP0 + r_index * 2U);

	r_ap &= AUX_MPU_RDP_ATTR_MASK;

	if (write) {
		return ((r_ap & (AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW)) ==
			(AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW));
	}

	return ((r_ap & (AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR)) ==
		(AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR));
}

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param type MPU region type
 * @param base base address in RAM
 * @param size size of the region
 */
static inline int _mpu_configure(u8_t type, u32_t base, u32_t size)
{
	s32_t region_index =  get_region_index_by_type(type);
	u32_t region_attr = get_region_attr_by_type(type);

	LOG_DBG("Region info: 0x%x 0x%x", base, size);

	if (region_attr == 0U || region_index < 0) {
		return -EINVAL;
	}

	/*
	 * For ARC MPU v2, MPU regions can be overlapped, smaller
	 * region index has higher priority.
	 */
	_region_init(region_index, base, size, region_attr);

	return 0;
}

/* ARC Core MPU Driver API Implementation for ARC MPUv2 */

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

#if defined(CONFIG_MPU_STACK_GUARD)
#if defined(CONFIG_USERSPACE)
	if ((thread->base.user_options & K_USER) != 0) {
		/* the areas before and after the user stack of thread is
		 * kernel only. These area can be used as stack guard.
		 * -----------------------
		 * |  kernel only area   |
		 * |---------------------|
		 * |  user stack         |
		 * |---------------------|
		 * |privilege stack guard|
		 * |---------------------|
		 * |  privilege stack    |
		 * -----------------------
		 */
		if (_mpu_configure(THREAD_STACK_GUARD_REGION,
			thread->arch.priv_stack_start - STACK_GUARD_SIZE,
			STACK_GUARD_SIZE) < 0) {
			LOG_ERR("thread %p's stack guard failed", thread);
			return;
		}
	} else {
		if (_mpu_configure(THREAD_STACK_GUARD_REGION,
			thread->stack_info.start - STACK_GUARD_SIZE,
			STACK_GUARD_SIZE) < 0) {
			LOG_ERR("thread %p's stack guard failed", thread);
			return;
		}
	}
#else
	if (_mpu_configure(THREAD_STACK_GUARD_REGION,
		thread->stack_info.start - STACK_GUARD_SIZE,
		STACK_GUARD_SIZE) < 0) {
		LOG_ERR("thread %p's stack guard failed", thread);
		return;
	}
#endif
#endif

#if defined(CONFIG_USERSPACE)
	/* configure stack region of user thread */
	if (thread->base.user_options & K_USER) {
		LOG_DBG("configure user thread %p's stack", thread);
		if (_mpu_configure(THREAD_STACK_USER_REGION,
		(u32_t)thread->stack_obj, thread->stack_info.size) < 0) {
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
void arc_core_mpu_default(u32_t region_attr)
{
	u32_t val =  z_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) &
		    (~AUX_MPU_RDP_ATTR_MASK);

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
int arc_core_mpu_region(u32_t index, u32_t base, u32_t size,
			 u32_t region_attr)
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
	int region_index =
		get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
	u32_t num_partitions;
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
			_region_init(region_index, pparts->start,
			 pparts->size, pparts->attr);
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

	int region_index =
		get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

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
void arc_core_mpu_remove_mem_partition(struct k_mem_domain *domain,
			u32_t part_id)
{
	ARG_UNUSED(domain);

	int region_index =
		get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

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
int arc_core_mpu_buffer_validate(void *addr, size_t size, int write)
{
	int r_index;

	/*
	 * For ARC MPU v2, smaller region number takes priority.
	 * we can stop the iteration immediately once we find the
	 * matched region that grants permission or denies access.
	 *
	 */
	for (r_index = 0; r_index < get_num_regions(); r_index++) {
		if (!_is_enabled_region(r_index) ||
		    !_is_in_region(r_index, (u32_t)addr, size)) {
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
static int arc_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	u32_t num_regions;
	u32_t i;

	num_regions = get_num_regions();

	/* ARC MPU supports up to 16 Regions */
	if (mpu_config.num_regions > num_regions) {
		__ASSERT(0,
		"Request to configure: %u regions (supported: %u)\n",
		mpu_config.num_regions, num_regions);
		return -EINVAL;
	}

	/* Disable MPU */
	arc_core_mpu_disable();

	int r_index;
	/*
	 * the MPU regions are filled in the reverse order.
	 * According to ARCv2 ISA, the MPU region with smaller
	 * index has higher priority. The static background MPU
	 * regions in mpu_config will be in the bottom. Then
	 * the special type regions will be above.
	 *
	 */
	r_index = num_regions - mpu_config.num_regions;

	/* clear all the regions first */
	for (i = 0U; i < r_index; i++) {
		_region_init(i, 0, 0, 0);
	}

	/* configure the static regions */
	for (i = 0U; i < mpu_config.num_regions; i++) {
		_region_init(r_index,
			     mpu_config.mpu_regions[i].base,
			     mpu_config.mpu_regions[i].size,
			     mpu_config.mpu_regions[i].attr);
		r_index++;
	}

	/* default region: no read, write and execute */
	arc_core_mpu_default(0);

	/* Enable MPU */
	arc_core_mpu_enable();

	return 0;
}

SYS_INIT(arc_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V2_INTERNAL_H_ */
