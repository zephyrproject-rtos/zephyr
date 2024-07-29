/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_retention

#include <string.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/retention/retention.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retention, CONFIG_RETENTION_LOG_LEVEL);

#define DATA_VALID_VALUE 1

#define INST_HAS_CHECKSUM(n) DT_INST_PROP(n, checksum) ||

#define INST_HAS_PREFIX(n) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prefix), (1), (0)) ||

/*
 * STM32H74x/H75x - RM0433 : when an incomplete word is written to an internal SRAM
 * and a reset occurs, the last incomplete word is not really written.
 * This is due to the ECC behavior.
 * To ensure that an incomplete word is written to SRAM, write an additional dummy
 * incomplete word to the same RAM at a different address before issuing a reset.
 * Choosing this different address to be offset 2 * offset of the checksum
 * TODO: consider other stm32 series than stm32H7
 */
#define INST_HAS_WRITE32(n) DT_INST_PROP(n, write32) ||

#if (DT_INST_FOREACH_STATUS_OKAY(INST_HAS_CHECKSUM) 0)
#define ANY_HAS_CHECKSUM
#endif

#if (DT_INST_FOREACH_STATUS_OKAY(INST_HAS_PREFIX) 0)
#define ANY_HAS_PREFIX
#endif

#if (DT_INST_FOREACH_STATUS_OKAY(INST_HAS_WRITE32) 0)
#define ANY_HAS_WRITE32
#endif

enum {
	CHECKSUM_NONE = 0,
	CHECKSUM_CRC8,
	CHECKSUM_CRC16,
	CHECKSUM_UNUSED,
	CHECKSUM_CRC32,
};

struct retention_data {
	bool header_written;
#ifdef CONFIG_RETENTION_MUTEXES
	struct k_mutex lock;
#endif
};

struct retention_config {
	const struct device *parent;
	size_t offset;
	size_t size;
	size_t reserved_size;
	uint8_t checksum_size;
	uint8_t prefix_len;
	uint8_t prefix[];
};

static inline void retention_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETENTION_MUTEXES
	struct retention_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void retention_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETENTION_MUTEXES
	struct retention_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

#ifdef ANY_HAS_CHECKSUM
static int retention_checksum(const struct device *dev, uint32_t *output)
{
	const struct retention_config *config = dev->config;
	int rc = -ENOSYS;

	if (config->checksum_size == CHECKSUM_CRC8 ||
	    config->checksum_size == CHECKSUM_CRC16 ||
	    config->checksum_size == CHECKSUM_CRC32) {
		size_t pos = config->offset + config->prefix_len;
		size_t end = config->offset + config->size - config->checksum_size;
		uint8_t buffer[CONFIG_RETENTION_BUFFER_SIZE];

		*output = 0;

		while (pos < end) {
			uint16_t read_size = MIN((end - pos), sizeof(buffer));

			rc = retained_mem_read(config->parent, pos, buffer, read_size);

			if (rc < 0) {
				goto finish;
			}

			if (config->checksum_size == CHECKSUM_CRC8) {
				*output = (uint32_t)crc8(buffer, read_size, 0x12,
							 (uint8_t)*output, false);
			} else if (config->checksum_size == CHECKSUM_CRC16) {
				*output = (uint32_t)crc16_itu_t((uint16_t)*output,
								buffer, read_size);
			} else if (config->checksum_size == CHECKSUM_CRC32) {
				*output = crc32_ieee_update(*output, buffer, read_size);
			}

			pos += read_size;
		}
	}

finish:
	return rc;
}
#endif

static int retention_init(const struct device *dev)
{
	const struct retention_config *config = dev->config;
#ifdef CONFIG_RETENTION_MUTEXES
	struct retention_data *data = dev->data;
#endif
	ssize_t area_size;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Parent device is not ready");
		return -ENODEV;
	}

	/* Ensure backend has a large enough storage area for the requirements of
	 * this retention area
	 */
	area_size = retained_mem_size(config->parent);

	if (area_size < 0) {
		LOG_ERR("Parent initialisation failure: %d", area_size);
		return area_size;
	}

	if ((config->offset + config->size) > area_size) {
		/* Backend storage is insufficient */
		LOG_ERR("Underlying area size is insufficient, requires: 0x%x, has: 0x%x",
			(config->offset + config->size), area_size);
		return -EINVAL;
	}

#ifdef CONFIG_RETENTION_MUTEXES
	k_mutex_init(&data->lock);
#endif

	return 0;
}

