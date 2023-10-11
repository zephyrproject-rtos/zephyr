/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <kernel_arch_func.h>
#include <zephyr/arch/arm64/mm.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
#include <kernel_internal.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>

LOG_MODULE_REGISTER(mpu, CONFIG_MPU_LOG_LEVEL);

#define NODE_HAS_PROP_AND_OR(node_id, prop) \
	DT_NODE_HAS_PROP(node_id, prop) ||

BUILD_ASSERT((DT_FOREACH_STATUS_OKAY_NODE_VARGS(
	      NODE_HAS_PROP_AND_OR, zephyr_memory_region_mpu) false) == false,
	      "`zephyr,memory-region-mpu` was deprecated in favor of `zephyr,memory-attr`");

#define MPU_DYNAMIC_REGION_AREAS_NUM	3

#if defined(CONFIG_USERSPACE) || defined(CONFIG_ARM64_STACK_PROTECTION)
static struct dynamic_region_info
	      sys_dyn_regions[CONFIG_MP_MAX_NUM_CPUS][MPU_DYNAMIC_REGION_AREAS_NUM];
static int sys_dyn_regions_num[CONFIG_MP_MAX_NUM_CPUS];

static void dynamic_regions_init(void);
static int dynamic_areas_init(uintptr_t start, size_t size);
static int flush_dynamic_regions_to_mpu(struct dynamic_region_info *dyn_regions,
					uint8_t region_num);

#if defined(CONFIG_USERSPACE)
#define MPU_DYNAMIC_REGIONS_AREA_START ((uintptr_t)&_app_smem_start)
#else
#define MPU_DYNAMIC_REGIONS_AREA_START ((uintptr_t)&__kernel_ram_start)
#endif

#define MPU_DYNAMIC_REGIONS_AREA_SIZE  ((size_t)((uintptr_t)&__kernel_ram_end - \
	MPU_DYNAMIC_REGIONS_AREA_START))
#endif

/*
 * AArch64 Memory Model Feature Register 0
 * Provides information about the implemented memory model and memory
 * management support in AArch64 state.
 * See Arm Architecture Reference Manual Supplement
 *  Armv8, for Armv8-R AArch64 architecture profile, G1.3.7
 *
 * ID_AA64MMFR0_MSA_FRAC, bits[55:52]
 * ID_AA64MMFR0_MSA, bits [51:48]
 */
#define ID_AA64MMFR0_MSA_msk		(0xFFUL << 48U)
#define ID_AA64MMFR0_PMSA_EN		(0x1FUL << 48U)
#define ID_AA64MMFR0_PMSA_VMSA_EN	(0x2FUL << 48U)

/*
 * Global status variable holding the number of HW MPU region indices, which
 * have been reserved by the MPU driver to program the static (fixed) memory
 * regions.
 */
static uint8_t static_regions_num;

/* Get the number of supported MPU regions. */
static ALWAYS_INLINE uint8_t get_num_regions(void)
{
	uint64_t type;

	type = read_mpuir_el1();
	type = type & MPU_IR_REGION_Msk;

	return (uint8_t)type;
}

/* ARM Core MPU Driver API Implementation for ARM MPU */

/**
 * @brief enable the MPU
 *
 * On the SMP system, The function that enables MPU can not insert stack protector
 * code because the canary values read by the secondary CPUs before enabling MPU
 * and after enabling it are not equal due to cache coherence issues.
 */
