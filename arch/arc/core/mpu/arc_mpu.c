/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arc/v2/aux_regs.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#include <linker/linker-defs.h>
#include <logging/sys_log.h>


#define AUX_MPU_RDB_VALID_MASK (0x1)
#define AUX_MPU_EN_ENABLE   (0x40000000)
#define AUX_MPU_EN_DISABLE  (0xBFFFFFFF)

#define AUX_MPU_RDP_REGION_SIZE(bits)  \
			(((bits - 1) & 0x3) | (((bits - 1) & 0x1C) << 7))

#define AUX_MPU_RDP_ATTR_MASK (0xFFF)

#define _ARC_V2_MPU_EN   (0x409)
#define _ARC_V2_MPU_RDB0 (0x422)
#define _ARC_V2_MPU_RDP0 (0x423)

/* aux regs added in MPU version 3 */
#define _ARC_V2_MPU_INDEX	(0x448) /* MPU index */
#define _ARC_V2_MPU_RSTART	(0x449) /* MPU region start address */
#define _ARC_V2_MPU_REND	(0x44A) /* MPU region end address */
#define _ARC_V2_MPU_RPER	(0x44B) /* MPU region permission register */
#define _ARC_V2_MPU_PROBE	(0x44C) /* MPU probe register */

/* For MPU version 2, the minimum protection region size is 2048 bytes */
/* FOr MPU version 3, the minimum protection region size is 32 bytes */
#if CONFIG_ARC_MPU_VER == 2
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 11
#elif CONFIG_ARC_MPU_VER == 3
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 5
#endif

#define CALC_REGION_END_ADDR(start, size) \
		(start + size - (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS))


/**
 * @brief Get the number of supported MPU regions
 *
 */
static inline u8_t _get_num_regions(void)
{
	u32_t num = _arc_v2_aux_reg_read(_ARC_V2_MPU_BUILD);

	num = (num & 0xFF00) >> 8;

	return (u8_t)num;
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct parameter set.
 */
static inline u32_t _get_region_attr_by_type(u32_t type)
{
	switch (type) {
	case THREAD_STACK_USER_REGION:
		return REGION_RAM_ATTR;
	case THREAD_STACK_REGION:
		return  AUX_MPU_RDP_KW | AUX_MPU_RDP_KR;
	case THREAD_APP_DATA_REGION:
		return REGION_RAM_ATTR;
	case THREAD_STACK_GUARD_REGION:
	/* no Write and Execute to guard region */
		return AUX_MPU_RDP_UR | AUX_MPU_RDP_KR;
	default:
		/* Size 0 region */
		return 0;
	}
}

static inline void _region_init(u32_t index, u32_t region_addr, u32_t size,
			 u32_t region_attr)
{
/* ARC MPU version 2 and version 3 have different aux reg interface */
#if CONFIG_ARC_MPU_VER == 2
	u8_t bits = find_msb_set(size) - 1;
	index = 2 * index;

	if (bits < ARC_FEATURE_MPU_ALIGNMENT_BITS) {
		bits = ARC_FEATURE_MPU_ALIGNMENT_BITS;
	}

	if ((1 << bits) < size) {
		bits++;
	}

	if (size > 0) {
		region_attr |= AUX_MPU_RDP_REGION_SIZE(bits);
		region_addr |= AUX_MPU_RDB_VALID_MASK;
	} else {
		region_addr = 0;
	}

	_arc_v2_aux_reg_write(_ARC_V2_MPU_RDP0 + index, region_attr);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RDB0 + index, region_addr);

#elif CONFIG_ARC_MPU_VER == 3
#define AUX_MPU_RPER_SID1	0x10000
	if (size < (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS)) {
		size = (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS);
	}

/* all MPU regions SID are the same: 1, the default SID */
	if (region_attr) {
		region_attr |=  (AUX_MPU_RDB_VALID_MASK | AUX_MPU_RDP_S |
				 AUX_MPU_RPER_SID1);
	}

	_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RSTART, region_addr);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_REND,
				CALC_REGION_END_ADDR(region_addr, size));
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RPER, region_attr);
#endif
}