ssize_t retention_size(const struct device *dev)
{
	const struct retention_config *config = dev->config;

	return (config->size - config->reserved_size);
}

int retention_is_valid(const struct device *dev)
{
	const struct retention_config *config = dev->config;
	int rc = 0;

	retention_lock_take(dev);

	/* If neither the header or checksum are enabled, return a not supported error */
	if (config->prefix_len == 0 && config->checksum_size == 0) {
		LOG_ERR("Neither header nor checksum are enabled");
		rc = -ENOTSUP;
		goto finish;
	}

#ifdef ANY_HAS_PREFIX
	if (config->prefix_len != 0) {
		/* Check magic header is present at the start of the section */
		struct retention_data *data = dev->data;
		uint8_t buffer[CONFIG_RETENTION_BUFFER_SIZE];
		off_t pos = 0;

		while (pos < config->prefix_len) {
			uint16_t read_size = MIN((config->prefix_len - pos), sizeof(buffer));

			rc = retained_mem_read(config->parent, (config->offset + pos), buffer,
					       read_size);

			if (rc < 0) {
				goto finish;
			}

			if (memcmp(&config->prefix[pos], buffer, read_size) != 0) {
				/* If the magic header does not match, do not check the rest of
				 * the validity of the data, assume it is invalid
				 */
				data->header_written = false;
				rc = 0;
				goto finish;
			}

			pos += read_size;
		}

		/* Header already exists so no need to re-write it again */
		data->header_written = true;
	}
#endif

#ifdef ANY_HAS_CHECKSUM
	if (config->checksum_size != 0) {
		/* Check the checksum validity, for this all the data must be read out */
		uint32_t checksum = 0;
		uint32_t expected_checksum = 0;
		ssize_t data_size = config->size - config->checksum_size;

		rc = retention_checksum(dev, &checksum);

		if (rc < 0) {
			goto finish;
		}

		if (config->checksum_size == CHECKSUM_CRC8) {
			uint8_t read_checksum;

			rc = retained_mem_read(config->parent, (config->offset + data_size),
					       (void *)&read_checksum, sizeof(read_checksum));
			expected_checksum = (uint32_t)read_checksum;
		} else if (config->checksum_size == CHECKSUM_CRC16) {
			uint16_t read_checksum;

			rc = retained_mem_read(config->parent, (config->offset + data_size),
					       (void *)&read_checksum, sizeof(read_checksum));
			expected_checksum = (uint32_t)read_checksum;
		} else if (config->checksum_size == CHECKSUM_CRC32) {
			rc = retained_mem_read(config->parent, (config->offset + data_size),
					       (void *)&expected_checksum,
					       sizeof(expected_checksum));
		}

		if (rc < 0) {
			goto finish;
		}

		if (checksum != expected_checksum) {
			goto finish;
		}
	}
#endif

	/* At this point, checks have passed (if enabled), mark data as being valid */
	rc = DATA_VALID_VALUE;

finish:
	retention_lock_release(dev);

	return rc;
}

int retention_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size)
{
	const struct retention_config *config = dev->config;
	int rc;

	if (offset < 0 || ((size_t)offset + size) > (config->size - config->reserved_size)) {
		/* Disallow reading past the virtual data size or before it */
		return -EINVAL;
	}

	retention_lock_take(dev);

	rc = retained_mem_read(config->parent, (config->offset + config->prefix_len +
			       (size_t)offset), buffer, size);

	retention_lock_release(dev);

	return rc;
}

