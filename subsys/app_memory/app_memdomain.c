#include <app_memory/app_memdomain.h>
#include <misc/dlist.h>
#include <stdarg.h>
#include <string.h>

/*
 * Initializes a double linked-list for the calculation of
 * memory subsections.
 */
sys_dlist_t app_mem_list = SYS_DLIST_STATIC_INIT(&app_mem_list);

/*
 * The following zeroizes each "bss" part of each subsection
 * as per the entries in the list.
 */
static inline void app_bss_zero(void)
{
	sys_dnode_t *node, *next_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&app_mem_list, node, next_node)
	{
		struct app_region *region =
			CONTAINER_OF(node, struct app_region, lnode);
		(void)memset(region->bmem_start, 0, region->bmem_size);
	}
}

/*
 * "Initializes" by calculating subsection sizes and then
 * zeroizing "bss" regions.
 */
void appmem_init_app_memory(void)
{
#ifdef CONFIG_ARC
	/*
	 * appmem_init_app_memory will access all partitions
	 * For CONFIG_ARC_MPU_VER == 3, these partitions are not added
	 * into MPU now, so need to disable mpu first to do app_bss_zero()
	 */
	extern void arc_core_mpu_disable(void);
	arc_core_mpu_disable();
#endif

	app_bss_zero();

#ifdef CONFIG_ARC
	extern void arc_core_mpu_enable(void);
	arc_core_mpu_enable();
#endif
}
