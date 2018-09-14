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
 * partition to the linke dlist and initialize the partition.
 */
#define appmem_partition(name) \
	extern char *data_smem_##name; \
	extern char *data_smem_##name##b; \
	_app_dmem_pad(name) char name##_dmem_pad; \
	_app_bmem_pad(name) char name##_bmem_pad; \
	__kernel struct k_mem_partition mem_domain_##name; \
	__kernel struct app_region name; \
	static inline void appmem_init_part_##name(void) \
	{ \
		name.dmem_start = (char *)&data_smem_##name; \
		name.bmem_start = (char *)&data_smem_##name##b; \
		sys_dlist_append(&app_mem_list, &name.lnode); \
		mem_domain_##name.start = (u32_t) name.dmem_start; \
		mem_domain_##name.attr = K_MEM_PARTITION_P_RW_U_RW; \
		name.partition = &mem_domain_##name; \
	}

/*
 * A wrapper around the k_mem_domain_* functions. Goal here was
 * to a) differentiate these operations from the k_mem_domain*
 * functions, and b) to simply the usage and handling of data
 * types (i.e. app_region, k_mem_domain, etc).
 */
#define appmem_domain(name) \
	__kernel struct k_mem_domain domain_##name; \
	static inline void appmem_add_thread_##name(k_tid_t thread) \
	{ \
		k_mem_domain_add_thread(&domain_##name, thread); \
	} \
	static inline void appmem_rm_thread_##name(k_tid_t thread) \
	{ \
		k_mem_domain_remove_thread(thread); \
	} \
	static inline void appmem_add_part_##name(struct app_region region) \
	{ \
		k_mem_domain_add_partition(&domain_##name, \
			&region.partition[0]); \
	} \
	static inline void appmem_rm_part_##name(struct app_region region) \
	{ \
		k_mem_domain_remove_partition(&domain_##name, \
			&region.partition[0]); \
	} \
	static inline void appmem_init_domain_##name(struct app_region region) \
	{ \
		k_mem_domain_init(&domain_##name, 1, &region.partition); \
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