int retention_write(const struct device *dev, off_t offset, const uint8_t *buffer, size_t size)
{
	const struct retention_config *config = dev->config;
	int rc;

#ifdef ANY_HAS_PREFIX
	struct retention_data *data = dev->data;
#endif

	retention_lock_take(dev);

	if (offset < 0 || ((size_t)offset + size) > (config->size - config->reserved_size)) {
		/* Disallow writing past the virtual data size or before it */
		rc = -EINVAL;
		goto finish;
	}

	rc = retained_mem_write(config->parent, (config->offset + config->prefix_len +
				(size_t)offset), buffer, size);

	if (rc < 0) {
		goto finish;
	}

#ifdef ANY_HAS_PREFIX
	/* Write optional header and footer information, these are done last to ensure data
	 * validity before marking it as being valid
	 */
	if (config->prefix_len != 0 && data->header_written == false) {
		rc = retained_mem_write(config->parent,
					config->offset,
					(void *)config->prefix,
					config->prefix_len);

		if (rc < 0) {
			goto finish;
		}

		data->header_written = true;
	}
#endif /* ANY_HAS_PREFIX */

#ifdef ANY_HAS_CHECKSUM
	if (config->checksum_size != 0) {
		/* Generating a checksum requires reading out all the data in the region */
		uint32_t checksum = 0;

		rc = retention_checksum(dev, &checksum);

		if (rc < 0) {
			goto finish;
		}

		if (config->checksum_size == CHECKSUM_CRC8) {
			uint8_t output_checksum = (uint8_t)checksum;

			rc = retained_mem_write(config->parent,
					(config->offset + config->size - config->checksum_size),
					(void *)&output_checksum, sizeof(output_checksum));
#ifdef ANY_HAS_WRITE32
			/* To write an incomplete 32-bit word : write again */
			rc |= retained_mem_write(config->parent,
					(config->offset + config->size - config->checksum_size),
					(void *)&output_checksum, sizeof(output_checksum));
#endif /* ANY_HAS_WRITE32 */
		} else if (config->checksum_size == CHECKSUM_CRC16) {
			uint16_t output_checksum = (uint16_t)checksum;
			rc = retained_mem_write(config->parent,
					(config->offset + config->size - config->checksum_size),
					(void *)&output_checksum, sizeof(output_checksum));
#ifdef ANY_HAS_WRITE32
			/* To write an incomplete 32-bit word : write again */
			rc |= retained_mem_write(config->parent,
					(config->offset + config->size - config->checksum_size),
					(void *)&output_checksum, sizeof(output_checksum));
#endif /* ANY_HAS_WRITE32 */
		} else if (config->checksum_size == CHECKSUM_CRC32) {
			rc = retained_mem_write(config->parent,
					(config->offset + config->size - config->checksum_size),
					(void *)&checksum, sizeof(checksum));
		}
	}

#endif /* ANY_HAS_CHECKSUM */

#if defined(ANY_HAS_PREFIX) && defined(ANY_HAS_WRITE32)
	/* Need to write again incomplete word to validate the prefix */
	if (config->prefix_len != 0 && data->header_written == false) {
		rc = retained_mem_write(config->parent,
					config->offset,
					(void *)config->prefix,
					config->prefix_len);

		if (rc < 0) {
			goto finish;
		}

		data->header_written = true;
	}
#endif /* ANY_HAS_PREFIX */

finish:
	retention_lock_release(dev);

	return rc;
}

int retention_clear(const struct device *dev)
{
	const struct retention_config *config = dev->config;
	struct retention_data *data = dev->data;
	int rc = 0;
	uint8_t buffer[CONFIG_RETENTION_BUFFER_SIZE];
	off_t pos = 0;

	memset(buffer, 0, sizeof(buffer));

	retention_lock_take(dev);

	data->header_written = false;

	while (pos < config->size) {
		rc = retained_mem_write(config->parent, (config->offset + pos), buffer,
					MIN((config->size - pos), sizeof(buffer)));

		if (rc < 0) {
			goto finish;
		}

		pos += MIN((config->size - pos), sizeof(buffer));
	}

finish:
	retention_lock_release(dev);

	return rc;
}

static const struct retention_api retention_api = {
	.size = retention_size,
	.is_valid = retention_is_valid,
	.read = retention_read,
	.write = retention_write,
	.clear = retention_clear,
};

#define RETENTION_DEVICE(inst)									\
	static struct retention_data								\
		retention_data_##inst = {							\
		.header_written = false,							\
	};											\
	static const struct retention_config							\
		retention_config_##inst = {							\
		.parent = DEVICE_DT_GET(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),		\
		.checksum_size = DT_INST_PROP(inst, checksum),					\
		.offset = DT_INST_REG_ADDR(inst),						\
		.size = DT_INST_REG_SIZE(inst),							\
		.reserved_size = (COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, prefix),		\
					      (DT_INST_PROP_LEN(inst, prefix)), (0)) +		\
				  DT_INST_PROP(inst, checksum)),				\
		.prefix_len = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, prefix),			\
					  (DT_INST_PROP_LEN(inst, prefix)), (0)),		\
		.prefix = DT_INST_PROP_OR(inst, prefix, {0}),					\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &retention_init,							\
			      NULL,								\
			      &retention_data_##inst,						\
			      &retention_config_##inst,						\
			      POST_KERNEL,							\
			      CONFIG_RETENTION_INIT_PRIORITY,					\
			      &retention_api);

DT_INST_FOREACH_STATUS_OKAY(RETENTION_DEVICE)
