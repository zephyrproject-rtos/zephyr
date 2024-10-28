/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V4_INTERNAL_H_
#define ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V4_INTERNAL_H_

#define AUX_MPU_RPER_SID1       0x10000
/* valid mask: SID1+secure+valid */
#define AUX_MPU_RPER_VALID_MASK ((0x1) | AUX_MPU_RPER_SID1 | AUX_MPU_ATTR_S)

#define AUX_MPU_RPER_ATTR_MASK (0x1FF)

/* For MPU version 4, the minimum protection region size is 32 bytes */
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 5

#define CALC_REGION_END_ADDR(start, size) \
	(start + size - (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS))

/* ARC MPU version 4 does not support mpu region overlap in hardware
 * so if we want to allocate MPU region dynamically, e.g. thread stack,
 * memory domain from a background region, a dynamic region splitting
 * approach is designed. pls see comments in
 *          _dynamic_region_allocate_and_init
 * But this approach has an impact on performance of thread switch.
 * As a trade off, we can use the default mpu region as the background region
 * to avoid the dynamic region splitting. This will give more privilege to
 * codes in kernel mode which can access the memory region not covered by
 * explicit mpu entry. Considering  memory protection is mainly used to
 * isolate malicious codes in user mode, it makes sense to get better
 * thread switch performance through default mpu region.
 * CONFIG_MPU_GAP_FILLING is used to turn this on/off.
 *
 */
#if defined(CONFIG_MPU_GAP_FILLING)

#if defined(CONFIG_USERSPACE) && defined(CONFIG_MPU_STACK_GUARD)
/* 1 for stack guard , 1 for user thread, 1 for split */
#define MPU_REGION_NUM_FOR_THREAD 3
#elif defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD)
/* 1 for stack guard or user thread stack , 1 for split */
#define MPU_REGION_NUM_FOR_THREAD 2
#else
#define MPU_REGION_NUM_FOR_THREAD 0
#endif

#define MPU_DYNAMIC_REGION_AREAS_NUM 2

/**
 * @brief internal structure holding information of
 * memory areas where dynamic MPU programming is allowed.
 */
struct dynamic_region_info {
	uint8_t index;
	uint32_t base;
	uint32_t size;
	uint32_t attr;
};

static uint8_t dynamic_regions_num;
static uint8_t dynamic_region_index;

/**
 * Global array, holding the MPU region index of
 * the memory region inside which dynamic memory
 * regions may be configured.
 */
static struct dynamic_region_info dyn_reg_info[MPU_DYNAMIC_REGION_AREAS_NUM];
#endif /* CONFIG_MPU_GAP_FILLING */

static uint8_t static_regions_num;

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
/* \todo through secure service to access mpu */
static inline void _region_init(uint32_t index, uint32_t region_addr, uint32_t size,
				uint32_t region_attr)
{
}

static inline void _region_set_attr(uint32_t index, uint32_t attr)
{

}

static inline uint32_t _region_get_attr(uint32_t index)
{
	return 0;
}

static inline uint32_t _region_get_start(uint32_t index)
{
	return 0;
}

static inline void _region_set_start(uint32_t index, uint32_t start)
{

}

static inline uint32_t _region_get_end(uint32_t index)
{
	return 0;
}

static inline void _region_set_end(uint32_t index, uint32_t end)
{
}

/**
 * This internal function probes the given addr's MPU index.if not
 * in MPU, returns error
 */
static inline int _mpu_probe(uint32_t addr)
{
	return -EINVAL;
}

/**
 * This internal function checks if MPU region is enabled or not
 */
static inline bool _is_enabled_region(uint32_t r_index)
{
	return false;
}

/**
 * This internal function check if the region is user accessible or not
 */
