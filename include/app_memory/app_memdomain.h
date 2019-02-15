#ifndef ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_
#define ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_

#include <linker/linker-defs.h>
#include <misc/dlist.h>
#include <kernel.h>

#if defined(CONFIG_X86)
#define MEM_DOMAIN_ALIGN_SIZE _STACK_BASE_ALIGN
#elif defined(STACK_ALIGN)
#define MEM_DOMAIN_ALIGN_SIZE STACK_ALIGN
#else
#error "Not implemented for this architecture"
#endif


/**
 * @brief Name of the data section for a particular partition
 *
 * Useful for defining memory pools, or any other macro that takes a
 * section name as a parameter.
 *
 * @param id Partition name
 */
#define K_APP_DMEM_SECTION(id) data_smem_##id##_data

/**
 * @brief Name of the bss section for a particular partition
 *
 * Useful for defining memory pools, or any other macro that takes a
 * section name as a parameter.
 *
 * @param id Partition name
 */
#define K_APP_BMEM_SECTION(id) data_smem_##id##_bss

/**
 * @brief Place data in a partition's data section
 *
 * Globals tagged with this will end up in the data section for the
 * specified memory partition. This data should be initialized to some
 * desired value.
 *
 * @param id Name of the memory partition to associate this data
 */
#define K_APP_DMEM(id) _GENERIC_SECTION(K_APP_DMEM_SECTION(id))

/**
 * @brief Place data in a partition's bss section
 *
 * Globals tagged with this will end up in the bss section for the
 * specified memory partition. This data will be zeroed at boot.
 *
 * @param id Name of the memory partition to associate this data
 */
#define K_APP_BMEM(id) _GENERIC_SECTION(K_APP_BMEM_SECTION(id))

struct z_app_region {
	void *bss_start;
	size_t bss_size;
};

#define Z_APP_START(id) data_smem_##id##_start
#define Z_APP_SIZE(id) data_smem_##id##_size
#define Z_APP_BSS_START(id) data_smem_##id##_bss_start
#define Z_APP_BSS_SIZE(id) data_smem_##id##_bss_size

/**
 * @brief Define an application memory partition with linker support
 *
 * Defines a k_mem_paritition with the provided name.
 * This name may be used with the K_APP_DMEM and K_APP_BMEM macros to
 * place globals automatically in this partition.
 *
 * NOTE: placeholder char variable is defined here to prevent build errors
 * if a partition is defined but nothing ever placed in it.
 *
 * @param name Name of the k_mem_partition to declare
 */
#define K_APPMEM_PARTITION_DEFINE(name) \
	extern char Z_APP_START(name)[]; \
	extern char Z_APP_SIZE(name)[]; \
	struct k_mem_partition name = { \
		.start = (u32_t) &Z_APP_START(name), \
		.size = (u32_t) &Z_APP_SIZE(name), \
		.attr = K_MEM_PARTITION_P_RW_U_RW \
	}; \
	extern char Z_APP_BSS_START(name)[]; \
	extern char Z_APP_BSS_SIZE(name)[]; \
	_GENERIC_SECTION(.app_regions.name) \
	struct z_app_region name##_region = { \
		.bss_start = &Z_APP_BSS_START(name), \
		.bss_size = (size_t) &Z_APP_BSS_SIZE(name) \
	}; \
	K_APP_BMEM(name) char name##_placeholder;

#endif /* ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_ */
