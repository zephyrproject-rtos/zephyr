/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arm64/cortex_r/mpu/arm_mpu.h>
#include <linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(mpu);

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
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static int arm_mpu_init(const struct device *arg)
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
		return -1;
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

SYS_INIT(arm_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