static inline bool _is_user_accessible_region(uint32_t r_index, int write)
{
	return false;
}
#else /* CONFIG_ARC_NORMAL_FIRMWARE */
/* the following functions are prepared for SECURE_FIRMWARE */
static inline void _region_init(uint32_t index, uint32_t region_addr, uint32_t size,
				uint32_t region_attr)
{
	if (size < (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS)) {
		size = (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS);
	}

	if (region_attr) {
		region_attr &= AUX_MPU_RPER_ATTR_MASK;
		region_attr |=  AUX_MPU_RPER_VALID_MASK;
	}

	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RSTART, region_addr);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_REND,
			CALC_REGION_END_ADDR(region_addr, size));
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RPER, region_attr);
}

static inline void _region_set_attr(uint32_t index, uint32_t attr)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RPER, attr |
				 AUX_MPU_RPER_VALID_MASK);
}

static inline uint32_t _region_get_attr(uint32_t index)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);

	return z_arc_v2_aux_reg_read(_ARC_V2_MPU_RPER);
}

static inline uint32_t _region_get_start(uint32_t index)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);

	return z_arc_v2_aux_reg_read(_ARC_V2_MPU_RSTART);
}

static inline void _region_set_start(uint32_t index, uint32_t start)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RSTART, start);
}

static inline uint32_t _region_get_end(uint32_t index)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);

	return z_arc_v2_aux_reg_read(_ARC_V2_MPU_REND) +
		(1 << ARC_FEATURE_MPU_ALIGNMENT_BITS);
}

static inline void _region_set_end(uint32_t index, uint32_t end)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_REND, end -
		(1 << ARC_FEATURE_MPU_ALIGNMENT_BITS));
}

/**
 * This internal function probes the given addr's MPU index.if not
 * in MPU, returns error
 */
static inline int _mpu_probe(uint32_t addr)
{
	uint32_t val;

	z_arc_v2_aux_reg_write(_ARC_V2_MPU_PROBE, addr);
	val = z_arc_v2_aux_reg_read(_ARC_V2_MPU_INDEX);

	/* if no match or multiple regions match, return error */
	if (val & 0xC0000000) {
		return -EINVAL;
	} else {
		return val;
	}
}

/**
 * This internal function checks if MPU region is enabled or not
 */
static inline bool _is_enabled_region(uint32_t r_index)
{
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, r_index);
	return ((z_arc_v2_aux_reg_read(_ARC_V2_MPU_RPER) &
		 AUX_MPU_RPER_VALID_MASK) == AUX_MPU_RPER_VALID_MASK);
}

/**
 * This internal function check if the region is user accessible or not
 */
static inline bool _is_user_accessible_region(uint32_t r_index, int write)
{
	uint32_t r_ap;

	z_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, r_index);
	r_ap = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RPER);
	r_ap &= AUX_MPU_RPER_ATTR_MASK;

	if (write) {
		return ((r_ap & (AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW)) ==
			(AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW));
	}

	return ((r_ap & (AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR)) ==
		(AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR));
}

#endif /* CONFIG_ARC_NORMAL_FIRMWARE */

/**
 * This internal function checks the area given by (start, size)
 * and returns the index if the area match one MPU entry
 */
static inline int _get_region_index(uint32_t start, uint32_t size)
{
	int index = _mpu_probe(start);

	if (index > 0 && index == _mpu_probe(start + size - 1)) {
		return index;
	}

	return -EINVAL;
}

#if defined(CONFIG_MPU_GAP_FILLING)
/**
 * This internal function allocates a dynamic MPU region and returns
 * the index or error
 */
static inline int _dynamic_region_allocate_index(void)
{
	if (dynamic_region_index >= get_num_regions()) {
		LOG_ERR("no enough mpu entries %d", dynamic_region_index);
		return -EINVAL;
	}

	return dynamic_region_index++;
}

/* @brief allocate and init a dynamic MPU region
 *
 * This internal function performs the allocation and initialization of
 * a dynamic MPU region
 *
 * @param base region base
 * @param size region size
 * @param attr region attribute
 * @return  <0 failure, >0  allocated dynamic region index
 */
