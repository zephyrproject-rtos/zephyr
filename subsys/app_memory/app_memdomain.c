#include <zephyr.h>
#include <init.h>
#include <app_memory/app_memdomain.h>
#include <string.h>
#include <misc/__assert.h>

extern char __app_shmem_regions_start[];
extern char __app_shmem_regions_end[];

static int app_shmem_bss_zero(struct device *unused)
{
	struct z_app_region *region, *end;

	end = (struct z_app_region *)&__app_shmem_regions_end;
	region = (struct z_app_region *)&__app_shmem_regions_start;

	for ( ; region < end; region++) {
		(void)memset(region->bss_start, 0, region->bss_size);
	}

	return 0;
}

SYS_INIT(app_shmem_bss_zero, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