#if CONFIG_ARC_MPU_VER == 3
static inline s32_t _mpu_probe(u32_t addr)
{
	u32_t val;

	_arc_v2_aux_reg_write(_ARC_V2_MPU_PROBE, addr);
	val = _arc_v2_aux_reg_read(_ARC_V2_MPU_INDEX);

	/* if no match or multiple regions match, return error */
	if (val & 0xC0000000) {
		return -1;
	} else {
		return val;
	}
}
#endif

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct region index.
 */
static inline u32_t _get_region_index_by_type(u32_t type)
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
	 * For ARC MPU v3, each index has the same priority, so the index is
	 * allocated from small to big. Static regions start from 0, then
	 * thread related regions.
	 */
	switch (type) {
#if CONFIG_ARC_MPU_VER  == 2
	case THREAD_STACK_USER_REGION:
		return _get_num_regions() - mpu_config.num_regions
		 - THREAD_STACK_REGION;
	case THREAD_STACK_REGION:
	case THREAD_APP_DATA_REGION:
	case THREAD_STACK_GUARD_REGION:
		return _get_num_regions() - mpu_config.num_regions - type;
	case THREAD_DOMAIN_PARTITION_REGION:
#if defined(CONFIG_MPU_STACK_GUARD)
		return _get_num_regions() - mpu_config.num_regions - type;
#else
		/*
		 * Start domain partition region from stack guard region
		 * since stack guard is not enabled.
		 */
		return _get_num_regions() - mpu_config.num_regions - type + 1;
#endif
#elif CONFIG_ARC_MPU_VER == 3
	case THREAD_STACK_USER_REGION:
		return mpu_config.num_regions + THREAD_STACK_REGION - 1;
	case THREAD_STACK_REGION:
	case THREAD_APP_DATA_REGION:
	case THREAD_STACK_GUARD_REGION:
		return mpu_config.num_regions + type - 1;
	case THREAD_DOMAIN_PARTITION_REGION:
#if defined(CONFIG_MPU_STACK_GUARD)
		return mpu_config.num_regions + type - 1;
#else
		/*
		 * Start domain partition region from stack guard region
		 * since stack guard is not enabled.
		 */
		return mpu_config.num_regions + type - 2;
#endif
#endif
	default:
		__ASSERT(0, "Unsupported type");
		return 0;
	}
}

/**
 * This internal function checks if region is enabled or not
 */
static inline int _is_enabled_region(u32_t r_index)
{
#if CONFIG_ARC_MPU_VER == 2
	return ((_arc_v2_aux_reg_read(_ARC_V2_MPU_RDB0 + 2 * r_index)
		& AUX_MPU_RDB_VALID_MASK) == AUX_MPU_RDB_VALID_MASK);
#elif CONFIG_ARC_MPU_VER == 3
	_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, r_index);
	return ((_arc_v2_aux_reg_read(_ARC_V2_MPU_RPER) &
		 AUX_MPU_RDB_VALID_MASK) == AUX_MPU_RDB_VALID_MASK);
#endif
}

/**
 * This internal function check if the given buffer in in the region
 */
static inline int _is_in_region(u32_t r_index, u32_t start, u32_t size)
{
#if CONFIG_ARC_MPU_VER == 2
	u32_t r_addr_start;
	u32_t r_addr_end;
	u32_t r_size_lshift;

	r_addr_start = _arc_v2_aux_reg_read(_ARC_V2_MPU_RDB0 + 2 * r_index)
			& (~AUX_MPU_RDB_VALID_MASK);
	r_size_lshift = _arc_v2_aux_reg_read(_ARC_V2_MPU_RDP0 + 2 * r_index)
			& AUX_MPU_RDP_ATTR_MASK;
	r_size_lshift = (r_size_lshift & 0x3) | ((r_size_lshift >> 7) & 0x1C);
	r_addr_end = r_addr_start  + (1 << (r_size_lshift + 1));

	if (start >= r_addr_start && (start + size) < r_addr_end) {
		return 1;
	}

#elif CONFIG_ARC_MPU_VER == 3

	if ((r_index == _mpu_probe(start)) &&
		(r_index == _mpu_probe(start + size))) {
		return 1;
	}
#endif



	return 0;
}

/**
 * This internal function check if the region is user accessible or not
 */
