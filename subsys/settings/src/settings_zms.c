/* Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "settings/settings_zms.h"
#include "settings_priv.h"

#include <zephyr/settings/settings.h>
#include <zephyr/sys/hash_function.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

#if DT_HAS_CHOSEN(zephyr_settings_partition)
#define SETTINGS_PARTITION DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_settings_partition))
#else
#define SETTINGS_PARTITION FIXED_PARTITION_ID(storage_partition)
#endif

struct settings_zms_read_fn_arg {
	struct zms_fs *fs;
	uint32_t id;
};

static int settings_zms_load(struct settings_store *cs, const struct settings_load_arg *arg);
static ssize_t settings_zms_load_one(struct settings_store *cs, const char *name, char *buf,
				     size_t buf_len);
static int settings_zms_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len);
static void *settings_zms_storage_get(struct settings_store *cs);
static int settings_zms_get_last_hash_ids(struct settings_zms *cf);

static struct settings_store_itf settings_zms_itf = {.csi_load = settings_zms_load,
						     .csi_load_one = settings_zms_load_one,
						     .csi_save = settings_zms_save,
						     .csi_storage_get = settings_zms_storage_get};

static ssize_t settings_zms_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_zms_read_fn_arg *rd_fn_arg;

	rd_fn_arg = (struct settings_zms_read_fn_arg *)back_end;

	return zms_read(rd_fn_arg->fs, rd_fn_arg->id, data, len);
}

static int settings_zms_src(struct settings_zms *cf)
{
	cf->cf_store.cs_itf = &settings_zms_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

static int settings_zms_dst(struct settings_zms *cf)
{
	cf->cf_store.cs_itf = &settings_zms_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

#ifndef CONFIG_SETTINGS_ZMS_NO_LL_DELETE
static int settings_zms_unlink_ll_node(struct settings_zms *cf, uint32_t name_hash)
{
	int rc = 0;
	struct settings_hash_linked_list settings_element;
	struct settings_hash_linked_list settings_update_element;

	/* let's update the linked list */
	rc = zms_read(&cf->cf_zms, ZMS_LL_NODE_FROM_NAME_ID(name_hash), &settings_element,
		      sizeof(struct settings_hash_linked_list));
	if (rc < 0) {
		return rc;
	}

	/* update the previous element */
	if (settings_element.previous_hash) {
		rc = zms_read(&cf->cf_zms, settings_element.previous_hash, &settings_update_element,
			      sizeof(struct settings_hash_linked_list));
		if (rc < 0) {
			return rc;
		}
		if (!settings_element.next_hash) {
			/* we are deleting the last element of the linked list,
			 * let's update the second_to_last_hash_id
			 */
			cf->second_to_last_hash_id = settings_update_element.previous_hash;
		}
		settings_update_element.next_hash = settings_element.next_hash;
		rc = zms_write(&cf->cf_zms, settings_element.previous_hash,
			       &settings_update_element, sizeof(struct settings_hash_linked_list));
		if (rc < 0) {
			return rc;
		}
	}

	/* Now delete the current linked list element */
	rc = zms_delete(&cf->cf_zms, ZMS_LL_NODE_FROM_NAME_ID(name_hash));
	if (rc < 0) {
		return rc;
	}

	/* update the next element */
	if (settings_element.next_hash) {
		rc = zms_read(&cf->cf_zms, settings_element.next_hash, &settings_update_element,
			      sizeof(struct settings_hash_linked_list));
		if (rc < 0) {
			return rc;
		}
		settings_update_element.previous_hash = settings_element.previous_hash;
		rc = zms_write(&cf->cf_zms, settings_element.next_hash, &settings_update_element,
			       sizeof(struct settings_hash_linked_list));
		if (rc < 0) {
			return rc;
		}
		if (!settings_update_element.next_hash) {
			/* update second_to_last_hash_id */
			cf->second_to_last_hash_id = settings_element.previous_hash;
		}
	} else {
		/* we are deleting the last element of the linked list
		 * let's update the last_hash_id.
		 */
		cf->last_hash_id = settings_element.previous_hash;
	}

	return 0;
}
#endif /* CONFIG_SETTINGS_ZMS_NO_LL_DELETE */

