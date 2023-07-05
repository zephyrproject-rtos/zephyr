//
// Created by peress on 29/06/23.
//

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sensing/sensing.h>

LOG_MODULE_REGISTER(sensing_userspace, CONFIG_SENSING_LOG_LEVEL);

K_APPMEM_PARTITION_DEFINE(sensing_mem_partition);

static int sensing_mem_init(void)
{
	int rc;

	rc = k_mem_domain_add_partition(&k_mem_domain_default, &sensing_mem_partition);
	if (rc != 0) {
		LOG_ERR("Failed to add sensing memory partition to the default domain");
		return rc;
	}
	rc = k_mem_domain_add_partition(&k_mem_domain_default, &rtio_partition);
	if (rc != 0) {
		LOG_ERR("Failed to add rtio partition to the default domain");
		return rc;
	}
	return 0;
}

SYS_INIT(sensing_mem_init, POST_KERNEL, 99);