static inline int _is_user_accessible_region(u32_t r_index, int write)
{
	u32_t r_ap;

#if CONFIG_ARC_MPU_VER == 2
	r_ap = _arc_v2_aux_reg_read(_ARC_V2_MPU_RDP0 + 2 * r_index);
#elif CONFIG_ARC_MPU_VER == 3
	_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, r_index);
	r_ap = _arc_v2_aux_reg_read(_ARC_V2_MPU_RPER);
#endif
	r_ap &= AUX_MPU_RDP_ATTR_MASK;

	if (write) {
		return ((r_ap & (AUX_MPU_RDP_UW | AUX_MPU_RDP_KW)) ==
			(AUX_MPU_RDP_UW | AUX_MPU_RDP_KW));
	}

	return ((r_ap & (AUX_MPU_RDP_UR | AUX_MPU_RDP_KR)) ==
			(AUX_MPU_RDP_UR | AUX_MPU_RDP_KR));
}

/* ARC Core MPU Driver API Implementation for ARC MPU */

/**
 * @brief enable the MPU
 */
void arc_core_mpu_enable(void)
{
#if CONFIG_ARC_MPU_VER == 2
	/* Enable MPU */
	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) | AUX_MPU_EN_ENABLE);

	/* MPU is always enabled, use default region to
	 * simulate MPU enable
	 */
#elif CONFIG_ARC_MPU_VER == 3
#define MPU_ENABLE_ATTR   0
	arc_core_mpu_default(MPU_ENABLE_ATTR);
#endif
}

/**
 * @brief disable the MPU
 */
void arc_core_mpu_disable(void)
{
#if CONFIG_ARC_MPU_VER == 2
	/* Disable MPU */
	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) & AUX_MPU_EN_DISABLE);
#elif CONFIG_ARC_MPU_VER == 3
	/* MPU is always enabled, use default region to
	 * simulate MPU disable
	 */
	arc_core_mpu_default(REGION_ALL_ATTR);
#endif
}

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arc_core_mpu_configure(u8_t type, u32_t base, u32_t size)
{
	u32_t region_index =  _get_region_index_by_type(type);
	u32_t region_attr = _get_region_attr_by_type(type);

	SYS_LOG_DBG("Region info: 0x%x 0x%x", base, size);

	if (region_attr == 0) {
		return;
	}
	/*
	 * The new MPU regions are allocated per type before
	 * the statically configured regions.
	 */
#if CONFIG_ARC_MPU_VER == 2
	/*
	 * For ARC MPU v2, MPU regions can be overlapped, smaller
	 * region index has higher priority.
	 */
	_region_init(region_index, base, size, region_attr);
#elif CONFIG_ARC_MPU_VER == 3
	static s32_t last_index;
	s32_t index;
	u32_t last_region = _get_num_regions() - 1;


	/* use hardware probe to find the region maybe split.
	 * another way is to look up the mpu_config.mpu_regions
	 * in software, which is time consuming.
	 */
	index = _mpu_probe(base);

	/* ARC MPU version 3 doesn't support region overlap.
	 * So it can not be directly used for stack/stack guard protect
	 * One way to do this is splitting the ram region as follow:
	 *
	 *  Take THREAD_STACK_GUARD_REGION as example:
	 *  RAM region 0: the ram region before THREAD_STACK_GUARD_REGION, rw
	 *  RAM THREAD_STACK_GUARD_REGION: RO
	 *  RAM region 1: the region after THREAD_STACK_GUARD_REGION, same
	 *                as region 0
	 *  if region_index == index, it means the same thread comes back,
	 *  no need to split
	 */

	if (index >= 0 && region_index != index) {
		/* need to split, only 1 split is allowed */
		/* find the correct region to mpu_config.mpu_regions */
		if (index == last_region) {
			/* already split */
			index = last_index;
		} else {
			/* new split */
			last_index = index;
		}

		_region_init(index,
			mpu_config.mpu_regions[index].base,
			base - mpu_config.mpu_regions[index].base,
			mpu_config.mpu_regions[index].attr);

#if defined(CONFIG_MPU_STACK_GUARD)
		if (type != THREAD_STACK_USER_REGION)
			/*
			 * USER REGION is continuous with MPU_STACK_GUARD.
			 * In current implementation, THREAD_STACK_GUARD_REGION is
			 * configured before THREAD_STACK_USER_REGION
			 */
#endif
			_region_init(last_region, base + size,
				(mpu_config.mpu_regions[index].base +
				mpu_config.mpu_regions[index].size - base - size),
				mpu_config.mpu_regions[index].attr);
	}

	_region_init(region_index, base, size, region_attr);
#endif
}

