/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdbool.h>
#include <zephyr.h>

#include "settings_priv.h"

#include <logging/log.h>
#include <sys/mutex.h>
#include <sys/sys_heap.h>
#include <stdalign.h>

LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

/* Alignment of memory blocks returned by malloc. */
const static size_t kMallocAlignment = alignof(long long);
uint8_t sHeapMemory[CONFIG_SETTINGS_RAM_HEAP_SIZE];
struct sys_heap sHeap;
SYS_MUTEX_DEFINE(heap_lock);

struct settings_ram_entry {
	sys_snode_t node;
	char *key;
	char *value; /* Dynamic allocation of values */
	size_t value_len;
};

struct settings_read_fn_arg {
	const char *name;
};

/* Implementations of csi_load and csi_save */
static int settings_ram_load(struct settings_store *cs, const struct settings_load_arg *arg);
static int settings_ram_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len);

/* Helper function to save settings store itf */
static struct settings_store_itf settings_ram_itf = {
	.csi_load = settings_ram_load,
	.csi_save = settings_ram_save,
};

/* Main struct that includes settings ram backend */
static struct settings_store settings_ram_store = { .cs_itf = &settings_ram_itf };

/* the RAM settings backend uses static SList to store settings in heap */
static sys_slist_t settings_list = SYS_SLIST_STATIC_INIT(&settings_list);

/**
 * @brief Try to update already existing entry
 * 
 * Search along the settings list and try to find existing entry.
 * If entry exist, free its value and reserve the memory for a new entry.
 * 
 * @param name name of the entry / key
 * @param value value to be stored in settings
 * @param len length of the value
 * @return true if entry has been stored to the setting list
 * @return false if entry does not exist or there is no RAM space to store bigger value.
 */
static bool try_to_update_entry(const char *name, const char *value, const size_t len)
{
	if (!name || len == 0 || !value) {
		return false;
	}

	struct settings_ram_entry *entry = NULL;
	SYS_SLIST_FOR_EACH_CONTAINER (&settings_list, entry, node) {
		if (strncmp(entry->key, name, SETTINGS_MAX_NAME_LEN) == 0) {
			/* first free old memory */
			if (0 != sys_mutex_lock(&heap_lock, K_FOREVER)) {
				return false;
			}
			sys_heap_free(&sHeap, entry->value);
			entry->value = sys_heap_aligned_alloc(&sHeap, kMallocAlignment, len);
			sys_mutex_unlock(&heap_lock);
			if (entry->value) {
				memcpy(entry->value, value, len);
				entry->value_len = len;
				return true;
			}
		}
	}
	return false;
}

/**
 * @brief try to get an entry from the settings list
 * 
 * Searches along the settings list and 
 * if the entry is found update value and len arguments.
 * 
 * @param name name of the entry
 * @param value pointer to memory where the entry's value will be stored
 * @param len length of the read out entry
 * @return true if entry has been found and read out
 * @return false if could not find given entry
 */
static bool get_entry(const char *name, char *value, size_t *len)
{
	if (!name || !value || !len) {
		return false;
	}
	struct settings_ram_entry *entry = NULL;
	SYS_SLIST_FOR_EACH_CONTAINER (&settings_list, entry, node) {
		if (strncmp(entry->key, name, SETTINGS_MAX_NAME_LEN) == 0) {
			memcpy(value, entry->value, entry->value_len);
			*len = entry->value_len;
#ifdef CONFIG_SETTINGS_RAM_DEBUG
			LOG_PRINTK("Reading setting: %s with size: %zu : [", name, *len);
			for (size_t i = 0; i < *len; i++) {
				LOG_PRINTK("0x%" PRIx8 ", ", value[i]);
			}
			LOG_PRINTK("]\n");
			LOG_PRINTK("\n");
#endif
			return true;
		}
	}
	return false;
}

/**
 * @brief Delete existing entry
 * 
 * Search along the settings list to find the given entry,
 * if the entry is found, free memory of the entry's value 
 * and free memory for the full entry.
 * 
 * @param name entry name to be deleted
 * @return true if entry has been deleted
 * @return false entry has not been found
 */
static bool delete_entry(const char *name)
{
	struct settings_ram_entry *entry = NULL;
	bool rc = false;
	SYS_SLIST_FOR_EACH_CONTAINER (&settings_list, entry, node) {
		if (strncmp(entry->key, name, SETTINGS_MAX_NAME_LEN) == 0) {
			if (0 != sys_mutex_lock(&heap_lock, K_FOREVER)) {
				return false;
			}
			if (entry->value) {
				/* free memory for the entry value */
				sys_heap_free(&sHeap, entry->value);
			}
			/* free memory for the entry */
			LOG_INF("Deleting entry: %s with size: %zu", name, entry->value_len);
			rc = sys_slist_find_and_remove(&settings_list, &entry->node);
			sys_heap_free(&sHeap, entry);
			sys_mutex_unlock(&heap_lock);
			return rc;
		}
	}
	return false;
}

