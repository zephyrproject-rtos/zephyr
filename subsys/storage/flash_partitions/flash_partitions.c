/*
 * Copyright (c) 2024, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_partitions.h>
#include <string.h>

LOG_MODULE_REGISTER(flash_partitions, CONFIG_FLASH_PARTITIONS_LOG_LEVEL);

size_t flash_partition_get_size(const struct flash_partition *partition)
{
	if (partition == NULL) {
		return 0U;
	}

	return partition->size;
}

size_t flash_partition_get_ebs(const struct flash_partition *partition)
{
	if (partition == NULL) {
		return 0U;
	}

	return partition->erase_block_size;
}

const char *flash_partition_get_label(const struct flash_partition *partition)
{
#if CONFIG_FLASH_PARTITIONS_LABELS
	return partition->label;
#else
	return NULL;
#endif
}

int flash_partition_open(const struct flash_partition *partition)
{
	int rc;

	if (partition == NULL) {
		rc = -EINVAL;
		goto end;
	}

	if (partition->open == NULL) {
		rc = -ENOTSUP;
		goto end;
	}

	rc = partition->open(partition);
end:
	LOG_DBG("open: [%d]", rc);
	return rc;
}

int flash_partition_read(const struct flash_partition *partition, size_t start,
			 void *data, size_t len)
{
	int rc;

	if (partition == NULL) {
		rc = -EINVAL;
		goto end;
	}

	if (partition->read == NULL) {
		rc = -ENOTSUP;
		goto end;
	}

	if ((start + len) > partition->size) {
		rc = -EINVAL;
		goto end;
	}

	rc = partition->read(partition, partition->offset + start, data, len);
end:
	LOG_DBG("read %d byte at 0x%x: [%d]", len, start, rc);
	return rc;
}

int flash_partition_write(const struct flash_partition *partition, size_t start,
			  const void *data, size_t len)
{
	int rc;

	if (partition == NULL) {
		rc = -EINVAL;
		goto end;
	}

	if (partition->write == NULL) {
		rc = -ENOTSUP;
		goto end;
	}

	if ((start + len) > partition->size) {
		rc = -EINVAL;
		goto end;
	}

	rc = partition->write(partition, partition->offset + start, data, len);
end:
	LOG_DBG("write %d byte at 0x%x: [%d]", len, start, rc);
	return rc;
}

int flash_partition_erase(const struct flash_partition *partition, size_t start,
			  size_t len)
{
	int rc;

	if (partition == NULL) {
		rc = -EINVAL;
		goto end;
	}

	if (partition->erase == NULL) {
		rc = -ENOTSUP;
		goto end;
	}

	if ((start + len) > partition->size) {
		rc = -EINVAL;
		goto end;
	}

	rc = partition->erase(partition, partition->offset + start, len);
end:
	LOG_DBG("erase %d byte at 0x%x: [%d]", len, start, rc);
	return rc;
}

int flash_partition_close(const struct flash_partition *partition)
{
	int rc;

	if (partition == NULL) {
		rc = -EINVAL;
		goto end;
	}

	if (partition->close == NULL) {
		rc = -ENOTSUP;
		goto end;
	}

	rc = partition->close(partition);
end:
	LOG_DBG("close: [%d]", rc);
	return rc;
}

static int fp_open(const struct flash_partition *partition)
{
	int rc = 0;

	if (!device_is_ready(partition->fldev)) {
		rc = -ENODEV;
		goto end;
	}

	if (IS_ENABLED(CONFIG_FLASH_PARTITIONS_RUNTIME_VERIFY)) {
		const size_t end = partition->offset + partition->size;
		struct flash_pages_info page = {
			.start_offset = (off_t)partition->offset,
		};

		while (page.start_offset < end) {
			rc = flash_get_page_info_by_offs(
				partition->fldev, page.start_offset, &page);
			if (rc != 0) {
				LOG_ERR("failed to get flash page info");
				break;
			}

			if ((partition->erase_block_size % page.size) != 0U) {
				LOG_ERR("erase-block-size configuration error");
				rc = -EINVAL;
				break;
			}

			page.start_offset += page.size;
		}
	}
end:
	return rc;
}

static int fp_read(const struct flash_partition *partition, size_t start,
		   void *data, size_t len)
{
	return flash_read(partition->fldev, (off_t)start, data, len);
}

static int fp_write(const struct flash_partition *partition, size_t start,
		    const void *data, size_t len)
{
	return flash_write(partition->fldev, (off_t)start, data, len);
}

static int fp_erase(const struct flash_partition *partition, size_t start,
		    size_t len)
{
	return flash_erase(partition->fldev, (off_t)start, len);
}

static int fp_close(const struct flash_partition *partition)
{
	return 0;
}

#define FLASH_PARTITION_SIZE(inst) DT_REG_SIZE(inst)
#define FLASH_PARTITION_OFF(inst)  DT_REG_ADDR(inst)
#define FLASH_PARTITION_GPDEV(inst)                                             \
	COND_CODE_1(DT_NODE_HAS_COMPAT(inst, soc_nv_flash),                     \
		    (DEVICE_DT_GET(DT_PARENT(inst))), (DEVICE_DT_GET(inst)))
#define FLASH_PARTITION_DEV(inst) FLASH_PARTITION_GPDEV(DT_GPARENT(inst))
#define FLASH_PARTITION_EBS(inst)                                               \
	DT_PROP_OR(inst, erase_block_size,                                      \
		   (DT_PROP_OR(DT_GPARENT(inst), erase_block_size,              \
			       (FLASH_PARTITION_SIZE(inst)))))
#define FLASH_PARTITION_WRITE_DISABLE(inst) DT_PROP_OR(inst, read_only, false)
#define FLASH_PARTITION_ERASE_DISABLE(inst)                                     \
	DT_PROP_OR(inst, read_only, false) ||                                   \
		DT_NODE_HAS_COMPAT(DT_GPARENT(inst), zephyr_flash_no_erase)
#define FLASH_PARTITION_LABEL(inst) DT_PROP_OR(inst, label, NULL)

#if CONFIG_FLASH_PARTITIONS_LABELS
#define FLASH_PARTITION_DEFINE(inst)                                            \
	const struct flash_partition flash_partition_##inst = {                 \
		.fldev = FLASH_PARTITION_DEV(inst),                             \
		.size = FLASH_PARTITION_SIZE(inst),                             \
		.offset = FLASH_PARTITION_OFF(inst),                            \
		.erase_block_size = FLASH_PARTITION_EBS(inst),                  \
		.open = fp_open,                                                \
		.read = fp_read,                                                \
		.write = FLASH_PARTITION_WRITE_DISABLE(inst) ? NULL : fp_write, \
		.erase = FLASH_PARTITION_ERASE_DISABLE(inst) ? NULL : fp_erase, \
		.close = fp_close,                                              \
		.label = FLASH_PARTITION_LABEL(inst),                           \
	};
#else
#define FLASH_PARTITION_DEFINE(inst)                                            \
	const struct flash_partition flash_partition_##inst = {                 \
		.fldev = FLASH_PARTITION_DEV(inst),                             \
		.size = FLASH_PARTITION_SIZE(inst),                             \
		.offset = FLASH_PARTITION_OFF(inst),                            \
		.erase_block_size = FLASH_PARTITION_EBS(inst),                  \
		.open = fp_open,                                                \
		.read = fp_read,                                                \
		.write = FLASH_PARTITION_WRITE_DISABLE(inst) ? NULL : fp_write, \
		.erase = FLASH_PARTITION_ERASE_DISABLE(inst) ? NULL : fp_erase, \
		.close = fp_close,                                              \
	};
#endif

#define FLASH_PARTITIONS_DEFINE(inst)                                           \
	DT_FOREACH_CHILD(inst, FLASH_PARTITION_DEFINE)

DT_FOREACH_STATUS_OKAY(zephyr_flash_partitions, FLASH_PARTITIONS_DEFINE)
