//
// Created by peress on 29/06/23.
//

#include <zephyr/rtio/rtio.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/app_memory/app_memdomain.h>

K_APPMEM_PARTITION_DEFINE(sensing_mem_partition);

static int sensing_mem_init(void)
{
	int rc;

	rc = k_mem_domain_add_partition(&k_mem_domain_default, &sensing_mem_partition);
	if (rc != 0) {
		return rc;
	}
	return k_mem_domain_add_partition(&k_mem_domain_default, &rtio_partition);
}

SYS_INIT(sensing_mem_init, POST_KERNEL, 99);
