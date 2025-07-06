/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include "arm_core_mpu_dev.h"
#include <zephyr/linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mpu);

extern void arm_core_mpu_enable(void);
extern void arm_core_mpu_disable(void);

/*
 * Maximum number of dynamic memory partitions that may be supplied to the MPU
 * driver for programming during run-time. Note that the actual number of the
 * available MPU regions for dynamic programming depends on the number of the
 * static MPU regions currently being programmed, and the total number of HW-
 * available MPU regions. This macro is only used internally in function
 * z_arm_configure_dynamic_mpu_regions(), to reserve sufficient area for the
 * array of dynamic regions passed to the underlying driver.
 */
#if defined(CONFIG_USERSPACE)
#define _MAX_DYNAMIC_MPU_REGIONS_NUM \
	CONFIG_MAX_DOMAIN_PARTITIONS + /* User thread stack */ 1 + \
	(IS_ENABLED(CONFIG_MPU_STACK_GUARD) ? 1 : 0)
#else
#define _MAX_DYNAMIC_MPU_REGIONS_NUM \
	(IS_ENABLED(CONFIG_MPU_STACK_GUARD) ? 1 : 0)
#endif /* CONFIG_USERSPACE */

/* Convenience macros to denote the start address and the size of the system
 * memory area, where dynamic memory regions may be programmed at run-time.
 */
#if defined(CONFIG_USERSPACE)
#define _MPU_DYNAMIC_REGIONS_AREA_START ((uint32_t)&_app_smem_start)
#else
#define _MPU_DYNAMIC_REGIONS_AREA_START ((uint32_t)&__kernel_ram_start)
#endif /* CONFIG_USERSPACE */
#define _MPU_DYNAMIC_REGIONS_AREA_SIZE ((uint32_t)&__kernel_ram_end - \
		_MPU_DYNAMIC_REGIONS_AREA_START)

#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_MPU_STACK_GUARD)
K_THREAD_STACK_DECLARE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING) \
	&& defined(CONFIG_MPU_STACK_GUARD)
uint32_t z_arm_mpu_stack_guard_and_fpu_adjust(struct k_thread *thread);
#endif

#if defined(CONFIG_CODE_DATA_RELOCATION_SRAM)
extern char __ram_text_reloc_start[];
extern char __ram_text_reloc_size[];
#endif

#if defined(CONFIG_SRAM_VECTOR_TABLE)
extern char _sram_vector_start[];
extern char _sram_vector_size[];
#endif

static const struct z_arm_mpu_partition static_regions[] = {
#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
	{
		/* GCOV code coverage accounting area. Needs User permissions
		 * to function
		 */
		.start = (uint32_t)&__gcov_bss_start,
		.size = (uint32_t)&__gcov_bss_size,
		.attr = K_MEM_PARTITION_P_RW_U_RW,
	},
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */
#if defined(CONFIG_NOCACHE_MEMORY)
	{
		/* Special non-cacheable RAM area */
		.start = (uint32_t)&_nocache_ram_start,
		.size = (uint32_t)&_nocache_ram_size,
		.attr = K_MEM_PARTITION_P_RW_U_NA_NOCACHE,
	},
#endif /* CONFIG_NOCACHE_MEMORY */
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
	{
		/* Special RAM area for program text */
		.start = (uint32_t)&__ramfunc_start,
		.size = (uint32_t)&__ramfunc_size,
#if defined(CONFIG_ARM_MPU_PXN) && defined(CONFIG_USERSPACE)
		.attr = K_MEM_PARTITION_P_R_U_RX,
#else
		.attr = K_MEM_PARTITION_P_RX_U_RX,
#endif
	},
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */
#if defined(CONFIG_CODE_DATA_RELOCATION_SRAM)
	{
		/* RAM area for relocated text */
		.start = (uint32_t)&__ram_text_reloc_start,
		.size = (uint32_t)&__ram_text_reloc_size,
#if defined(CONFIG_ARM_MPU_PXN) && defined(CONFIG_USERSPACE)
		.attr = K_MEM_PARTITION_P_R_U_RX,
#else
		.attr = K_MEM_PARTITION_P_RX_U_RX,
#endif
	},
#endif /* CONFIG_CODE_DATA_RELOCATION_SRAM */
#if defined(CONFIG_SRAM_VECTOR_TABLE)
	{
		/* Vector table in SRAM */
		.start = (uint32_t)&_sram_vector_start,
		.size = (uint32_t)&_sram_vector_size,
#if defined(CONFIG_ARM_MPU_PXN) && defined(CONFIG_USERSPACE)
		.attr = K_MEM_PARTITION_P_R_U_RX,
#else
		.attr = K_MEM_PARTITION_P_RO_U_RO,
#endif
	},
#endif /* CONFIG_SRAM_VECTOR_TABLE */
#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_MPU_STACK_GUARD)
	/* Main stack MPU guard to detect overflow.
	 * Note:
	 * FPU_SHARING and USERSPACE are not supported features
	 * under CONFIG_MULTITHREADING=n, so the MPU guard (if
	 * exists) is reserved aside of CONFIG_MAIN_STACK_SIZE
	 * and there is no requirement for larger guard area (FP
	 * context is not stacked).
	 */
	{
		.start = (uint32_t)z_main_stack,
		.size = (uint32_t)MPU_GUARD_ALIGN_AND_SIZE,
		.attr = K_MEM_PARTITION_P_RO_U_NA,
	},
#endif /* !CONFIG_MULTITHREADING && CONFIG_MPU_STACK_GUARD */
};