FUNC_NO_STACK_PROTECTOR void arm_core_mpu_enable(void)
{
	uint64_t val;

	val = read_sctlr_el1();
	val |= SCTLR_M_BIT;
	write_sctlr_el1(val);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	uint64_t val;

	/* Force any outstanding transfers to complete before disabling MPU */
	barrier_dmem_fence_full();

	val = read_sctlr_el1();
	val &= ~SCTLR_M_BIT;
	write_sctlr_el1(val);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/* ARM MPU Driver Initial Setup
 *
 * Configure the cache-ability attributes for all the
 * different types of memory regions.
 */
static void mpu_init(void)
{
	/* Device region(s): Attribute-0
	 * Flash region(s): Attribute-1
	 * SRAM region(s): Attribute-2
	 * SRAM no cache-able regions(s): Attribute-3
	 */
	uint64_t mair = MPU_MAIR_ATTRS;

	write_mair_el1(mair);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/*
 * Changing the MPU region may change the cache related attribute and cause
 * cache coherence issues, so it's necessary to avoid invoking functions in such
 * critical scope to avoid memory access before the MPU regions are all configured.
 */
static ALWAYS_INLINE void mpu_set_region(uint32_t rnr, uint64_t rbar,
				  uint64_t rlar)
{
	write_prselr_el1(rnr);
	barrier_dsync_fence_full();
	write_prbar_el1(rbar);
	write_prlar_el1(rlar);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static ALWAYS_INLINE void mpu_clr_region(uint32_t rnr)
{
	write_prselr_el1(rnr);
	barrier_dsync_fence_full();
	/*
	 * Have to set limit register first as the enable/disable bit of the
	 * region is in the limit register.
	 */
	write_prlar_el1(0);
	write_prbar_el1(0);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/*
 * This internal functions performs MPU region initialization.
 *
 * Changing the MPU region may change the cache related attribute and cause
 * cache coherence issues, so it's necessary to avoid invoking functions in such
 * critical scope to avoid memory access before the MPU regions are all configured.
 */
static ALWAYS_INLINE void region_init(const uint32_t index,
			const struct arm_mpu_region *region_conf)
{
	uint64_t rbar = region_conf->base & MPU_RBAR_BASE_Msk;
	uint64_t rlar = (region_conf->limit - 1) & MPU_RLAR_LIMIT_Msk;

	rbar |= region_conf->attr.rbar &
		(MPU_RBAR_XN_Msk | MPU_RBAR_AP_Msk | MPU_RBAR_SH_Msk);
	rlar |= (region_conf->attr.mair_idx << MPU_RLAR_AttrIndx_Pos) &
		MPU_RLAR_AttrIndx_Msk;
	rlar |= MPU_RLAR_EN_Msk;

	mpu_set_region(index, rbar, rlar);
}

#define _BUILD_REGION_CONF(reg, _ATTR)						\
	(struct arm_mpu_region) { .name  = (reg).dt_name,			\
				  .base  = (reg).dt_addr,			\
				  .limit = (reg).dt_addr + (reg).dt_size,	\
				  .attr  = _ATTR,				\
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
		struct arm_mpu_region region_conf;

		switch (DT_MEM_ARM_GET(region[idx].dt_attr)) {
		case DT_MEM_ARM_MPU_RAM:
			region_conf = _BUILD_REGION_CONF(region[idx], REGION_RAM_ATTR);
			break;
#ifdef REGION_RAM_NOCACHE_ATTR
		case DT_MEM_ARM_MPU_RAM_NOCACHE:
			region_conf = _BUILD_REGION_CONF(region[idx], REGION_RAM_NOCACHE_ATTR);
			__ASSERT(!(region[idx].dt_attr & DT_MEM_CACHEABLE),
				 "RAM_NOCACHE with DT_MEM_CACHEABLE attribute\n");
			break;
#endif
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

		region_init((*reg_index), (const struct arm_mpu_region *) &region_conf);

		(*reg_index)++;
	}

	return 0;
}

/*
 * @brief MPU default configuration
 *
 * This function here provides the default configuration mechanism
 * for the Memory Protection Unit (MPU).
 *
 * On the SMP system, The function that enables MPU can not insert stack protector
 * code because the canary values read by the secondary CPUs before enabling MPU
 * and after enabling it are not equal due to cache coherence issues.
 */
FUNC_NO_STACK_PROTECTOR void z_arm64_mm_init(bool is_primary_core)
{
	uint64_t val;
	uint32_t r_index;
	uint8_t tmp_static_num;

	/* Current MPU code supports only EL1 */
	val = read_currentel();
	__ASSERT(GET_EL(val) == MODE_EL1,
		 "Exception level not EL1, MPU not enabled!\n");

	/* Check whether the processor supports MPU */
	val = read_id_aa64mmfr0_el1() & ID_AA64MMFR0_MSA_msk;
	if ((val != ID_AA64MMFR0_PMSA_EN) &&
	    (val != ID_AA64MMFR0_PMSA_VMSA_EN)) {
		__ASSERT(0, "MPU not supported!\n");
		return;
	}

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
			 get_num_regions());
		return;
	}

	arm_core_mpu_disable();

	/* Architecture-specific configuration */
	mpu_init();

	/* Program fixed regions configured at SOC definition. */
	for (r_index = 0U; r_index < mpu_config.num_regions; r_index++) {
		region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

	/* Update the number of programmed MPU regions. */
	tmp_static_num = mpu_config.num_regions;

	/* DT-defined MPU regions. */
	if (mpu_configure_regions_from_dt(&tmp_static_num) == -EINVAL) {
		__ASSERT(0, "Failed to allocate MPU regions from DT\n");
		return;
	}

	arm_core_mpu_enable();

	if (!is_primary_core) {
		/*
		 * primary core might reprogram the sys_regions, so secondary cores
		 * should re-flush the sys regions
		 */
		goto out;
	}

	/* Only primary core init the static_regions_num */
	static_regions_num = tmp_static_num;

#if defined(CONFIG_USERSPACE) || defined(CONFIG_ARM64_STACK_PROTECTION)
	dynamic_regions_init();
	/* Only primary core do the dynamic_areas_init. */
	int rc = dynamic_areas_init(MPU_DYNAMIC_REGIONS_AREA_START,
				    MPU_DYNAMIC_REGIONS_AREA_SIZE);
	if (rc < 0) {
		__ASSERT(0, "Dynamic areas init fail");
		return;
	}
#endif

out:
#if defined(CONFIG_ARM64_STACK_PROTECTION)
	(void)flush_dynamic_regions_to_mpu(sys_dyn_regions[arch_curr_cpu()->id],
					   sys_dyn_regions_num[arch_curr_cpu()->id]);
#endif
	return;
}

#if defined(CONFIG_USERSPACE) || defined(CONFIG_ARM64_STACK_PROTECTION)
static int insert_region(struct dynamic_region_info *dyn_regions, uint8_t region_num,
			 uintptr_t start, size_t size, struct arm_mpu_region_attr *attr);

static void arm_core_mpu_background_region_enable(void)
{
	uint64_t val;

	val = read_sctlr_el1();
	val |= SCTLR_BR_BIT;
	write_sctlr_el1(val);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static void arm_core_mpu_background_region_disable(void)
{
	uint64_t val;

	/* Force any outstanding transfers to complete before disabling MPU */
	barrier_dmem_fence_full();
	val = read_sctlr_el1();
	val &= ~SCTLR_BR_BIT;
	write_sctlr_el1(val);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static void dynamic_regions_init(void)
{
	for (int cpuid = 0; cpuid < arch_num_cpus(); cpuid++) {
		for (int i = 0; i < MPU_DYNAMIC_REGION_AREAS_NUM; i++) {
			sys_dyn_regions[cpuid][i].index = -1;
		}
	}
}

static int dynamic_areas_init(uintptr_t start, size_t size)
{
	const struct arm_mpu_region *region;
	struct dynamic_region_info *tmp_info;
	int ret = -ENOENT;

	uint64_t base = start;
	uint64_t limit = base + size;

	for (int cpuid = 0; cpuid < arch_num_cpus(); cpuid++) {
		/* Check the following searching does not overflow the room */
		if (sys_dyn_regions_num[cpuid] + 1 > MPU_DYNAMIC_REGION_AREAS_NUM) {
			return -ENOSPC;
		}

		ret = -ENOENT;

		for (int i = 0; i < mpu_config.num_regions; i++) {
			region = &mpu_config.mpu_regions[i];
			tmp_info = &sys_dyn_regions[cpuid][sys_dyn_regions_num[cpuid]];

			if (base >= region->base && limit <= region->limit) {
				tmp_info->index = i;
				tmp_info->region_conf = *region;
				sys_dyn_regions_num[cpuid] += 1;
				/* find the region, reset ret to no error */
				ret = 0;
				break;
			}
		}
#if defined(CONFIG_ARM64_STACK_PROTECTION)
		ret = insert_region(sys_dyn_regions[cpuid],
				    MPU_DYNAMIC_REGION_AREAS_NUM,
				    (uintptr_t)z_interrupt_stacks[cpuid],
				    Z_ARM64_STACK_GUARD_SIZE,
				    NULL /* delete this region */);
		if (ret < 0) {
			break;
		}
		/*
		 * No need to check here if (sys_dyn_regions[cpuid] + ret) overflows,
		 * because the insert_region has checked it.
		 */
		sys_dyn_regions_num[cpuid] += ret;
#endif
	}

	return ret < 0 ? ret : 0;
}

static void set_region(struct arm_mpu_region *region,
		       uint64_t base, uint64_t limit,
		       struct arm_mpu_region_attr *attr)
{
	region->base = base;
	region->limit = limit;
	if (attr != NULL) {
		region->attr = *attr;
	} else {
		memset(&region->attr, 0, sizeof(struct arm_mpu_region_attr));
	}
}

static void clear_region(struct arm_mpu_region *region)
{
	set_region(region, 0, 0, NULL);
}

static int dup_dynamic_regions(struct dynamic_region_info *dst, int len)
{
	size_t i;
	int num = sys_dyn_regions_num[arch_curr_cpu()->id];

	if (num >= len) {
		LOG_ERR("system dynamic region nums too large.");
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		dst[i] = sys_dyn_regions[arch_curr_cpu()->id][i];
	}
	for (; i < len; i++) {
		clear_region(&dst[i].region_conf);
		dst[i].index = -1;
	}

	return num;
}

static struct dynamic_region_info *get_underlying_region(struct dynamic_region_info *dyn_regions,
							 uint8_t region_num, uint64_t base,
							 uint64_t limit)
{
	for (int idx = 0; idx < region_num; idx++) {
		struct arm_mpu_region *region = &(dyn_regions[idx].region_conf);

		if (base >= region->base && limit <= region->limit) {
			return &(dyn_regions[idx]);
		}
	}

	return NULL;
}

static struct dynamic_region_info *find_available_region(struct dynamic_region_info *dyn_regions,
							 uint8_t region_num)
{
	return get_underlying_region(dyn_regions, region_num, 0, 0);
}

/*
 * return -ENOENT if there is no more available region
 * do nothing if attr is NULL
 */
static int _insert_region(struct dynamic_region_info *dyn_regions, uint8_t region_num,
			  uint64_t base, uint64_t limit, struct arm_mpu_region_attr *attr)
{
	struct dynamic_region_info *tmp_region;

	if (attr == NULL) {
		return 0;
	}

	tmp_region = find_available_region(dyn_regions, region_num);

	if (tmp_region == NULL) {
		return -ENOENT;
	}

	set_region(&tmp_region->region_conf, base, limit, attr);

	return 0;
}

static int insert_region(struct dynamic_region_info *dyn_regions, uint8_t region_num,
			 uintptr_t start, size_t size, struct arm_mpu_region_attr *attr)
{

	int ret = 0;
	/* base: inclusive, limit: exclusive */
	uint64_t base = (uint64_t)start;
	uint64_t limit = base + size;
	struct dynamic_region_info *u_region;
	uint64_t u_base;
	uint64_t u_limit;
	struct arm_mpu_region_attr u_attr;

	int count = 0;

	u_region = get_underlying_region(dyn_regions, region_num, base, limit);

	if (u_region == NULL) {
		return -ENOENT;
	}

	/* restore the underlying region range and attr */
	u_base = u_region->region_conf.base;
	u_limit = u_region->region_conf.limit;
	u_attr = u_region->region_conf.attr;

	clear_region(&u_region->region_conf);
	count--;

	/* if attr is NULL, meaning we are going to delete a region */
	if (base == u_base && limit == u_limit) {
		/*
		 * The new region overlaps entirely with the
		 * underlying region. Simply update the attr.
		 */
		ret += _insert_region(dyn_regions, region_num, base, limit, attr);
		count++;
	} else if (base == u_base) {
		ret += _insert_region(dyn_regions, region_num, limit, u_limit, &u_attr);
		count++;
		ret += _insert_region(dyn_regions, region_num, base, limit, attr);
		count++;
	} else if (limit == u_limit) {
		ret += _insert_region(dyn_regions, region_num, u_base, base, &u_attr);
		count++;
		ret += _insert_region(dyn_regions, region_num, base, limit, attr);
		count++;
	} else {
		ret += _insert_region(dyn_regions, region_num, u_base, base, &u_attr);
		count++;
		ret += _insert_region(dyn_regions, region_num, base, limit, attr);
		count++;
		ret += _insert_region(dyn_regions, region_num, limit, u_limit, &u_attr);
		count++;
	}

	if (ret < 0) {
		return -ENOENT;
	}

	if (attr == NULL) {
		/* meanning we removed a region, so fix the count by decreasing 1 */
		count--;
	}

	return count;
}

static int flush_dynamic_regions_to_mpu(struct dynamic_region_info *dyn_regions,
					uint8_t region_num)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "mpu flushing must be called with IRQs disabled");

	int reg_avail_idx = static_regions_num;

	if (region_num >= get_num_regions()) {
		LOG_ERR("Out-of-bounds error for mpu regions. "
			"region num: %d, total mpu regions: %d",
			region_num, get_num_regions());
		return -ENOENT;
	}

	arm_core_mpu_background_region_enable();

	/*
	 * Clean the dynamic regions
	 * Before cleaning them, we need to flush dyn_regions to memory, because we need to read it
	 * in updating mpu region.
	 */
	sys_cache_data_flush_range(dyn_regions, sizeof(struct dynamic_region_info) * region_num);
	for (size_t i = reg_avail_idx; i < get_num_regions(); i++) {
		mpu_clr_region(i);
	}

	/*
	 * flush the dyn_regions to MPU
	 */
	for (size_t i = 0; i < region_num; i++) {
		int region_idx = dyn_regions[i].index;
		/*
		 * dyn_regions has two types of regions:
		 * 1) The fixed dyn background region which has a real index.
		 * 2) The normal region whose index will accumulate from
		 *    static_regions_num.
		 *
		 * Region_idx < 0 means not the fixed dyn background region.
		 * In this case, region_idx should be the reg_avail_idx which
		 * is accumulated from static_regions_num.
		 */
		if (region_idx < 0) {
			region_idx = reg_avail_idx++;
		}

		region_init(region_idx, &(dyn_regions[i].region_conf));
	}
	arm_core_mpu_background_region_disable();

	return 0;
}

static int configure_dynamic_mpu_regions(struct k_thread *thread)
{
	__ASSERT(read_daif() & DAIF_IRQ_BIT, "must be called with IRQs disabled");

	struct dynamic_region_info *dyn_regions = thread->arch.regions;
	const uint8_t max_region_num = ARM64_MPU_MAX_DYNAMIC_REGIONS;
	int region_num;
	int ret = 0;

	/* Busy wait if it is flushing somewhere else */
	while (!atomic_cas(&thread->arch.flushing, 0, 1)) {
	}

	thread->arch.region_num = 0;

	ret = dup_dynamic_regions(dyn_regions, max_region_num);

	if (ret < 0) {
		goto out;
	}

	region_num = ret;

#if defined(CONFIG_USERSPACE)
	struct k_mem_domain *mem_domain = thread->mem_domain_info.mem_domain;

	if (mem_domain) {
		LOG_DBG("configure domain: %p", mem_domain);

		uint32_t num_parts = mem_domain->num_partitions;
		uint32_t max_parts = CONFIG_MAX_DOMAIN_PARTITIONS;
		struct k_mem_partition *partition;

		for (size_t i = 0; i < max_parts && num_parts > 0; i++, num_parts--) {
			partition = &mem_domain->partitions[i];
			if (partition->size == 0) {
				continue;
			}
			LOG_DBG("set region 0x%lx 0x%lx\n",
				partition->start, partition->size);
			ret = insert_region(dyn_regions,
					    max_region_num,
					    partition->start,
					    partition->size,
					    &partition->attr);

			if (ret < 0) {
				goto out;
			}

			region_num += ret;
		}
	}

	LOG_DBG("configure user thread %p's context", thread);
	if ((thread->base.user_options & K_USER) != 0) {
		/* K_USER thread stack needs a region */
		ret = insert_region(dyn_regions,
				    max_region_num,
				    thread->stack_info.start,
				    thread->stack_info.size,
				    &K_MEM_PARTITION_P_RW_U_RW);
		if (ret < 0) {
			goto out;
		}

		region_num += ret;
	}
#endif

#if defined(CONFIG_ARM64_STACK_PROTECTION)
	uintptr_t guard_start;

	if (thread->arch.stack_limit != 0) {
		guard_start = (uintptr_t)thread->arch.stack_limit - Z_ARM64_STACK_GUARD_SIZE;
		ret = insert_region(dyn_regions,
				    max_region_num,
				    guard_start,
				    Z_ARM64_STACK_GUARD_SIZE,
				    NULL);
		if (ret < 0) {
			goto out;
		}
		region_num += ret;
	}
#endif

	/*
	 * There is no need to check if region_num is overflow the uint8_t,
	 * because the insert_region make sure there is enough room to store a region,
	 * otherwise the insert_region will return a negtive error number
	 */
	thread->arch.region_num = (uint8_t)region_num;

	if (thread == _current) {
		ret = flush_dynamic_regions_to_mpu(dyn_regions, region_num);
	}

out:
	atomic_clear(&thread->arch.flushing);
	return ret < 0 ? ret : 0;
}
#endif /* defined(CONFIG_USERSPACE) || defined(CONFIG_HW_STACK_PROTECTION) */

#if defined(CONFIG_USERSPACE)
int arch_mem_domain_max_partitions_get(void)
{
	int remaining_regions = get_num_regions() - static_regions_num + 1;

	/*
	 * Check remianing regions, should more than ARM64_MPU_MAX_DYNAMIC_REGIONS
	 * which equals CONFIG_MAX_DOMAIN_PARTITIONS + necessary regions (stack, guard)
	 */
	if (remaining_regions < ARM64_MPU_MAX_DYNAMIC_REGIONS) {
		LOG_WRN("MPU regions not enough, demand: %d, regions: %d",
			ARM64_MPU_MAX_DYNAMIC_REGIONS, remaining_regions);
		return remaining_regions;
	}

	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

static int configure_domain_partitions(struct k_mem_domain *domain)
{
	struct k_thread *thread;
	int ret;

	SYS_DLIST_FOR_EACH_CONTAINER(&domain->mem_domain_q, thread,
				     mem_domain_info.mem_domain_q_node) {
		ret = configure_dynamic_mpu_regions(thread);
		if (ret != 0) {
			return ret;
		}
	}
#ifdef CONFIG_SMP
	/* the thread could be running on another CPU right now */
	z_arm64_mem_cfg_ipi();
#endif

	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(partition_id);

	return configure_domain_partitions(domain);
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(partition_id);

	return configure_domain_partitions(domain);
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	int ret = 0;

	ret = configure_dynamic_mpu_regions(thread);
#ifdef CONFIG_SMP
	if (ret == 0 && thread != _current) {
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
	}
#endif

	return ret;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	int ret = 0;

	ret = configure_dynamic_mpu_regions(thread);
#ifdef CONFIG_SMP
	if (ret == 0 && thread != _current) {
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
	}
#endif

	return ret;
}
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_USERSPACE) || defined(CONFIG_ARM64_STACK_PROTECTION)
void z_arm64_thread_mem_domains_init(struct k_thread *thread)
{
	unsigned int key = arch_irq_lock();

	configure_dynamic_mpu_regions(thread);
	arch_irq_unlock(key);
}

void z_arm64_swap_mem_domains(struct k_thread *thread)
{
	int cpuid = arch_curr_cpu()->id;

	/* Busy wait if it is configuring somewhere else */
	while (!atomic_cas(&thread->arch.flushing, 0, 1)) {
	}

	if (thread->arch.region_num == 0) {
		(void)flush_dynamic_regions_to_mpu(sys_dyn_regions[cpuid],
						   sys_dyn_regions_num[cpuid]);
	} else {
		(void)flush_dynamic_regions_to_mpu(thread->arch.regions,
						   thread->arch.region_num);
	}

	atomic_clear(&thread->arch.flushing);
}
#endif
