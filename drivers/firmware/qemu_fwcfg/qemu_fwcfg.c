/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qemu_fwcfg.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <string.h>

LOG_MODULE_REGISTER(qemu_fwcfg, CONFIG_LOG_DEFAULT_LEVEL);

#define FWCFG_DMA_CTL_ERROR    BIT(0)
#define FWCFG_DMA_CTL_READ     BIT(1)
#define FWCFG_DMA_CTL_SELECT   BIT(3)
#define FWCFG_DMA_CTL_WRITE    BIT(4)
#define FWCFG_DMA_SELECT_SHIFT 16
#define FWCFG_FILE_NAME_LEN    56

struct fwcfg_file_entry {
	uint32_t size;
	uint16_t select;
	uint16_t reserved;
	char name[FWCFG_FILE_NAME_LEN];
};

static inline struct fwcfg_data *data_of(const struct device *dev)
{
	return (struct fwcfg_data *)dev->data;
}

static int fwcfg_stream_read_be16(const struct device *dev, uint16_t *value)
{
	uint8_t buf[2];
	int rc = fwcfg_read_selected(dev, buf, sizeof(*value));

	if (rc != 0) {
		return rc;
	}

	*value = sys_get_be16(buf);
	return 0;
}

static int fwcfg_stream_read_be32(const struct device *dev, uint32_t *value)
{
	uint8_t buf[4];
	int rc = fwcfg_read_selected(dev, buf, sizeof(*value));

	if (rc != 0) {
		return rc;
	}

	*value = sys_get_be32(buf);
	return 0;
}

static int fwcfg_dma_xfer(const struct device *dev, uint32_t control, uintptr_t phys_buffer,
			  size_t len)
{
	struct fwcfg_data *data = data_of(dev);
	struct fwcfg_dma_access *desc = &data->dma_desc;
	volatile struct fwcfg_dma_access *vdesc = desc;
	uint64_t descriptor_addr;
	int poll_iter;
	int rc;

	__ASSERT(len > 0U, "DMA length must not be zero");
	__ASSERT(len <= UINT32_MAX, "DMA length exceeds descriptor capacity");

	desc->control = sys_cpu_to_be32(control);
	desc->length = sys_cpu_to_be32((uint32_t)len);
	desc->address = sys_cpu_to_be64((uint64_t)phys_buffer);

	descriptor_addr = (uint64_t)k_mem_phys_addr(desc);
	barrier_dmem_fence_full();
	rc = fwcfg_dma_kick(dev, descriptor_addr);

	if (rc != 0) {
		return rc;
	}

	control = sys_be32_to_cpu(vdesc->control);
	poll_iter = 0;
	while (control != 0U && control != FWCFG_DMA_CTL_ERROR &&
	       poll_iter < FWCFG_DMA_POLL_MAX_ITER) {
		k_busy_wait(FWCFG_DMA_POLL_WAIT_US);
		control = sys_be32_to_cpu(vdesc->control);
		poll_iter++;
	}

	if (control == FWCFG_DMA_CTL_ERROR) {
		return -EIO;
	}
	if (control != 0U) {
		return -ETIMEDOUT;
	}

	barrier_dmem_fence_full();
	return 0;
}

static int fwcfg_dma_read(const struct device *dev, uint16_t key, void *buf, size_t len)
{
	struct fwcfg_data *data = data_of(dev);
	uint8_t *dst = (uint8_t *)buf;
	bool first = true;
	size_t remaining = len;
	uint32_t control;
	size_t chunk_len;
	int rc;

	while (remaining > 0U) {
		chunk_len = MIN(remaining, sizeof(data->dma_bounce));
		control = FWCFG_DMA_CTL_READ;
		if (first) {
			control |= FWCFG_DMA_CTL_SELECT | ((uint32_t)key << FWCFG_DMA_SELECT_SHIFT);
		}

		rc = fwcfg_dma_xfer(dev, control, k_mem_phys_addr(data->dma_bounce), chunk_len);
		if (rc != 0) {
			return rc;
		}

		memcpy(dst, data->dma_bounce, chunk_len);
		dst += chunk_len;
		remaining -= chunk_len;
		first = false;
	}

	return 0;
}

static int fwcfg_dma_write(const struct device *dev, uint16_t key, const void *buf, size_t len)
{
	struct fwcfg_data *data = data_of(dev);
	const uint8_t *src = (const uint8_t *)buf;
	bool first = true;
	size_t remaining = len;
	uint32_t control;
	size_t chunk_len;
	int rc;

	while (remaining > 0U) {
		chunk_len = MIN(remaining, sizeof(data->dma_bounce));
		memcpy(data->dma_bounce, src, chunk_len);

		control = FWCFG_DMA_CTL_WRITE;
		if (first) {
			control |= FWCFG_DMA_CTL_SELECT | ((uint32_t)key << FWCFG_DMA_SELECT_SHIFT);
		}

		rc = fwcfg_dma_xfer(dev, control, k_mem_phys_addr(data->dma_bounce), chunk_len);
		if (rc != 0) {
			return rc;
		}

		src += chunk_len;
		remaining -= chunk_len;
		first = false;
	}

	return 0;
}

