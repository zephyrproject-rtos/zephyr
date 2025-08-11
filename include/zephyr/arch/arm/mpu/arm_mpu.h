/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_MPU_ARM_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_MPU_ARM_MPU_H_

#if defined(CONFIG_CPU_CORTEX_M0PLUS) || defined(CONFIG_CPU_CORTEX_M3) ||                          \
	defined(CONFIG_CPU_CORTEX_M4) || defined(CONFIG_CPU_CORTEX_M7) || defined(CONFIG_ARMV7_R)
#include <zephyr/arch/arm/mpu/arm_mpu_v7m.h>
#elif defined(CONFIG_CPU_CORTEX_M23) || defined(CONFIG_CPU_CORTEX_M33) ||                          \
	defined(CONFIG_CPU_CORTEX_M52) || defined(CONFIG_CPU_CORTEX_M55) ||                        \
	defined(CONFIG_CPU_CORTEX_M85) || defined(CONFIG_AARCH32_ARMV8_R)
#include <zephyr/arch/arm/mpu/arm_mpu_v8.h>
#else
#error "Unsupported ARM CPU"
#endif

#if defined(CONFIG_ARMV8_M_MAINLINE) || defined(CONFIG_ARMV8_M_BASELINE)
/* PMSAv8 MPU */
#define Z_ARM_CPU_HAS_PMSAV8_MPU 1
#else
/* PMSAv6 / PMSAv7 (MPU is identical) */
#define Z_ARM_CPU_HAS_PMSAV8_MPU 0
#endif

#if defined(CONFIG_ARMV8_M_MAINLINE)
#define Z_ARM_MPU_MAX_REGIONS 16U
#else
#define Z_ARM_MPU_MAX_REGIONS 8U
#endif


#ifndef _ASMLANGUAGE
#include <stdint.h>

/* Region definition data structure */
struct arm_mpu_region {
	/* Region Base Address */
	uint32_t base;
	/* Region Name */
	const char *name;
#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	/* Region Size */
	uint32_t size;
#endif
	/* Region Attributes */
	arm_mpu_region_attr_t attr;
};

/* MPU configuration data structure */
struct arm_mpu_config {
	/* Number of regions */
	uint32_t num_regions;
	/* Regions */
	const struct arm_mpu_region *mpu_regions;
};

#if defined(CONFIG_ARMV7_R)
#define MPU_REGION_ENTRY(_name, _base, _size, _attr)                                               \
	{                                                                                          \
		.name = _name,                                                                     \
		.base = _base,                                                                     \
		.size = _size,                                                                     \
		.attr = _attr,                                                                     \
	}
#else
#define MPU_REGION_ENTRY(_name, _base, _attr)                                                      \
	{                                                                                          \
		.name = _name,                                                                     \
		.base = _base,                                                                     \
		.attr = _attr,                                                                     \
	}
#endif

/* Reference to the MPU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MPU
 * regions enabled during kernel initialization. Dynamic MPU regions (e.g.
 * for Thread Stack, Stack Guards, etc.) are programmed during runtime, thus,
 * not kept here.
 */
extern const struct arm_mpu_config mpu_config;

#if defined(CONFIG_CPU_CORTEX_M)
/**
 * @brief MPU context structure to retain MPU register state across deep sleep.
 *
 * This structure holds the MPU region base and attribute registers,
 * as well as the MPU control register and a valid region count.
 *
 * The implemented architecture dictates which MPU registers exist:
 * - ARMv8-M has per-region RBAR+RLAR, and global MAIR0~1
 * - ARMv6/v7-M have per-region RBAR+RASR
 */
struct z_mpu_context_retained {
	uint32_t rbar[Z_ARM_MPU_MAX_REGIONS];
	uint32_t rasr_rlar[Z_ARM_MPU_MAX_REGIONS];
#if Z_ARM_CPU_HAS_PMSAV8_MPU
	uint32_t mair[2];
#endif
	uint32_t ctrl;
	uint32_t num_valid_regions;
};

/**
 * @brief Save the current MPU configuration into the provided context struct.
 *
 * @param ctx Pointer to the MPU context structure to save into.
 */
void z_arm_save_mpu_context(struct z_mpu_context_retained *ctx);

/**
 * @brief Restore the MPU configuration from the provided context struct.
 *
 * @param ctx Pointer to the MPU context structure to restore from.
 */
void z_arm_restore_mpu_context(const struct z_mpu_context_retained *ctx);

#endif /* CONFIG_CPU_CORTEX_M */
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_MPU_ARM_MPU_H_ */
