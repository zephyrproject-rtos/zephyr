/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <kernel_structs.h>

#include "arm_core_mpu_dev.h"
#include <linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpu);

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
extern K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
#endif

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
#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
		const struct k_mem_partition gcov_region =
		{
		.start = (uint32_t)&__gcov_bss_start,
		.size = (uint32_t)&__gcov_bss_size,
		.attr = K_MEM_PARTITION_P_RW_U_RW,
		};
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */
#if defined(CONFIG_NOCACHE_MEMORY)
		const struct k_mem_partition nocache_region =
		{
		.start = (uint32_t)&_nocache_ram_start,
		.size = (uint32_t)&_nocache_ram_size,
		.attr = K_MEM_PARTITION_P_RW_U_NA_NOCACHE,
		};
#endif /* CONFIG_NOCACHE_MEMORY */
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
		const struct k_mem_partition ramfunc_region =
		{
		.start = (uint32_t)&_ramfunc_ram_start,
		.size = (uint32_t)&_ramfunc_ram_size,
		.attr = K_MEM_PARTITION_P_RX_U_RX,
		};
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */

#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_MPU_STACK_GUARD)
		/* Main stack MPU guard to detect overflow.
		 * Note:
		 * FPU_SHARING and USERSPACE are not supported features
		 * under CONFIG_MULTITHREADING=n, so the MPU guard (if
		 * exists) is reserved aside of CONFIG_MAIN_STACK_SIZE
		 * and there is no requirement for larger guard area (FP
		 * context is not stacked).
		 */
		const struct k_mem_partition main_stack_guard_region = {
			.start = (uint32_t)z_main_stack,
			.size = (uint32_t)MPU_GUARD_ALIGN_AND_SIZE,
			.attr = K_MEM_PARTITION_P_RO_U_NA,
		};
#endif /* !CONFIG_MULTITHREADING && CONFIG_MPU_STACK_GUARD */
	/* Define a constant array of k_mem_partition objects
	 * to hold the configuration of the respective static
	 * MPU regions.
	 */
	const struct k_mem_partition *static_regions[] = {
#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
		&gcov_region,
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */
#if defined(CONFIG_NOCACHE_MEMORY)
		&nocache_region,
#endif /* CONFIG_NOCACHE_MEMORY */
#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_MPU_STACK_GUARD)
		&main_stack_guard_region,
#endif /* !CONFIG_MULTITHREADING && CONFIG_MPU_STACK_GUARD */
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
		&ramfunc_region
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */
	};

	/* Configure the static MPU regions within firmware SRAM boundaries.
	 * Start address of the image is given by _image_ram_start. The end
	 * of the firmware SRAM area is marked by __kernel_ram_end, taking
	 * into account the unused SRAM area, as well.
	 */
	arm_core_mpu_configure_static_mpu_regions(static_regions,
		ARRAY_SIZE(static_regions),
		(uint32_t)&_image_ram_start,
		(uint32_t)&__kernel_ram_end);

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
	defined(CONFIG_MULTITHREADING)
	/* Define a constant array of k_mem_partition objects that holds the
	 * boundaries of the areas, inside which dynamic region programming
	 * is allowed. The information is passed to the underlying driver at
	 * initialization.
	 */
	const struct k_mem_partition dyn_region_areas[] = {
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
 */
void z_arm_configure_dynamic_mpu_regions(struct k_thread *thread)
{
	/* Define an array of k_mem_partition objects to hold the configuration
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
	struct k_mem_partition *dynamic_regions[_MAX_DYNAMIC_MPU_REGIONS_NUM];

	uint8_t region_num = 0U;

#if defined(CONFIG_USERSPACE)
	struct k_mem_partition thread_stack;

	/* Memory domain */
	LOG_DBG("configure thread %p's domain", thread);
	struct k_mem_domain *mem_domain = thread->mem_domain_info.mem_domain;

	if (mem_domain) {
		LOG_DBG("configure domain: %p", mem_domain);
		uint32_t num_partitions = mem_domain->num_partitions;
		struct k_mem_partition partition;
		int i;

		LOG_DBG("configure domain: %p", mem_domain);

		for (i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
			partition = mem_domain->partitions[i];
			if (partition.size == 0) {
				/* Zero size indicates a non-existing
				 * memory partition.
				 */
				continue;
			}
			LOG_DBG("set region 0x%lx 0x%x",
				partition.start, partition.size);
			__ASSERT(region_num < _MAX_DYNAMIC_MPU_REGIONS_NUM,
				"Out-of-bounds error for dynamic region map.");
			dynamic_regions[region_num] =
				&mem_domain->partitions[i];

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
		uint32_t base = (uint32_t)thread->stack_obj;
		uint32_t size = thread->stack_info.size +
			(thread->stack_info.start - base);

		__ASSERT(region_num < _MAX_DYNAMIC_MPU_REGIONS_NUM,
			"Out-of-bounds error for dynamic region map.");
		thread_stack = (const struct k_mem_partition)
			{base, size, K_MEM_PARTITION_P_RW_U_RW};

		dynamic_regions[region_num] = &thread_stack;

		region_num++;
	}
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_MPU_STACK_GUARD)
	/* Define a stack guard region for either the thread stack or the
	 * supervisor/privilege mode stack depending on the type of thread
	 * being mapped.
	 */
	struct k_mem_partition guard;

	/* Privileged stack guard */
	uint32_t guard_start;
	uint32_t guard_size = MPU_GUARD_ALIGN_AND_SIZE;

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		guard_size = MPU_GUARD_ALIGN_AND_SIZE_FLOAT;
	}
#endif

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) {
		/* A K_USER thread has the stack guard protecting the privilege
		 * stack and not on the usermode stack because the user mode
		 * stack already has its own defined memory region.
		 */
		guard_start = thread->arch.priv_stack_start - guard_size;

		__ASSERT((uint32_t)&z_priv_stacks_ram_start <= guard_start,
		"Guard start: (0x%x) below privilege stacks boundary: (0x%x)",
		guard_start, (uint32_t)&z_priv_stacks_ram_start);
	} else {
		/* A supervisor thread only has the normal thread stack to
		 * protect with a stack guard.
		 */
		guard_start = thread->stack_info.start - guard_size;
	__ASSERT((uint32_t)thread->stack_obj == guard_start,
		"Guard start (0x%x) not beginning at stack object (0x%x)\n",
		guard_start, (uint32_t)thread->stack_obj);
	}