static int settings_zms_delete(struct settings_zms *cf, uint32_t name_hash)
{
	int rc = 0;

	rc = zms_delete(&cf->cf_zms, name_hash);
	if (rc >= 0) {
		rc = zms_delete(&cf->cf_zms, name_hash + ZMS_DATA_ID_OFFSET);
	}
	if (rc < 0) {
		return rc;
	}

#ifndef CONFIG_SETTINGS_ZMS_NO_LL_DELETE
#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
	cf->ll_has_changed = true;
#endif
	rc = settings_zms_unlink_ll_node(cf, name_hash);
	if (rc < 0) {
		return rc;
	}

#endif /* CONFIG_SETTINGS_ZMS_NO_LL_DELETE */
	return rc;
}

#ifdef CONFIG_SETTINGS_ZMS_LOAD_SUBTREE_PATH
/* Loads first the key which is defined by the name found in "subtree" root.
 * If the key is not found or further keys under the same subtree are needed
 * by the caller, returns 0.
 */
static int settings_zms_load_subtree(struct settings_store *cs, const struct settings_load_arg *arg)
{
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);
	struct settings_zms_read_fn_arg read_fn_arg;
	char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	ssize_t rc1;
	ssize_t rc2;
	uint32_t name_hash;

	name_hash = sys_hash32(arg->subtree, strlen(arg->subtree)) & ZMS_HASH_MASK;
	for (int i = 0; i <= cf->hash_collision_num; i++) {
		name_hash = ZMS_UPDATE_COLLISION_NUM(name_hash, i);
		/* Get the name entry from ZMS */
		rc1 = zms_read(&cf->cf_zms, ZMS_NAME_ID_FROM_HASH(name_hash), &name,
			       sizeof(name) - 1);
		/* get the length of data and verify that it exists */
		rc2 = zms_get_data_length(&cf->cf_zms,
					  ZMS_NAME_ID_FROM_HASH(name_hash) + ZMS_DATA_ID_OFFSET);
		if ((rc1 <= 0) || (rc2 <= 0)) {
			/* Name or data doesn't exist */
			continue;
		}
		/* Found a name, this might not include a trailing \0 */
		name[rc1] = '\0';
		if (strcmp(arg->subtree, name)) {
			/* Names are not equal let's continue to the next collision hash
			 * if it exists.
			 */
			continue;
		}
		/* At this steps the names are equal, let's set the handler */
		read_fn_arg.fs = &cf->cf_zms;
		read_fn_arg.id = ZMS_NAME_ID_FROM_HASH(name_hash) + ZMS_DATA_ID_OFFSET;

		/* We should return here as there is no need to look for the next
		 * hash collision
		 */
		return settings_call_set_handler(arg->subtree, rc2, settings_zms_read_fn, &read_fn_arg,
						 (void *)arg);
	}

	return 0;
}
#endif /* CONFIG_SETTINGS_ZMS_LOAD_SUBTREE_PATH */

static ssize_t settings_zms_load_one(struct settings_store *cs, const char *name, char *buf,
				     size_t buf_len)
{
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);
	char r_name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	ssize_t rc = 0;
	uint32_t name_hash;
	uint32_t value_id;

	/* verify that name is not NULL */
	if (!name || !buf) {
		return -EINVAL;
	}

	name_hash = sys_hash32(name, strlen(name)) & ZMS_HASH_MASK;
	for (int i = 0; i <= cf->hash_collision_num; i++) {
		name_hash = ZMS_UPDATE_COLLISION_NUM(name_hash, i);
		/* Get the name entry from ZMS */
		rc = zms_read(&cf->cf_zms, ZMS_NAME_ID_FROM_HASH(name_hash), r_name,
			      sizeof(r_name) - 1);
		if (rc <= 0) {
			/* Name doesn't exist */
			continue;
		}
		/* Found a name, this might not include a trailing \0 */
		r_name[rc] = '\0';
		if (strcmp(name, r_name)) {
			/* Names are not equal let's continue to the next collision hash
			 * if it exists.
			 */
			continue;
		}

		/* At this steps the names are equal, let's read the data */
		value_id = ZMS_NAME_ID_FROM_HASH(name_hash) + ZMS_DATA_ID_OFFSET;
		rc = zms_read(&cf->cf_zms, value_id, buf, buf_len);

		return rc == buf_len ? zms_get_data_length(&cf->cf_zms, value_id) : rc;
	}

	return rc;
}

