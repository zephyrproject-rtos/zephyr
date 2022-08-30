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

LOG_MODULE_REGISTER(mpu, CONFIG_MPU_LOG_LEVEL);

#define MPU_DYNAMIC_REGION_AREAS_NUM	1

#define _MAX_DYNAMIC_MPU_REGIONS_NUM                                                               \
	((IS_ENABLED(CONFIG_USERSPACE) ? (CONFIG_MAX_DOMAIN_PARTITIONS + 1) : 0) +                 \
	 (IS_ENABLED(CONFIG_MPU_STACK_GUARD) ? 1 : 0))

#ifdef CONFIG_USERSPACE
static int dynamic_areas_init(uintptr_t start, size_t size);
#define MPU_DYNAMIC_REGIONS_AREA_START ((uintptr_t)&_app_smem_start)
#else
#define MPU_DYNAMIC_REGIONS_AREA_START ((uintptr_t)&__kernel_ram_start)
#endif
#define MPU_DYNAMIC_REGIONS_AREA_SIZE                                                             \
	((size_t)((uintptr_t)&__kernel_ram_end - MPU_DYNAMIC_REGIONS_AREA_START))

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
static inline uint8_t get_num_regions(void)
{
	uint64_t type;

	type = read_mpuir_el1();
	type = type & MPU_IR_REGION_Msk;

	return (uint8_t)type;
}

/* ARM Core MPU Driver API Implementation for ARM MPU */

/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void)
{
	uint64_t val;

	val = read_sctlr_el1();
	val |= SCTLR_M_BIT;
	write_sctlr_el1(val);
	dsb();
	isb();
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	uint64_t val;

	/* Force any outstanding transfers to complete before disabling MPU */
	dmb();

	val = read_sctlr_el1();
	val &= ~SCTLR_M_BIT;
	write_sctlr_el1(val);
	dsb();
	isb();
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
	dsb();
	isb();
}

static inline void mpu_set_region(uint32_t rnr, uint64_t rbar,
				  uint64_t rlar)
{
	write_prselr_el1(rnr);
	dsb();
	write_prbar_el1(rbar);
	write_prlar_el1(rlar);
	dsb();
	isb();
}

