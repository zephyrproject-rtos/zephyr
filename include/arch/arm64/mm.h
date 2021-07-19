/*
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_MM_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_MM_H_

#if defined(CONFIG_ARM_MMU)
#include <arch/arm64/arm_mmu.h>
#elif defined(CONFIG_ARM_MPU)
#include <arch/arm64/cortex_r/arm_mpu.h>
#endif

#ifndef _ASMLANGUAGE

struct k_thread;
void z_arm64_thread_mem_domains_init(struct k_thread *thread);
void z_arm64_swap_mem_domains(struct k_thread *thread);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_MM_H_ */