static int _dynamic_region_allocate_and_init(uint32_t base, uint32_t size,
	 uint32_t attr)
{
	int u_region_index = _get_region_index(base, size);
	int region_index;

	LOG_DBG("Region info: base 0x%x size 0x%x attr 0x%x", base, size, attr);

	if (u_region_index == -EINVAL) {
		/* no underlying region */

		region_index = _dynamic_region_allocate_index();

		if (region_index > 0) {
		/* a new region */
			_region_init(region_index, base, size, attr);
		}

		return region_index;
	}

	/*
	 * The new memory region is to be placed inside the underlying
	 * region, possibly splitting the underlying region into two.
	 */

	uint32_t u_region_start = _region_get_start(u_region_index);
	uint32_t u_region_end = _region_get_end(u_region_index);
	uint32_t u_region_attr = _region_get_attr(u_region_index);
	uint32_t end = base + size;


	if ((base == u_region_start) && (end == u_region_end)) {
		/* The new region overlaps entirely with the
		 * underlying region. In this case we simply
		 * update the partition attributes of the
		 * underlying region with those of the new
		 * region.
		 */
		_region_init(u_region_index, base, size, attr);
		region_index = u_region_index;
	} else if (base == u_region_start) {
		/* The new region starts exactly at the start of the
		 * underlying region; the start of the underlying
		 * region needs to be set to the end of the new region.
		 */
		_region_set_start(u_region_index, base + size);
		_region_set_attr(u_region_index, u_region_attr);

		region_index = _dynamic_region_allocate_index();

		if (region_index > 0) {
			_region_init(region_index, base, size, attr);
		}

	} else if (end == u_region_end) {
		/* The new region ends exactly at the end of the
		 * underlying region; the end of the underlying
		 * region needs to be set to the start of the
		 * new region.
		 */
		_region_set_end(u_region_index, base);
		_region_set_attr(u_region_index, u_region_attr);

		region_index = _dynamic_region_allocate_index();

		if (region_index > 0) {
			_region_init(region_index, base, size, attr);
		}

	} else {
		/* The new region lies strictly inside the
		 * underlying region, which needs to split
		 * into two regions.
		 */

		_region_set_end(u_region_index, base);
		_region_set_attr(u_region_index, u_region_attr);

		region_index = _dynamic_region_allocate_index();

		if (region_index > 0) {
			_region_init(region_index, base, size, attr);

			region_index = _dynamic_region_allocate_index();

			if (region_index > 0) {
				_region_init(region_index, base + size,
				 u_region_end - end, u_region_attr);
			}
		}
	}

	return region_index;
}

/* @brief reset the dynamic MPU regions
 *
 * This internal function performs the reset of dynamic MPU regions
 */
static void _mpu_reset_dynamic_regions(void)
{
	uint32_t i;
	uint32_t num_regions = get_num_regions();

	for (i = static_regions_num; i < num_regions; i++) {
		_region_init(i, 0, 0, 0);
	}

	for (i = 0U; i < dynamic_regions_num; i++) {
		_region_init(
			dyn_reg_info[i].index,
			dyn_reg_info[i].base,
			dyn_reg_info[i].size,
			dyn_reg_info[i].attr);
	}

	/* dynamic regions are after static regions */
	dynamic_region_index = static_regions_num;
}

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
static inline int _mpu_configure(uint8_t type, uint32_t base, uint32_t size)
{
	uint32_t region_attr = get_region_attr_by_type(type);

	return _dynamic_region_allocate_and_init(base, size, region_attr);
}
#else
/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct region index.
 */
static inline int get_region_index_by_type(uint32_t type)
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
		return static_regions_num + THREAD_STACK_REGION;
	case THREAD_STACK_REGION:
	case THREAD_APP_DATA_REGION:
	case THREAD_STACK_GUARD_REGION:
		return static_regions_num + type;
	case THREAD_DOMAIN_PARTITION_REGION:
#if defined(CONFIG_MPU_STACK_GUARD)
		return static_regions_num + type;
