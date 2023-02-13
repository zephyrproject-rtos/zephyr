/*
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_MM_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_MM_H_

#if defined(CONFIG_ARM_MMU)
#include <zephyr/arch/arm64/arm_mmu.h>
/*
 * When mmu enabled, some section addresses need to be aligned with
 * page size which is CONFIG_MMU_PAGE_SIZE
 */
#define MEM_DOMAIN_ALIGN_AND_SIZE CONFIG_MMU_PAGE_SIZE
#elif defined(CONFIG_ARM_MPU)
#include <zephyr/arch/arm64/cortex_r/arm_mpu.h>
/*
 * When mpu enabled, some section addresses need to be aligned with
 * mpu region min align size which is
 * CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
 */
#define MEM_DOMAIN_ALIGN_AND_SIZE CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#endif

#ifndef _ASMLANGUAGE

struct k_thread;
void z_arm64_thread_mem_domains_init(struct k_thread *thread);
void z_arm64_swap_mem_domains(struct k_thread *thread);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_MM_H_ */
