#include <zephyr.h>
#include <init.h>
#include <app_memory/app_memdomain.h>
#include <string.h>
#include <misc/__assert.h>

extern char __app_shmem_regions_start[];
extern char __app_shmem_regions_end[];

static int app_shmem_setup(struct device *unused)
{
	struct app_region *region, *end;
	size_t bss_size, data_size;

	end = (struct app_region *)&__app_shmem_regions_end;
	region = (struct app_region *)&__app_shmem_regions_start;

	for ( ; region < end; region++) {
		/* Determine bounds and set partition appropriately */
#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
		data_size = region->bmem_start - region->dmem_start;
		if (region == end - 1) {
			bss_size = _app_smem_end - region->bmem_start;
		} else {
			struct app_region *next = region + 1;

			__ASSERT_NO_MSG(region->bmem_start < next->dmem_start);
			bss_size = next->dmem_start - region->bmem_start;
		}
#else
		data_size = region->bmem_start - region->dmem_start;
		bss_size = region->smem_size - data_size;
#endif
		region->partition->size = data_size + bss_size;

		/* Zero out BSS area of each partition */
		(void)memset(region->bmem_start, 0, bss_size);
	}

	return 0;
}

SYS_INIT(app_shmem_setup, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