static int fwcfg_xfer_read_item(const struct device *dev, uint16_t key, void *buf, size_t len)
{
	int rc;

	if (qemu_fwcfg_dma_supported(dev)) {
		return fwcfg_dma_read(dev, key, buf, len);
	}

	rc = fwcfg_select(dev, key);

	if (rc != 0) {
		return rc;
	}

	return fwcfg_read_selected(dev, buf, len);
}

static int fwcfg_xfer_write_item(const struct device *dev, uint16_t key, const void *buf,
				 size_t len)
{
	if (qemu_fwcfg_dma_supported(dev)) {
		return fwcfg_dma_write(dev, key, buf, len);
	}

	return -ENOTSUP;
}

static int fwcfg_read_file_entry(const struct device *dev, struct fwcfg_file_entry *entry)
{
	int rc;

	rc = fwcfg_stream_read_be32(dev, &entry->size);
	if (rc != 0) {
		return rc;
	}

	rc = fwcfg_stream_read_be16(dev, &entry->select);
	if (rc != 0) {
		return rc;
	}

	rc = fwcfg_stream_read_be16(dev, &entry->reserved);
	if (rc != 0) {
		return rc;
	}

	rc = fwcfg_read_selected(dev, entry->name, sizeof(entry->name));
	if (rc != 0) {
		return rc;
	}

	entry->name[ARRAY_SIZE(entry->name) - 1] = '\0';
	return 0;
}

static int fwcfg_find_file_impl(const struct device *dev, const char *file, uint16_t *select_out,
				uint32_t *size_out)
{
	uint32_t count;
	int rc;

	rc = fwcfg_select(dev, FW_CFG_FILE_DIR);
	if (rc != 0) {
		return rc;
	}

	rc = fwcfg_stream_read_be32(dev, &count);
	if (rc != 0) {
		return rc;
	}

	LOG_DBG("fw_cfg file count: %u", count);

	for (uint32_t i = 0; i < count; i++) {
		struct fwcfg_file_entry entry;

		rc = fwcfg_read_file_entry(dev, &entry);
		if (rc != 0) {
			return rc;
		}

		LOG_DBG("entry[%u]: select=0x%04x size=%u name=%s", i, entry.select, entry.size,
			entry.name);

		if (strcmp(entry.name, file) == 0) {
			*select_out = entry.select;
			*size_out = entry.size;
			return 0;
		}
	}

	return -ENOENT;
}

int qemu_fwcfg_read_item(const struct device *dev, uint16_t key, void *buf, size_t len)
{
	int rc;

	__ASSERT(dev != NULL, "device must not be NULL");
	__ASSERT(buf != NULL, "buffer must not be NULL");
	__ASSERT(len > 0U, "length must not be zero");

	k_mutex_lock(&data_of(dev)->lock, K_FOREVER);

	rc = fwcfg_xfer_read_item(dev, key, buf, len);

	k_mutex_unlock(&data_of(dev)->lock);
	return rc;
}

int qemu_fwcfg_write_item(const struct device *dev, uint16_t key, const void *buf, size_t len)
{
	int rc;

	__ASSERT(dev != NULL, "device must not be NULL");
	__ASSERT(buf != NULL, "buffer must not be NULL");
	__ASSERT(len > 0U, "length must not be zero");

	k_mutex_lock(&data_of(dev)->lock, K_FOREVER);

	rc = fwcfg_xfer_write_item(dev, key, buf, len);

	k_mutex_unlock(&data_of(dev)->lock);
	return rc;
}

int qemu_fwcfg_get_features(const struct device *dev, uint32_t *features)
{
	__ASSERT(dev != NULL, "device must not be NULL");
	__ASSERT(features != NULL, "features must not be NULL");

	*features = data_of(dev)->features;
	return 0;
}

bool qemu_fwcfg_dma_supported(const struct device *dev)
{
	const struct fwcfg_api *api = fwcfg_get_api(dev);

	__ASSERT(api != NULL, "fwcfg api must be present");

	return (data_of(dev)->features & FW_CFG_ID_F_DMA) != 0U && api->dma_kick != NULL;
}

int fwcfg_init_common(const struct device *dev)
{
	struct fwcfg_data *data;
	uint8_t sig[4];
	uint32_t id_le;
	int rc;

	k_mutex_init(&data_of(dev)->lock);

	data = data_of(dev);

	rc = fwcfg_xfer_read_item(dev, FW_CFG_SIGNATURE, sig, sizeof(sig));
	if (rc != 0) {
		return rc;
	}

	if (memcmp(sig, "QEMU", 4) != 0) {
		return -ENODEV;
	}

	rc = fwcfg_xfer_read_item(dev, FW_CFG_ID, &id_le, sizeof(id_le));
	if (rc != 0) {
		return rc;
	}

	data->features = sys_le32_to_cpu(id_le);
	return 0;
}

int qemu_fwcfg_find_file(const struct device *dev, const char *file, uint16_t *select,
			 uint32_t *size)
{
	int rc;

	__ASSERT(dev != NULL, "device must not be NULL");
	__ASSERT(file != NULL, "file name must not be NULL");
	__ASSERT(select != NULL, "select output must not be NULL");
	__ASSERT(size != NULL, "size output must not be NULL");

	k_mutex_lock(&data_of(dev)->lock, K_FOREVER);

	rc = fwcfg_find_file_impl(dev, file, select, size);

	k_mutex_unlock(&data_of(dev)->lock);
	return rc;
}