/**
 * @brief Use the HW-specific MPU driver to program
 *        the static MPU regions.
 *
 * Program the static MPU regions using the HW-specific MPU driver. The
 * function is meant to be invoked only once upon system initialization.
 *
 * If the function attempts to configure a number of regions beyond the
 * MPU HW limitations, the system behavior will be undefined.
 *
 * For some MPU architectures, such as the unmodified ARMv8-M MPU,
 * the function must execute with MPU enabled.
 */
void z_arm_configure_static_mpu_regions(void)
{
	/* Configure the static MPU regions within firmware SRAM boundaries.
	 * Start address of the image is given by _image_ram_start. The end
	 * of the firmware SRAM area is marked by __kernel_ram_end, taking
	 * into account the unused SRAM area, as well.
	 */
#ifdef CONFIG_AARCH32_ARMV8_R
	arm_core_mpu_disable();
#endif
	arm_core_mpu_configure_static_mpu_regions(static_regions,
		ARRAY_SIZE(static_regions),
		(uint32_t)&_image_ram_start,
		(uint32_t)&__kernel_ram_end);
#ifdef CONFIG_AARCH32_ARMV8_R
	arm_core_mpu_enable();
#endif

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
	defined(CONFIG_MULTITHREADING)
	/* Define a constant array of z_arm_mpu_partition objects that holds the
	 * boundaries of the areas, inside which dynamic region programming
	 * is allowed. The information is passed to the underlying driver at
	 * initialization.
	 */
	const struct z_arm_mpu_partition dyn_region_areas[] = {
		{
		.start = _MPU_DYNAMIC_REGIONS_AREA_START,
		.size =  _MPU_DYNAMIC_REGIONS_AREA_SIZE,
		}
	};

	arm_core_mpu_mark_areas_for_dynamic_regions(dyn_region_areas,
		ARRAY_SIZE(dyn_region_areas));
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
}

/**
 * @brief Use the HW-specific MPU driver to program
 *        the dynamic MPU regions.
 *
 * Program the dynamic MPU regions using the HW-specific MPU
 * driver. This function is meant to be invoked every time the
 * memory map is to be re-programmed, e.g during thread context
 * switch, entering user mode, reconfiguring memory domain, etc.
 *
 * For some MPU architectures, such as the unmodified ARMv8-M MPU,
 * the function must execute with MPU enabled.
 *
 * This function is not inherently thread-safe, but the memory domain
 * spinlock needs to be held anyway.
 */
