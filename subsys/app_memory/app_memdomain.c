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
#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
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

#else
		/* For power of 2 MPUs linker provides support to help us
		 * calculate the region sizes.
		 */
		struct app_region *region =
				CONTAINER_OF(node, struct app_region, lnode);

		region->dmem_size = (char *)region->bmem_start -
			(char *)region->dmem_start;
		region->bmem_size = (char *)region->smem_size -
			(char *)region->dmem_size;

		region->partition[0].size = (s32_t)region->smem_size;
#endif	/* CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT */
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

	app_calc_size();
	app_bss_zero();

#ifdef CONFIG_ARC
	extern void arc_core_mpu_enable(void);
	arc_core_mpu_enable();
#endif
}