#else
		/*
		 * Start domain partition region from stack guard region
		 * since stack guard is not enabled.
		 */
		return static_regions_num + type - 1;
#endif
	default:
		__ASSERT(0, "Unsupported type");
		return -EINVAL;
	}
}

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
static inline int _mpu_configure(uint8_t type, uint32_t base, uint32_t size)
{
	int region_index =  get_region_index_by_type(type);
	uint32_t region_attr = get_region_attr_by_type(type);

	LOG_DBG("Region info: 0x%x 0x%x", base, size);

	if (region_attr == 0U || region_index < 0) {
		return -EINVAL;
	}

	_region_init(region_index, base, size, region_attr);

	return 0;
}
#endif

/* ARC Core MPU Driver API Implementation for ARC MPUv3 */

/**
 * @brief enable the MPU
 */
void arc_core_mpu_enable(void)
{
#ifdef CONFIG_ARC_SECURE_FIRMWARE
/* the default region:
 * secure:0x8000, SID:0x10000, KW:0x100 KR:0x80
 */
#define MPU_ENABLE_ATTR	0x18180
#else
#define MPU_ENABLE_ATTR	0
#endif
	arc_core_mpu_default(MPU_ENABLE_ATTR);
}

/**
 * @brief disable the MPU
 */
void arc_core_mpu_disable(void)
{
	/* MPU is always enabled, use default region to
	 * simulate MPU disable
	 */
	arc_core_mpu_default(REGION_ALL_ATTR | AUX_MPU_ATTR_S |
				AUX_MPU_RPER_SID1);
}

/**
 * @brief configure the thread's mpu regions
 *
 * @param thread the target thread
 */
void arc_core_mpu_configure_thread(struct k_thread *thread)
{
#if defined(CONFIG_MPU_GAP_FILLING)
/* the mpu entries of ARC MPUv4 are divided into 2 parts:
 * static entries: global mpu entries, not changed in context switch
 * dynamic entries: MPU entries changed in context switch and
 * memory domain configure, including:
 *    MPU entries for user thread stack
 *    MPU entries for stack guard
 *    MPU entries for mem domain
 *    MPU entries for other thread specific regions
 * before configuring thread specific mpu entries, need to reset dynamic
 * entries
 */
	_mpu_reset_dynamic_regions();
#endif
#if defined(CONFIG_MPU_STACK_GUARD)
	uint32_t guard_start;

	/* Set location of guard area when the thread is running in
	 * supervisor mode. For a supervisor thread, this is just low
	 * memory in the stack buffer. For a user thread, it only runs
	 * in supervisor mode when handling a system call on the privilege
	 * elevation stack.
	 */
#if defined(CONFIG_USERSPACE)
	if ((thread->base.user_options & K_USER) != 0U) {
		guard_start = thread->arch.priv_stack_start;
	} else
#endif
	{
		guard_start = thread->stack_info.start;
	}
	guard_start -= Z_ARC_STACK_GUARD_SIZE;

	if (_mpu_configure(THREAD_STACK_GUARD_REGION, guard_start,
		Z_ARC_STACK_GUARD_SIZE) < 0) {
		LOG_ERR("thread %p's stack guard failed", thread);
		return;
	}
#endif /* CONFIG_MPU_STACK_GUARD */

#if defined(CONFIG_USERSPACE)
	/* configure stack region of user thread */
	if (thread->base.user_options & K_USER) {
		LOG_DBG("configure user thread %p's stack", thread);
		if (_mpu_configure(THREAD_STACK_USER_REGION,
				   (uint32_t)thread->stack_info.start,
				   thread->stack_info.size) < 0) {
			LOG_ERR("thread %p's stack failed", thread);
			return;
		}
	}

#if defined(CONFIG_MPU_GAP_FILLING)
	uint32_t num_partitions;
	struct k_mem_partition *pparts;
	struct k_mem_domain *mem_domain = thread->mem_domain_info.mem_domain;

	/* configure thread's memory domain */
	if (mem_domain) {
		LOG_DBG("configure thread %p's domain: %p",
		 thread, mem_domain);
		num_partitions = mem_domain->num_partitions;
		pparts = mem_domain->partitions;
	} else {
		num_partitions = 0U;
		pparts = NULL;
	}

	for (uint32_t i = 0; i < num_partitions; i++) {
		if (pparts->size) {
			if (_dynamic_region_allocate_and_init(pparts->start,
				pparts->size, pparts->attr) < 0) {
				LOG_ERR(
				"thread %p's mem region: %p failed",
				 thread, pparts);
				return;
			}
		}
		pparts++;
	}
#else
	arc_core_mpu_configure_mem_domain(thread);
#endif
#endif
}