/**
 * @brief configure the default region
 *
 * @param   region_attr region attribute of default region
 */
void arc_core_mpu_default(u32_t region_attr)
{
	u32_t val =  _arc_v2_aux_reg_read(_ARC_V2_MPU_EN) &
			(~AUX_MPU_RDP_ATTR_MASK);

	region_attr &= AUX_MPU_RDP_ATTR_MASK;

	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN, region_attr | val);
}

/**
 * @brief configure the MPU region
 *
 * @param   index   MPU region index
 * @param   base    base address
 * @param   region_attr region attribute
 */
void arc_core_mpu_region(u32_t index, u32_t base, u32_t size,
			u32_t region_attr)
{
	if (index >= _get_num_regions()) {
		return;
	}

	region_attr &= AUX_MPU_RDP_ATTR_MASK;

	_region_init(index, base, size, region_attr);
}

#if defined(CONFIG_USERSPACE)
void arc_core_mpu_configure_user_context(struct k_thread *thread)
{
	u32_t base = (u32_t)thread->stack_obj;
	u32_t size = thread->stack_info.size;

	/* for kernel threads, no need to configure user context */
	if (!thread->arch.priv_stack_start) {
		return;
	}

	arc_core_mpu_configure(THREAD_STACK_USER_REGION, base, size);

	/* configure app data portion */
#ifdef CONFIG_APPLICATION_MEMORY
#if CONFIG_ARC_MPU_VER == 2
	/*
	 * _app_ram_size is guaranteed to be power of two, and
	 * _app_ram_start is guaranteed to be aligned _app_ram_size
	 * in linker template
	 */
	base = (u32_t)&__app_ram_start;
	size = (u32_t)&__app_ram_size;

	/* set up app data region if exists, otherwise disable */
	if (size > 0) {
		arc_core_mpu_configure(THREAD_APP_DATA_REGION, base, size);
	}
#elif CONFIG_ARC_MPU_VER == 3
	/*
	 * ARC MPV v3 doesn't support MPU region overlap.
	 * Application memory should be a static memory, defined in mpu_config
	 */
#endif
#endif
}

/**
 * @brief configure MPU regions for the memory partitions of the memory domain
 *
 * @param   mem_domain    memory domain that thread belongs to
 */