#else
	guard_start = thread->stack_info.start - guard_size;
#endif /* CONFIG_USERSPACE */

	__ASSERT(region_num < _MAX_DYNAMIC_MPU_REGIONS_NUM,
		"Out-of-bounds error for dynamic region map.");
	guard = (const struct k_mem_partition)
	{
		guard_start,
		guard_size,
		K_MEM_PARTITION_P_RO_U_NA
	};
	dynamic_regions[region_num] = &guard;

	region_num++;
#endif /* CONFIG_MPU_STACK_GUARD */

	/* Configure the dynamic MPU regions */
	arm_core_mpu_configure_dynamic_mpu_regions(
		(const struct k_mem_partition **)dynamic_regions,
		region_num);
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

void arch_mem_domain_thread_add(struct k_thread *thread)
{
	if (_current != thread) {
		return;
	}

	/* Request to configure memory domain for a thread.
	 * This triggers re-programming of the entire dynamic
	 * memory map.
	 */
	z_arm_configure_dynamic_mpu_regions(thread);
}

void arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	/* This function will reset the access permission configuration
	 * of the active partitions of the memory domain.
	 */
	int i;
	struct k_mem_partition partition;

	if (_current->mem_domain_info.mem_domain != domain) {
		return;
	}

	/* Partitions belonging to the memory domain will be reset
	 * to default (Privileged RW, Unprivileged NA) permissions.
	 */
	k_mem_partition_attr_t reset_attr = K_MEM_PARTITION_P_RW_U_NA;

	for (i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
		partition = domain->partitions[i];
		if (partition.size == 0U) {
			/* Zero size indicates a non-existing
			 * memory partition.
			 */
			continue;
		}
		arm_core_mpu_mem_partition_config_update(&partition,
			&reset_attr);
	}
}

void arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				      uint32_t partition_id)
{
	/* Request to remove a partition from a memory domain.
	 * This resets the access permissions of the partition
	 * to default (Privileged RW, Unprivileged NA).
	 */
	k_mem_partition_attr_t reset_attr = K_MEM_PARTITION_P_RW_U_NA;

	if (_current->mem_domain_info.mem_domain != domain) {
		return;
	}

	arm_core_mpu_mem_partition_config_update(
		&domain->partitions[partition_id], &reset_attr);
}

void arch_mem_domain_partition_add(struct k_mem_domain *domain,
				   uint32_t partition_id)
{
	/* No-op on this architecture */
}

void arch_mem_domain_thread_remove(struct k_thread *thread)
{
	if (_current != thread) {
		return;
	}

	arch_mem_domain_destroy(thread->mem_domain_info.mem_domain);
}

int arch_buffer_validate(void *addr, size_t size, int write)
{
	return arm_core_mpu_buffer_validate(addr, size, write);
}

#endif /* CONFIG_USERSPACE */