/**
 * @brief configure the default region
 *
 * @param region_attr region attribute of default region
 */
void arc_core_mpu_default(uint32_t region_attr)
{
#ifdef CONFIG_ARC_NORMAL_FIRMWARE
/* \todo through secure service to access mpu */
#else
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_EN, region_attr);
#endif
}

/**
 * @brief configure the MPU region
 *
 * @param index MPU region index
 * @param base  base address
 * @param size  region size
 * @param region_attr region attribute
 */
int arc_core_mpu_region(uint32_t index, uint32_t base, uint32_t size,
			 uint32_t region_attr)
{
	if (index >= get_num_regions()) {
		return -EINVAL;
	}

	region_attr &= AUX_MPU_RPER_ATTR_MASK;

	_region_init(index, base, size, region_attr);

	return 0;
}

#if defined(CONFIG_USERSPACE)
/**
 * @brief configure MPU regions for the memory partitions of the memory domain
 *
 * @param thread the thread which has memory domain
 */
#if defined(CONFIG_MPU_GAP_FILLING)
void arc_core_mpu_configure_mem_domain(struct k_thread *thread)
{
	arc_core_mpu_configure_thread(thread);
}
#else
void arc_core_mpu_configure_mem_domain(struct k_thread *thread)
{
	uint32_t region_index;
	uint32_t num_partitions;
	uint32_t num_regions;
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

	num_regions = get_num_regions();
	region_index = get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

	while (num_partitions && region_index < num_regions) {
		if (pparts->size > 0) {
			LOG_DBG("set region 0x%x 0x%lx 0x%x",
				region_index, pparts->start, pparts->size);
			_region_init(region_index, pparts->start,
				     pparts->size, pparts->attr);
			region_index++;
		}
		pparts++;
		num_partitions--;
	}

	while (region_index < num_regions) {
		/* clear the left mpu entries */
		_region_init(region_index, 0, 0, 0);
		region_index++;
	}
}
#endif

/**
 * @brief remove MPU regions for the memory partitions of the memory domain
 *
 * @param mem_domain the target memory domain
 */
void arc_core_mpu_remove_mem_domain(struct k_mem_domain *mem_domain)
{
	uint32_t num_partitions;
	struct k_mem_partition *pparts;
	int index;

	if (mem_domain) {
		LOG_DBG("configure domain: %p", mem_domain);
		num_partitions = mem_domain->num_partitions;
		pparts = mem_domain->partitions;
	} else {
		LOG_DBG("disable domain partition regions");
		num_partitions = 0U;
		pparts = NULL;
	}

	for (uint32_t i = 0; i < num_partitions; i++) {
		if (pparts->size) {
			index = _get_region_index(pparts->start,
			 pparts->size);
			if (index > 0) {
#if defined(CONFIG_MPU_GAP_FILLING)
				_region_set_attr(index,
				REGION_KERNEL_RAM_ATTR);
#else
				_region_init(index, 0, 0, 0);
#endif
			}
		}
		pparts++;
	}
}

/**
 * @brief reset MPU region for a single memory partition
 *
 * @param partition_id  memory partition id
 */