void arc_core_mpu_configure_mem_domain(struct k_mem_domain *mem_domain)
{
	s32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
	u32_t num_partitions;
	struct k_mem_partition *pparts;

	if (mem_domain) {
		SYS_LOG_DBG("configure domain: %p", mem_domain);
		num_partitions = mem_domain->num_partitions;
		pparts = mem_domain->partitions;
	} else {
		SYS_LOG_DBG("disable domain partition regions");
		num_partitions = 0;
		pparts = NULL;
	}
#if CONFIG_ARC_MPU_VER == 2
	for (; region_index >= 0; region_index--) {
#elif CONFIG_ARC_MPU_VER == 3
/*
 * Note: For ARC MPU v3, overlapping is not allowed, so the following
 * partitions/region may be overlapped with each other or regions in
 * mpu_config. This will cause EV_MachineCheck exception (ECR = 0x030600).
 * Although split mechanism is used for stack guard region to avoid this,
 * it doesn't work for memory domain, because the dynamic region numbers.
 * So be careful to avoid the overlap situation.
 */
	u32_t num_regions = _get_num_regions() - 1;
	for (; region_index < num_regions; region_index++) {
#endif
		if (num_partitions && pparts->size) {
			SYS_LOG_DBG("set region 0x%x 0x%x 0x%x",
				    region_index, pparts->start, pparts->size);
			_region_init(region_index, pparts->start, pparts->size,
					pparts->attr);
			num_partitions--;
		} else {
			SYS_LOG_DBG("disable region 0x%x", region_index);
			/* Disable region */
			_region_init(region_index, 0, 0, 0);
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
void arc_core_mpu_configure_mem_partition(u32_t part_index,
					  struct k_mem_partition *part)
{
	u32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

	SYS_LOG_DBG("configure partition index: %u", part_index);

	if (part) {
		SYS_LOG_DBG("set region 0x%x 0x%x 0x%x",
			    region_index + part_index, part->start, part->size);
		_region_init(region_index, part->start, part->size,
				part->attr);
	} else {
		SYS_LOG_DBG("disable region 0x%x", region_index + part_index);
		/* Disable region */
		_region_init(region_index + part_index, 0, 0, 0);
	}
}

/**
 * @brief Reset MPU region for a single memory partition
 *
 * @param   part_index  memory partition index
 */
void arc_core_mpu_mem_partition_remove(u32_t part_index)
{
	u32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);

	SYS_LOG_DBG("disable region 0x%x", region_index + part_index);
	/* Disable region */
	_region_init(region_index + part_index, 0, 0, 0);
}

/**
 * @brief get the maximum number of free regions for memory domain partitions
 */
int arc_core_mpu_get_max_domain_partition_regions(void)
{
#if CONFIG_ARC_MPU_VER == 2
	return _get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION) + 1;
#elif CONFIG_ARC_MPU_VER == 3
	/*
	 * Subtract the start of domain partition regions and 1 reserved region
	 * from total regions will get the maximum number of free regions for
	 * memory domain partitions.
	 */
	return _get_num_regions() -
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION) - 1;
#endif
}

/**
 * @brief validate the given buffer is user accessible or not
 */
int arc_core_mpu_buffer_validate(void *addr, size_t size, int write)
{
	s32_t r_index;


	/*
	 * For ARC MPU v2, smaller region number takes priority.
	 * we can stop the iteration immediately once we find the
	 * matched region that grants permission or denies access.
	 *
	 * For ARC MPU v3, overlapping is not supported.
	 * we can stop the iteration immediately once we find the
	 * matched region that grants permission or denies access.
	 */
#if CONFIG_ARC_MPU_VER == 2
	for (r_index = 0; r_index < _get_num_regions(); r_index++) {
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
#elif CONFIG_ARC_MPU_VER == 3
	r_index = _mpu_probe((u32_t)addr);
	/*  match and the area is in one region */
	if (r_index >= 0 && r_index == _mpu_probe((u32_t)addr + size)) {
		if (_is_user_accessible_region(r_index, write)) {
			return 0;
		} else {
			return -EPERM;
		}
	}
#endif

	return -EPERM;
}
#endif /* CONFIG_USERSPACE */

/* ARC MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static void _arc_mpu_config(void)
{
	u32_t num_regions;
	u32_t i;

	num_regions = _get_num_regions();

	/* ARC MPU supports up to 16 Regions */
	if (mpu_config.num_regions > num_regions) {
		return;
	}

	/* Disable MPU */
	arc_core_mpu_disable();

#if CONFIG_ARC_MPU_VER == 2
	u32_t r_index;
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
	for (i = 0; i < r_index; i++) {
		_region_init(i, 0, 0, 0);
	}

	/* configure the static regions */
	for (i = 0; i < mpu_config.num_regions; i++) {
		_region_init(r_index,
			mpu_config.mpu_regions[i].base,
			mpu_config.mpu_regions[i].size,
			mpu_config.mpu_regions[i].attr);
		r_index++;
	}

	/* default region: no read, write and execute */
	arc_core_mpu_default(0);

#elif CONFIG_ARC_MPU_VER == 3
	for (i = 0; i < mpu_config.num_regions; i++) {
		_region_init(i,
			mpu_config.mpu_regions[i].base,
			mpu_config.mpu_regions[i].size,
			mpu_config.mpu_regions[i].attr);
	}

	for (; i < num_regions; i++) {
		_region_init(i, 0, 0, 0);
	}
#endif
	/* Enable MPU */
	arc_core_mpu_enable();
}

static int arc_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_arc_mpu_config();

	return 0;
}

SYS_INIT(arc_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
