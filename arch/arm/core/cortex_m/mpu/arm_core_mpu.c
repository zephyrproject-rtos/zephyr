/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>

#include <arch/arm/cortex_m/mpu/arm_core_mpu_dev.h>
#include <linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpu);


#if defined(CONFIG_APP_SHARED_MEM)
#define _MPU_DYNAMIC_REGIONS_AREA_START ((u32_t)&_app_smem_start)
#else
#define _MPU_DYNAMIC_REGIONS_AREA_START ((u32_t)&__kernel_ram_start)
#endif /* CONFIG_APP_SHARED_MEM */
#define _MPU_DYNAMIC_REGIONS_AREA_SIZE ((u32_t)&__kernel_ram_end - \
		_MPU_DYNAMIC_REGIONS_AREA_START)

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
void _arch_configure_static_mpu_regions(void)
{
	/* Define a constant array of k_mem_partition objects
	 * to hold the configuration of the respective static
	 * MPU regions.
	 */
	const struct k_mem_partition static_regions[] = {
#if defined(CONFIG_APPLICATION_MEMORY)
		{
		.start = (u32_t)&__app_ram_start,
		.size = (u32_t)&__app_ram_end - (u32_t)&__app_ram_start,
		.attr = K_MEM_PARTITION_P_RW_U_RW,
		},
#endif /* CONFIG_APPLICATION_MEMORY */
#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
		{
		.start = (u32_t)&__gcov_bss_start,
		.size = (u32_t)&__gcov_bss_end - (u32_t)&__gcov_bss_start,
		.attr = K_MEM_PARTITION_P_RW_U_RW,
		},
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */
#if defined(CONFIG_NOCACHE_MEMORY)
		{
		.start = (u32_t)&_nocache_ram_start,
		.size = (u32_t)&_nocache_ram_end - (u32_t)&_nocache_ram_start,
		.attr = K_MEM_PARTITION_P_RW_U_NA_NOCACHE,
		}
#endif /* CONFIG_NOCACHE_MEMORY */
	};

	/* Configure the static MPU regions within firmware SRAM boundaries.
	 * Start address of the image is given by _image_ram_start. The end
	 * of the firmware SRAM area is marked by __kernel_ram_end, taking
	 * into account the unused SRAM area, as well.
	 */
	arm_core_mpu_configure_static_mpu_regions(static_regions,
		ARRAY_SIZE(static_regions),
		(u32_t)&_image_ram_start,
		(u32_t)&__kernel_ram_end);

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
	/* Define a constant array of k_mem_partition objects that holds the
	 * boundaries of the areas, inside which dynamic region programming
	 * is allowed. The information is passed to the underlying driver at
	 * initialization.
	 */
	const struct k_mem_partition dyn_region_areas[] = {
#if defined(CONFIG_APPLICATION_MEMORY)
	/* Dynamic areas are also allowed in Application Memory. */
		{
		.start = (u32_t)&__app_ram_start,
		.size = (u32_t)&__app_ram_end - (u32_t)&__app_ram_start,
		},
#endif /* CONFIG_APPLICATION_MEMORY */
		{
		.start = _MPU_DYNAMIC_REGIONS_AREA_START,
		.size =  _MPU_DYNAMIC_REGIONS_AREA_SIZE,
		}
	};
	arm_core_mpu_mark_areas_for_dynamic_regions(dyn_region_areas,
		ARRAY_SIZE(dyn_region_areas));
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
}


#if defined(CONFIG_MPU_STACK_GUARD)
/*
 * @brief Configure MPU stack guard
 *
 * This function configures per thread stack guards reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_stack_guard(struct k_thread *thread)
{
	u32_t guard_size = MPU_GUARD_ALIGN_AND_SIZE;
#if defined(CONFIG_USERSPACE)
	u32_t guard_start = thread->arch.priv_stack_start ?
			    (u32_t)thread->arch.priv_stack_start :
			    (u32_t)thread->stack_obj;
#else
	u32_t guard_start = thread->stack_info.start;
#endif

	arm_core_mpu_disable();
	arm_core_mpu_configure(THREAD_STACK_GUARD_REGION, guard_start,
			       guard_size);
	arm_core_mpu_enable();
}
#endif

#if defined(CONFIG_USERSPACE)
/*
 * @brief Configure MPU user context
 *
 * This function configures the thread's user context.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_user_context(struct k_thread *thread)
{
	LOG_DBG("configure user thread %p's context", thread);
	arm_core_mpu_disable();
	arm_core_mpu_configure_user_context(thread);
	arm_core_mpu_enable();
}

/*
 * @brief Configure MPU memory domain
 *
 * This function configures per thread memory domain reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_mem_domain(struct k_thread *thread)
{
	LOG_DBG("configure thread %p's domain", thread);
	arm_core_mpu_disable();
	arm_core_mpu_configure_mem_domain(thread->mem_domain_info.mem_domain);
	arm_core_mpu_enable();
}

void _arch_mem_domain_configure(struct k_thread *thread)
{
	configure_mpu_mem_domain(thread);
}

int _arch_mem_domain_max_partitions_get(void)
{
	return arm_core_mpu_get_max_domain_partition_regions();
}

/*
 * Reset MPU region for a single memory partition
 */
void _arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				       u32_t  partition_id)
{
	ARG_UNUSED(domain);

	arm_core_mpu_disable();
	arm_core_mpu_mem_partition_remove(partition_id);
	arm_core_mpu_enable();

}

/*
 * Destroy MPU regions for the mem domain
 */
void _arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	ARG_UNUSED(domain);

	arm_core_mpu_disable();
	arm_core_mpu_configure_mem_domain(NULL);
	arm_core_mpu_enable();
}

/*
 * Validate the given buffer is user accessible or not
 */
int _arch_buffer_validate(void *addr, size_t size, int write)
{
	return arm_core_mpu_buffer_validate(addr, size, write);
}

#endif