/* This internal functions performs MPU region initialization. */
static void region_init(const uint32_t index,
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

/*
 * @brief MPU default configuration
 *
 * This function here provides the default configuration mechanism
 * for the Memory Protection Unit (MPU).
 */
void z_arm64_mm_init(bool is_primary_core)
{
	uint64_t val;
	uint32_t r_index;

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

	if (!is_primary_core) {
		return;
	}

#ifdef CONFIG_USERSPACE
	/* Only primary core do the dynamic_areas_init. */
	int rc = dynamic_areas_init(MPU_DYNAMIC_REGIONS_AREA_START,
				    MPU_DYNAMIC_REGIONS_AREA_SIZE);
	if (rc <= 0) {
		__ASSERT(0, "Dynamic areas init fail");
		return;
	}
#endif

}

#ifdef CONFIG_USERSPACE

struct dynamic_region_info {
	int index;
	struct arm_mpu_region region_conf;
};

static struct dynamic_region_info sys_dyn_regions[MPU_DYNAMIC_REGION_AREAS_NUM];
static int sys_dyn_regions_num;

static int dynamic_areas_init(uintptr_t start, size_t size)
{
	const struct arm_mpu_region *region;
	struct dynamic_region_info *tmp_info;

	uint64_t base = start;
	uint64_t limit = base + size;

	if (sys_dyn_regions_num + 1 > MPU_DYNAMIC_REGION_AREAS_NUM) {
		return -1;
	}

	for (size_t i = 0; i < mpu_config.num_regions; i++) {
		region = &mpu_config.mpu_regions[i];
		tmp_info = &sys_dyn_regions[sys_dyn_regions_num];
		if (base >= region->base && limit <= region->limit) {
			tmp_info->index = i;
			tmp_info->region_conf = *region;
			return ++sys_dyn_regions_num;
		}
	}

	return -1;
}

static int dup_dynamic_regions(struct dynamic_region_info *dst, int len)
{
	size_t i;
	int ret = sys_dyn_regions_num;

	CHECKIF(!(sys_dyn_regions_num < len)) {
		LOG_ERR("system dynamic region nums too large.");
		ret = -EINVAL;
		goto out;
	}

	for (i = 0; i < sys_dyn_regions_num; i++) {
		dst[i] = sys_dyn_regions[i];
	}
	for (; i < len; i++) {
		dst[i].index = -1;
	}

out:
	return ret;
}

static void set_region(struct arm_mpu_region *region,
		       uint64_t base, uint64_t limit,
		       struct arm_mpu_region_attr *attr)
{
	region->base = base;
	region->limit = limit;
	region->attr = *attr;
}

static int get_underlying_region_idx(struct dynamic_region_info *dyn_regions,
				     uint8_t region_num, uint64_t base,
				     uint64_t limit)
{
	for (size_t idx = 0; idx < region_num; idx++) {
		struct arm_mpu_region *region = &(dyn_regions[idx].region_conf);

		if (base >= region->base && limit <= region->limit) {
			return idx;
		}
	}
	return -1;
}

static int insert_region(struct dynamic_region_info *dyn_regions,
			 uint8_t region_idx, uint8_t region_num,
			 uintptr_t start, size_t size,
			 struct arm_mpu_region_attr *attr)
{

	/* base: inclusive, limit: exclusive */
	uint64_t base = (uint64_t)start;
	uint64_t limit = base + size;
	int u_idx;
	struct arm_mpu_region *u_region;
	uint64_t u_base;
	uint64_t u_limit;
	struct arm_mpu_region_attr *u_attr;
	int ret = 0;

	CHECKIF(!(region_idx < region_num)) {
		LOG_ERR("Out-of-bounds error for dynamic region map. "
			"region idx: %d, region num: %d",
			region_idx, region_num);
		ret = -EINVAL;
		goto out;
	}

	u_idx = get_underlying_region_idx(dyn_regions, region_idx, base, limit);

	CHECKIF(!(u_idx >= 0)) {
		LOG_ERR("Invalid underlying region index");
		ret = -ENOENT;
		goto out;
	}

	/* Get underlying region range and attr */
	u_region = &(dyn_regions[u_idx].region_conf);
	u_base = u_region->base;
	u_limit = u_region->limit;
	u_attr = &u_region->attr;

	/* Temporally holding new region available to be configured */
	struct arm_mpu_region *curr_region = &(dyn_regions[region_idx].region_conf);

	if (base == u_base && limit == u_limit) {
		/*
		 * The new region overlaps entirely with the
		 * underlying region. Simply update the attr.
		 */
		set_region(u_region, base, limit, attr);
	} else if (base == u_base) {
		set_region(curr_region, base, limit, attr);
		set_region(u_region, limit, u_limit, u_attr);
		region_idx++;
	} else if (limit == u_limit) {
		set_region(u_region, u_base, base, u_attr);
		set_region(curr_region, base, limit, attr);
		region_idx++;
	} else {
		set_region(u_region, u_base, base, u_attr);
		set_region(curr_region, base, limit, attr);
		region_idx++;
		curr_region = &(dyn_regions[region_idx].region_conf);
		set_region(curr_region, limit, u_limit, u_attr);
		region_idx++;
	}

	ret = region_idx;

out:
	return ret;
}

static int flush_dynamic_regions_to_mpu(struct dynamic_region_info *dyn_regions,
					uint8_t region_num)
{
	int reg_avail_idx = static_regions_num;
	int ret = 0;

	/*
	 * Clean the dynamic regions
	 */
	for (size_t i = reg_avail_idx; i < get_num_regions(); i++) {
		mpu_set_region(i, 0, 0);
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
		CHECKIF(!(region_idx < get_num_regions())) {
			LOG_ERR("Out-of-bounds error for mpu regions. "
				"region idx: %d, total mpu regions: %d",
				region_idx, get_num_regions());
			ret = -ENOENT;
		}

		region_init(region_idx, &(dyn_regions[i].region_conf));
	}

	return ret;
}

static int configure_dynamic_mpu_regions(struct k_thread *thread)
{
	/*
	 * Allocate double space for dyn_regions. Because when split
	 * the background dynamic regions, it will cause double regions numbers
	 * generated.
	 */
	struct dynamic_region_info dyn_regions[_MAX_DYNAMIC_MPU_REGIONS_NUM * 2];
	const uint8_t max_region_num = ARRAY_SIZE(dyn_regions);
	uint8_t region_num;
	int ret = 0, ret2;

	ret2 = dup_dynamic_regions(dyn_regions, max_region_num);
	CHECKIF(ret2 < 0) {
		ret = ret2;
		goto out;
	}

	region_num = (uint8_t)ret2;

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
			LOG_DBG("set region 0x%lx 0x%lx",
				partition->start, partition->size);
			ret2 = insert_region(dyn_regions,
					     region_num,
					     max_region_num,
					     partition->start,
					     partition->size,
					     &partition->attr);
			CHECKIF(ret2 != 0) {
				ret = ret2;
			}

			region_num = (uint8_t)ret2;
		}
	}

	LOG_DBG("configure user thread %p's context", thread);
	if ((thread->base.user_options & K_USER) != 0) {
		/* K_USER thread stack needs a region */
		ret2 = insert_region(dyn_regions,
				     region_num,
				     max_region_num,
				     thread->stack_info.start,
				     thread->stack_info.size,
				     &K_MEM_PARTITION_P_RW_U_RW);
		CHECKIF(ret2 != 0) {
			ret = ret2;
		}

		region_num = (uint8_t)ret2;
	}

	arm_core_mpu_disable();
	ret = flush_dynamic_regions_to_mpu(dyn_regions, region_num);
	arm_core_mpu_enable();

out:
	return ret;
}

int arch_mem_domain_max_partitions_get(void)
{
	int max_parts = get_num_regions() - static_regions_num;

	if (max_parts > CONFIG_MAX_DOMAIN_PARTITIONS) {
		max_parts = CONFIG_MAX_DOMAIN_PARTITIONS;
	}

	return max_parts;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(domain);
	ARG_UNUSED(partition_id);

	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(domain);
	ARG_UNUSED(partition_id);

	return 0;
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	int ret = 0;

	if (thread == _current) {
		ret = configure_dynamic_mpu_regions(thread);
	}
#ifdef CONFIG_SMP
	else {
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
	}
#endif

	return ret;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	int ret = 0;

	if (thread == _current) {
		ret = configure_dynamic_mpu_regions(thread);
	}
#ifdef CONFIG_SMP
	else {
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
	}
#endif

	return ret;
}

void z_arm64_thread_mem_domains_init(struct k_thread *thread)
{
	configure_dynamic_mpu_regions(thread);
}

void z_arm64_swap_mem_domains(struct k_thread *thread)
{
	configure_dynamic_mpu_regions(thread);
}

#endif /* CONFIG_USERSPACE */