void arc_core_mpu_remove_mem_partition(struct k_mem_domain *domain,
			uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];

	int region_index = _get_region_index(partition->start,
			 partition->size);

	if (region_index < 0) {
		return;
	}

	LOG_DBG("remove region 0x%x", region_index);
#if defined(CONFIG_MPU_GAP_FILLING)
	_region_set_attr(region_index, REGION_KERNEL_RAM_ATTR);
#else
	_region_init(region_index, 0, 0, 0);
#endif
}

/**
 * @brief get the maximum number of free regions for memory domain partitions
 */
int arc_core_mpu_get_max_domain_partition_regions(void)
{
#if defined(CONFIG_MPU_GAP_FILLING)
	/* consider the worst case: each partition requires split */
	return (get_num_regions() - MPU_REGION_NUM_FOR_THREAD) / 2;
#else
	return get_num_regions() -
	       get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION) - 1;
#endif
}

/**
 * @brief validate the given buffer is user accessible or not
 */
int arc_core_mpu_buffer_validate(const void *addr, size_t size, int write)
{
	int r_index;
	int key = arch_irq_lock();

	/*
	 * For ARC MPU v4, overlapping is not supported.
	 * we can stop the iteration immediately once we find the
	 * matched region that grants permission or denies access.
	 */
	r_index = _mpu_probe((uint32_t)addr);
	/*  match and the area is in one region */
	if (r_index >= 0 && r_index == _mpu_probe((uint32_t)addr + (size - 1))) {
		if (_is_user_accessible_region(r_index, write)) {
			r_index = 0;
		} else {
			r_index = -EPERM;
		}
	} else {
		r_index = -EPERM;
	}

	arch_irq_unlock(key);

	return r_index;
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
	uint32_t num_regions;
	uint32_t i;

	num_regions = get_num_regions();

	/* ARC MPU supports up to 16 Regions */
	if (mpu_config.num_regions > num_regions) {
		__ASSERT(0,
		"Request to configure: %u regions (supported: %u)\n",
		mpu_config.num_regions, num_regions);
		return;
	}

	static_regions_num = 0U;

	/* Disable MPU */
	arc_core_mpu_disable();

	for (i = 0U; i < mpu_config.num_regions; i++) {
		/* skip empty region */
		if (mpu_config.mpu_regions[i].size == 0) {
			continue;
		}
#if defined(CONFIG_MPU_GAP_FILLING)
		_region_init(static_regions_num,
			     mpu_config.mpu_regions[i].base,
			     mpu_config.mpu_regions[i].size,
			     mpu_config.mpu_regions[i].attr);

		/* record the static region which can be split */
		if (mpu_config.mpu_regions[i].attr & REGION_DYNAMIC) {
			if (dynamic_regions_num >=
			MPU_DYNAMIC_REGION_AREAS_NUM) {
				LOG_ERR("not enough dynamic regions %d",
				 dynamic_regions_num);
				return;
			}

			dyn_reg_info[dynamic_regions_num].index = i;
			dyn_reg_info[dynamic_regions_num].base =
				mpu_config.mpu_regions[i].base;
			dyn_reg_info[dynamic_regions_num].size =
				mpu_config.mpu_regions[i].size;
			dyn_reg_info[dynamic_regions_num].attr =
				mpu_config.mpu_regions[i].attr;

			dynamic_regions_num++;
		}
		static_regions_num++;
#else
		/* dynamic region will be covered by default mpu setting
		 * no need to configure
		 */
		if (!(mpu_config.mpu_regions[i].attr & REGION_DYNAMIC)) {
			_region_init(static_regions_num,
			     mpu_config.mpu_regions[i].base,
			     mpu_config.mpu_regions[i].size,
			     mpu_config.mpu_regions[i].attr);
			static_regions_num++;
		}
#endif
	}

	for (i = static_regions_num; i < num_regions; i++) {
		_region_init(i, 0, 0, 0);
	}

	/* Enable MPU */
	arc_core_mpu_enable();

	return;
}


#endif /* ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V4_INTERNAL_H_ */
