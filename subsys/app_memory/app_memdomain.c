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
void app_bss_zero(void)
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
 * The following calculates the size of each subsection and adds
 * the computed sizes to the region structures. These calculations
 * are needed both for zeroizing "bss" parts of the partitions and
 * for the creation of the k_mem_partition.
 */
void app_calc_size(void)
{
	sys_dnode_t *node, *next_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&app_mem_list, node, next_node)
	{
		if (sys_dlist_is_tail(&app_mem_list, node)) {
			struct app_region *region =
				CONTAINER_OF(node, struct app_region, lnode);
			region->bmem_size =
				_app_smem_end -
				(char *)region->bmem_start;
			region->dmem_size =
				(char *)region->bmem_start -
				(char *)region->dmem_start;
			region->smem_size =
				region->bmem_size + region->dmem_size;
			region->partition[0].size =
				region->dmem_size + region->bmem_size;
		} else {
			struct app_region *region =
				CONTAINER_OF(node, struct app_region, lnode);
			struct app_region *nRegion =
				CONTAINER_OF(next_node, struct app_region,
					lnode);
			region->bmem_size =
				(char *)nRegion->dmem_start -
				(char *)region->bmem_start;
			region->dmem_size =
				(char *)region->bmem_start -
				(char *)region->dmem_start;
			region->smem_size =
				region->bmem_size + region->dmem_size;
			region->partition[0].size =
				region->dmem_size + region->bmem_size;
		}
	}
}

/*
 * "Initializes" by calculating subsection sizes and then
 * zeroizing "bss" regions.
 */
void appmem_init_app_memory(void)
{
	app_calc_size();
	app_bss_zero();
}
