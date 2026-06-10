/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/amp/amp_openamp_ops_storage.h>

#include <zephyr/arch/cache.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(amp_openamp_memcpy_mapping, CONFIG_AMP_OPENAMP_OPS_LOG_LEVEL);

static K_MUTEX_DEFINE(loading_mutex);
static struct metal_io_region io_region;
static metal_phys_addr_t phys_base_address;

/*
 * Emulate get_mem so the address where a section needs to be loaded is provided
 * as "virtual" address from the own processor view.
 * This assumes a flat memory map as of now.
 */
struct remoteproc_mem *zephyr_openamp_get_mem(struct remoteproc *rproc, const char *name,
						     metal_phys_addr_t pa, metal_phys_addr_t da,
						     void *va, size_t size,
						     struct remoteproc_mem *buf)
{
	const struct amp_full_identification *full_id = rproc->priv;
	struct amp_memory_mapping map = {
		.target_device_address = da,
		.target_area_size = size,
	};

	int ret = amp_get_virtual_address(full_id->dev, &full_id->core_id, &map);
	if (ret < 0) {
		LOG_ERR("Error getting device address mapping");
		return NULL;
	}

	metal_io_init(&io_region, (void*) map.own_virtual_address_start, &phys_base_address, map.mapped_region_size, -1, 0, NULL);
	phys_base_address = map.target_device_area_start;

	buf->pa = map.target_device_address;
	buf->da = map.target_device_address;

	buf->size = map.mapped_region_size;

	buf->io = &io_region;

	return buf;
}

int zephyr_openamp_load_memcpy_open(void *store, const char *path, const void **img_data) {
	struct amp_memcpy_options *options = store;
	int ret;

	LOG_DBG("Storage open called");

	ret = k_mutex_lock(&loading_mutex, K_MSEC(CONFIG_AMP_OPENAMP_OPS_PROVIDE_STORAGE_OPS_LOCK_TIMEOUT));
	if (ret < 0) {
		LOG_ERR("Timeout while waiting for OpenAMP memcpy mutex %d", ret);
		return NULL;
	}
	*img_data = (void*) options->start_address;
	return options->image_size;
}

void zephyr_openamp_load_memcpy_close(void *store) {
	LOG_DBG("Storage close called");
	int ret = k_mutex_unlock(&loading_mutex);
	if (ret < 0) {
		LOG_ERR("Error unlocking OpenAMP memcpy mutex %d", ret);
	}
}

int zephyr_openamp_load_memcpy_load(void *store, size_t offset, size_t size,
		const void **data,
		metal_phys_addr_t pa,
		struct metal_io_region *io, char is_blocking)
{
	struct amp_memcpy_options *options = store;

	LOG_DBG("Storage load called");

	uintptr_t in_elf_start = options->start_address + offset;

	if (pa == METAL_BAD_PHYS) {
		*data = (void*) in_elf_start;
	} else {
		void *dest = metal_io_phys_to_virt(io, pa);
		LOG_DBG("Doing memcpy to %p from %p with size 0x%zx", dest, (void*) in_elf_start, size);
		memcpy(dest, (void*) in_elf_start, size);
		sys_cache_data_flush_range(dest, size);
	}
	return (int)size;
}