static int settings_zms_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	int ret = 0;
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);
	struct settings_zms_read_fn_arg read_fn_arg;
	struct settings_hash_linked_list settings_element;
	char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	ssize_t rc1;
	ssize_t rc2;
	uint32_t ll_hash_id;
#ifdef CONFIG_SETTINGS_ZMS_LOAD_SUBTREE_PATH
	/* If arg->subtree is not null we must first load settings in that subtree */
	if (arg->subtree != NULL) {
		ret = settings_zms_load_subtree(cs, arg);
		if (ret) {
			return ret;
		}
	}
#endif /* CONFIG_SETTINGS_ZMS_LOAD_SUBTREE_PATH */

#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
	uint32_t ll_cache_index = 0;

	if (cf->ll_has_changed) {
		/* reload the linked list in cache */
		ret = settings_zms_get_last_hash_ids(cf);
		if (ret < 0) {
			return ret;
		}
	}
	settings_element = cf->ll_cache[ll_cache_index++];
#else
	ret = zms_read(&cf->cf_zms, ZMS_LL_HEAD_HASH_ID, &settings_element,
		       sizeof(struct settings_hash_linked_list));
	if (ret < 0) {
		return ret;
	}
#endif /* CONFIG_SETTINGS_ZMS_LL_CACHE */
	ll_hash_id = settings_element.next_hash;

	/* If subtree is NULL then we must load all found Settings */
	while (ll_hash_id) {
		/* In the ZMS backend, each setting item is stored in two ZMS
		 * entries one for the setting's name and one with the
		 * setting's value.
		 */
		rc1 = zms_read(&cf->cf_zms, ZMS_NAME_ID_FROM_LL_NODE(ll_hash_id), &name,
			       sizeof(name) - 1);
		/* get the length of data and verify that it exists */
		rc2 = zms_get_data_length(&cf->cf_zms, ZMS_NAME_ID_FROM_LL_NODE(ll_hash_id) +
							       ZMS_DATA_ID_OFFSET);

		if ((rc1 <= 0) || (rc2 <= 0)) {
			/* In case we are not updating the linked list, this is an empty mode
			 * Just continue
			 */
#ifndef CONFIG_SETTINGS_ZMS_NO_LL_DELETE
			/* Otherwise, Settings item is not stored correctly in the ZMS.
			 * ZMS entry's name or value is either missing or deleted.
			 * Clean dirty entries to make space for future settings items.
			 */
			ret = settings_zms_delete(cf, ZMS_NAME_ID_FROM_LL_NODE(ll_hash_id));
			if (ret < 0) {
				return ret;
			}
#endif /* CONFIG_SETTINGS_ZMS_NO_LL_DELETE */
#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
			if (ll_cache_index < cf->ll_cache_next) {
				settings_element = cf->ll_cache[ll_cache_index++];
			}  else if (ll_hash_id == cf->second_to_last_hash_id) {
				/* The last ll node is not stored in the cache as it is already
				 * in the cf->last_hash_id.
				 */
				settings_element.next_hash = cf->last_hash_id;
			} else {
#endif
				ret = zms_read(&cf->cf_zms, ll_hash_id, &settings_element,
					       sizeof(struct settings_hash_linked_list));
				if (ret < 0) {
					return ret;
				}
#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
			}
#endif
			/* update next ll_hash_id */
			ll_hash_id = settings_element.next_hash;
			continue;
		}

		/* Found a name, this might not include a trailing \0 */
		name[rc1] = '\0';
		read_fn_arg.fs = &cf->cf_zms;
		read_fn_arg.id = ZMS_NAME_ID_FROM_LL_NODE(ll_hash_id) + ZMS_DATA_ID_OFFSET;

		ret = settings_call_set_handler(name, rc2, settings_zms_read_fn, &read_fn_arg,
						(void *)arg);
		if (ret) {
			break;
		}

#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
		if (ll_cache_index < cf->ll_cache_next) {
			settings_element = cf->ll_cache[ll_cache_index++];
		} else if (ll_hash_id == cf->second_to_last_hash_id) {
			settings_element.next_hash = cf->last_hash_id;
		} else {
#endif
			ret = zms_read(&cf->cf_zms, ll_hash_id, &settings_element,
				       sizeof(struct settings_hash_linked_list));
			if (ret < 0) {
				return ret;
			}
#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
		}
#endif
		/* update next ll_hash_id */
		ll_hash_id = settings_element.next_hash;
	}

	return ret;
}

