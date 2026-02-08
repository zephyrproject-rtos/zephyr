/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fwcfg.h"

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

LOG_MODULE_REGISTER(fwcfg, CONFIG_LOG_DEFAULT_LEVEL);

static inline const struct fwcfg_config *cfg_of(const struct device *dev)
{
	return (const struct fwcfg_config *)dev->config;
}

static inline struct fwcfg_data *data_of(const struct device *dev)
{
	return (struct fwcfg_data *)dev->data;
}

static int fwcfg_validate_dev(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (!data_of(dev)->present) {
		return -ENODEV;
	}

	return 0;
}

struct fwcfg_file_entry {
	uint32_t size;
	uint16_t select;
	uint16_t reserved;
	char name[56];
};

static int fwcfg_xfer_read_selected(const struct device *dev, void *buf, size_t len)
{
	const struct fwcfg_config *cfg = cfg_of(dev);

	__ASSERT(cfg != NULL, "fwcfg config is NULL");
	__ASSERT(cfg->ops != NULL, "fwcfg ops are NULL");
	__ASSERT(cfg->ops->read != NULL, "fwcfg read op is NULL");

	return cfg->ops->read(dev, (uint8_t *)buf, len);
}

static int fwcfg_stream_read_be16(const struct device *dev, uint16_t *value)
{
	uint8_t buf[sizeof(*value)];
	int rc = fwcfg_xfer_read_selected(dev, buf, sizeof(buf));

	if (rc != 0) {
		return rc;
	}

	*value = sys_get_be16(buf);
	return 0;
}

static int fwcfg_stream_read_be32(const struct device *dev, uint32_t *value)
{
	uint8_t buf[sizeof(*value)];
	int rc = fwcfg_xfer_read_selected(dev, buf, sizeof(buf));

	if (rc != 0) {
		return rc;
	}

	*value = sys_get_be32(buf);
	return 0;
}

static int fwcfg_xfer_read_item(const struct device *dev, uint16_t key, void *buf, size_t len)
{
	const struct fwcfg_config *cfg = cfg_of(dev);
	struct fwcfg_data *data = data_of(dev);

	if (data->dma_ok && cfg->ops->read_dma != NULL) {
		return cfg->ops->read_dma(dev, key, (uint8_t *)buf, len);
	}

	int rc = cfg->ops->select(dev, key);

	if (rc != 0) {
		return rc;
	}

	return fwcfg_xfer_read_selected(dev, buf, len);
}

static int fwcfg_xfer_write_item(const struct device *dev, uint16_t key, const void *buf,
				 size_t len)
{
	const struct fwcfg_config *cfg = cfg_of(dev);
	struct fwcfg_data *data = data_of(dev);

	if (data->dma_ok && cfg->ops->write_dma != NULL) {
		return cfg->ops->write_dma(dev, key, (const uint8_t *)buf, len);
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

	rc = fwcfg_xfer_read_selected(dev, entry->name, sizeof(entry->name));
	if (rc != 0) {
		return rc;
	}

	entry->name[ARRAY_SIZE(entry->name) - 1] = '\0';
	return 0;
}

static int fwcfg_find_file_impl(const struct device *dev, const char *file, uint16_t *select_out)
{
	uint32_t count;
	int rc;

	rc = cfg_of(dev)->ops->select(dev, FW_CFG_FILE_DIR);
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
			return 0;
		}
	}

	return -ENOENT;
}