void z_arm_configure_dynamic_mpu_regions(struct k_thread *thread)
{
	/* Define an array of z_arm_mpu_partition objects to hold the configuration
	 * of the respective dynamic MPU regions to be programmed for
	 * the given thread. The array of partitions (along with its
	 * actual size) will be supplied to the underlying MPU driver.
	 *
	 * The drivers of what regions get configured are CONFIG_USERSPACE,
	 * CONFIG_MPU_STACK_GUARD, and K_USER/supervisor threads.
	 *
	 * If CONFIG_USERSPACE is defined and the thread is a member of any
	 * memory domain then any partitions defined within that domain get a
	 * defined region.
	 *
	 * If CONFIG_USERSPACE is defined and the thread is a user thread
	 * (K_USER) the usermode thread stack is defined a region.
	 *
	 * IF CONFIG_MPU_STACK_GUARD is defined the thread is a supervisor
	 * thread, the stack guard will be defined in front of the
	 * thread->stack_info.start. On a K_USER thread, the guard is defined
	 * in front of the privilege mode stack, thread->arch.priv_stack_start.
	 */
	static struct z_arm_mpu_partition
			dynamic_regions[_MAX_DYNAMIC_MPU_REGIONS_NUM];

	uint8_t region_num = 0U;

#if defined(CONFIG_USERSPACE)
	/* Memory domain */
	LOG_DBG("configure thread %p's domain", thread);
	struct k_mem_domain *mem_domain = thread->mem_domain_info.mem_domain;

	if (mem_domain) {
		LOG_DBG("configure domain: %p", mem_domain);
		uint32_t num_partitions = mem_domain->num_partitions;
		struct k_mem_partition *partition;
		int i;

		LOG_DBG("configure domain: %p", mem_domain);

		for (i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
			partition = &mem_domain->partitions[i];
			if (partition->size == 0) {
				/* Zero size indicates a non-existing
				 * memory partition.
				 */
				continue;
			}
			LOG_DBG("set region 0x%lx 0x%x",
				partition->start, partition->size);
			__ASSERT(region_num < _MAX_DYNAMIC_MPU_REGIONS_NUM,
				"Out-of-bounds error for dynamic region map.");

			dynamic_regions[region_num].start = partition->start;
			dynamic_regions[region_num].size = partition->size;
			dynamic_regions[region_num].attr = partition->attr;

			region_num++;
			num_partitions--;
			if (num_partitions == 0U) {
				break;
			}
		}
	}
	/* Thread user stack */
	LOG_DBG("configure user thread %p's context", thread);
	if (thread->arch.priv_stack_start) {
		/* K_USER thread stack needs a region */
		uintptr_t base = (uintptr_t)thread->stack_obj;
		size_t size = thread->stack_info.size +
			(thread->stack_info.start - base);

		__ASSERT(region_num < _MAX_DYNAMIC_MPU_REGIONS_NUM,
			"Out-of-bounds error for dynamic region map.");

		dynamic_regions[region_num].start = base;
		dynamic_regions[region_num].size = size;
		dynamic_regions[region_num].attr = K_MEM_PARTITION_P_RW_U_RW;

		region_num++;
	}
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_MPU_STACK_GUARD)
	/* Define a stack guard region for either the thread stack or the
	 * supervisor/privilege mode stack depending on the type of thread
	 * being mapped.
	 */

	/* Privileged stack guard */
	uintptr_t guard_start;
	size_t guard_size = MPU_GUARD_ALIGN_AND_SIZE;

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	guard_size = z_arm_mpu_stack_guard_and_fpu_adjust(thread);
#endif

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) {
		/* A K_USER thread has the stack guard protecting the privilege
		 * stack and not on the usermode stack because the user mode
		 * stack already has its own defined memory region.
		 */
		guard_start = thread->arch.priv_stack_start - guard_size;

		__ASSERT((uintptr_t)&z_priv_stacks_ram_start <= guard_start,
		"Guard start: (0x%lx) below privilege stacks boundary: (%p)",
		guard_start, z_priv_stacks_ram_start);
	} else
#endif /* CONFIG_USERSPACE */
	{
		/* A supervisor thread only has the normal thread stack to
		 * protect with a stack guard.
		 */
		guard_start = thread->stack_info.start - guard_size;
#ifdef CONFIG_USERSPACE
		__ASSERT((uintptr_t)thread->stack_obj == guard_start,
			"Guard start (0x%lx) not beginning at stack object (%p)\n",
			guard_start, thread->stack_obj);
#endif /* CONFIG_USERSPACE */
	}

	__ASSERT(region_num < _MAX_DYNAMIC_MPU_REGIONS_NUM,
		"Out-of-bounds error for dynamic region map.");

	dynamic_regions[region_num].start = guard_start;
	dynamic_regions[region_num].size = guard_size;
	dynamic_regions[region_num].attr = K_MEM_PARTITION_P_RO_U_NA;

	region_num++;
#endif /* CONFIG_MPU_STACK_GUARD */

	/* Configure the dynamic MPU regions */
#ifdef CONFIG_AARCH32_ARMV8_R
	arm_core_mpu_disable();
#endif
	arm_core_mpu_configure_dynamic_mpu_regions(dynamic_regions,
						   region_num);
#ifdef CONFIG_AARCH32_ARMV8_R
	arm_core_mpu_enable();
#endif
}

#if defined(CONFIG_USERSPACE)
int arch_mem_domain_max_partitions_get(void)
{
	int available_regions = arm_core_mpu_get_max_available_dyn_regions();

	available_regions -=
		ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_THREAD_STACK;

	if (IS_ENABLED(CONFIG_MPU_STACK_GUARD)) {
		available_regions -=
			ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_MPU_STACK_GUARD;
	}

	return ARM_CORE_MPU_MAX_DOMAIN_PARTITIONS_GET(available_regions);
}

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	return arm_core_mpu_buffer_validate(addr, size, write);
}

#endif /* CONFIG_USERSPACE */