static int settings_zms_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len)
{
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);
	struct settings_hash_linked_list settings_element;
	char rdname[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	uint32_t name_hash;
	uint32_t collision_num = 0;
	bool delete;
	bool write_name;
	bool hash_collision;
	int rc = 0;
	int first_available_hash_index = -1;

	if (!name) {
		return -EINVAL;
	}

	/* Find out if we are doing a delete */
	delete = ((value == NULL) || (val_len == 0));

	name_hash = sys_hash32(name, strlen(name)) & ZMS_HASH_MASK;
	/* MSB is always 1 */
	name_hash |= BIT(31);

	/* Let's find out if there are hash collisions in the storage */
	write_name = true;
	hash_collision = true;

	for (int i = 0; i <= cf->hash_collision_num; i++) {
		rc = zms_read(&cf->cf_zms, name_hash + i * LSB_GET(ZMS_COLLISIONS_MASK), &rdname,
			      sizeof(rdname));
		if (rc == -ENOENT) {
			if (first_available_hash_index < 0) {
				first_available_hash_index = i;
			}
			continue;
		} else if (rc < 0) {
			/* error while reading */
			return rc;
		}
		/* Settings entry exist, let's verify if this is the same
		 * name
		 */
		rdname[rc] = '\0';
		if (!strcmp(name, rdname)) {
			/* Hash exist and the names are equal, we should
			 * not write the names again.
			 */
			write_name = false;
			name_hash += i * LSB_GET(ZMS_COLLISIONS_MASK);
			goto no_hash_collision;
		}
		/* At this step a Hash collision exists and names are different.
		 * If we are in the middle of the loop, we should continue checking
		 * all other possible hash collisions.
		 * If we reach the end of the loop, either we should select the first
		 * free hash value otherwise we increment it to the next free value and
		 * update hash_collision_num
		 */
		collision_num++;
	}

	if (collision_num <= cf->hash_collision_num) {
		/* At this step there is a free hash found */
		name_hash = ZMS_UPDATE_COLLISION_NUM(name_hash, first_available_hash_index);
		goto no_hash_collision;
	} else if (collision_num > cf->hash_collision_num) {
		/* We must create a new hash based on incremented collision_num */
		if (collision_num > ZMS_MAX_COLLISIONS) {
			/* At this step there is no more space to store hash values */
			LOG_ERR("Maximum hash collisions reached");
			return -ENOSPC;
		}
		cf->hash_collision_num = collision_num;
		name_hash = ZMS_UPDATE_COLLISION_NUM(name_hash, collision_num);
	}

no_hash_collision:
	if (delete) {
		if (write_name) {
			/* hash doesn't exist, do not write anything here */
			return 0;
		}

		rc = settings_zms_delete(cf, name_hash);
		return rc;
	}

	/* write the value */
	rc = zms_write(&cf->cf_zms, name_hash + ZMS_DATA_ID_OFFSET, value, val_len);
	if (rc < 0) {
		return rc;
	}

	/* write the name if required */
	if (write_name) {
		/* First let's update the linked list */
#ifdef CONFIG_SETTINGS_ZMS_NO_LL_DELETE
		/* verify that the ll_node doesn't exist otherwise do not update it */
		rc = zms_read(&cf->cf_zms, ZMS_LL_NODE_FROM_NAME_ID(name_hash), &settings_element,
			      sizeof(struct settings_hash_linked_list));
		if (rc >= 0) {
			goto no_ll_update;
		} else if (rc != -ENOENT) {
			return rc;
		}
		/* else the LL node doesn't exist let's update it */
#endif /* CONFIG_SETTINGS_ZMS_NO_LL_DELETE */
		/* write linked list structure element */
		settings_element.next_hash = 0;
		/* Verify first that the linked list last element is not broken.
		 * Settings subsystem uses ID that starts from ZMS_LL_HEAD_HASH_ID.
		 */
		if (cf->last_hash_id < ZMS_LL_HEAD_HASH_ID) {
			LOG_WRN("Linked list for hashes is broken, Trying to recover");
			rc = settings_zms_get_last_hash_ids(cf);
			if (rc < 0) {
				return rc;
			}
		}
		settings_element.previous_hash = cf->last_hash_id;
		rc = zms_write(&cf->cf_zms, ZMS_LL_NODE_FROM_NAME_ID(name_hash), &settings_element,
			       sizeof(struct settings_hash_linked_list));
		if (rc < 0) {
			return rc;
		}

		/* Now update the previous linked list element */
		settings_element.next_hash = ZMS_LL_NODE_FROM_NAME_ID(name_hash);
		settings_element.previous_hash = cf->second_to_last_hash_id;
		rc = zms_write(&cf->cf_zms, cf->last_hash_id, &settings_element,
			       sizeof(struct settings_hash_linked_list));
		if (rc < 0) {
			return rc;
		}
		cf->second_to_last_hash_id = cf->last_hash_id;
		cf->last_hash_id = ZMS_LL_NODE_FROM_NAME_ID(name_hash);
#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
		if (cf->ll_cache_next < CONFIG_SETTINGS_ZMS_LL_CACHE_SIZE) {
			cf->ll_cache[cf->ll_cache_next] = settings_element;
			cf->ll_cache_next = cf->ll_cache_next + 1;
		}
#endif
#ifdef CONFIG_SETTINGS_ZMS_NO_LL_DELETE
no_ll_update:
#endif /* CONFIG_SETTINGS_ZMS_NO_LL_DELETE */
		/* Now let's write the name */
		rc = zms_write(&cf->cf_zms, name_hash, name, strlen(name));
		if (rc < 0) {
			return rc;
		}
	}
	return 0;
}