int fwcfg_read_item(const struct device *dev, uint16_t key, void *buf, size_t len)
{
	int rc;

	if ((buf == NULL) && (len != 0U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	rc = fwcfg_validate_dev(dev);
	if (rc != 0) {
		return rc;
	}

	return fwcfg_xfer_read_item(dev, key, buf, len);
}

int fwcfg_write_item(const struct device *dev, uint16_t key, const void *buf, size_t len)
{
	int rc;

	if ((buf == NULL) && (len != 0U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	rc = fwcfg_validate_dev(dev);
	if (rc != 0) {
		return rc;
	}

	return fwcfg_xfer_write_item(dev, key, buf, len);
}

int fwcfg_probe(const struct device *dev)
{
	struct fwcfg_data *data = data_of(dev);
	uint8_t sig[4];
	uint32_t id_le;
	int rc;

	data->present = false;
	data->dma_ok = false;
	data->features = 0;

	rc = fwcfg_xfer_read_item(dev, FW_CFG_SIGNATURE, sig, sizeof(sig));
	if (rc != 0) {
		return rc;
	}

	if (sig[0] != 'Q' || sig[1] != 'E' || sig[2] != 'M' || sig[3] != 'U') {
		return -ENODEV;
	}

	rc = fwcfg_xfer_read_item(dev, FW_CFG_ID, &id_le, sizeof(id_le));
	if (rc != 0) {
		return rc;
	}

	data->features = sys_le32_to_cpu(id_le);
	data->dma_ok = (data->features & FW_CFG_ID_F_DMA) != 0U;
	data->present = true;
	return 0;
}

int fwcfg_get_features(const struct device *dev, uint32_t *features)
{
	int rc;

	if (features == NULL) {
		return -EINVAL;
	}

	rc = fwcfg_validate_dev(dev);
	if (rc != 0) {
		return rc;
	}

	*features = data_of(dev)->features;
	return 0;
}

bool fwcfg_dma_supported(const struct device *dev)
{
	if (fwcfg_validate_dev(dev) != 0) {
		return false;
	}

	return data_of(dev)->dma_ok;
}

static int fwcfg_init(const struct device *dev)
{
	const struct fwcfg_config *cfg = cfg_of(dev);

	if (cfg->transport == FWCFG_MMIO) {
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	}

	return fwcfg_probe(dev);
}

int fwcfg_find_file(const struct device *dev, const char *file, uint16_t *select)
{
	int rc;

	if ((file == NULL) || (select == NULL)) {
		return -EINVAL;
	}

	rc = fwcfg_validate_dev(dev);
	if (rc != 0) {
		return rc;
	}

	return fwcfg_find_file_impl(dev, file, select);
}

extern const struct fwcfg_ops fwcfg_mmio_ops;
extern const struct fwcfg_ops fwcfg_ioport_ops;

#if DT_HAS_COMPAT_STATUS_OKAY(qemu_fw_cfg_mmio)
#define FWCFG_MMIO_DEVICE_DEFINE(node_id)                                                          \
	static struct fwcfg_data fwcfg_data_mmio_##node_id;                                        \
	static const struct fwcfg_config fwcfg_cfg_mmio_##node_id = {                              \
		DEVICE_MMIO_ROM_INIT(node_id),                                                     \
		.ops = &fwcfg_mmio_ops,                                                            \
		.transport = FWCFG_MMIO,                                                           \
		.u.mmio.base = DT_REG_ADDR(node_id),                                               \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, fwcfg_init, NULL, &fwcfg_data_mmio_##node_id,                    \
			 &fwcfg_cfg_mmio_##node_id, POST_KERNEL,                                   \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL)

DT_FOREACH_STATUS_OKAY(qemu_fw_cfg_mmio, FWCFG_MMIO_DEVICE_DEFINE);
#undef FWCFG_MMIO_DEVICE_DEFINE
#endif

#if defined(CONFIG_X86) && DT_HAS_COMPAT_STATUS_OKAY(qemu_fw_cfg_ioport)
#define FWCFG_IOPORT_DEVICE_DEFINE(node_id)                                                        \
	static struct fwcfg_data fwcfg_data_ioport_##node_id;                                      \
	static const struct fwcfg_config fwcfg_cfg_ioport_##node_id = {                            \
		._mmio = {0},                                                                      \
		.ops = &fwcfg_ioport_ops,                                                          \
		.transport = FWCFG_IOPORT,                                                         \
		.u.io.sel_port = DT_REG_ADDR(node_id),                                             \
		.u.io.data_port = DT_REG_ADDR(node_id) + 1U,                                       \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, fwcfg_init, NULL, &fwcfg_data_ioport_##node_id,                  \
			 &fwcfg_cfg_ioport_##node_id, POST_KERNEL,                                 \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL)

DT_FOREACH_STATUS_OKAY(qemu_fw_cfg_ioport, FWCFG_IOPORT_DEVICE_DEFINE);
#undef FWCFG_IOPORT_DEVICE_DEFINE
#endif