/**
 * @brief Add entry to the settings list
 * 
 *  In the first stage, if the given entry contains a value length equal to 
 * 0 the entry should be deleted.
 * In the second stage find out the existing entry and try to replace only a value.
 * If it is a new value allocate memory for an entry and entry's value and store it.
 * 
 * @param name name of the entry (settings key)
 * @param value value to be stored in entry
 * @param len length of the value
 * @return true if entry has been stored or deleted successfully.
 * @return false can not add new entry. One of the reasons can be a lack of the memory space for a new entry.
 */
static bool add_entry(const char *name, const char *value, const size_t len)
{
	/* 0 B of length means that settings should be deleted */
	if (len == 0) {
		return delete_entry(name);
	}
	/* entry already exist, update the value */
	if (try_to_update_entry(name, value, len)) {
		LOG_INF("Updated entry %s with data length: %zu", name, len);
		return true;
	}

	if (0 != sys_mutex_lock(&heap_lock, K_FOREVER)) {
		return false;
	}
	struct settings_ram_entry *new_entry;
	new_entry =
		sys_heap_aligned_alloc(&sHeap, kMallocAlignment, sizeof(struct settings_ram_entry));
	if (!new_entry) {
		LOG_ERR("Could not allocate memory for new entry");
		sys_mutex_unlock(&heap_lock);
		return false;
	}

	new_entry->key = sys_heap_aligned_alloc(&sHeap, kMallocAlignment,
						strnlen(name, SETTINGS_MAX_NAME_LEN) + 1);
	if (!new_entry->key) {
		LOG_ERR("Could not allocate memory for new entry key");
		sys_heap_free(&sHeap, new_entry);
		sys_mutex_unlock(&heap_lock);
		return false;
	}

	new_entry->value = sys_heap_aligned_alloc(&sHeap, kMallocAlignment, len);
	if (!new_entry->value) {
		LOG_ERR("Could not allocate memory for new entry value");
		sys_heap_free(&sHeap, new_entry->key);
		sys_heap_free(&sHeap, new_entry);
		sys_mutex_unlock(&heap_lock);
		return false;
	}
	sys_mutex_unlock(&heap_lock);

	strncpy(new_entry->key, name, strnlen(name, SETTINGS_MAX_NAME_LEN) + 1);
	memcpy(new_entry->value, value, len);
	new_entry->value_len = len;
	LOG_INF("Adding entry %s with data len: %zu", name, len);
	sys_slist_append(&settings_list, &new_entry->node);

	return true;
}

/**
 * @brief settings backend function to read a key and a value
 */
static ssize_t settings_ram_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_read_fn_arg *rd_fn_arg;
	rd_fn_arg = (struct settings_read_fn_arg *)back_end;
	if (get_entry(rd_fn_arg->name, data, &len)) {
		LOG_DBG("Loaded setting: %s", rd_fn_arg->name);
		return len;
	}
	LOG_ERR("Failed to read setting: %s", rd_fn_arg->name);
	return 0;
}

int settings_ram_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	struct settings_read_fn_arg read_fn_arg;
	struct settings_ram_entry *entry;
	int ret = 0;

	SYS_SLIST_FOR_EACH_CONTAINER (&settings_list, entry, node) {
		read_fn_arg.name = entry->key;
		ret = settings_call_set_handler(entry->key, entry->value_len, settings_ram_read_fn,
						&read_fn_arg, (void *)arg);
		if (0 != ret) {
			break;
		}
	}
	return ret;
}

int settings_ram_save(struct settings_store *cs, const char *name, const char *value,
		      size_t val_len)
{
	/* check if name is null-terminated */
	if (strnlen(name, SETTINGS_MAX_NAME_LEN) == SETTINGS_MAX_NAME_LEN) {
		return -EINVAL;
	}

	if (add_entry(name, value, val_len)) {
#ifdef CONFIG_SETTINGS_RAM_DEBUG
		LOG_PRINTK("Saving setting: %s with size: %zu : [", name, val_len);
		for (size_t i = 0; i < val_len; i++) {
			LOG_PRINTK("0x%" PRIx8 ", ", value[i]);
		}
		LOG_PRINTK("]\n");
		LOG_PRINTK("\n");
#endif
		return 0;
	}
	return -ENOMEM;
}

int settings_backend_init(void)
{
	LOG_DBG("Initialize RAM settings backend");

	sys_heap_init(&sHeap, sHeapMemory, sizeof(sHeapMemory));

	/* register custom backend */
	settings_dst_register(&settings_ram_store);
	settings_src_register(&settings_ram_store);
	return 0;
}