static int settings_zms_get_last_hash_ids(struct settings_zms *cf)
{
	struct settings_hash_linked_list settings_element;
	uint32_t ll_last_hash_id = ZMS_LL_HEAD_HASH_ID;
	uint32_t previous_ll_hash_id = 0;
	int rc = 0;

#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
	cf->ll_cache_next = 0;
#endif
	cf->hash_collision_num = 0;
	do {
		rc = zms_read(&cf->cf_zms, ll_last_hash_id, &settings_element,
			      sizeof(settings_element));
		if (rc == -ENOENT) {
			/* header doesn't exist or linked list broken, reinitialize the header
			 * if it doesn't exist and recover it if it is broken
			 */
			if (ll_last_hash_id == ZMS_LL_HEAD_HASH_ID) {
				/* header doesn't exist */
				const struct settings_hash_linked_list settings_element = {
					.previous_hash = 0, .next_hash = 0};
				rc = zms_write(&cf->cf_zms, ZMS_LL_HEAD_HASH_ID, &settings_element,
					       sizeof(struct settings_hash_linked_list));
				if (rc < 0) {
					return rc;
				}
				cf->last_hash_id = ZMS_LL_HEAD_HASH_ID;
				cf->second_to_last_hash_id = 0;
			} else {
				/* let's recover it by keeping all nodes until the last one */
				const struct settings_hash_linked_list settings_element = {
					.previous_hash = cf->second_to_last_hash_id,
					.next_hash = 0};
				rc = zms_write(&cf->cf_zms, cf->last_hash_id, &settings_element,
					       sizeof(struct settings_hash_linked_list));
				if (rc < 0) {
					return rc;
				}
			}
			return 0;
		} else if (rc < 0) {
			return rc;
		}

		if (settings_element.previous_hash != previous_ll_hash_id) {
			/* This is a special case that can happen when a power down occurred
			 * when deleting a linked list node.
			 * If the power down occurred after updating the previous linked list node,
			 * then we would end up with a state where the previous_hash of the linked
			 * list is broken. Let's recover from this
			 */
			rc = zms_delete(&cf->cf_zms, settings_element.previous_hash);
			if (rc < 0) {
				return rc;
			}
			/* Now recover the linked list */
			settings_element.previous_hash = previous_ll_hash_id;
			zms_write(&cf->cf_zms, ll_last_hash_id, &settings_element,
				  sizeof(struct settings_hash_linked_list));
			if (rc < 0) {
				return rc;
			}
		}
		previous_ll_hash_id = ll_last_hash_id;

#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
		if ((cf->ll_cache_next < CONFIG_SETTINGS_ZMS_LL_CACHE_SIZE) &&
		    (settings_element.next_hash)) {
			cf->ll_cache[cf->ll_cache_next] = settings_element;
			cf->ll_cache_next = cf->ll_cache_next + 1;
		}
#endif
		/* increment hash collision number if necessary */
		if (ZMS_COLLISION_NUM(ll_last_hash_id) > cf->hash_collision_num) {
			cf->hash_collision_num = ZMS_COLLISION_NUM(ll_last_hash_id);
		}
		cf->last_hash_id = ll_last_hash_id;
		cf->second_to_last_hash_id = settings_element.previous_hash;
		ll_last_hash_id = settings_element.next_hash;
	} while (settings_element.next_hash);

#ifdef CONFIG_SETTINGS_ZMS_LL_CACHE
	cf->ll_has_changed = false;
#endif
	return 0;
}

