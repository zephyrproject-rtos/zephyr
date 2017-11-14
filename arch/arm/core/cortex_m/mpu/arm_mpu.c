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
#include <logging/sys_log.h>

#define ARM_MPU_DEV ((volatile struct arm_mpu *) ARM_MPU_BASE)

/* ARM MPU Enabled state */
static u8_t arm_mpu_enabled;

/**
 * The attributes referenced in this function are described at:
 * https://goo.gl/hMry3r
 * This function is private to the driver.
 */
static inline u32_t _get_region_attr(u32_t xn, u32_t ap, u32_t tex,
				     u32_t c, u32_t b, u32_t s,
				     u32_t srd, u32_t size)
{
	return ((xn << 28) | (ap) | (tex << 19) | (s << 18)
		| (c << 17) | (b << 16) | (srd << 5) | (size));
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct parameter set.
 */
static inline u32_t _get_region_attr_by_type(u32_t type, u32_t size)
{
	switch (type) {
	case THREAD_STACK_REGION:
		return 0;
	case THREAD_STACK_GUARD_REGION:
		return _get_region_attr(1, P_RO_U_RO, 0, 1, 0,
					1, 0, REGION_32B);
	default:
		/* Size 0 region */
		return 0;
	}
}

static inline u8_t _get_num_regions(void)
{
	u32_t type = ARM_MPU_DEV->type;

	type = (type & 0xFF00) >> 8;

	return (u8_t)type;
}

static void _region_init(u32_t index, u32_t region_addr,
			 u32_t region_attr)
{
	/* Select the region you want to access */
	ARM_MPU_DEV->rnr = index;
	/* Configure the region */
	ARM_MPU_DEV->rbar = (region_addr & REGION_BASE_ADDR_MASK)
				| REGION_VALID | index;
	ARM_MPU_DEV->rasr = region_attr | REGION_ENABLE;
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct region index.
 */
static inline u32_t _get_region_index_by_type(u32_t type)
{
	/*
	 * The new MPU regions are allocated per type after the statically
	 * configured regions. The type is one-indexed rather than
	 * zero-indexed, therefore we need to subtract by one to get the region
	 * index.
	 */
	switch (type) {
	case THREAD_STACK_REGION:
		return mpu_config.num_regions + type - 1;
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
	default:
		__ASSERT(0, "Unsupported type");
		return 0;
	}
}

static inline u32_t round_up_to_next_power_of_two(u32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

/**
 * This internal function converts the region size to
 * the SIZE field value of MPU_RASR.
 */
static inline u32_t _size_to_mpu_rasr_size(u32_t size)
{
	/* The minimal supported region size is 32 bytes */
	if (size <= 32) {
		return REGION_32B;
	}

	/*
	 * A size value greater than 2^31 could not be handled by
	 * round_up_to_next_power_of_two() properly. We handle
	 * it separately here.
	 */
	if (size > (1 << 31)) {
		return REGION_4G;
	}

	size = round_up_to_next_power_of_two(size);

	return (find_msb_set(size) - 2) << 1;
}

/**
 * This internal function check if region is enabled or not
 */
static inline int _is_enabled_region(u32_t r_index)
{
	ARM_MPU_DEV->rnr = r_index;

	return ARM_MPU_DEV->rasr & REGION_ENABLE_MASK;
}

/**
 * This internal function check if the given buffer in in the region
 */
static inline int _is_in_region(u32_t r_index, u32_t start, u32_t size)
{
	u32_t r_addr_start;
	u32_t r_size_lshift;
	u32_t r_addr_end;

	ARM_MPU_DEV->rnr = r_index;
	r_addr_start = ARM_MPU_DEV->rbar & REGION_BASE_ADDR_MASK;
	r_size_lshift = ((ARM_MPU_DEV->rasr & REGION_SIZE_MASK) >>
			REGION_SIZE_OFFSET) + 1;
	r_addr_end = r_addr_start + (1 << r_size_lshift) - 1;

	if (start >= r_addr_start && (start + size - 1) <= r_addr_end) {
		return 1;
	}

	return 0;
}

/**
 * This internal function check if the region is user accessible or not
 */
static inline int _is_user_accessible_region(u32_t r_index, int write)
{
	u32_t r_ap;

	ARM_MPU_DEV->rnr = r_index;
	r_ap = ARM_MPU_DEV->rasr & ACCESS_PERMS_MASK;

	if (write) {
		return r_ap == P_RW_U_RW;
	}

	/* For all user accessible permissions, their AP[1] bit is l */
	return r_ap & (0x2 << ACCESS_PERMS_OFFSET);
}

/* ARM Core MPU Driver API Implementation for ARM MPU */

/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void)
{
	if (arm_mpu_enabled == 0) {
		/* Enable MPU */
		ARM_MPU_DEV->ctrl = ARM_MPU_ENABLE | ARM_MPU_PRIVDEFENA;

		arm_mpu_enabled = 1;
	}
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	if (arm_mpu_enabled == 1) {
		/* Disable MPU */
		ARM_MPU_DEV->ctrl = 0;

		arm_mpu_enabled = 0;
	}
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
	SYS_LOG_DBG("Region info: 0x%x 0x%x", base, size);
	u32_t region_index = _get_region_index_by_type(type);
	u32_t region_attr = _get_region_attr_by_type(type, size);

	/* ARM MPU supports up to 16 Regions */
	if (region_index > _get_num_regions()) {
		return;
	}

	_region_init(region_index, base, region_attr);
}

#if defined(CONFIG_USERSPACE)
/**
 * @brief configure MPU regions for the memory partitions of the memory domain
 *
 * @param   mem_domain    memory domain that thread belongs to
 */
void arm_core_mpu_configure_mem_domain(struct k_mem_domain *mem_domain)
{
	u32_t region_index =
		_get_region_index_by_type(THREAD_DOMAIN_PARTITION_REGION);
	u32_t region_attr;
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

	for (; region_index < _get_num_regions(); region_index++) {
		if (num_partitions && pparts->size) {
			SYS_LOG_DBG("set region 0x%x 0x%x 0x%x",
				    region_index, pparts->start, pparts->size);
			region_attr = pparts->attr |
				      _size_to_mpu_rasr_size(pparts->size);
			_region_init(region_index, pparts->start, region_attr);
			num_partitions--;
		} else {
			SYS_LOG_DBG("disable region 0x%x", region_index);
			/* Disable region */
			ARM_MPU_DEV->rnr = region_index;
			ARM_MPU_DEV->rbar = 0;
			ARM_MPU_DEV->rasr = 0;
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
	u32_t region_attr;

	SYS_LOG_DBG("configure partition index: %u", part_index);

	if (part) {
		SYS_LOG_DBG("set region 0x%x 0x%x 0x%x",
			    region_index + part_index, part->start, part->size);
		region_attr = part->attr | _size_to_mpu_rasr_size(part->size);
		_region_init(region_index + part_index, part->start,
			     region_attr);
	} else {
		SYS_LOG_DBG("disable region 0x%x", region_index + part_index);
		/* Disable region */
		ARM_MPU_DEV->rnr = region_index + part_index;
		ARM_MPU_DEV->rbar = 0;
		ARM_MPU_DEV->rasr = 0;
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

	SYS_LOG_DBG("disable region 0x%x", region_index + part_index);
	/* Disable region */
	ARM_MPU_DEV->rnr = region_index + part_index;
	ARM_MPU_DEV->rbar = 0;
	ARM_MPU_DEV->rasr = 0;
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
 */
int arm_core_mpu_buffer_validate(void *addr, size_t size, int write)
{
	u32_t r_index;

	/* Iterate all mpu regions in reversed order */
	for (r_index = _get_num_regions() + 1; r_index-- > 0;) {
		if (!_is_enabled_region(r_index) ||
		    !_is_in_region(r_index, (u32_t)addr, size)) {
			continue;
		}

		/* For ARM MPU, higher region number takes priority.
		 * Since we iterate all mpu regions in reversed order, so
		 * we can stop the iteration immediately once we find the
		 * matched region that grants permission or denies access.
		 */
		if (_is_user_accessible_region(r_index, write)) {
			return 0;
		} else {
			return -EPERM;
		}
	}

	return -EPERM;
}
#endif /* CONFIG_USERSPACE */

/* ARM MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static void _arm_mpu_config(void)
{
	u32_t r_index;

	/* ARM MPU supports up to 16 Regions */
	if (mpu_config.num_regions > _get_num_regions()) {
		return;
	}

	/* Disable MPU */
	ARM_MPU_DEV->ctrl = 0;

	/* Configure regions */
	for (r_index = 0; r_index < mpu_config.num_regions; r_index++) {
		_region_init(r_index,
			     mpu_config.mpu_regions[r_index].base,
			     mpu_config.mpu_regions[r_index].attr);
	}

	/* Enable MPU */
	ARM_MPU_DEV->ctrl = ARM_MPU_ENABLE | ARM_MPU_PRIVDEFENA;

	arm_mpu_enabled = 1;

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

static int arm_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_arm_mpu_config();

	return 0;
}

SYS_INIT(arm_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
