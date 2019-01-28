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

/*
 * There has got to be a better way of doing this. This
 * tries to ensure that a) each subsection has a
 * data_smem_#id_b part and b) that each k_mem_partition
 * matches the page size or MPU region.  If there is no
 * data_smem_#id_b subsection, then the size calculations
 * will fail. Additionally, if each k_mem_partition does
 * not match the page size or MPU region, then the
 * partition will fail to be created.
 * checkpatch.pl complains that __aligned(size) is
 * preferred, but, if implemented, then complains about
 * complex macro without parentheses.
 */
#define _app_dmem_pad(id) \
	__attribute__((aligned(MEM_DOMAIN_ALIGN_SIZE), \
		section("data_smem_" #id)))

#define _app_bmem_pad(id) \
	__attribute__((aligned(MEM_DOMAIN_ALIGN_SIZE), \
		section("data_smem_" #id "b")))

/*
 * Qualifier to collect any object preceded with _app
 * and place into section "data_smem_".
 * _app_dmem(#) is for variables meant to be stored in .data .
 * _app_bmem(#) is intended for static variables that are
 * initialized to zero.
 */
#define _app_dmem(id) \
	__attribute__((section("data_smem_" #id)))

#define _app_bmem(id) \
	__attribute__((section("data_smem_" #id "b")))

/*
 * Creation of a struct to save start addresses, sizes, and
 * a pointer to a k_mem_partition.  It also adds a linked
 * list node.
 */
struct app_region {
	char *dmem_start;
	char *bmem_start;
	u32_t smem_size;
	u32_t dmem_size;
	u32_t bmem_size;
	struct k_mem_partition *partition;
	sys_dnode_t lnode;
};

/*
 * Declares a partition and provides a function to add the
 * partition to the linked list and initialize the partition.
 */
#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
/* For power of 2 MPUs linker provides support to help us
 * calculate the region sizes.
 */
#define smem_size_declare(name) extern char data_smem_##name##_size[]
#define smem_size_assign(name) name##_region.smem_size = (u32_t)&data_smem_##name##_size
#else
#define smem_size_declare(name)
#define smem_size_assign(name)
#endif	/* CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT */

#define appmem_partition(name) \
	extern char *data_smem_##name; \
	extern char *data_smem_##name##b; \
	smem_size_declare(name);		  \
	_app_dmem_pad(name) char name##_dmem_pad; \
	_app_bmem_pad(name) char name##_bmem_pad; \
	__kernel struct k_mem_partition name; \
	__kernel struct app_region name##_region; \
	static inline void appmem_init_part_##name(void) \
	{ \
		name##_region.dmem_start = (char *)&data_smem_##name; \
		name##_region.bmem_start = (char *)&data_smem_##name##b; \
		smem_size_assign(name);				\
		sys_dlist_append(&app_mem_list, &name##_region.lnode); \
		name.start = (u32_t) name##_region.dmem_start; \
		name.attr = K_MEM_PARTITION_P_RW_U_RW; \
		name##_region.partition = &name; \
	}

/*
 * The following allows the FOR_EACH macro to call each partition's
 * appmem_init_part_##name . Note: semicolon needed or else compiler
 * complains as semicolon needed for function call once expanded by
 * macro.
 */
#define appmem_init_part(name) \
	appmem_init_part_##name();

extern sys_dlist_t app_mem_list;

extern void app_bss_zero(void);

extern void app_calc_size(void);

extern void appmem_init_app_memory(void);

#endif /* ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_ */
