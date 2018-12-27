#ifndef ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_
#define ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_

#include <linker/linker-defs.h>
#include <misc/dlist.h>
#include <kernel.h>

#ifdef CONFIG_APP_SHARED_MEM

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
		section("data_smem_" #id "_data")))

#define _app_bmem_pad(id) \
	__attribute__((aligned(MEM_DOMAIN_ALIGN_SIZE), \
		section("data_smem_" #id "_bss")))

/*
 * Qualifier to collect any object preceded with _app
 * and place into section "data_smem_".
 * _app_dmem(#) is for variables meant to be stored in .data .
 * _app_bmem(#) is intended for static variables that are
 * initialized to zero.
 */
#define _app_dmem(id) \
	__attribute__((section("data_smem_" #id "_data")))

#define _app_bmem(id) \
	__attribute__((section("data_smem_" #id "_bss")))

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
#define appmem_partition(name) \
	__kernel struct k_mem_partition mem_partition_##name; \
	__kernel struct app_region name;

#define appmem_partition_header(name) \
	extern char data_smem_##name##_data_start[];			\
	extern char data_smem_##name##_bss_start[];			\
	extern char data_smem_##name##_size[];				\
	extern char data_smem_##name##_data_size[];			\
	extern char data_smem_##name##_bss_size[];			\
	extern struct k_mem_partition mem_partition_##name;		\
	extern struct app_region name;					\
	static inline void appmem_init_part_##name(void)		\
	{								\
		name.dmem_start = (char *)&data_smem_##name##_data_start; \
		name.bmem_start = (char *)&data_smem_##name##_bss_start; \
		name.smem_size = (u32_t)data_smem_##name##_size;	\
		name.dmem_size = (u32_t)data_smem_##name##_data_size;	\
		name.bmem_size = (u32_t)data_smem_##name##_bss_size;	\
		sys_dlist_append(&app_mem_list, &name.lnode);		\
		mem_partition_##name.start = (u32_t) name.dmem_start;	\
		mem_partition_##name.attr = K_MEM_PARTITION_P_RW_U_RW;	\
		mem_partition_##name.size = name.smem_size;		\
		name.partition = &mem_partition_##name;			\
	}

/* A helper function that should be called to define multiple partitions*/
#define APPMEM_PARTITION_OBJECT_DEFINE(...)\
	FOR_EACH(appmem_partition, __VA_ARGS__)

/* This defines all the functions that are needed to configure
 * the memory partitions. Usually goes in a common header file.
 */
#define APPMEM_PARTITION_HEADER_DEFINE(...)\
	FOR_EACH(appmem_partition_header, __VA_ARGS__)

/* Returns the mem_partition struct */
#define APPMEM_PARTITION_GET(name) (&mem_partition_##name)

/*
 * A wrapper around the k_mem_domain_* functions. Goal here was
 * to a) differentiate these operations from the k_mem_domain*
 * functions, and b) to simply the usage and handling of data
 * types (i.e. app_region, k_mem_domain, etc).
 */
#define appmem_domain(name) \
	__kernel struct k_mem_domain domain_##name;

#define appmem_domain_header(name) \
	extern struct k_mem_domain domain_##name; \
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
	static inline void appmem_add_custom_part_##name(struct		\
							 k_mem_partition\
							 *partition)	\
	{ \
		k_mem_domain_add_partition(&domain_##name, \
			partition); \
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

/* A helper function that should be called to define multiple domains
 * Only an object will get defined. Must be placed in a .c file.
 */
#define APPMEM_DOMAIN_OBJECT_DEFINE(...)\
	FOR_EACH(appmem_domain, __VA_ARGS__)

/* This defines all the functions that are needed to configure
 * the memory domains. Usually goes in a common header file.
 */
#define APPMEM_DOMAIN_HEADER_DEFINE(...)\
	FOR_EACH(appmem_domain_header, __VA_ARGS__)

/*
 * The following allows the FOR_EACH macro to call each partition's
 * appmem_init_part_##name . Note: semicolon needed or else compiler
 * complains as semicolon needed for function call once expanded by
 * macro.
 */
#define appmem_init_part(name) \
	appmem_init_part_##name();

/* A helper function that should be called to initialize multiple partitions */
#define APPMEM_INIT_PARTITIONS(...)		\
	FOR_EACH(appmem_init_part, __VA_ARGS__)

extern sys_dlist_t app_mem_list;

extern void appmem_init_app_memory(void);


#else  /* NO APP_SHARED_MEM */
#define APPMEM_INIT_PARTITIONS(...)
#define APPMEM_PARTITION_HEADER_DEFINE(...)
#define APPMEM_PARTITION_OBJECT_DEFINE(...)

#define APPMEM_DOMAIN_HEADER_DEFINE(...)
#define APPMEM_DOMAIN_OBJECT_DEFINE(...)

#define _app_dmem_pad(id)
#define _app_bmem_pad(id)

#define _app_dmem(id)
#define _app_bmem(id)

#endif /* CONFIG_APP_SHARED_MEM */

#endif /* ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_ */
