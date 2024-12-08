/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bindesc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>

LOG_MODULE_REGISTER(bindesc_read, CONFIG_BINDESC_READ_LOG_LEVEL);

struct find_user_data {
	const void *result;
	size_t size;
	uint16_t tag;
};

/**
 * A callback used by the bindesc_find_* functions.
 */
static int find_callback(const struct bindesc_entry *entry, void *user_data)
{
	struct find_user_data *data = (struct find_user_data *)user_data;

	if (data->tag == entry->tag) {
		data->result = (const void *)&(entry->data);
		data->size = entry->len;
		return 1;
	}

	return 0;
}

/**
 * A callback used by the bindesc_get_size function.
 */
static int get_size_callback(const struct bindesc_entry *entry, void *user_data)
{
	size_t *result = (size_t *)user_data;

	*result += WB_UP(BINDESC_ENTRY_HEADER_SIZE + entry->len);

	return 0;
}

/**
 * This helper function is used to abstract the different methods of reading
 * data from the binary descriptors.
 * For RAM and memory mapped flash, the implementation is very simple, as both
 * are memory mapped and can simply return a pointer to the data.
 * Flash is more complex because it needs to read the data from flash, and do
 * error checking.
 */
static inline int get_entry(struct bindesc_handle *handle, const uint8_t *address,
			    const struct bindesc_entry **entry)
{
	int retval = 0;
	int flash_retval;

	/* Check if reading from flash is enabled, if not, this if/else will be optimized out */
	if (IS_ENABLED(CONFIG_BINDESC_READ_FLASH) && handle->type == BINDESC_HANDLE_TYPE_FLASH) {
		flash_retval = flash_read(handle->flash_device, (size_t)address,
					  handle->buffer, BINDESC_ENTRY_HEADER_SIZE);
		if (flash_retval) {
			LOG_ERR("Flash read error: %d", flash_retval);
			return -EIO;
		}

		/* Make sure buffer is large enough for the data */
		if (((const struct bindesc_entry *)handle->buffer)->len + BINDESC_ENTRY_HEADER_SIZE
				> sizeof(handle->buffer)) {
			LOG_WRN("Descriptor too large to copy, skipping");
			retval = -ENOMEM;
		} else {
			flash_retval = flash_read(handle->flash_device,
					(size_t)address + BINDESC_ENTRY_HEADER_SIZE,
					handle->buffer + BINDESC_ENTRY_HEADER_SIZE,
					((const struct bindesc_entry *)handle->buffer)->len);
			if (flash_retval) {
				LOG_ERR("Flash read error: %d", flash_retval);
				return -EIO;
			}
		}
		*entry = (const struct bindesc_entry *)handle->buffer;
	} else {
		*entry = (const struct bindesc_entry *)address;
	}

	return retval;
}

#if IS_ENABLED(CONFIG_BINDESC_READ_MEMORY_MAPPED_FLASH)
int bindesc_open_memory_mapped_flash(struct bindesc_handle *handle, size_t offset)
{
	uint8_t *address = (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset;

	if (*(uint64_t *)address != BINDESC_MAGIC) {
		LOG_ERR("Magic not found in given address");
		return -ENOENT;
	}

	handle->address = address;
	handle->type = BINDESC_HANDLE_TYPE_MEMORY_MAPPED_FLASH;
	handle->size_limit = UINT16_MAX;
	return 0;
}
#endif /* IS_ENABLED(CONFIG_BINDESC_READ_RAM) */

#if IS_ENABLED(CONFIG_BINDESC_READ_RAM)
int bindesc_open_ram(struct bindesc_handle *handle, const uint8_t *address, size_t max_size)
{
	if (!IS_ALIGNED(address, BINDESC_ALIGNMENT)) {
		LOG_ERR("Given address is not aligned");
		return -EINVAL;
	}

	if (*(uint64_t *)address != BINDESC_MAGIC) {
		LOG_ERR("Magic not found in given address");
		return -ENONET;
	}

	handle->address = address;
	handle->type = BINDESC_HANDLE_TYPE_RAM;
	handle->size_limit = max_size;
	return 0;
}
#endif /* IS_ENABLED(CONFIG_BINDESC_READ_RAM) */

#if IS_ENABLED(CONFIG_BINDESC_READ_FLASH)
int bindesc_open_flash(struct bindesc_handle *handle, size_t offset,
		       const struct device *flash_device)
{
	int retval;

	retval = flash_read(flash_device, offset, handle->buffer, sizeof(BINDESC_MAGIC));
	if (retval) {
		LOG_ERR("Flash read error: %d", retval);
		return -EIO;
	}

	if (*(uint64_t *)handle->buffer != BINDESC_MAGIC) {
		LOG_ERR("Magic not found in given address");
		return -ENOENT;
	}

	handle->address = (uint8_t *)offset;
	handle->type = BINDESC_HANDLE_TYPE_FLASH;
	handle->flash_device = flash_device;
	handle->size_limit = UINT16_MAX;
	return 0;
}
#endif /* IS_ENABLED(CONFIG_BINDESC_READ_FLASH) */

int bindesc_foreach(struct bindesc_handle *handle, bindesc_callback_t callback, void *user_data)
{
	const struct bindesc_entry *entry;
	const uint8_t *address = handle->address;
	int retval;

	address += sizeof(BINDESC_MAGIC);

	do {
		retval = get_entry(handle, address, &entry);
		if (retval == -EIO) {
			return -EIO;
		}
		address += WB_UP(BINDESC_ENTRY_HEADER_SIZE + entry->len);
		if (retval) {
			continue;
		}

		retval = callback(entry, user_data);
		if (retval) {
			return retval;
		}
	} while ((entry->tag != BINDESC_TAG_DESCRIPTORS_END) &&
		((address - handle->address) <= handle->size_limit));

	return 0;
}

int bindesc_find_str(struct bindesc_handle *handle, uint16_t id, const char **result)
{
	struct find_user_data data = {
		.tag = BINDESC_TAG(STR, id),
	};

	if (!bindesc_foreach(handle, find_callback, &data)) {
		LOG_WRN("The requested descriptor was not found");
		return -ENOENT;
	}
	*result = (char *)data.result;
	return 0;
}

int bindesc_find_uint(struct bindesc_handle *handle, uint16_t id, const uint32_t **result)
{
	struct find_user_data data = {
		.tag = BINDESC_TAG(UINT, id),
	};

	if (!bindesc_foreach(handle, find_callback, &data)) {
		LOG_WRN("The requested descriptor was not found");
		return -ENOENT;
	}
	*result = (const uint32_t *)data.result;
	return 0;
}

int bindesc_find_bytes(struct bindesc_handle *handle, uint16_t id, const uint8_t **result,
		       size_t *result_size)
{
	struct find_user_data data = {
		.tag = BINDESC_TAG(BYTES, id),
	};

	if (!bindesc_foreach(handle, find_callback, &data)) {
		LOG_WRN("The requested descriptor was not found");
		return -ENOENT;
	}
	*result = (const uint8_t *)data.result;
	*result_size = data.size;
	return 0;
}

int bindesc_get_size(struct bindesc_handle *handle, size_t *result)
{
	*result = sizeof(BINDESC_MAGIC);

	return bindesc_foreach(handle, get_size_callback, result);
}