/* Initialize the zms backend. */
static int settings_zms_backend_init(struct settings_zms *cf)
{
	int rc;

	cf->cf_zms.flash_device = cf->flash_dev;
	if (cf->cf_zms.flash_device == NULL) {
		return -ENODEV;
	}

	rc = zms_mount(&cf->cf_zms);
	if (rc) {
		return rc;
	}

	cf->hash_collision_num = 0;

	rc = settings_zms_get_last_hash_ids(cf);

	LOG_DBG("ZMS backend initialized");
	return rc;
}

int settings_backend_init(void)
{
	static struct settings_zms default_settings_zms;
	int rc;
	uint32_t cnt = 0;
	size_t zms_sector_size;
	const struct flash_area *fa;
	struct flash_sector hw_flash_sector;
	uint32_t sector_cnt = 1;

	rc = flash_area_open(SETTINGS_PARTITION, &fa);
	if (rc) {
		return rc;
	}

	rc = flash_area_get_sectors(SETTINGS_PARTITION, &sector_cnt, &hw_flash_sector);
	if (rc != 0 && rc != -ENOMEM) {
		return rc;
	}

	zms_sector_size = CONFIG_SETTINGS_ZMS_SECTOR_SIZE_MULT * hw_flash_sector.fs_size;

	if (zms_sector_size > UINT32_MAX) {
		return -EDOM;
	}

#if defined(CONFIG_SETTINGS_ZMS_CUSTOM_SECTOR_COUNT)
	size_t zms_size = 0;

	while (cnt < CONFIG_SETTINGS_ZMS_SECTOR_COUNT) {
		zms_size += zms_sector_size;
		if (zms_size > fa->fa_size) {
			break;
		}
		cnt++;
	}
#else
	cnt = fa->fa_size / zms_sector_size;
#endif
	/* initialize the zms file system structure using the page_info */
	default_settings_zms.cf_zms.sector_size = zms_sector_size;
	default_settings_zms.cf_zms.sector_count = cnt;
	default_settings_zms.cf_zms.offset = fa->fa_off;
	default_settings_zms.flash_dev = fa->fa_dev;

	rc = settings_zms_backend_init(&default_settings_zms);
	if (rc) {
		return rc;
	}

	rc = settings_zms_src(&default_settings_zms);

	if (rc) {
		return rc;
	}

	rc = settings_zms_dst(&default_settings_zms);

	return rc;
}

static void *settings_zms_storage_get(struct settings_store *cs)
{
	struct settings_zms *cf = CONTAINER_OF(cs, struct settings_zms, cf_store);

	return &cf->cf_zms;
}
